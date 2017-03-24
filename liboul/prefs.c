#include "internal.h"

#include "prefs.h"
#include "util.h"
#include "xmlnode.h"

typedef struct _OulPref{
    OulPrefType       type;
    char                *name;
    union{
        gpointer        generic;
        gboolean        boolean;
        int             integer;
        char            *string;
        GList           *stringlist;
    }value;
    GSList               *callbacks;
    struct _OulPref   *parent; 
    struct _OulPref   *sibling; 
    struct _OulPref   *first_child; 
}OulPref;

typedef struct _OulPrefCb{
    OulPrefCallback   func;
    gpointer            data;
    guint               id;
    void                *handle;
}OulPrefCb;


static GHashTable   *prefs_hash = NULL;
static gboolean     prefs_loaded = FALSE;
static guint        save_timer = 0;
static GList        *prefs_stack = NULL;
static OulPref    	prefs = {
    OUL_PREF_NONE,
    NULL,
    {NULL},
    NULL,
    NULL,
    NULL,
    NULL
};

static OulPref*     find_pref(const char *name);
static void         schedule_prefs_save(void);
static void         prefs_save_cb(const char *name, OulPrefType type, 
                        gconstpointer val, gpointer user_data);
static gboolean     save_cb(gpointer data);
static void         sync_prefs(void);
static xmlnode*     prefs_to_xmlnode(void); 
static void         pref_to_xmlnode(xmlnode *parent, OulPref *pref);
static void         free_pref_value(OulPref *pref);  
static char*        pref_full_name(OulPref *pref);
static char*        get_path_basename(const char *name);
static void         prefs_start_element_handler(GMarkupParseContext *context,
                            const gchar *element_name,
                            const gchar **attribute_names,
                            const gchar **attribute_values,
                            gpointer user_data, GError **error);

static void         prefs_end_element_handler(GMarkupParseContext *context, 
                        const gchar *element_name,
                        gpointer user_data, GError **error);

static GMarkupParser prefs_parser = {
    prefs_start_element_handler,
    prefs_end_element_handler,
    NULL,
    NULL,
    NULL
};
/************************************************************
 ***********private functions********************************
 ***********************************************************/
static void
disco_callback_helper_handle(OulPref *pref, void *handle)
{
    GSList *cbs;
    OulPref *child;

    if(!pref) return;

    cbs = pref->callbacks;
    while(cbs != NULL){
        OulPrefCb *cb = cbs->data;
        if(cb->handle == handle) {
            pref->callbacks = g_slist_delete_link(pref->callbacks, cbs);
            g_free(cb);
            cbs = pref->callbacks;
        }else{
            cbs = cbs->next;
        }
    }
    
    for(child = pref->first_child; child; child = child->sibling)
        disco_callback_helper_handle(child, handle);
}

static void
do_callbacks(const char *name, OulPref *pref)
{
    GSList *cbs;
    OulPref *cb_pref;

    for(cb_pref = pref; cb_pref; cb_pref = cb_pref->parent){
        for(cbs = cb_pref->callbacks; cbs; cbs = cbs->next){
            OulPrefCb *cb = cbs->data;
            cb->func(name, pref->type, pref->value.generic, cb->data);
        }
    }
}

