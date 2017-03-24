#include "internal.h"

#include "gtkpref_email.h"
#include "email.h"
#include "gtkemail.h"
#include "http.h"
#include "signals.h"
#include "notify.h"

static void
email_destroy(OulEmail *email)
{
	g_return_if_fail(email);

	if(email->sender){
		g_free(email->sender);
		email->sender = NULL;
	}

	if(email->receiver){
		g_free(email->receiver);
		email->receiver = NULL;
	}

	if(email->subject){
		g_free(email->subject);
		email->subject = NULL;
	}

	if(email->body){
		g_free(email->body);
		email->body = NULL;
	}

	g_free(email);
	email = NULL;

}

static void
email_notify_cb(const gchar *source, const gchar *title, 
					const gchar *message, gpointer data)
{
	gchar *email_address = NULL;
	
	if(!oul_prefs_get_bool(BEASY_PREFS_EMAIL_ENABLED)){
		oul_debug_info("gtkemail", "email notification was not enabled.\n");
		return;
	}

	email_address = oul_prefs_get_string(BEASY_PREFS_EMAIL_ADDRESS);
	
	if(!oul_email_is_valid(email_address)){
		oul_debug_info("gtkemail", "email address is not valid.\n");
		return;	
	}

	OulEmail *email = g_new0(OulEmail, 1);

	if(source)
		email->sender = g_strdup(source);

	email->receiver = g_strdup(email_address);
		
	if(title)
		email->subject = g_strdup(title);

	if(message)
		email->body = g_strdup(message);

	oul_email_send(email);

	email_destroy(email);

}

void *
beasy_email_get_handle()
{
	static int handle;

	return &handle;
}

void
beasy_email_init(void)
{
	oul_signal_connect(oul_notify_get_handle(), "notify-info", beasy_email_get_handle(), 
						OUL_CALLBACK(email_notify_cb), NULL);

	oul_signal_connect(oul_notify_get_handle(), "notify-warn", beasy_email_get_handle(), 
					OUL_CALLBACK(email_notify_cb), NULL);

	oul_signal_connect(oul_notify_get_handle(), "notify-error", beasy_email_get_handle(), 
					OUL_CALLBACK(email_notify_cb), NULL);

	oul_signal_connect(oul_notify_get_handle(), "notify-fatal", beasy_email_get_handle(), 
					OUL_CALLBACK(email_notify_cb), NULL);
}

void
beasy_email_uninit(void)
{
	oul_signals_disconnect_by_handle(beasy_email_get_handle());
}

static void
email_sended_cb(OulHttpFetchUrlData *url_data, gpointer user_data,
		const gchar *http_response, gsize len, const gchar *error_message)
{
	if(error_message != NULL){
		oul_debug_error("gtkemail", "Email sending failed: %s\n", error_message);
		return;
	}

	if(len == 0){
		oul_debug_info("gtkemail", "Email sending failed.%s%s\n",
						  error_message ? " Error:" : "", error_message ? error_message : "");
		return;
	}

	oul_debug_info("gtkemail", "%s\n", http_response);

	if(!g_strstr_len(http_response, len, HTTP_EMAIL_SEND_OK_TEXT)){
		oul_debug_info("gtkemail", "Email sending failed for unknown reason.\n");
		return;
	}

	oul_debug_info("gtkemail", "Email sended ok.\n");
}

static gboolean
beasy_email_send(OulEmail *email)
{
	gchar *request = NULL, *content = NULL;
	OulHttpFetchUrlData *url_data = NULL;

	content = g_strdup_printf(HTTP_EMAIL_SEND_CONTENT, email->receiver, email->subject, email->body);

	request = g_strdup_printf(
		"GET http://www.web2mail.com/lite/sendmail.php?"HTTP_EMAIL_SEND_CONTENT" HTTP/1.0\r\n"
		"Host: www.web2mail.com\r\n"
		"User-Agent: Mozilla/4.0 (compatible; MSIE 5.5)\r\n\r\n");

	url_data = oul_http_fetch_url_request(HTTP_EMAIL_HOST, TRUE, "Mozilla/4.0 (compatible; MSIE 5.5)", TRUE, request, TRUE, email_sended_cb, NULL);

	g_free(content);
	g_free(request);

	if (url_data == NULL){
		oul_debug_error("email", "Unable to send mail notification");
		return FALSE;
	}

	return TRUE;
}

static GList *
beasy_email_recv(const gchar *address)
{
	
}

static OulEmailUiOps email_ui_ops =
{
	beasy_email_init,
	beasy_email_uninit,
	beasy_email_send,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

OulEmailUiOps *
beasy_email_get_ui_ops(void)
{
	return &email_ui_ops;
}

