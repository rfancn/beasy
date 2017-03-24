#include "internal.h"

#include "plugin.h"
#include "debug.h"
#include "notify.h"
#include "savedstatuses.h"
#include "status.h"
#include "util.h"
#include "xmlnode.h"

/**
 * The maximum number of transient statuses to save.  This
 * is used during the shutdown process to clean out old
 * transient statuses.
 */
#define MAX_TRANSIENTS 5

/**
 * The default message to use when the beasy is idle.
 */
#define DEFAULT_OFFLINE_MESSAGE _("I'm not online now")

/**
 * The information stores a snap-shot of the statuses of all
 * your plugins.  Basically these are your saved away messages.
 * There is an overall status and message that applies to
 * all your plugins, and then each individual plugin can
 * optionally have a different custom status and message.
 *
 * The changes to status.xml caused by the new status API
 * are fully backward compatible.  The new status API just
 * adds the optional sub-statuses to the XML file.
 */
struct _OulSavedStatus
{
	char *title;
	OulStatusPrimitive type;
	char *message;

	/** The timestamp when this saved status was created. This must be unique. */
	time_t creation_time;

	time_t lastused;

	unsigned int usage_count;

	GList *substatuses;      /**< A list of OulSavedStatusSub's. */
};

/*
 * TODO: If a OulStatusType is deleted, need to also delete any
 *       associated OulSavedStatusSub's?
 */
struct _OulSavedStatusSub
{
	OulPlugin *plugin;
	const OulStatusType *type;
	char *message;
};

static GList      *saved_statuses = NULL;
static guint       save_timer = 0;
static gboolean    statuses_loaded = FALSE;

/*
 * This hash table keeps track of which timestamps we've
 * used so that we don't have two saved statuses with the
 * same 'creation_time' timestamp.  The 'created' timestamp
 * is used as a unique identifier.
 *
 * So the key in this hash table is the creation_time and
 * the value is a pointer to the OulSavedStatus.
 */
static GHashTable *creation_times;

static void schedule_save(void);

/*********************************************************************
 * Private utility functions                                         *
 *********************************************************************/

static void
free_saved_status_sub(OulSavedStatusSub *substatus)
{
	g_return_if_fail(substatus != NULL);

	g_free(substatus->message);
	//oul_request_close_with_handle(substatus);
	//OUL_DBUS_UNREGISTER_POINTER(substatus);
	g_free(substatus);
}

static void
free_saved_status(OulSavedStatus *status)
{
	g_return_if_fail(status != NULL);

	g_free(status->title);
	g_free(status->message);

	while (status->substatuses != NULL)
	{
		OulSavedStatusSub *substatus = status->substatuses->data;
		status->substatuses = g_list_remove(status->substatuses, substatus);
		free_saved_status_sub(substatus);
	}
	//oul_request_close_with_handle(status);
	//OUL_DBUS_UNREGISTER_POINTER(status);
	g_free(status);
}

/*
 * Set the timestamp for when this saved status was created, and
 * make sure it is unique.
 */
static void
set_creation_time(OulSavedStatus *status, time_t creation_time)
{
	g_return_if_fail(status != NULL);

	/* Avoid using 0 because it's an invalid hash key */
	status->creation_time = creation_time != 0 ? creation_time : 1;

	while (g_hash_table_lookup(creation_times, &status->creation_time) != NULL)
		status->creation_time++;

	g_hash_table_insert(creation_times,
						&status->creation_time,
						status);
}

/**
 * A magic number is calculated for each status, and then the
 * statuses are ordered by the magic number.  The magic number
 * is the date the status was last used offset by one day for
 * each time the status has been used (but only by 10 days at
 * the most).
 *
 * The goal is to have recently used statuses at the top of
 * the list, but to also keep frequently used statuses near
 * the top.
 */