static void
prefs_start_element_handler(GMarkupParseContext *context, 
                    const gchar *element_name,
                    const gchar **attribute_names, 
                    const gchar **attribute_values,
                    gpointer user_data, GError **error)
{
    OulPrefType pref_type = OUL_PREF_NONE;
    int i;
    const char *pref_name = NULL, *pref_value = NULL;
    GString *pref_name_full;
    GList *tmp;

    if(strcmp(element_name, "pref") && strcmp(element_name, "item"))
        return;

	/* get it's name , type and value */
    for(i = 0; attribute_names[i]; i++) {
		if(!strcmp(attribute_names[i], "name")){
            pref_name = attribute_values[i]; 
        }else if(!strcmp(attribute_names[i], "type")){
            if(!strcmp(attribute_values[i], "bool"))
                pref_type = OUL_PREF_BOOLEAN;
            else if(!strcmp(attribute_values[i], "int" ))
                pref_type = OUL_PREF_INT;
            else if(!strcmp(attribute_values[i], "string" ))
                pref_type = OUL_PREF_STRING;
            else if(!strcmp(attribute_values[i], "stringlist" ))
                pref_type = OUL_PREF_STRING_LIST;
            else if(!strcmp(attribute_values[i], "path" ))
                pref_type = OUL_PREF_PATH;
            else if(!strcmp(attribute_values[i], "pathlist" ))
                pref_type = OUL_PREF_PATH_LIST;
            else
                return;
        }else if(!strcmp(attribute_names[i], "value")){
            pref_value = attribute_values[i];    
        }
    }

    /* element like: <item value = ""> */    
    if(!strcmp(element_name, "item")){
		OulPref *pref;
        
        pref_name_full = g_string_new("");
        for(tmp = prefs_stack; tmp; tmp = tmp->next){
            pref_name_full = g_string_prepend(pref_name_full, tmp->data);
            pref_name_full = g_string_prepend_c(pref_name_full, '/');
        }

        pref = find_pref(pref_name_full->str);
		if(pref){
            if(pref->type == OUL_PREF_STRING_LIST){
                pref->value.stringlist = g_list_append(pref->value.stringlist, g_strdup(pref_value));
            }else if(pref->type == OUL_PREF_PATH_LIST){
                pref->value.stringlist = g_list_append(pref->value.stringlist, 
                    g_filename_from_utf8(pref_value, -1, NULL, NULL, NULL));
            }
        }
        g_string_free(pref_name_full, TRUE);

    }else{ /* element like: <pref name = "" type = "" value = ""> */
		char *decoded;
        if(!pref_name || !strcmp(pref_name, "/"))  
            return;
       
        pref_name_full = g_string_new(pref_name); 
        for(tmp = prefs_stack; tmp; tmp = tmp->next){
            pref_name_full = g_string_prepend_c(pref_name_full, '/');
            pref_name_full = g_string_prepend(pref_name_full, tmp->data);
        }
        pref_name_full = g_string_prepend_c(pref_name_full, '/');
		
        switch(pref_type){
            case OUL_PREF_NONE:
                oul_prefs_add_none(pref_name_full->str);
                break;
            case OUL_PREF_BOOLEAN:
                oul_prefs_set_bool(pref_name_full->str, atoi(pref_value));
                break;
            case OUL_PREF_INT:
                oul_prefs_set_int(pref_name_full->str, atoi(pref_value));
                break;
            case OUL_PREF_STRING:
                oul_prefs_set_string(pref_name_full->str, pref_value);
                break;
            case OUL_PREF_STRING_LIST:
                oul_prefs_set_string_list(pref_name_full->str, NULL);
                break;
            case OUL_PREF_PATH:
                if(pref_value){
                    decoded = g_filename_from_utf8(pref_value, -1, NULL, NULL, NULL);
                    oul_prefs_set_path(pref_name_full->str, decoded);
                    g_free(decoded);
                }else{
                    oul_prefs_set_path(pref_name_full->str, NULL);
                }
                break;
            case OUL_PREF_PATH_LIST:
                oul_prefs_set_path_list(pref_name_full->str, NULL);
                break;
        }

        prefs_stack = g_list_prepend(prefs_stack, g_strdup(pref_name));
        g_string_free(pref_name_full, TRUE);
        
    }
}

static void
prefs_end_element_handler(GMarkupParseContext *context, const gchar *element_name,
        gpointer user_data, GError **error)
{
    if(prefs_stack && !strcmp(element_name, "pref")){
        g_free(prefs_stack->data);
        prefs_stack = g_list_delete_link(prefs_stack, prefs_stack);
    }
}

static char *
get_path_basename(const char *name)
{
    const char *c;

    if((c = strrchr(name, '/')) != NULL){
        return g_strdup(c+1);
    }

    return g_strdup(name);
}

