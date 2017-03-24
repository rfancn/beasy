#ifndef __OUL_EMAIL_H
#define __OUL_EMAIL_H

typedef struct _OulEmail
{
	gchar *sender;
	gchar *receiver;
	gchar *subject;
	gchar *body;
}OulEmail;

typedef struct _OulEmailUiOps
{
	void		(*init)(void);
	void		(*uninit)(void);
	gboolean	(*email_send)(OulEmail *email);
	GList	 *	(*email_recv)(const gchar *address);

	void (*_oul_reserved1)(void);
	void (*_oul_reserved2)(void);
	void (*_oul_reserved3)(void);
	void (*_oul_reserved4)(void);
} OulEmailUiOps;


#endif