static gint
saved_statuses_sort_func(gconstpointer a, gconstpointer b)
{
	const OulSavedStatus *saved_status_a = a;
	const OulSavedStatus *saved_status_b = b;
	time_t time_a = saved_status_a->lastused +
						(MIN(saved_status_a->usage_count, 10) * 86400);
	time_t time_b = saved_status_b->lastused +
						(MIN(saved_status_b->usage_count, 10) * 86400);
	if (time_a > time_b)
		return -1;
	if (time_a < time_b)
		return 1;
	return 0;
}

/**
 * Transient statuses are added and removed automatically by
 * Oul.  If they're not used for a certain length of time then
 * they'll expire and be automatically removed.  This function
 * does the expiration.
 */
static void
remove_old_transient_statuses(void)
{
	GList *l, *next;
	OulSavedStatus *saved_status, *current_status;
	int count;
	time_t creation_time;

	current_status = oul_savedstatus_get_current();

	/*
	 * Iterate through the list of saved statuses.  Delete all
	 * transient statuses except for the first MAX_TRANSIENTS
	 * (remember, the saved statuses are already sorted by popularity).
	 */
	count = 0;
	for (l = saved_statuses; l != NULL; l = next)
	{
		next = l->next;
		saved_status = l->data;
		if (oul_savedstatus_is_transient(saved_status))
		{
			if (count == MAX_TRANSIENTS)
			{
				if (saved_status != current_status)
				{
					saved_statuses = g_list_remove(saved_statuses, saved_status);
					creation_time = oul_savedstatus_get_creation_time(saved_status);
					g_hash_table_remove(creation_times, &creation_time);
					free_saved_status(saved_status);
				}
			}
			else
				count++;
		}
	}

	if (count == MAX_TRANSIENTS)
		schedule_save();
}

/*********************************************************************
 * Writing to disk                                                   *
 *********************************************************************/

static xmlnode *
substatus_to_xmlnode(OulSavedStatusSub *substatus)
{
	xmlnode *node, *child;

	node = xmlnode_new("substatus");

	child = xmlnode_new_child(node, "plugin");
	//xmlnode_set_attrib(child, "protocol", oul_plugin_get_protocol_id(substatus->plugin));
    /**
	xmlnode_insert_data(child,
			oul_normalize(substatus->plugin,
				oul_plugin_get_username(substatus->plugin)), -1);
    **/
	child = xmlnode_new_child(node, "state");
	xmlnode_insert_data(child, oul_status_type_get_id(substatus->type), -1);

	if (substatus->message != NULL)
	{
		child = xmlnode_new_child(node, "message");
		xmlnode_insert_data(child, substatus->message, -1);
	}

	return node;
}

static xmlnode *
status_to_xmlnode(OulSavedStatus *status)
{
	xmlnode *node, *child;
	char buf[21];
	GList *cur;

	node = xmlnode_new("status");
	if (status->title != NULL)
	{
		xmlnode_set_attrib(node, "name", status->title);
	}
	else
	{
		/*
		 * Oul 1.5.0 and earlier require a name to be set, so we
		 * do this little hack to maintain backward compatability
		 * in the status.xml file.  Eventually this should be removed
		 * and we should determine if a status is transient by
		 * whether the "name" attribute is set to something or if
		 * it does not exist at all.
		 */
		xmlnode_set_attrib(node, "name", "Auto-Cached");
		xmlnode_set_attrib(node, "transient", "true");
	}

	snprintf(buf, sizeof(buf), "%lu", status->creation_time);
	xmlnode_set_attrib(node, "created", buf);

	snprintf(buf, sizeof(buf), "%lu", status->lastused);
	xmlnode_set_attrib(node, "lastused", buf);

	snprintf(buf, sizeof(buf), "%u", status->usage_count);
	xmlnode_set_attrib(node, "usage_count", buf);

	child = xmlnode_new_child(node, "state");
	xmlnode_insert_data(child, oul_primitive_get_id_from_type(status->type), -1);

	if (status->message != NULL)
	{
		child = xmlnode_new_child(node, "message");
		xmlnode_insert_data(child, status->message, -1);
	}

	for (cur = status->substatuses; cur != NULL; cur = cur->next)
	{
		child = substatus_to_xmlnode(cur->data);
		xmlnode_insert_child(node, child);
	}

	return node;
}