static char *
get_path_dirname(const char *name)
{
    char *c, *str;
    str = g_strdup(name);
   
    if((c = strrchr(str, '/'))!=NULL){
        *c = '\0';
        
        if(*str == '\0'){
            g_free(str);
            str = g_strdup("/");
        } 
    }else{
        g_free(str);
        str = g_strdup("/");
    }
    
    return str;
}

static OulPref *
find_pref_parent(const char *name)
{
    char *parent_name = get_path_dirname(name);
    OulPref *ret = &prefs;
    
    if(strcmp(parent_name, "/")){
        ret = find_pref(parent_name);
    }

    g_free(parent_name); 
    return ret;
}

static OulPref * 
add_pref(OulPrefType type, const char *name)
{
    OulPref *parent;
    OulPref *me;
    OulPref *sibling;
    char *myname;

    parent = find_pref_parent(name);
    if(!parent)
      return NULL; 

    myname = get_path_basename(name);
    for(sibling = parent->first_child; sibling; sibling = sibling->sibling){
        if(!strcmp(sibling->name, myname)){
            g_free(myname);
            return NULL;
        }
    }


    me = g_new0(OulPref, 1);
    me->type = type;
    me->name = myname;

    me->parent = parent;
    if(parent->first_child) {
        for(sibling = parent->first_child; sibling->sibling; sibling = sibling->sibling);
        sibling->sibling = me;
    }else{
        parent->first_child = me;
    }

    g_hash_table_insert(prefs_hash, g_strdup(name), (gpointer)me);
    
    return me;
}

static OulPref * 
find_pref(const char *name){
    g_return_val_if_fail(name != NULL && name[0] == '/', NULL);
    
    if(name[1] == '\0') return &prefs;

   /* when we are initializing, the debug system is initialized
     * before the prefs system, but debug calls will end up
     * calling prefs functions, so we need to deal cleanly here.*/
    if(prefs_hash)
        return g_hash_table_lookup(prefs_hash, name);
    else
        return NULL;
}

static void
schedule_prefs_save(void)
{
    if(save_timer == 0)
        save_timer = oul_timeout_add_seconds(5, save_cb, NULL);
}

static void
prefs_save_cb(const char *name, OulPrefType type, gconstpointer val, gpointer user_data)
{
    /* make sure oul_prefs_load had been executed */
    if(!prefs_loaded){
        oul_debug_info("prefs", "oul_prefs_load still not executed\n"); 
        return;
    }

    oul_debug_misc("prefs", "%s changed, sheduling save.\n", name); 
    
    schedule_prefs_save();
}

static gboolean
save_cb(gpointer data)
{
    sync_prefs();
    save_timer = 0;
    return FALSE;
}

static void
sync_prefs(void)
{
    xmlnode *node;
    char *data;

    if(!prefs_loaded){
        oul_debug_error("prefs", "Attempted to save prefs before they were read!\n");
        return;
    }

    node = prefs_to_xmlnode();
    data = xmlnode_to_formatted_str(node, NULL);
    oul_util_write_data_to_file("prefs.xml", data, -1);
    g_free(data);
    xmlnode_free(node);
}

static xmlnode *
prefs_to_xmlnode(void)
{
    xmlnode *node;
    OulPref *pref, *child;

    pref = &prefs;

    /* create the root preference node */
    node = xmlnode_new("pref");
    xmlnode_set_attrib(node, "version", "1");
    xmlnode_set_attrib(node, "name", "/");

    for(child = pref->first_child; child != NULL; child = child->sibling)
        pref_to_xmlnode(node, child);

    return node;
}

