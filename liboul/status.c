#include "internal.h"

#include "status.h"
#include "core.h"
#include "debug.h"
#include "notify.h"
#include "prefs.h"
#include "value.h"

/**
 * A type of status.
 */
struct _OulStatusType
{
	OulStatusPrimitive primitive;

	char *id;
	char *name;
	char *primary_attr_id;

	gboolean saveable;
	gboolean user_settable;
	gboolean independent;

	GList *attrs;
};

/**
 * A status attribute.
 */
struct _OulStatusAttr
{
	char *id;
	char *name;
	OulValue *value_type;
};


/**
 * An active status.
 */
struct _OulStatus
{
	OulStatusType *type;

	const char *title;

	gboolean active;

	GHashTable *attr_values;
};

static int primitive_scores[] =
{
	0,      /* unset                    	*/
	-500,	/* offline				*/
	100,   	/* online	                  	*/
	10,    	/* busy		           	*/
	50      /* unread messages	*/
};

/**************************************************************************
 * OulStatusPrimitive API
 **************************************************************************/
static struct OulStatusPrimitiveMap
{
	OulStatusPrimitive type;
	const char *id;
	const char *name;

} const status_primitive_map[] =
{
	{ OUL_STATUS_UNSET,			"unset",		N_("Unset")						},
	{ OUL_STATUS_OFFLINE,		"offline",		N_("Offline")					},
	{ OUL_STATUS_ONLINE,		"online",		N_("Online")					},
	{ OUL_STATUS_BUSY,			"busy",			N_("Busy")						},
	{ OUL_STATUS_UNREAD_MSG,	"unread_msg",	N_("Have not readed message")	}
};

const char *
oul_primitive_get_id_from_type(OulStatusPrimitive type)
{
    int i;

    for (i = 0; i < OUL_STATUS_NUM_PRIMITIVES; i++)
    {
		if (type == status_primitive_map[i].type)
			return status_primitive_map[i].id;
    }

    return status_primitive_map[0].id;
}

const char *
oul_primitive_get_name_from_type(OulStatusPrimitive type)
{
    int i;

    for (i = 0; i < OUL_STATUS_NUM_PRIMITIVES; i++)
    {
	if (type == status_primitive_map[i].type)
		return _(status_primitive_map[i].name);
    }

    return _(status_primitive_map[0].name);
}

OulStatusPrimitive
oul_primitive_get_type_from_id(const char *id)
{
    int i;

    g_return_val_if_fail(id != NULL, OUL_STATUS_UNSET);

    for (i = 0; i < OUL_STATUS_NUM_PRIMITIVES; i++)
    {
        if (!strcmp(id, status_primitive_map[i].id))
            return status_primitive_map[i].type;
    }

    return status_primitive_map[0].type;
}


/**************************************************************************
 * OulStatusType API
 **************************************************************************/
OulStatusType *
oul_status_type_new_full(OulStatusPrimitive primitive, const char *id,
						  const char *name, gboolean saveable,
						  gboolean user_settable, gboolean independent)
{
	OulStatusType *status_type;

	g_return_val_if_fail(primitive != OUL_STATUS_UNSET, NULL);

	status_type = g_new0(OulStatusType, 1);
	//OUL_DBUS_REGISTER_POINTER(status_type, OulStatusType);

	status_type->primitive     = primitive;
	status_type->saveable      = saveable;
	status_type->user_settable = user_settable;
	status_type->independent   = independent;

	if (id != NULL)
		status_type->id = g_strdup(id);
	else
		status_type->id = g_strdup(oul_primitive_get_id_from_type(primitive));

	if (name != NULL)
		status_type->name = g_strdup(name);
	else
		status_type->name = g_strdup(oul_primitive_get_name_from_type(primitive));

	return status_type;
}

OulStatusType *
oul_status_type_new(OulStatusPrimitive primitive, const char *id,
					 const char *name, gboolean user_settable)
{
	g_return_val_if_fail(primitive != OUL_STATUS_UNSET, NULL);

	return oul_status_type_new_full(primitive, id, name, FALSE,
			user_settable, FALSE);
}