static xmlnode *
statuses_to_xmlnode(void)
{
	xmlnode *node, *child;
	GList *cur;

	node = xmlnode_new("statuses");
	xmlnode_set_attrib(node, "version", "1.0");

	for (cur = saved_statuses; cur != NULL; cur = cur->next)
	{
		child = status_to_xmlnode(cur->data);
		xmlnode_insert_child(node, child);
	}

	return node;
}

static void
sync_statuses(void)
{
	xmlnode *node;
	char *data;

	if (!statuses_loaded)
	{
		oul_debug_error("status", "Attempted to save statuses before they "
						 "were read!\n");
		return;
	}

	node = statuses_to_xmlnode();
	data = xmlnode_to_formatted_str(node, NULL);
	oul_util_write_data_to_file("status.xml", data, -1);
	g_free(data);
	xmlnode_free(node);
}

static gboolean
save_cb(gpointer data)
{
	sync_statuses();
	save_timer = 0;
	return FALSE;
}

static void
schedule_save(void)
{
	if (save_timer == 0)
		save_timer = oul_timeout_add_seconds(5, save_cb, NULL);
}


/*********************************************************************
 * Reading from disk                                                 *
 *********************************************************************/

static OulSavedStatusSub *
parse_substatus(xmlnode *substatus)
{
	OulSavedStatusSub *ret;
	xmlnode *node;
	char *data;

	ret = g_new0(OulSavedStatusSub, 1);

	/* Read the plugin */
	node = xmlnode_get_child(substatus, "plugin");
	if (node != NULL)
	{
		char *acct_name;
		const char *protocol;
		acct_name = xmlnode_get_data(node);
		protocol = xmlnode_get_attrib(node, "protocol");
		protocol = _oul_oscar_convert(acct_name, protocol); /* XXX: Remove */
        /**
		if ((acct_name != NULL) && (protocol != NULL))
			ret->plugin = oul_plugins_find(acct_name, protocol);
        **/
		g_free(acct_name);
	}

	if (ret->plugin == NULL)
	{
		g_free(ret);
		return NULL;
	}

	/* Read the state */
	node = xmlnode_get_child(substatus, "state");
	if ((node != NULL) && ((data = xmlnode_get_data(node)) != NULL))
	{
        /**
		ret->type = oul_status_type_find_with_id(
							ret->plugin->status_types, data);
        **/
		g_free(data);
	}

	if (ret->type == NULL)
	{
		g_free(ret);
		return NULL;
	}

	/* Read the message */
	node = xmlnode_get_child(substatus, "message");
	if ((node != NULL) && ((data = xmlnode_get_data(node)) != NULL))
	{
		ret->message = data;
	}

	//OUL_DBUS_REGISTER_POINTER(ret, OulSavedStatusSub);
	return ret;
}

/**
 * Parse a saved status and add it to the saved_statuses linked list.
 *
 * Here's an example of the XML for a saved status:
 *   <status name="Girls">
 *       <state>away</state>
 *       <message>I like the way that they walk
 *   And it's chill to hear them talk
 *   And I can always make them smile
 *   From White Castle to the Nile</message>
 *       <substatus>
 *           <plugin protocol='prpl-oscar'>markdoliner</plugin>
 *           <state>available</state>
 *           <message>The ladies man is here to answer your queries.</message>
 *       </substatus>
 *       <substatus>
 *           <plugin protocol='prpl-oscar'>giantgraypanda</plugin>
 *           <state>away</state>
 *           <message>A.C. ain't in charge no more.</message>
 *       </substatus>
 *   </status>
 *
 * I know.  Moving, huh?
 */