static void
pref_to_xmlnode(xmlnode *parent, OulPref *pref)
{
    xmlnode *node, *childnode;
    OulPref *child;
    char buf[20];
    GList *cur;
    gchar *encoded;

    /* create a new node */
    node = xmlnode_new_child(parent, "pref");
    xmlnode_set_attrib(node, "name", pref->name);

    /* set the type of this node (if type==OUL_PREF_NONE then do nothing) */
    switch(pref->type){
        case OUL_PREF_INT:
            xmlnode_set_attrib(node, "type", "int");
            snprintf(buf, sizeof(buf), "%d", pref->value.integer);
            xmlnode_set_attrib(node, "value", buf);
            break;
        case OUL_PREF_STRING:
            xmlnode_set_attrib(node, "type", "string");
            xmlnode_set_attrib(node, "value", pref->value.string?pref->value.string:"");
            break;
        case OUL_PREF_STRING_LIST:
            xmlnode_set_attrib(node, "type", "stringlist"); 
            for(cur = pref->value.stringlist; cur != NULL; cur=cur->next){
                childnode = xmlnode_new_child(node, "item");
                xmlnode_set_attrib(childnode, "value", cur->data?cur->data:"");
            }
            break;
        case OUL_PREF_PATH:
            encoded = g_filename_to_utf8(pref->value.string ? pref->value.string : "", -1, NULL, NULL, NULL);
            xmlnode_set_attrib(node, "type", "path");
            xmlnode_set_attrib(node, "value", encoded);
            g_free(encoded);
            break;
        case OUL_PREF_PATH_LIST:
            xmlnode_set_attrib(node, "type", "pathlist");
            for(cur = pref->value.stringlist; cur != NULL; cur = cur->next){
                encoded = g_filename_to_utf8(cur->data?cur->data:"", -1, NULL, NULL, NULL);
                childnode = xmlnode_new_child(node, "item");
                xmlnode_set_attrib(childnode, "value", encoded);
                g_free(encoded);
            }
            break;
        case OUL_PREF_BOOLEAN:
            xmlnode_set_attrib(node, "type", "bool");
            snprintf(buf, sizeof(buf), "%d", pref->value.boolean);
            xmlnode_set_attrib(node, "value", buf);
            break;
        default:
            break;
    }


    /* all my children */
    for(child = pref->first_child; child != NULL; child = child->sibling)
        pref_to_xmlnode(node, child);

}

static void
remove_pref(OulPref *pref)
{
    char *name;
    GSList *list;

    if(!pref || pref == &prefs)
        return;

    while(pref->first_child)
        remove_pref(pref->first_child);

    if(pref->parent->first_child == pref){
        pref->parent->first_child = pref->sibling;
    }else{
        OulPref *sib = pref->parent->first_child;
        while(sib && sib->sibling != pref)
            sib = sib->sibling;

        if(sib)
            sib->sibling = pref->sibling;
    }    
    
    name = pref_full_name(pref);

    oul_debug_info("prefs", "removing pref %s\n", name);
    
    g_hash_table_remove(prefs_hash, name);
    g_free(name);

    free_pref_value(pref);

    while((list = pref->callbacks) != NULL){
        pref->callbacks = pref->callbacks->next;
        g_free(list->data);
        g_slist_free_1(list);
    }
    g_free(pref->name);
    g_free(pref);
}

static void
free_pref_value(OulPref *pref)
{
    switch(pref->type){
        case OUL_PREF_BOOLEAN:
            pref->value.boolean = FALSE;
            break;
        case OUL_PREF_INT:
            pref->value.integer = 0;
            break;
        case OUL_PREF_STRING:
        case OUL_PREF_PATH:
            g_free(pref->value.string);
            pref->value.string = NULL;
            break;
        case OUL_PREF_STRING_LIST:
        case OUL_PREF_PATH_LIST:
            {
                g_list_foreach(pref->value.stringlist, (GFunc)g_free, NULL);
                g_list_free(pref->value.stringlist);
            }
            break;
        case OUL_PREF_NONE:
            break;
    }
}

