#include "internal.h"

#ifdef USE_GSTREAMER
# include <gst/gst.h>
#endif /* USE_GSTREAMER */


#include "beasy.h"
#include "util.h"
#include "prefs.h"
#include "signals.h"
#include "notify.h"
#include "sound.h"

#include "gtksound.h"
#include "gtkpref_sound.h"

#ifdef USE_GSTREAMER
static gboolean gst_init_failed;
#endif /* USE_GSTREAMER */

static guint 	mute_connected_sounds_timeout = 0;
static gboolean mute_connected_sounds = FALSE;



struct beasy_sound_event {
	char *label;
	char *pref;
	char *def;
};

static const struct beasy_sound_event sounds[OUL_NUM_SOUNDS] = {
	{N_("Information Notification"),	"info_ntf", 	"info.wav"},
	{N_("Warning Notification"), 		"warn_ntf", 	"warn.wav"},
	{N_("Error Notification"), 			"error_ntf", 	"error.wav"},
	{N_("Fatal Notification"), 			"fatal_ntf", 	"fatal.wav"}
};


/****************************************************************
  *************private functions definition *****************************
  ***************************************************************/
static void beasy_sound_play_file(const char *filename);
static void beasy_sound_play_event(OulSoundEventID event);
static gboolean expire_old_child(gpointer data);


static OulSoundUiOps sound_ui_ops =
{
	beasy_sound_init,
	beasy_sound_uninit,
	beasy_sound_play_file,
	beasy_sound_play_event,
	NULL,
	NULL,
	NULL,
	NULL
};




/****************************************************************
  *************private functions definition *****************************
  ***************************************************************/
static void
beasy_sound_play_event(OulSoundEventID event)
{
	char *enable_pref;
	char *file_pref;

	if (mute_connected_sounds)
		return;

	if (event >= OUL_NUM_SOUNDS) {
		oul_debug_error("sound", "got request for unknown sound: %d\n", event);
		return;
	}

	enable_pref = g_strdup_printf(BEASY_PREFS_SND_ENABLED_ROOT "/%s", sounds[event].pref);
	file_pref = g_strdup_printf(BEASY_PREFS_SND_FILE_ROOT "/%s", sounds[event].pref);

	/* check NULL for sounds that don't have an option */
	if (oul_prefs_get_bool(enable_pref)) {
		char *filename = g_strdup(oul_prefs_get_path(file_pref));
		if(!filename || !strlen(filename)) {
			g_free(filename);
			filename = g_build_filename(PKGDATADIR, "sounds", sounds[event].def, NULL);
		}

		oul_sound_play_file(filename, NULL);
		g_free(filename);
	}

	g_free(enable_pref);
	g_free(file_pref);
}

#ifdef USE_GSTREAMER
static gboolean
bus_call_cb (GstBus *bus, GstMessage *msg, gpointer data)
{
	GstElement *play = data;
	GError *err = NULL;

	switch(GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_ERROR:
			gst_message_parse_error(msg, &err, NULL);
			oul_debug_error("gstreamer", "%s\n", err->message);
			g_error_free(err);
			/* fall-through and clean up */
		case GST_MESSAGE_EOS:
			gst_element_set_state(play, GST_STATE_NULL);
			gst_object_unref(GST_OBJECT(play));
			break;
		case GST_MESSAGE_WARNING:
			gst_message_parse_warning(msg, &err, NULL);
			oul_debug_warning("gstreamer", "%s\n", err->message);
			g_error_free(err);
			break;
		default:
			break;
	}
	return TRUE;
}
#endif


static void
beasy_sound_play_file(const char *filename)
{
	const char *method;
#ifdef USE_GSTREAMER
	float 		volume;
	char 		*uri;
	GstElement 	*sink = NULL;
	GstElement 	*play = NULL;
	GstBus 		*bus = NULL;
#endif

	if (oul_prefs_get_bool(BEASY_PREFS_SND_MUTE))
		return;

	method = oul_prefs_get_string(BEASY_PREFS_SND_METHOD);

	if (!strcmp(method, "none")) {
		return;
	} else if (!strcmp(method, "beep")) {
		gdk_beep();
		return;
	}

	if (!g_file_test(filename, G_FILE_TEST_EXISTS)) {
		oul_debug_error("gtksound", "sound file (%s) does not exist.\n", filename);
		return;
	}

	if (!strcmp(method, "custom")) {
		const char 	*sound_cmd;
		char 		*command;
		char 		*esc_filename;
		char 		**argv = NULL;
		GError 		*error = NULL;
		GPid 		pid;

		sound_cmd = oul_prefs_get_path(BEASY_PREFS_SND_COMMAND);

		if (!sound_cmd || *sound_cmd == '\0') {
			oul_debug_error("gtksound",
					 "'Command' sound method has been chosen, "
					 "but no command has been set.");
			return;
		}

		esc_filename = g_shell_quote(filename);

		if(strstr(sound_cmd, "%s"))
			command = oul_strreplace(sound_cmd, "%s", esc_filename);
		else
			command = g_strdup_printf("%s %s", sound_cmd, esc_filename);

		if (!g_shell_parse_argv(command, NULL, &argv, &error)) {
			oul_debug_error("gtksound", "error parsing command %s (%s)\n",
							   command, error->message);
			g_error_free(error);
			g_free(esc_filename);
			g_free(command);
			return;
		}

		if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
						  NULL, NULL, &pid, &error)) {
			oul_debug_error("gtksound", "sound command could not be launched: %s\n",
							   error->message);
			g_error_free(error);
		} else {
			oul_timeout_add_seconds(15, expire_old_child, GINT_TO_POINTER(pid));
		}

		g_strfreev(argv);
		g_free(esc_filename);
		g_free(command);
		return;
	}