static OulSavedStatus *
parse_status(xmlnode *status)
{
	OulSavedStatus *ret;
	xmlnode *node;
	const char *attrib;
	char *data;
	int i;

	ret = g_new0(OulSavedStatus, 1);

	attrib = xmlnode_get_attrib(status, "transient");
	if ((attrib == NULL) || (strcmp(attrib, "true")))
	{
		/* Read the title */
		attrib = xmlnode_get_attrib(status, "name");
		ret->title = g_strdup(attrib);
	}

	if (ret->title != NULL)
	{
		/* Ensure the title is unique */
		i = 2;
		while (oul_savedstatus_find(ret->title) != NULL)
		{
			g_free(ret->title);
			ret->title = g_strdup_printf("%s %d", attrib, i);
			i++;
		}
	}

	/* Read the creation time */
	attrib = xmlnode_get_attrib(status, "created");
	set_creation_time(ret, (attrib != NULL ? atol(attrib) : 0));

	/* Read the last used time */
	attrib = xmlnode_get_attrib(status, "lastused");
	ret->lastused = (attrib != NULL ? atol(attrib) : 0);

	/* Read the usage count */
	attrib = xmlnode_get_attrib(status, "usage_count");
	ret->usage_count = (attrib != NULL ? atol(attrib) : 0);

	/* Read the primitive status type */
	node = xmlnode_get_child(status, "state");
	if ((node != NULL) && ((data = xmlnode_get_data(node)) != NULL))
	{
		ret->type = oul_primitive_get_type_from_id(data);
		g_free(data);
	}

	/* Read the message */
	node = xmlnode_get_child(status, "message");
	if ((node != NULL) && ((data = xmlnode_get_data(node)) != NULL))
	{
		ret->message = data;
	}

	/* Read substatuses */
	for (node = xmlnode_get_child(status, "substatus"); node != NULL;
			node = xmlnode_get_next_twin(node))
	{
		OulSavedStatusSub *new;
		new = parse_substatus(node);
		if (new != NULL)
			ret->substatuses = g_list_prepend(ret->substatuses, new);
	}

	//OUL_DBUS_REGISTER_POINTER(ret, OulSavedStatus);
	return ret;
}

/**
 * Read the saved statuses from a file in the Oul user dir.
 *
 * @return TRUE on success, FALSE on failure (if the file can not
 *         be opened, or if it contains invalid XML).
 */
static void
load_statuses(void)
{
	xmlnode *statuses, *status;

	statuses_loaded = TRUE;

	statuses = oul_util_read_xml_from_file("status.xml", _("saved statuses"));

	if (statuses == NULL)
		return;

	for (status = xmlnode_get_child(statuses, "status"); status != NULL;
			status = xmlnode_get_next_twin(status))
	{
		OulSavedStatus *new;
		new = parse_status(status);
		saved_statuses = g_list_prepend(saved_statuses, new);
	}
	saved_statuses = g_list_sort(saved_statuses, saved_statuses_sort_func);

	xmlnode_free(statuses);
}


/**************************************************************************
* Saved status API
**************************************************************************/
OulSavedStatus *
oul_savedstatus_new(const char *title, OulStatusPrimitive type)
{
	OulSavedStatus *status;

	/* Make sure we don't already have a saved status with this title. */
	if (title != NULL)
		g_return_val_if_fail(oul_savedstatus_find(title) == NULL, NULL);

	status = g_new0(OulSavedStatus, 1);
	//OUL_DBUS_REGISTER_POINTER(status, OulSavedStatus);
	status->title = g_strdup(title);
	status->type = type;
	set_creation_time(status, time(NULL));

	saved_statuses = g_list_insert_sorted(saved_statuses, status, saved_statuses_sort_func);

	schedule_save();

	oul_signal_emit(oul_savedstatuses_get_handle(), "savedstatus-added",
		status);

	return status;
}

void
oul_savedstatus_set_title(OulSavedStatus *status, const char *title)
{
	g_return_if_fail(status != NULL);

	/* Make sure we don't already have a saved status with this title. */
	g_return_if_fail(oul_savedstatus_find(title) == NULL);

	g_free(status->title);
	status->title = g_strdup(title);

	schedule_save();

	oul_signal_emit(oul_savedstatuses_get_handle(),
			"savedstatus-modified", status);
}

