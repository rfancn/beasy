#include "internal.h"

#include "signals.h"
#include "value.h"
#include "prefs.h"
#include "sound.h"

static time_t 			last_played[OUL_NUM_SOUNDS];
static OulSoundUiOps 	*sound_ui_ops = NULL;

void *
oul_sound_get_handle()
{
	static int handle;

	return &handle;
}

void
oul_sound_set_ui_ops(OulSoundUiOps *ops)
{
	if(sound_ui_ops && sound_ui_ops->uninit)
		sound_ui_ops->uninit();

	sound_ui_ops = ops;

	if(sound_ui_ops && sound_ui_ops->init)
		sound_ui_ops->init();
	
	sound_ui_ops = ops;
}

void
oul_sound_play_event(OulSoundEventID event)
{
	if (time(NULL) - last_played[event] < 2)
		return;
	
	last_played[event] = time(NULL);

	if(sound_ui_ops && sound_ui_ops->play_event) {
		sound_ui_ops->play_event(event);
	}
}

void
oul_sound_play_file(const char *filename)
{
	if(sound_ui_ops && sound_ui_ops->play_file)
		sound_ui_ops->play_file(filename);
}

void
oul_sound_init()
{
#if 0
	void *handle = oul_sounds_get_handle();
	oul_signal_register(handle, "playing-sound-event",
	                     oul_marshal_BOOLEAN__INT_POINTER,
	                     oul_value_new(OUL_TYPE_BOOLEAN), 1,
	                     oul_value_new(OUL_TYPE_INT));
#endif

	memset(last_played, 0, sizeof(last_played));
}


void
oul_sound_uninit()
{
	if(sound_ui_ops && sound_ui_ops->uninit)
		sound_ui_ops->uninit();

	oul_signals_unregister_by_instance(oul_sound_get_handle());
}