static char *
pref_full_name(OulPref *pref)
{
    GString *name;
    OulPref *parent;

    if(!pref) return NULL;
    
    if(pref = &prefs)
        return g_strdup("/");

    name = g_string_new(pref->name);
    parent = pref->parent;

    for(parent = pref->parent; parent && parent->name; parent = parent->parent){
        name = g_string_prepend_c(name, '/');
        name = g_string_prepend(name, parent->name);
    }
    name = g_string_prepend_c(name, '/');
    
    return g_string_free(name, FALSE);
}

/************************************************************
 ***********public functions*********************************
 ***********************************************************/
void *
oul_prefs_get_handle(void){
    static int handle;
    
    return &handle;
}

void
oul_prefs_init(void){
    oul_debug_info("prefs", "prefs sub system init...\n");

    void *handle = oul_prefs_get_handle();

    prefs_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    
    oul_prefs_connect_callback(handle, "/", prefs_save_cb, NULL); 

	/* basic pref elements */
	oul_prefs_add_none("/oul");

	oul_prefs_load();    
}

void
oul_prefs_uninit()
{
	if (save_timer != 0)
	{
		oul_timeout_remove(save_timer);
		save_timer = 0;
		sync_prefs();
	}

	oul_prefs_disconnect_by_handle(oul_prefs_get_handle());
}

static gboolean
disco_callback_helper(OulPref *pref, guint callback_id)
{
	GSList *cbs;
	OulPref *child;

	if(!pref)
		return FALSE;

	for(cbs = pref->callbacks; cbs; cbs = cbs->next) {
		PrefCb *cb = cbs->data;
		if(cb->id == callback_id) {
			pref->callbacks = g_slist_delete_link(pref->callbacks, cbs);
			g_free(cb);
			return TRUE;
		}
	}

	for(child = pref->first_child; child; child = child->sibling) {
		if(disco_callback_helper(child, callback_id))
			return TRUE;
	}

	return FALSE;
}


void
oul_prefs_disconnect_callback(guint callback_id)
{
	disco_callback_helper(&prefs, callback_id);
}


guint
oul_prefs_connect_callback(void *handle, const char *name, 
								OulPrefCallback func, gpointer data)
{
    OulPref *pref;
    OulPrefCb *cb;
    static guint cb_id = 0;
 
    g_return_val_if_fail(name != NULL, 0); 
    g_return_val_if_fail(func != NULL, 0); 

    pref = find_pref(name);    
    if(pref == NULL){
        oul_debug_error("prefs", "oul_prefs_connect_callback: Unknown pref %s\n", name);     
        return 0;
    }

    cb = g_new0(OulPrefCb, 1);
    cb->func = func;
    cb->data = data;
    cb->id = ++cb_id;
    cb->handle = handle;

    pref->callbacks = g_slist_append(pref->callbacks, cb);

    return cb->id;
}

void
oul_prefs_add_none(const char *name)
{
    add_pref(OUL_PREF_NONE, name);
}

void
oul_prefs_add_bool(const char *name, gboolean value)
{
    OulPref *pref = add_pref(OUL_PREF_BOOLEAN, name);
   
    if(!pref) return;

    pref->value.boolean = value;
}

void
oul_prefs_add_int(const char *name, int value)
{
    OulPref *pref = add_pref(OUL_PREF_INT, name); 

    if(!pref) return;

    pref->value.integer = value; 
}

void
oul_prefs_add_string(const char *name, const char *value)
{
    OulPref *pref;
    if(value != NULL && !g_utf8_validate(value, -1, NULL)){
        oul_debug_error("prefs", 
                "oul_prefs_add_string: cannot store invalid UTF8 for string pref %s\n", name);
        return;
    }
    
    pref = add_pref(OUL_PREF_STRING, name);

    if(!pref) return;
    
    pref->value.string = g_strdup(value);
}

void
oul_prefs_add_string_list(const char *name, GList *value)
{
    OulPref *pref = add_pref(OUL_PREF_STRING_LIST, name);
    GList *tmp;

    if(!pref) return;

    for(tmp = value; tmp; tmp = tmp->next) {
        if(tmp->data != NULL && !g_utf8_validate(tmp->data, -1, NULL)){
            oul_debug_error("prefs",
                    "oul_prefs_add_string_list:Skipping invalid UTF8 for string list pref $s\n", name);
            continue;
        }
        pref->value.stringlist = g_list_append(pref->value.stringlist,
            g_strdup(tmp->data));
    }

}