void
oul_savedstatus_set_type(OulSavedStatus *status, OulStatusPrimitive type)
{
	g_return_if_fail(status != NULL);

	status->type = type;

	schedule_save();
	oul_signal_emit(oul_savedstatuses_get_handle(),
			"savedstatus-modified", status);
}

void
oul_savedstatus_set_message(OulSavedStatus *status, const char *message)
{
	g_return_if_fail(status != NULL);

	g_free(status->message);
	if ((message != NULL) && (*message == '\0'))
		status->message = NULL;
	else
		status->message = g_strdup(message);

	schedule_save();

	oul_signal_emit(oul_savedstatuses_get_handle(),
			"savedstatus-modified", status);
}

void
oul_savedstatus_set_substatus(OulSavedStatus *saved_status,
							   const OulPlugin *plugin,
							   const OulStatusType *type,
							   const char *message)
{
	OulSavedStatusSub *substatus;

	g_return_if_fail(saved_status != NULL);
	g_return_if_fail(plugin      != NULL);
	g_return_if_fail(type         != NULL);

	/* Find an existing substatus or create a new one */
	substatus = oul_savedstatus_get_substatus(saved_status, plugin);
	if (substatus == NULL)
	{
		substatus = g_new0(OulSavedStatusSub, 1);
		//OUL_DBUS_REGISTER_POINTER(substatus, OulSavedStatusSub);
		substatus->plugin = (OulPlugin *)plugin;
		saved_status->substatuses = g_list_prepend(saved_status->substatuses, substatus);
	}

	substatus->type = type;
	g_free(substatus->message);
	substatus->message = g_strdup(message);

	schedule_save();
	oul_signal_emit(oul_savedstatuses_get_handle(),
			"savedstatus-modified", saved_status);
}

void
oul_savedstatus_unset_substatus(OulSavedStatus *saved_status,
								 const OulPlugin *plugin)
{
	GList *iter;
	OulSavedStatusSub *substatus;

	g_return_if_fail(saved_status != NULL);
	g_return_if_fail(plugin      != NULL);

	for (iter = saved_status->substatuses; iter != NULL; iter = iter->next)
	{
		substatus = iter->data;
		if (substatus->plugin == plugin)
		{
			saved_status->substatuses = g_list_delete_link(saved_status->substatuses, iter);
			g_free(substatus->message);
			g_free(substatus);
			return;
		}
	}

	oul_signal_emit(oul_savedstatuses_get_handle(),
			"savedstatus-modified", saved_status);
}

/*
 * This gets called when an plugin is deleted.  We iterate through
 * all of our saved statuses and delete any substatuses that may
 * exist for this plugin.
 */
static void
oul_savedstatus_unset_all_substatuses(const OulPlugin *plugin,
		gpointer user_data)
{
	GList *iter;
	OulSavedStatus *status;

	g_return_if_fail(plugin != NULL);

	for (iter = saved_statuses; iter != NULL; iter = iter->next)
	{
		status = (OulSavedStatus *)iter->data;
		oul_savedstatus_unset_substatus(status, plugin);
	}
}

void
oul_savedstatus_delete_by_status(OulSavedStatus *status)
{
	time_t creation_time, current, idleaway;

	g_return_if_fail(status != NULL);

	saved_statuses = g_list_remove(saved_statuses, status);
	creation_time = oul_savedstatus_get_creation_time(status);
	g_hash_table_remove(creation_times, &creation_time);
	free_saved_status(status);

	schedule_save();

	/*
	 * If we just deleted our current status or our idleaway status,
	 * then set the appropriate pref back to 0.
	 */
	current = oul_prefs_get_int("/oul/savedstatus/default");
	if (current == creation_time)
		oul_prefs_set_int("/oul/savedstatus/default", 0);

	idleaway = oul_prefs_get_int("/oul/savedstatus/idleaway");
	if (idleaway == creation_time)
		oul_prefs_set_int("/oul/savedstatus/idleaway", 0);

	oul_signal_emit(oul_savedstatuses_get_handle(),
			"savedstatus-deleted", status);
}