#ifdef USE_GSTREAMER
	if (gst_init_failed)  /* Perhaps do gdk_beep instead? */
		return;
	volume = (float)(CLAMP(oul_prefs_get_int(BEASY_PREFS_SND_VOLUMN),0,100)) / 50;
	if (!strcmp(method, "automatic")) {
		if (oul_running_gnome()) {
			sink = gst_element_factory_make("gconfaudiosink", "sink");
		}
		if (!sink)
			sink = gst_element_factory_make("autoaudiosink", "sink");
		if (!sink) {
			oul_debug_error("sound", "Unable to create GStreamer audiosink.\n");
			return;
		}
	}
	else if (!strcmp(method, "esd")) {
			sink = gst_element_factory_make("esdsink", "sink");
		if (!sink) {
			oul_debug_error("sound", "Unable to create GStreamer audiosink.\n");
			return;
		}
	} else if (!strcmp(method, "alsa")) {
		sink = gst_element_factory_make("alsasink", "sink");
		if (!sink) {
			oul_debug_error("sound", "Unable to create GStreamer audiosink.\n");
			return;
		}
	}
	else {
		oul_debug_error("sound", "Unknown sound method '%s'\n", method);
		return;
	}

	play = gst_element_factory_make("playbin", "play");

	if (play == NULL) {
		return;
	}

	uri = g_strdup_printf("file://%s", filename);

	g_object_set(G_OBJECT(play), "uri", uri, "volume", volume,"audio-sink", sink, NULL);

	bus = gst_pipeline_get_bus(GST_PIPELINE(play));
	gst_bus_add_watch(bus, bus_call_cb, play);

	gst_element_set_state(play, GST_STATE_PLAYING);

	gst_object_unref(bus);
	g_free(uri);

#else /* #ifdef USE_GSTREAMER */

	gdk_beep();

#endif /* USE_GSTREAMER */
}

static gboolean
expire_old_child(gpointer data)
{
	pid_t pid = GPOINTER_TO_INT(data);

	if (waitpid(pid, NULL, WNOHANG | WUNTRACED) < 0) {
		if (errno == ECHILD)
			return FALSE;
		else
			oul_debug_warning("gtksound", "Child is ill, pid: %d (%s)\n", pid, strerror(errno));
	}

	if (kill(pid, SIGKILL) < 0)
		oul_debug_error("gtksound", "Killing process %d failed (%s)\n", pid, strerror(errno));

	return FALSE;
}

static void
sound_notify_cb(const gchar *source, const gchar *title, 
						const gchar *message, OulSoundEventID event)
{
	oul_sound_play_event(event);
}

/****************************************************************
  **************public functions definition *****************************
  ***************************************************************/
void
beasy_sound_init(void)
{

#ifdef USE_GSTREAMER
	GError *error = NULL;
#endif

#ifdef USE_GSTREAMER
	oul_debug_info("sound", "Initializing sound output drivers.\n");

#ifdef GST_CAN_DISABLE_FORKING
	gst_registry_fork_set_enabled (FALSE);
#endif
	if ((gst_init_failed = !gst_init_check(NULL, NULL, &error))) {
		oul_notify_error(NULL, _("GStreamer Failure"), 
					_("GStreamer failed to initialize."),
					error ? error->message : "");
		if (error) {
			g_error_free(error);
			error = NULL;
		}
	}
#endif /* USE_GSTREAMER */

	oul_signal_connect(oul_notify_get_handle(), "notify-info", beasy_sound_get_handle(), 
						OUL_CALLBACK(sound_notify_cb), GINT_TO_POINTER(OUL_SOUND_INFO_NTF));

	oul_signal_connect(oul_notify_get_handle(), "notify-warn", beasy_sound_get_handle(), 
					OUL_CALLBACK(sound_notify_cb), GINT_TO_POINTER(OUL_SOUND_WARN_NTF));

	oul_signal_connect(oul_notify_get_handle(), "notify-error", beasy_sound_get_handle(), 
					OUL_CALLBACK(sound_notify_cb), GINT_TO_POINTER(OUL_SOUND_ERROR_NTF));

	oul_signal_connect(oul_notify_get_handle(), "notify-fatal", beasy_sound_get_handle(), 
					OUL_CALLBACK(sound_notify_cb), GINT_TO_POINTER(OUL_SOUND_FATAL_NTF));


}


void
beasy_sound_uninit(void)
{
#ifdef USE_GSTREAMER
	if (!gst_init_failed)
			gst_deinit();
#endif
	
	oul_signals_disconnect_by_handle(beasy_sound_get_handle());
}



void *
beasy_sound_get_handle()
{
	static int handle;

	return &handle;
}

OulSoundUiOps *
beasy_sound_get_ui_ops(void)
{
	return &sound_ui_ops;
}

const char *
beasy_sound_get_event_label(OulSoundEventID event)
{
	if(event >= OUL_NUM_SOUNDS)
		return NULL;

	return sounds[event].label;
}

const char *
beasy_sound_get_event_option(OulSoundEventID event)
{
	if(event >= OUL_NUM_SOUNDS)
		return 0;

	return sounds[event].pref;
}