void
oul_prefs_add_path(const char *name, const char *value)
{
    OulPref *pref = add_pref(OUL_PREF_PATH, name);

    if(!pref)
        return;

    pref->value.string = g_strdup(value);
}

void
oul_prefs_add_path_list(const char *name, GList *value)
{
    OulPref *pref = add_pref(OUL_PREF_PATH_LIST, name);
    GList *tmp;

    if(!pref) return;

    for(tmp = value; tmp; tmp = tmp->next)
        pref->value.stringlist = g_list_append(pref->value.stringlist,
            g_strdup(tmp->data));

}

gboolean
oul_prefs_load(void)
{
    gchar *filename = g_build_filename(oul_user_dir(), "prefs.xml", NULL);
    gchar *contents = NULL;
    gsize length;
    GMarkupParseContext *context;
    GError *error = NULL;
    
    if(!filename) {
        prefs_loaded = TRUE;
        return FALSE;
    }

    /* try to read from home dir */
    oul_debug_info("prefs", "Reading %s\n", filename);        
    if(!g_file_get_contents(filename, &contents, &length, &error)){
        g_free(filename);
        g_error_free(error);
        
        error = NULL;

        /* try to read from sysconfdir */ 
        filename = g_build_filename(SYSCONFDIR, "oul", "prefs.xml", NULL);
        oul_debug_info("prefs", "Reading %s\n", filename);
        if(!g_file_get_contents(filename, &contents, &length, &error)){
            oul_debug_error("prefs", "Error reading prefs: %s\n", error->message);
            g_error_free(error);
            g_free(filename);
        
            prefs_loaded = TRUE; 

            return FALSE;
        }
    }
    
    if(length == 0){
        oul_debug_error("prefs", "prefs.xml content is empty.\n");
        g_free(filename);

        prefs_loaded = TRUE;
        return FALSE;
    }

    /* begin to parse prefs.xml */ 
    context = g_markup_parse_context_new(&prefs_parser, 0, NULL, NULL);
    if(!g_markup_parse_context_parse(context, contents, length, NULL)){
        g_markup_parse_context_free(context);
        g_free(contents);
        g_free(filename);
    
        prefs_loaded = TRUE;   
        return FALSE;
    }

   /* end parsing prefs.xml */ 
    if(!g_markup_parse_context_end_parse(context, NULL)){
        oul_debug_error("prefs", "Error parsing %s\n", filename);
        g_markup_parse_context_free(context);
        g_free(contents);
        g_free(filename);
    
        prefs_loaded = TRUE;   
        return FALSE;
    }

    oul_debug_info("prefs", "Finished reading %s\n", filename);
    g_markup_parse_context_free(context);
    g_free(contents);
    g_free(filename);
    prefs_loaded = TRUE;
    
    return TRUE;
}

void
oul_prefs_set_generic(const char *name, gpointer value)
{
    OulPref *pref = find_pref(name);

    if(!pref) {
        oul_debug_error("prefs",
                "oul_prefs_set_generic: Unknown pref %s\n", name);
        return;
    }

    pref->value.generic = value;
    do_callbacks(name, pref);
}

void
oul_prefs_set_bool(const char *name, gboolean value)
{
    OulPref *pref = find_pref(name);

    if(pref) {
        if(pref->type != OUL_PREF_BOOLEAN) {
            oul_debug_error("prefs",
                    "oul_prefs_set_bool: %s not a boolean pref\n", name);
            return;
        }

        if(pref->value.boolean != value) {
            pref->value.boolean = value;
            do_callbacks(name, pref);
        }
    } else {
        oul_prefs_add_bool(name, value);
    }
}

