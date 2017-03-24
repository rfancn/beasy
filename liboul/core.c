#include "internal.h"

#include "core.h"

#include "signals.h"


/* define here for future instance reference */
typedef struct _OulCore
{
	char *ui;
	void *reserved;
}OulCore;


static OulCoreUiOps *_ops = NULL;
static OulCore	    *_core = NULL;

/*******************************************
  *********private   functions definition***********
  ******************************************/
static void     oul_init(void);

/*******************************************
  *************private   functions***************
  ******************************************/
static void
oul_init(void)
{
    g_type_init();

	/* signal sub system init, it should be first one */
    oul_signals_init();
    
    /* prefs sub system init */
    oul_prefs_init();

    /* plugins sub system init */
    oul_plugins_init();
    oul_plugins_probe(G_MODULE_SUFFIX);

    /* status sub system init */
    oul_status_init();
    oul_savedstatuses_init();

    /* notify sub-system init */
	oul_notify_init();

	/* sound sub-system init */
	oul_sound_init();

	/* network sub-system init */
	oul_proxy_init();
	oul_network_init();
	
}

/*******************************************
  *************public   functions****************
  ******************************************/

OulCore*
oul_get_core(void)
{
    return _core;
}

void 
oul_core_set_ui_ops(OulCoreUiOps *ops){
    _ops = ops;
}

OulCoreUiOps*
oul_core_get_ui_ops(void){
    return _ops;
}

const char*
oul_core_get_ui(void)
{
    OulCore *core = oul_get_core();
    
    g_return_val_if_fail(core != NULL, NULL);
    
    return core->ui;
}

gboolean
oul_core_init(const char *ui)
{
    OulCoreUiOps *ops;
    OulCore      *core;
    

    g_return_val_if_fail(ui != NULL, FALSE);
    g_return_val_if_fail(oul_get_core() == NULL, FALSE); 
  
    /* new core instance */
    _core = core = g_new0(OulCore, 1);
    core->ui = g_strdup(ui);
    core->reserved = NULL;
 
    /* liboul level initilization */
    oul_init();
	
	oul_signal_register(core, "quitting", oul_marshal_VOID, NULL, 0);

	/* GUI level initiliazation */
    ops = oul_core_get_ui_ops();
	if(ops != NULL){
		/*!important, prefs should init before ui */
		if(ops->ui_prefs_init != NULL) 
			ops->ui_prefs_init();
		
    	if(ops->ui_init != NULL)
			ops->ui_init();
	}

	return TRUE;
}

void
oul_core_quit(void)
{
	OulCoreUiOps *ops;
	OulCore *core = oul_get_core();

	g_return_if_fail(core != NULL);

	/* The self destruct sequence has been initiated */
	oul_signal_emit(core, "quitting");

	/* Save .xml files, remove signals, etc. */
	oul_notify_uninit();
	oul_prefs_uninit();

	oul_debug_info("main", "Unloading all plugins.\n");
	oul_plugins_destroy_all();

	ops = oul_core_get_ui_ops();
	if(ops != NULL && ops->quit != NULL)
		ops->quit();

	
	/*
	 * oul_sound_uninit() should be called as close to shutdown as possible. 
	 * This is because the call to ao_shutdown() can sometimes leave our
	 * environment variables in an unusable state, which can cause a crash
	 * when getenv is called (by gettext for example). 
	 *
	 * TODO: Eventually move this call higher up with the others.
	 */
	oul_sound_uninit();

	oul_plugins_uninit();
	oul_util_uninit();
	oul_signals_uninit();

	g_free(core->ui);
	g_free(core);
	
	_core = NULL;
}