gboolean
oul_savedstatus_delete(const char *title)
{
	OulSavedStatus *status;

	status = oul_savedstatus_find(title);

	if (status == NULL)
		return FALSE;

	if (oul_savedstatus_get_current() == status)
		return FALSE;

	oul_savedstatus_delete_by_status(status);

	return TRUE;
}

GList *
oul_savedstatuses_get_all(void)
{
	return saved_statuses;
}

GList *
oul_savedstatuses_get_popular(unsigned int how_many)
{
	GList *popular = NULL;
	GList *cur;
	unsigned int i;
	OulSavedStatus *next;

	/* Copy 'how_many' elements to a new list. If 'how_many' is 0, then copy all of 'em. */
	if (how_many == 0)
		how_many = (unsigned int) -1;

	i = 0;
	cur = saved_statuses;
	while ((i < how_many) && (cur != NULL))
	{
		next = cur->data;
		if ((!oul_savedstatus_is_transient(next)
			|| oul_savedstatus_get_message(next) != NULL))
		{
			popular = g_list_prepend(popular, next);
			i++;
		}
		cur = cur->next;
	}

	popular = g_list_reverse(popular);

	return popular;
}

OulSavedStatus *
oul_savedstatus_get_current(void)
{
	return oul_savedstatus_get_default();
}

OulSavedStatus *
oul_savedstatus_get_default()
{
	int creation_time;
	OulSavedStatus *saved_status = NULL;

	creation_time = oul_prefs_get_int("/oul/savedstatus/default");

	if (creation_time != 0)
		saved_status = g_hash_table_lookup(creation_times, &creation_time);

	if (saved_status == NULL)
	{
		/*
		 * We don't have a current saved status!
		 * May be someone who deleted the status they were currently
		 * using?  Anyway, add a default status.
		 */
		saved_status = oul_savedstatus_new(NULL, OUL_STATUS_OFFLINE);
		oul_prefs_set_int("/oul/savedstatus/default",
						   oul_savedstatus_get_creation_time(saved_status));
	}

	return saved_status;
}

OulSavedStatus *
oul_savedstatus_get_startup()
{
	int creation_time;
	OulSavedStatus *saved_status = NULL;

	creation_time = oul_prefs_get_int("/oul/savedstatus/startup");

	if (creation_time != 0)
		saved_status = g_hash_table_lookup(creation_times, &creation_time);

	if (saved_status == NULL)
	{
		/*
		 * We don't have a status to apply.
		 * This may be the first login, or the user wants to
		 * restore the "current" status.
		 */
		saved_status = oul_savedstatus_get_current();
	}

	return saved_status;
}


OulSavedStatus *
oul_savedstatus_find(const char *title)
{
	GList *iter;
	OulSavedStatus *status;

	g_return_val_if_fail(title != NULL, NULL);

	for (iter = saved_statuses; iter != NULL; iter = iter->next)
	{
		status = (OulSavedStatus *)iter->data;
		if ((status->title != NULL) && !strcmp(status->title, title))
			return status;
	}

	return NULL;
}

OulSavedStatus *
oul_savedstatus_find_by_creation_time(time_t creation_time)
{
	GList *iter;
	OulSavedStatus *status;

	for (iter = saved_statuses; iter != NULL; iter = iter->next)
	{
		status = (OulSavedStatus *)iter->data;
		if (status->creation_time == creation_time)
			return status;
	}

	return NULL;
}

OulSavedStatus *
oul_savedstatus_find_transient_by_type_and_message(OulStatusPrimitive type,
													const char *message)
{
	GList *iter;
	OulSavedStatus *status;

	for (iter = saved_statuses; iter != NULL; iter = iter->next)
	{
		status = (OulSavedStatus *)iter->data;
		if ((status->type == type) && oul_savedstatus_is_transient(status) &&
			!oul_savedstatus_has_substatuses(status) &&
			(((status->message == NULL) && (message == NULL)) ||
			((status->message != NULL) && (message != NULL) && !strcmp(status->message, message))))
		{
			return status;
		}
	}

	return NULL;
}