OulStatusType *
oul_status_type_new_with_attrs(OulStatusPrimitive primitive,
		const char *id, const char *name,
		gboolean saveable, gboolean user_settable,
		gboolean independent, const char *attr_id,
		const char *attr_name, OulValue *attr_value,
		...)
{
	OulStatusType *status_type;
	va_list args;

	g_return_val_if_fail(primitive  != OUL_STATUS_UNSET, NULL);
	g_return_val_if_fail(attr_id    != NULL,              NULL);
	g_return_val_if_fail(attr_name  != NULL,              NULL);
	g_return_val_if_fail(attr_value != NULL,              NULL);

	status_type = oul_status_type_new_full(primitive, id, name, saveable,
			user_settable, independent);

	/* Add the first attribute */
	oul_status_type_add_attr(status_type, attr_id, attr_name, attr_value);

	va_start(args, attr_value);
	oul_status_type_add_attrs_vargs(status_type, args);
	va_end(args);

	return status_type;
}

void
oul_status_type_destroy(OulStatusType *status_type)
{
	g_return_if_fail(status_type != NULL);

	g_free(status_type->id);
	g_free(status_type->name);
	g_free(status_type->primary_attr_id);

	g_list_foreach(status_type->attrs, (GFunc)oul_status_attr_destroy, NULL);
	g_list_free(status_type->attrs);

	//OUL_DBUS_UNREGISTER_POINTER(status_type);
	g_free(status_type);
}

void
oul_status_type_set_primary_attr(OulStatusType *status_type, const char *id)
{
	g_return_if_fail(status_type != NULL);

	g_free(status_type->primary_attr_id);
	status_type->primary_attr_id = g_strdup(id);
}

void
oul_status_type_add_attr(OulStatusType *status_type, const char *id,
		const char *name, OulValue *value)
{
	OulStatusAttr *attr;

	g_return_if_fail(status_type != NULL);
	g_return_if_fail(id          != NULL);
	g_return_if_fail(name        != NULL);
	g_return_if_fail(value       != NULL);

	attr = oul_status_attr_new(id, name, value);

	status_type->attrs = g_list_append(status_type->attrs, attr);
}

void
oul_status_type_add_attrs_vargs(OulStatusType *status_type, va_list args)
{
	const char *id, *name;
	OulValue *value;

	g_return_if_fail(status_type != NULL);

	while ((id = va_arg(args, const char *)) != NULL)
	{
		name = va_arg(args, const char *);
		g_return_if_fail(name != NULL);

		value = va_arg(args, OulValue *);
		g_return_if_fail(value != NULL);

		oul_status_type_add_attr(status_type, id, name, value);
	}
}

void
oul_status_type_add_attrs(OulStatusType *status_type, const char *id,
		const char *name, OulValue *value, ...)
{
	va_list args;

	g_return_if_fail(status_type != NULL);
	g_return_if_fail(id          != NULL);
	g_return_if_fail(name        != NULL);
	g_return_if_fail(value       != NULL);

	/* Add the first attribute */
	oul_status_type_add_attr(status_type, id, name, value);

	va_start(args, value);
	oul_status_type_add_attrs_vargs(status_type, args);
	va_end(args);
}

OulStatusPrimitive
oul_status_type_get_primitive(const OulStatusType *status_type)
{
	g_return_val_if_fail(status_type != NULL, OUL_STATUS_UNSET);

	return status_type->primitive;
}

const char *
oul_status_type_get_id(const OulStatusType *status_type)
{
	g_return_val_if_fail(status_type != NULL, NULL);

	return status_type->id;
}

const char *
oul_status_type_get_name(const OulStatusType *status_type)
{
	g_return_val_if_fail(status_type != NULL, NULL);

	return status_type->name;
}

gboolean
oul_status_type_is_saveable(const OulStatusType *status_type)
{
	g_return_val_if_fail(status_type != NULL, FALSE);

	return status_type->saveable;
}

gboolean
oul_status_type_is_user_settable(const OulStatusType *status_type)
{
	g_return_val_if_fail(status_type != NULL, FALSE);

	return status_type->user_settable;
}

gboolean
oul_status_type_is_independent(const OulStatusType *status_type)
{
	g_return_val_if_fail(status_type != NULL, FALSE);

	return status_type->independent;
}

gboolean
oul_status_type_is_exclusive(const OulStatusType *status_type)
{
	g_return_val_if_fail(status_type != NULL, FALSE);

	return !status_type->independent;
}

const char *
oul_status_type_get_primary_attr(const OulStatusType *status_type)
{
	g_return_val_if_fail(status_type != NULL, NULL);

	return status_type->primary_attr_id;
}