void
oul_prefs_set_int(const char *name, int value)
{
    OulPref *pref = find_pref(name);

    if(pref) {
        if(pref->type != OUL_PREF_INT) {
            oul_debug_error("prefs",
                    "oul_prefs_set_int: %s not an integer pref\n", name);
            return;
        }

        if(pref->value.integer != value) {
            pref->value.integer = value;
            do_callbacks(name, pref);
        }
    } else {
        oul_prefs_add_int(name, value);
    }
}

void
oul_prefs_set_string(const char *name, const char *value)
{
    OulPref *pref = find_pref(name);

    if(value != NULL && !g_utf8_validate(value, -1, NULL)) {
        oul_debug_error("prefs", "oul_prefs_set_string: Cannot store invalid UTF8 for string pref %s\n", name);
        return;
    }

    if(pref) {
        if(pref->type != OUL_PREF_STRING && pref->type != OUL_PREF_PATH) {
            oul_debug_error("prefs",
                    "oul_prefs_set_string: %s not a string pref\n", name);
            return;
        }

        if((value && !pref->value.string) ||
                (!value && pref->value.string) ||
                (value && pref->value.string &&
                 strcmp(pref->value.string, value))) {
            g_free(pref->value.string);
            pref->value.string = g_strdup(value);
            do_callbacks(name, pref);
        }
    } else {
        oul_prefs_add_string(name, value);
    }
}

void
oul_prefs_set_string_list(const char *name, GList *value)
{
    OulPref *pref = find_pref(name);
    if(pref) {
        GList *tmp;

        if(pref->type != OUL_PREF_STRING_LIST) {
            oul_debug_error("prefs",
                    "oul_prefs_set_string_list: %s not a string list pref\n",
                    name);
            return;
        }

        g_list_foreach(pref->value.stringlist, (GFunc)g_free, NULL);
        g_list_free(pref->value.stringlist);
        pref->value.stringlist = NULL;

        for(tmp = value; tmp; tmp = tmp->next) {
            if(tmp->data != NULL && !g_utf8_validate(tmp->data, -1, NULL)) {
                oul_debug_error("prefs", "oul_prefs_set_string_list: Skipping invalid UTF8 for string list pref %s\n", name);
                continue;
            }
            pref->value.stringlist = g_list_prepend(pref->value.stringlist,
                    g_strdup(tmp->data));
        }
        pref->value.stringlist = g_list_reverse(pref->value.stringlist);

        do_callbacks(name, pref);

    } else {
        oul_prefs_add_string_list(name, value);
    }
}

void
oul_prefs_set_path(const char *name, const char *value)
{
    OulPref *pref = find_pref(name);

    if(pref) {
        if(pref->type != OUL_PREF_PATH) {
            oul_debug_error("prefs",
                    "oul_prefs_set_path: %s not a path pref\n", name);
            return;
        }

        if((value && !pref->value.string) ||
                (!value && pref->value.string) ||
                (value && pref->value.string &&
                 strcmp(pref->value.string, value))) {
            g_free(pref->value.string);
            pref->value.string = g_strdup(value);
            do_callbacks(name, pref);
        }
    } else {
        oul_prefs_add_path(name, value);
    }
}

void
oul_prefs_set_path_list(const char *name, GList *value)
{
    OulPref *pref = find_pref(name);
    if(pref) {
        GList *tmp;

        if(pref->type != OUL_PREF_PATH_LIST) {
            oul_debug_error("prefs",
                    "oul_prefs_set_path_list: %s not a path list pref\n",
                    name);
            return;
        }

        g_list_foreach(pref->value.stringlist, (GFunc)g_free, NULL);
        g_list_free(pref->value.stringlist);
        pref->value.stringlist = NULL;

        for(tmp = value; tmp; tmp = tmp->next)
            pref->value.stringlist = g_list_prepend(pref->value.stringlist,
                    g_strdup(tmp->data));
        pref->value.stringlist = g_list_reverse(pref->value.stringlist);

        do_callbacks(name, pref);

    } else {
        oul_prefs_add_path_list(name, value);
    }
}