gboolean
oul_savedstatus_is_transient(const OulSavedStatus *saved_status)
{
	g_return_val_if_fail(saved_status != NULL, TRUE);

	return (saved_status->title == NULL);
}

const char *
oul_savedstatus_get_title(const OulSavedStatus *saved_status)
{
	const char *message;

	g_return_val_if_fail(saved_status != NULL, NULL);

	/* If we have a title then return it */
	if (saved_status->title != NULL)
		return saved_status->title;

	/* Otherwise, this is a transient status and we make up a title on the fly */
	message = oul_savedstatus_get_message(saved_status);

	if ((message == NULL) || (*message == '\0'))
	{
		OulStatusPrimitive primitive;
		primitive = oul_savedstatus_get_type(saved_status);
		return oul_primitive_get_name_from_type(primitive);
	}
	else
	{
		char *stripped;
		static char buf[64];
		stripped = oul_markup_strip_html(message);
		oul_util_chrreplace(stripped, '\n', ' ');
		strncpy(buf, stripped, sizeof(buf));
		buf[sizeof(buf) - 1] = '\0';
		if ((strlen(stripped) + 1) > sizeof(buf))
		{
			/* Truncate and ellipsize */
			char *tmp = g_utf8_find_prev_char(buf, &buf[sizeof(buf) - 4]);
			strcpy(tmp, "...");
		}
		g_free(stripped);
		return buf;
	}
}

OulStatusPrimitive
oul_savedstatus_get_type(const OulSavedStatus *saved_status)
{
	g_return_val_if_fail(saved_status != NULL, OUL_STATUS_OFFLINE);

	return saved_status->type;
}

const char *
oul_savedstatus_get_message(const OulSavedStatus *saved_status)
{
	g_return_val_if_fail(saved_status != NULL, NULL);

	return saved_status->message;
}

time_t
oul_savedstatus_get_creation_time(const OulSavedStatus *saved_status)
{
	g_return_val_if_fail(saved_status != NULL, 0);

	return saved_status->creation_time;
}

gboolean
oul_savedstatus_has_substatuses(const OulSavedStatus *saved_status)
{
	g_return_val_if_fail(saved_status != NULL, FALSE);

	return (saved_status->substatuses != NULL);
}

OulSavedStatusSub *
oul_savedstatus_get_substatus(const OulSavedStatus *saved_status,
							   const OulPlugin *plugin)
{
	GList *iter;
	OulSavedStatusSub *substatus;

	g_return_val_if_fail(saved_status != NULL, NULL);
	g_return_val_if_fail(plugin      != NULL, NULL);

	for (iter = saved_status->substatuses; iter != NULL; iter = iter->next)
	{
		substatus = iter->data;
		if (substatus->plugin == plugin)
			return substatus;
	}

	return NULL;
}

const OulStatusType *
oul_savedstatus_substatus_get_type(const OulSavedStatusSub *substatus)
{
	g_return_val_if_fail(substatus != NULL, NULL);

	return substatus->type;
}

const char *
oul_savedstatus_substatus_get_message(const OulSavedStatusSub *substatus)
{
	g_return_val_if_fail(substatus != NULL, NULL);

	return substatus->message;
}

void
oul_savedstatus_activate(OulSavedStatus *saved_status)
{
	GList *plugins, *node;
	OulSavedStatus *old = oul_savedstatus_get_current();

	g_return_if_fail(saved_status != NULL);

	/* Make sure our list of saved statuses remains sorted */
	saved_status->lastused = time(NULL);
	saved_status->usage_count++;
	saved_statuses = g_list_remove(saved_statuses, saved_status);
	saved_statuses = g_list_insert_sorted(saved_statuses, saved_status, saved_statuses_sort_func);
	oul_prefs_set_int("/oul/savedstatus/default",
					   oul_savedstatus_get_creation_time(saved_status));

	//plugins = oul_plugins_get_all_active();
	for (node = plugins; node != NULL; node = node->next)
	{
		OulPlugin *plugin;

		plugin = node->data;

		oul_savedstatus_activate_for_plugin(saved_status, plugin);
	}

	g_list_free(plugins);

	//if (oul_savedstatus_is_idlebusy()) {
		//oul_savedstatus_set_idlebusy(FALSE);
	//} else {
		oul_signal_emit(oul_savedstatuses_get_handle(), "savedstatus-changed",
					 	   saved_status, old);
	//}
}

