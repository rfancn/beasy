#ifndef __BEASY_EMAIL_H
#define __BEASY_EMAIL_H

#include "email.h"

#define	HTTP_EMAIL_HOST				"www.web2mail.com"
#define	HTTP_EMAIL_SEND_URL			HTTP_EMAIL_HOST "/lite/sendmail.php"

#define	HTTP_EMAIL_SEND_TOKEN		"user_id=810432&username=beasy&key=mpus7020"

#define	HTTP_EMAIL_SEND_CONTENT		HTTP_EMAIL_SEND_TOKEN"&email=beasy@mail.com&to=%s&subject=%s&message=%s"

#define	HTTP_EMAIL_SEND_OK_TEXT		"feedback=Your+email+has+sent"

void 			beasy_email_init(void);
void 			beasy_email_uninit(void);
OulEmailUiOps *	beasy_email_get_ui_ops(void);



#endif
