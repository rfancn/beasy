#include "internal.h"

#include "email.h"

static OulEmailUiOps 	*email_ui_ops = NULL;

void *
oul_email_get_handle()
{
	static int handle;

	return &handle;
}

void
oul_email_set_ui_ops(OulEmailUiOps *ops)
{
	email_ui_ops = ops;
}

gboolean
oul_email_send(OulEmail *email)
{
	if(email_ui_ops && email_ui_ops->email_send) {
		email_ui_ops->email_send(email);
	}

}

GList *
oul_email_recv(const gchar *address)
{
	if(email_ui_ops && email_ui_ops->email_recv) {
		email_ui_ops->email_recv(address);
	}
}

void
oul_email_uninit()
{
	if(email_ui_ops && email_ui_ops->uninit)
		email_ui_ops->uninit();

	oul_signals_unregister_by_instance(oul_email_get_handle());
}