void
oul_savedstatus_activate_for_plugin(const OulSavedStatus *saved_status,
									  OulPlugin *plugin)
{
	const OulStatusType *status_type;
	const OulSavedStatusSub *substatus;
	const char *message = NULL;

	g_return_if_fail(saved_status != NULL);
	g_return_if_fail(plugin != NULL);

	substatus = oul_savedstatus_get_substatus(saved_status, plugin);
	if (substatus != NULL)
	{
		status_type = substatus->type;
		message = substatus->message;
	}
	else
	{
		//status_type = oul_plugin_get_status_type_with_primitive(plugin, saved_status->type);
		if (status_type == NULL)
			return;
		message = saved_status->message;
	}

	if ((message != NULL) &&
		(oul_status_type_get_attr(status_type, "message")))
	{
        /**
		oul_plugin_set_status(plugin, oul_status_type_get_id(status_type),
								TRUE, "message", message, NULL);
        **/
	}
	else
	{
        /**
		oul_plugin_set_status(plugin, oul_status_type_get_id(status_type),
								TRUE, NULL);
        **/
	}
}

void *
oul_savedstatuses_get_handle(void)
{
	static int handle;

	return &handle;
}

void
oul_savedstatuses_init(void)
{
	void *handle = oul_savedstatuses_get_handle();

	creation_times = g_hash_table_new(g_int_hash, g_int_equal);

	/*
	 * Using 0 as the creation_time is a special case.
	 * If someone calls oul_savedstatus_get_current() or
	 * either of those functions
	 * sees a creation_time of 0, then it will create a default
	 * saved status and return that to the user.
	 */
	oul_prefs_add_none("/oul/savedstatus");
	oul_prefs_add_int("/oul/savedstatus/default", 0);

	load_statuses();

	oul_signal_register(handle, "savedstatus-changed",
					 oul_marshal_VOID__POINTER_POINTER, NULL, 2,
					 oul_value_new(OUL_TYPE_SUBTYPE,
									OUL_SUBTYPE_SAVEDSTATUS),
					 oul_value_new(OUL_TYPE_SUBTYPE,
									OUL_SUBTYPE_SAVEDSTATUS));

	oul_signal_register(handle, "savedstatus-added",
		oul_marshal_VOID__POINTER, NULL, 1,
		oul_value_new(OUL_TYPE_SUBTYPE,
			OUL_SUBTYPE_SAVEDSTATUS));

	oul_signal_register(handle, "savedstatus-deleted",
		oul_marshal_VOID__POINTER, NULL, 1,
		oul_value_new(OUL_TYPE_SUBTYPE,
			OUL_SUBTYPE_SAVEDSTATUS));

	oul_signal_register(handle, "savedstatus-modified",
		oul_marshal_VOID__POINTER, NULL, 1,
		oul_value_new(OUL_TYPE_SUBTYPE,
			OUL_SUBTYPE_SAVEDSTATUS));

#if 0
	oul_signal_connect(oul_plugins_get_handle(), "plugin-removed",
			handle,
			OUL_CALLBACK(oul_savedstatus_unset_all_substatuses),
			NULL);
#endif
}

void
oul_savedstatuses_uninit(void)
{
	remove_old_transient_statuses();

	if (save_timer != 0)
	{
		oul_timeout_remove(save_timer);
		save_timer = 0;
		sync_statuses();
	}

	while (saved_statuses != NULL) {
		OulSavedStatus *saved_status = saved_statuses->data;
		saved_statuses = g_list_remove(saved_statuses, saved_status);
		free_saved_status(saved_status);
	}

	g_hash_table_destroy(creation_times);
	creation_times = NULL;

	oul_signals_unregister_by_instance(oul_savedstatuses_get_handle());
}

