#ifndef _BEASY_SOUND_H_
#define _BEASY_SOUND_H_

#include "sound.h"

void * 			beasy_sound_get_handle(void);
const gchar *	beasy_sound_get_event_label(OulSoundEventID event);

void 			beasy_sound_init(void);
void 			beasy_sound_uninit(void);



#endif
