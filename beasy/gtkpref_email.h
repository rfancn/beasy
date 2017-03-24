#ifndef	__BEASY_GTKPREF_SOUND_H
#define	__BEASY_GTKPREF_SOUND_H

#include "gtkprefs.h"

#define		BEASY_PREFS_EMAIL_ROOT 			BEASY_PREFS_ROOT"/email"

#define		BEASY_PREFS_EMAIL_ENABLED		BEASY_PREFS_EMAIL_ROOT "/enabled"
#define		BEASY_PREFS_EMAIL_ADDRESS		BEASY_PREFS_EMAIL_ROOT "/email_address"

GtkWidget *	beasy_email_prefpage_get(void);
void		beasy_email_pref_init(void);

#endif