void
oul_prefs_trigger_callback(const char *name)
{
    OulPref *pref = find_pref(name);
    
    if(!pref){
        oul_debug_error("prefs", "oul_prefs_trigger_callback: Unknown pref %s\n", name);
        return;
    }

    do_callbacks(name, pref);
}

OulPrefType
oul_prefs_get_type(const char *name)
{
    OulPref *pref = find_pref(name);

    if(pref == NULL) 
        return OUL_PREF_NONE;

    return (pref->type);
}

GList*
oul_prefs_get_path_list(const char *name)
{
    OulPref *pref = find_pref(name);
    GList *ret = NULL, *tmp;

    if(!pref){
        oul_debug_error("prefs",
                "oul_prefs_get_path_list: Unknown pref %s\n", name);
		return NULL;
	}else if(pref->type != OUL_PREF_PATH_LIST){
        oul_debug_error("prefs",
                "oul_prefs_get_path_list: %s not a path list pre\n", name);
        return NULL;
    }

	for(tmp = pref->value.stringlist; tmp; tmp = tmp->next){
		ret = g_list_prepend(ret, g_strdup(tmp->data));
	}
	ret = g_list_reverse(ret);

	return ret;
}

int
oul_prefs_get_int(const char *name)
{
    OulPref *pref = find_pref(name);

    if(!pref) {
        oul_debug_error("prefs",
                "oul_prefs_get_int: Unknown pref %s\n", name);
        return 0;
    } else if(pref->type != OUL_PREF_INT) {
        oul_debug_error("prefs",
                "oul_prefs_get_int: %s not an integer pref\n", name);
        return 0;
    }

    return pref->value.integer;
}

char*
oul_prefs_get_string(const char *name)
{
    OulPref *pref = find_pref(name);

    if(!pref) {
        oul_debug_error("prefs",
                "oul_prefs_get_string: Unknown pref %s\n", name);
        return NULL;
    } else if(pref->type != OUL_PREF_STRING) {
        oul_debug_error("prefs",
                "oul_prefs_get_string: %s not a string pref\n", name);
        return NULL;
    }

    return pref->value.string;
}

const char *
oul_prefs_get_path(const char *name)
{
	OulPref *pref = find_pref(name);

	if(!pref) {
		oul_debug_error("prefs",
				"oul_prefs_get_path: Unknown pref %s\n", name);
		return NULL;
	} else if(pref->type != OUL_PREF_PATH) {
		oul_debug_error("prefs",
				"oul_prefs_get_path: %s not a path pref\n", name);
		return NULL;
	}

	return pref->value.string;
}


gboolean
oul_prefs_get_bool(const char *name)
{
    OulPref *pref = find_pref(name);

    if(!pref) {
        oul_debug_error("prefs",
                "oul_prefs_get_bool: Unknown pref %s\n", name);
        return FALSE;
    } else if(pref->type != OUL_PREF_BOOLEAN) {
        oul_debug_error("prefs",
                "oul_prefs_get_bool: %s not a boolean pref\n", name);
        return FALSE;
    }

    return pref->value.boolean;
}


GList *
oul_prefs_get_string_list(const char *name)
{
	OulPref *pref = find_pref(name);
	GList *ret = NULL, *tmp;

	if(!pref) {
		oul_debug_error("prefs",
				"oul_prefs_get_string_list: Unknown pref %s\n", name);
		return NULL;
	} else if(pref->type != OUL_PREF_STRING_LIST) {
		oul_debug_error("prefs",
				"oul_prefs_get_string_list: %s not a string list pref\n", name);
		return NULL;
	}

	for(tmp = pref->value.stringlist; tmp; tmp = tmp->next)
		ret = g_list_prepend(ret, g_strdup(tmp->data));
	ret = g_list_reverse(ret);

	return ret;
}


void
oul_prefs_disconnect_by_handle(void *handle)
{
    g_return_if_fail(handle != NULL);
    
    disco_callback_helper_handle(&prefs, handle);
}