OulStatusAttr *
oul_status_type_get_attr(const OulStatusType *status_type, const char *id)
{
	GList *l;

	g_return_val_if_fail(status_type != NULL, NULL);
	g_return_val_if_fail(id          != NULL, NULL);

	for (l = status_type->attrs; l != NULL; l = l->next)
	{
		OulStatusAttr *attr = (OulStatusAttr *)l->data;

		if (!strcmp(oul_status_attr_get_id(attr), id))
			return attr;
	}

	return NULL;
}

GList *
oul_status_type_get_attrs(const OulStatusType *status_type)
{
	g_return_val_if_fail(status_type != NULL, NULL);

	return status_type->attrs;
}

const OulStatusType *
oul_status_type_find_with_id(GList *status_types, const char *id)
{
	OulStatusType *status_type;

	g_return_val_if_fail(id != NULL, NULL);

	while (status_types != NULL)
	{
		status_type = status_types->data;

		if (!strcmp(id, status_type->id))
			return status_type;

		status_types = status_types->next;
	}

	return NULL;
}


/**************************************************************************
* OulStatusAttr API
**************************************************************************/
OulStatusAttr *
oul_status_attr_new(const char *id, const char *name, OulValue *value_type)
{
	OulStatusAttr *attr;

	g_return_val_if_fail(id         != NULL, NULL);
	g_return_val_if_fail(name       != NULL, NULL);
	g_return_val_if_fail(value_type != NULL, NULL);

	attr = g_new0(OulStatusAttr, 1);
	//OUL_DBUS_REGISTER_POINTER(attr, OulStatusAttr);

	attr->id         = g_strdup(id);
	attr->name       = g_strdup(name);
	attr->value_type = value_type;

	return attr;
}

void
oul_status_attr_destroy(OulStatusAttr *attr)
{
	g_return_if_fail(attr != NULL);

	g_free(attr->id);
	g_free(attr->name);

	oul_value_destroy(attr->value_type);

	//OUL_DBUS_UNREGISTER_POINTER(attr);
	g_free(attr);
}

const char *
oul_status_attr_get_id(const OulStatusAttr *attr)
{
	g_return_val_if_fail(attr != NULL, NULL);

	return attr->id;
}

const char *
oul_status_attr_get_name(const OulStatusAttr *attr)
{
	g_return_val_if_fail(attr != NULL, NULL);

	return attr->name;
}

OulValue *
oul_status_attr_get_value(const OulStatusAttr *attr)
{
	g_return_val_if_fail(attr != NULL, NULL);

	return attr->value_type;
}


/**************************************************************************
* OulStatus API
**************************************************************************/
OulStatus *
oul_status_new(OulStatusType *status_type)
{
	OulStatus *status;
	GList *l;

	g_return_val_if_fail(status_type != NULL, NULL);

	status = g_new0(OulStatus, 1);
	//OUL_DBUS_REGISTER_POINTER(status, OulStatus);

	status->type     = status_type;

	status->attr_values =
		g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
		(GDestroyNotify)oul_value_destroy);

	for (l = oul_status_type_get_attrs(status_type); l != NULL; l = l->next)
	{
		OulStatusAttr *attr = (OulStatusAttr *)l->data;
		OulValue *value = oul_status_attr_get_value(attr);
		OulValue *new_value = oul_value_dup(value);

		g_hash_table_insert(status->attr_values,
							g_strdup(oul_status_attr_get_id(attr)),
							new_value);
	}

	return status;
}

/*
 * TODO: If the OulStatus is in a OulPresence, then
 *       remove it from the OulPresence?
 */
void
oul_status_destroy(OulStatus *status)
{
	g_return_if_fail(status != NULL);

	g_hash_table_destroy(status->attr_values);

	//OUL_DBUS_UNREGISTER_POINTER(status);
	g_free(status);
}

static void
status_has_changed(OulStatus *status)
{
	OulStatus *old_status;

	/*
	 * If this status is exclusive, then we must be setting it to "active."
	 * Since we are setting it to active, we want to set the currently
	 * active status to "inactive."
	 */
	if (oul_status_is_exclusive(status))
	{
		//old_status = oul_presence_get_active_status(NULL);
		if (old_status != NULL && (old_status != status))
			old_status->active = FALSE;
	}
	else
		old_status = NULL;

	//notify_status_update(NULL, old_status, status);
}

