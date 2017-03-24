/*******************************************
  *******************************************
  ***Arch:
  ***               send signal 
  *** (1)client -----------> LIB level ---> GUI level
  ***               invoke API
  *** (2)client------------> LIB level---->GUI level
  ***
  ***  LIB level init:  core_init
  ***  GUI level init: beasy_ui_init
  ***
  ********************************************/
  

#include "internal.h"

#include "beasy.h"
#include "beasyaux.h"

#include "gtkprefs.h"
#include "gtkplugin.h"
#include "gtksound.h"
#include "gtkemail.h"
#include "gtkdocklet.h"
#include "gtkpref_plugin.h"

#include "core.h"
#include "util.h"
#include "prefs.h"
#include "debug.h"


static char *segfault_msg = NULL;
static GHashTable *ui_info = NULL;


static void beasy_ui_init(void);
static void beasy_quit(void);

static OulCoreUiOps core_ops =
{
	beasy_prefs_init,
	beasy_ui_init,
	beasy_quit,
    NULL,
    NULL,
    NULL
};

/**************************************************
 **************private functions*******************
 *************************************************/
static int
ui_main(void)
{
	GList *icons = NULL;
	GdkPixbuf *icon = NULL;
	char *icon_path;
	int i;
	struct {
		const char *dir;
		const char *filename;
	} icon_sizes[] = {
		{"16x16", 		"beasy16.png"},
		{"24x24", 		"beasy24.png"},
		{"32x32", 		"beasy32.png"},
		{"48x48", 		"beasy48.png"},
		{"scalable", 	"beasy.svg"}
	};
	
	/* use the nice PNG icon for all the windows */
	for(i=0; i<G_N_ELEMENTS(icon_sizes); i++) {
		icon_path = g_build_filename(PKGDATADIR, "icons", "hicolor", icon_sizes[i].dir, "apps", icon_sizes[i].filename, NULL);
		icon = gdk_pixbuf_new_from_file(icon_path, NULL);
		g_free(icon_path);
		if (icon) {
			icons = g_list_append(icons,icon);
		} else {
			oul_debug_error("ui_main",
					"Failed to load the default window icon (%spx version)!\n", icon_sizes[i].dir);
		}
	}
	if(NULL == icons) {
		oul_debug_error("ui_main", "Unable to load any size of default window icon!\n");
	} else {
		gtk_window_set_default_icon_list(icons);

		g_list_foreach(icons, (GFunc)g_object_unref, NULL);
		g_list_free(icons);
	}

	return 0;
}


static OulCoreUiOps*
beasy_core_get_ui_ops()
{
    return &core_ops;
}


/* UI level */
static void
beasy_ui_init(void)
{
    /*beasy_stock_init*/
    oul_debug_info("beasy", "beasy_ui_init\n");
	
	oul_notify_set_ui_ops(beasy_notify_get_ui_ops());
	oul_sound_set_ui_ops(beasy_sound_get_ui_ops());
	oul_email_set_ui_ops(beasy_email_get_ui_ops());

	beasy_notify_init();
	beasy_sound_init();
	beasy_email_init();

    beasy_stock_init();
    beasy_docklet_init();
}

static void
beasy_quit(void)
{
    oul_debug_info("beasy", "beasy quiting...\n");

	/* Save the plugins we have loaded for next time. */
	beasy_plugins_save();

	beasy_notify_uninit();
	beasy_sound_uninit();
	beasy_email_uninit();
	
	beasy_docklet_uninit();
	
	if(ui_info != NULL)
		g_hash_table_destroy(ui_info);

	/* and end it all... */
	gtk_main_quit();
}


/**************************************************
 **************public functions********************
 *************************************************/
int
main(int argc, char *argv[]){
    char *search_path;
    gboolean gui_check;

    /* initialize GThread before calling any Glib or GTK+ functions */
	if(!g_thread_supported()) g_thread_init(NULL);
   
    beasy_set_i18_support();
    beasy_process_signals();    
   
    if(beasy_parse_cmd_options(argc, argv))
        return 0;

    /* add gtkrc-2.0 to for gtk_init */
    search_path = g_build_filename(oul_user_dir(), "gtkrc-2.0", NULL);
    gtk_rc_add_default_file(search_path);
    g_free(search_path);

    /* check whether GUI can be initialized */
    gui_check = gtk_init_check(&argc, &argv);
    if(!gui_check){
        char *display = gdk_get_display();
        printf("%s %s\n", PACKAGE_NAME, VERSION); 

        g_warning("cannot open display: %s", display ? display : "unset");       
        g_free(display);
        
        goto end;     
    }
    
    #if GLIB_CHECK_VERSION(2, 2, 0)
    /* when set it, it will display this name in task list or error messages */
    g_set_application_name(_("Beasy"));
    #endif

    oul_core_set_ui_ops(beasy_core_get_ui_ops());
    oul_eventloop_set_ui_ops(beasy_eventloop_get_ui_ops());

    if(!oul_core_init(BEASY_UI)){
        fprintf(stderr,
                "Initialization of the liboul core failed. Dumping core.\n"
                "Please report this!\n");

        #ifdef HAVE_SIGNAL_H
        g_free(segfault_msg);
        #endif
        abort();
    }

	/* load plugins we had when we quit */
	oul_plugins_load_saved(BEASY_PREFS_PLUGIN_LOADED);

	ui_main();

	gtk_main();
	
    return 0;

end:
    #ifdef HAVE_SIGNAL_H
    g_free(segfault_msg);  
    #endif

    return 1;
}

