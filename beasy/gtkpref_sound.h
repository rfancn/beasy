#ifndef	__BEASY_GTKPREF_SOUND_H
#define	__BEASY_GTKPREF_SOUND_H

#include "gtkprefs.h"

#define		BEASY_PREFS_SND_ROOT 			BEASY_PREFS_ROOT"/sound"

#define		BEASY_PREFS_SND_ENABLED_ROOT	BEASY_PREFS_SND_ROOT "/enabled"
#define		BEASY_PREFS_SND_ENABLED_INFO	BEASY_PREFS_SND_ENABLED_ROOT "/info_ntf"
#define		BEASY_PREFS_SND_ENABLED_WARN	BEASY_PREFS_SND_ENABLED_ROOT "/warn_ntf"
#define		BEASY_PREFS_SND_ENABLED_ERROR	BEASY_PREFS_SND_ENABLED_ROOT "/error_ntf"
#define		BEASY_PREFS_SND_ENABLED_FATAL	BEASY_PREFS_SND_ENABLED_ROOT "/fatal_ntf"

#define		BEASY_PREFS_SND_FILE_ROOT		BEASY_PREFS_SND_ROOT "/file"
#define		BEASY_PREFS_SND_FILE_INFO		BEASY_PREFS_SND_FILE_ROOT "/info_ntf"
#define		BEASY_PREFS_SND_FILE_WARN		BEASY_PREFS_SND_FILE_ROOT "/warn_ntf"
#define		BEASY_PREFS_SND_FILE_ERROR		BEASY_PREFS_SND_FILE_ROOT "/error_ntf"
#define		BEASY_PREFS_SND_FILE_FATAL		BEASY_PREFS_SND_FILE_ROOT "/fatal_ntf"

#define		BEASY_PREFS_SND_METHOD			BEASY_PREFS_SND_ROOT "/method"
#define		BEASY_PREFS_SND_COMMAND			BEASY_PREFS_SND_ROOT "/command"
#define		BEASY_PREFS_SND_VOLUMN			BEASY_PREFS_SND_ROOT "/volume"
#define		BEASY_PREFS_SND_MUTE			BEASY_PREFS_SND_ROOT "/mute"


GtkWidget *	beasy_sound_prefpage_get(void);
void		beasy_sound_prefpage_destroy(void);

void		beasy_sound_prefs_init(void);



#endif