void
oul_status_set_active(OulStatus *status, gboolean active)
{
	//oul_status_set_active_with_attrs_list(status, active, NULL);
}

/*
 * This used to parse the va_list directly, but now it creates a GList
 * and passes it to oul_status_set_active_with_attrs_list().  That
 * function was created because accounts.c needs to pass a GList of
 * attributes to the status API.
 */
void
oul_status_set_active_with_attrs(OulStatus *status, gboolean active, va_list args)
{
	GList *attrs = NULL;
	const gchar *id;
	gpointer data;

	while ((id = va_arg(args, const char *)) != NULL)
	{
		attrs = g_list_append(attrs, (char *)id);
		data = va_arg(args, void *);
		attrs = g_list_append(attrs, data);
	}
	//oul_status_set_active_with_attrs_list(status, active, attrs);
	g_list_free(attrs);
}

#ifdef adiofjasdjfoasdf
void
oul_status_set_active_with_attrs_list(OulStatus *status, gboolean active,
									   GList *attrs)
{
}
#endif

void
oul_status_set_attr_boolean(OulStatus *status, const char *id,
		gboolean value)
{
	OulValue *attr_value;

	g_return_if_fail(status != NULL);
	g_return_if_fail(id     != NULL);

	/* Make sure this attribute exists and is the correct type. */
	attr_value = oul_status_get_attr_value(status, id);
	g_return_if_fail(attr_value != NULL);
	g_return_if_fail(oul_value_get_type(attr_value) == OUL_TYPE_BOOLEAN);

	oul_value_set_boolean(attr_value, value);
}

void
oul_status_set_attr_int(OulStatus *status, const char *id, int value)
{
	OulValue *attr_value;

	g_return_if_fail(status != NULL);
	g_return_if_fail(id     != NULL);

	/* Make sure this attribute exists and is the correct type. */
	attr_value = oul_status_get_attr_value(status, id);
	g_return_if_fail(attr_value != NULL);
	g_return_if_fail(oul_value_get_type(attr_value) == OUL_TYPE_INT);

	//oul_value_set_int(attr_value, value);
}

void
oul_status_set_attr_string(OulStatus *status, const char *id,
		const char *value)
{
	OulValue *attr_value;

	g_return_if_fail(status != NULL);
	g_return_if_fail(id     != NULL);

	/* Make sure this attribute exists and is the correct type. */
	attr_value = oul_status_get_attr_value(status, id);
	/* This used to be g_return_if_fail, but it's failing a LOT, so
	 * let's generate a log error for now. */
	/* g_return_if_fail(attr_value != NULL); */
	if (attr_value == NULL) {
		oul_debug_error("status",
				 "Attempted to set status attribute '%s' for "
				 "status '%s', which is not legal.  Fix "
                                 "this!\n", id,
				 oul_status_type_get_name(oul_status_get_type(status)));
		return;
	}
	g_return_if_fail(oul_value_get_type(attr_value) == OUL_TYPE_STRING);

	/* XXX: Check if the value has actually changed. If it has, and the status
	 * is active, should this trigger 'status_has_changed'? */
	//oul_value_set_string(attr_value, value);
}

OulStatusType *
oul_status_get_type(const OulStatus *status)
{
	g_return_val_if_fail(status != NULL, NULL);

	return status->type;
}

const char *
oul_status_get_id(const OulStatus *status)
{
	g_return_val_if_fail(status != NULL, NULL);

	return oul_status_type_get_id(oul_status_get_type(status));
}

const char *
oul_status_get_name(const OulStatus *status)
{
	g_return_val_if_fail(status != NULL, NULL);

	return oul_status_type_get_name(oul_status_get_type(status));
}

gboolean
oul_status_is_independent(const OulStatus *status)
{
	g_return_val_if_fail(status != NULL, FALSE);

	return oul_status_type_is_independent(oul_status_get_type(status));
}

gboolean
oul_status_is_exclusive(const OulStatus *status)
{
	g_return_val_if_fail(status != NULL, FALSE);

	return oul_status_type_is_exclusive(oul_status_get_type(status));
}

gboolean
oul_status_is_active(const OulStatus *status)
{
	g_return_val_if_fail(status != NULL, FALSE);

	return status->active;
}


