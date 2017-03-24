#ifndef _OUL_PREFS_H
#define _OUL_PREFS_H

#include <glib.h>

/**
 * Pref data types.
 */
typedef enum _OulPrefType
{
	OUL_PREF_NONE,
	OUL_PREF_BOOLEAN,
	OUL_PREF_INT,
	OUL_PREF_STRING,
	OUL_PREF_STRING_LIST,
	OUL_PREF_PATH,
	OUL_PREF_PATH_LIST

}OulPrefType;



typedef void (* OulPrefCallback)(const char *name, OulPrefType type, 
            gconstpointer val, gpointer data);

typedef struct _PrefCb {
	OulPrefCallback func;
	gpointer data;
	guint id;
	void *handle;
}PrefCb;


#ifdef __cplusplus
extern "C" {
#endif

void*           oul_prefs_get_handler(void);

void            oul_prefs_init(void);
void            oul_prefs_uninit(void);
gboolean        oul_prefs_load(void);

void            oul_prefs_add_none(const char *name);
void            oul_prefs_add_bool(const char *name, gboolean value);
void            oul_prefs_add_int(const char *name, int value);
void            oul_prefs_add_string(const char *name, const char *value);
void            oul_prefs_add_string_list(const char *name, GList *value);

void            oul_prefs_set_generic(const char *name, gpointer value);
void            oul_prefs_set_bool(const char *name, gboolean value);
void            oul_prefs_set_int(const char *name, int value);
void            oul_prefs_set_string(const char *name, const char *value);
void            oul_prefs_set_string_list(const char *name, GList *value);
void            oul_prefs_set_path(const char *name, const char *value);
void            oul_prefs_set_path_list(const char *name, GList *value);

gboolean        oul_prefs_get_bool(const char *name);
int             oul_prefs_get_int(const char *name);
char*           oul_prefs_get_string(const char *name);
GList*          oul_prefs_get_string_list(const char *name);
const char*     oul_prefs_get_path(const char *name);
GList*          oul_prefs_get_path_list(const char *name);

void            oul_prefs_remove(const char *name);
void            oul_prefs_rename(const char *oldname, const char *newname);
void            oul_prefs_destroy(void);

gboolean        oul_prefs_exists(const char *name);
OulPrefType     oul_prefs_get_type(const char *name);
GList*          oul_prefs_get_children_names(const char *name); 

guint           oul_prefs_connect_callback(void *handle, const char *name, OulPrefCallback cb, gpointer data);

void            oul_prefs_disconnect_callback(guint callback_id); 
void            oul_prefs_disconnect_by_handle(void *handle);
void            oul_prefs_trigger_callback(const char *name);

 
#ifdef __cplusplus
}
#endif /* __cplusplus*/

#endif /* _OUL_PREFS_H */
