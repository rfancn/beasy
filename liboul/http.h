#ifndef __OUL_HTTP_H
#define	__OUL_HTTP_H

typedef struct _OulHttpFetchUrlData OulHttpFetchUrlData;


/**
 * This is the signature used for functions that act as the callback
 * to oul_http_fetch_url() or oul_httpl_fetch_url_request().
 *
 * @param url_data      The same value that was returned when you called
 *                      oul_fetch_url() or oul_fetch_url_request().
 * @param user_data     The user data that your code passed into either
 *                      oul_http_fetch_url() or oul_http_fetch_url_request().
 * @param url_text      This will be NULL on error.  Otherwise this
 *                      will contain the contents of the URL.
 * @param len           0 on error, otherwise this is the length of buf.
 * @param error_message If something went wrong then this will contain
 *                      a descriptive error message, and buf will be
 *                      NULL and len will be 0.
 */
typedef void (*OulHttpFetchUrlCallback)(OulHttpFetchUrlData *url_data, gpointer user_data, 
		const gchar *url_text, gsize len, const gchar *error_message);


OulHttpFetchUrlData *	oul_http_fetch_url_request(const char *url, gboolean full,
												const char *user_agent, gboolean http11,
												const char *request, gboolean include_headers,
												OulHttpFetchUrlCallback callback, void *user_data);

OulHttpFetchUrlData * oul_http_fetch_url_request_len(const char *url, gboolean full, const char *user_agent, gboolean http11,
														const char *request, gboolean include_headers, gssize max_len, 
														OulHttpFetchUrlCallback callback, void *user_data);

void			oul_http_fetch_url_cancel(OulHttpFetchUrlData *gfud);
const char *	oul_url_decode(const char *str);
const char *	oul_url_encode(const char *str);
gboolean		oul_url_parse(const char *url, char **ret_host, int *ret_port, char **ret_path, char **ret_user, char **ret_passwd);









#endif