OulValue *
oul_status_get_attr_value(const OulStatus *status, const char *id)
{
	g_return_val_if_fail(status != NULL, NULL);
	g_return_val_if_fail(id     != NULL, NULL);

	return (OulValue *)g_hash_table_lookup(status->attr_values, id);
}

gboolean
oul_status_get_attr_boolean(const OulStatus *status, const char *id)
{
    return TRUE;
}

int
oul_status_get_attr_int(const OulStatus *status, const char *id)
{
	const OulValue *value;

	g_return_val_if_fail(status != NULL, 0);
	g_return_val_if_fail(id     != NULL, 0);

	if ((value = oul_status_get_attr_value(status, id)) == NULL)
		return 0;

	g_return_val_if_fail(oul_value_get_type(value) == OUL_TYPE_INT, 0);

	//return oul_value_get_int(value);
    return FALSE;
}

const char *
oul_status_get_attr_string(const OulStatus *status, const char *id)
{
	const OulValue *value;

	g_return_val_if_fail(status != NULL, NULL);
	g_return_val_if_fail(id     != NULL, NULL);

	if ((value = oul_status_get_attr_value(status, id)) == NULL)
		return NULL;

	g_return_val_if_fail(oul_value_get_type(value) == OUL_TYPE_STRING, NULL);

	return oul_value_get_string(value);
}

gint
oul_status_compare(const OulStatus *status1, const OulStatus *status2)
{
	OulStatusType *type1, *type2;
	int score1 = 0, score2 = 0;

	if ((status1 == NULL && status2 == NULL) ||
			(status1 == status2))
	{
		return 0;
	}
	else if (status1 == NULL)
		return 1;
	else if (status2 == NULL)
		return -1;

	type1 = oul_status_get_type(status1);
	type2 = oul_status_get_type(status2);

	if (oul_status_is_active(status1))
		score1 = primitive_scores[oul_status_type_get_primitive(type1)];

	if (oul_status_is_active(status2))
		score2 = primitive_scores[oul_status_type_get_primitive(type2)];

	if (score1 > score2)
		return -1;
	else if (score1 < score2)
		return 1;

	return 0;
}

/**************************************************************************
* Status subsystem
**************************************************************************/
static void
score_pref_changed_cb(const char *name, OulPrefType type,
					  gconstpointer value, gpointer data)
{
	int index = GPOINTER_TO_INT(data);

	primitive_scores[index] = GPOINTER_TO_INT(value);
}

void *
oul_status_get_handle(void) {
	static int handle;

	return &handle;
}

void
oul_status_init(void)
{
	void *handle = oul_status_get_handle;

	oul_prefs_add_none("/oul/status");
	oul_prefs_add_none("/oul/status/scores");

	/* some primitive status need to save to prefs.xml
	  * 1. idle:    it is idle
	  * 2. busy:  it doing some logic
	  * 3. unread_msg: when there are exist unread messages
	  */
  	oul_prefs_add_int("/oul/status/scores/offline",
			primitive_scores[OUL_STATUS_OFFLINE]);
	
  	oul_prefs_add_int("/oul/status/scores/online",
			primitive_scores[OUL_STATUS_ONLINE]);
	
	oul_prefs_add_int("/oul/status/scores/busy",
			primitive_scores[OUL_STATUS_BUSY]);
	
	oul_prefs_add_int("/oul/status/scores/unread_msg",
			primitive_scores[OUL_STATUS_UNREAD_MSG]);

	oul_prefs_connect_callback(handle, "/oul/status/scores/offline",
		score_pref_changed_cb, GINT_TO_POINTER(OUL_STATUS_ONLINE));

	oul_prefs_connect_callback(handle, "/oul/status/scores/online",
		score_pref_changed_cb, GINT_TO_POINTER(OUL_STATUS_ONLINE));

	oul_prefs_connect_callback(handle, "/oul/status/scores/busy",
		score_pref_changed_cb, GINT_TO_POINTER(OUL_STATUS_BUSY));

	oul_prefs_connect_callback(handle, "/oul/status/scores/unread_msg",
		score_pref_changed_cb, GINT_TO_POINTER(OUL_STATUS_UNREAD_MSG));

	oul_prefs_trigger_callback("/oul/status/scores/offline");
	oul_prefs_trigger_callback("/oul/status/scores/online");
	oul_prefs_trigger_callback("/oul/status/scores/busy");
	oul_prefs_trigger_callback("/oul/status/scores/unread_msg");
}

void
oul_status_uninit(void)
{
}
