#include "prefs.h"
#include "gtkpref_plugin.h"

void 
beasy_plugin_prefs_init(void)
{
	oul_prefs_add_none("/beasy/plugins");
	oul_prefs_add_path_list("/beasy/plugins/loaded",  '\0');
}

