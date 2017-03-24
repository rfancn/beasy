#include "internal.h"

#include "util.h"
#include "proxy.h"
#include "http.h"


struct _OulHttpFetchUrlData
{
	OulHttpFetchUrlCallback callback;
	void *user_data;

	struct
	{
		char *user;
		char *passwd;
		char *address;
		int port;
		char *page;

	} website;

	char *url;
	int num_times_redirected;
	gboolean full;
	char *user_agent;
	gboolean http11;
	char *request;
	gsize request_written;
	gboolean include_headers;

	OulProxyConnectData *connect_data;
	int fd;
	guint inpa;

	gboolean got_headers;
	gboolean has_explicit_data_len;
	char *webdata;
	unsigned long len;
	unsigned long data_len;
	gssize max_len;
};

static void url_fetch_connect_cb(gpointer url_data, gint source, const gchar *error_message);

/**
 * The arguments to this function are similar to printf.
 */
static void
oul_http_fetch_url_error(OulHttpFetchUrlData *gfud, const char *format, ...)
{
	gchar *error_message;
	va_list args;

	va_start(args, format);
	error_message = g_strdup_vprintf(format, args);
	va_end(args);

	gfud->callback(gfud, gfud->user_data, NULL, 0, error_message);
	g_free(error_message);
	oul_http_fetch_url_cancel(gfud);
}

static gboolean
parse_redirect(const char *data, size_t data_len, OulHttpFetchUrlData *gfud)
{
	gchar *s;
	gchar *new_url, *temp_url, *end;
	gboolean full;
	int len;

	if ((s = g_strstr_len(data, data_len, "\nLocation: ")) == NULL)
		/* We're not being redirected */
		return FALSE;

	s += strlen("Location: ");
	end = strchr(s, '\r');

	/* Just in case :) */
	if (end == NULL)
		end = strchr(s, '\n');

	if (end == NULL)
		return FALSE;

	len = end - s;

	new_url = g_malloc(len + 1);
	strncpy(new_url, s, len);
	new_url[len] = '\0';

	full = gfud->full;

	if (*new_url == '/' || g_strstr_len(new_url, len, "://") == NULL)
	{
		temp_url = new_url;

		new_url = g_strdup_printf("%s:%d%s", gfud->website.address,
								  gfud->website.port, temp_url);

		g_free(temp_url);

		full = FALSE;
	}

	oul_debug_info("util", "Redirecting to %s\n", new_url);

	gfud->num_times_redirected++;
	if (gfud->num_times_redirected >= 5)
	{
		oul_http_fetch_url_error(gfud,
				_("Could not open %s: Redirected too many times"),
				gfud->url);
		return TRUE;
	}

	/*
	 * Try again, with this new location.  This code is somewhat
	 * ugly, but we need to reuse the gfud because whoever called
	 * us is holding a reference to it.
	 */
	g_free(gfud->url);
	gfud->url = new_url;
	gfud->full = full;
	g_free(gfud->request);
	gfud->request = NULL;

	oul_input_remove(gfud->inpa);
	gfud->inpa = 0;
	close(gfud->fd);
	gfud->fd = -1;
	gfud->request_written = 0;
	gfud->len = 0;
	gfud->data_len = 0;

	g_free(gfud->website.user);
	g_free(gfud->website.passwd);
	g_free(gfud->website.address);
	g_free(gfud->website.page);
	oul_url_parse(new_url, &gfud->website.address, &gfud->website.port,
				   &gfud->website.page, &gfud->website.user, &gfud->website.passwd);

	gfud->connect_data = oul_proxy_connect(NULL, 
			gfud->website.address, gfud->website.port,
			url_fetch_connect_cb, gfud);

	if (gfud->connect_data == NULL)
	{
		oul_http_fetch_url_error(gfud, _("Unable to connect to %s"),
				gfud->website.address);
	}

	return TRUE;
}

static size_t
parse_content_len(const char *data, size_t data_len)
{
	size_t content_len = 0;
	const char *p = NULL;

	/* This is still technically wrong, since headers are case-insensitive
	 * [RFC 2616, section 4.2], though this ought to catch the normal case.
	 * Note: data is _not_ nul-terminated.
	 */
	if(data_len > 16) {
		p = (strncmp(data, "Content-Length: ", 16) == 0) ? data : NULL;
		if(!p)
			p = (strncmp(data, "CONTENT-LENGTH: ", 16) == 0)
				? data : NULL;
		if(!p) {
			p = g_strstr_len(data, data_len, "\nContent-Length: ");
			if (p)
				p++;
		}
		if(!p) {
			p = g_strstr_len(data, data_len, "\nCONTENT-LENGTH: ");
			if (p)
				p++;
		}

		if(p)
			p += 16;
	}

	/* If we can find a Content-Length header at all, try to sscanf it.
	 * Response headers should end with at least \r\n, so sscanf is safe,
	 * if we make sure that there is indeed a \n in our header.
	 */
	if (p && g_strstr_len(p, data_len - (p - data), "\n")) {
		sscanf(p, "%" G_GSIZE_FORMAT, &content_len);
		oul_debug_misc("util", "parsed %" G_GSIZE_FORMAT "\n", content_len);
	}

	return content_len;
}


static void
url_fetch_recv_cb(gpointer url_data, gint source, OulInputCondition cond)
{
	OulHttpFetchUrlData *gfud = url_data;
	int len;
	char buf[4096];
	char *data_cursor;
	gboolean got_eof = FALSE;

	while((len = read(source, buf, sizeof(buf))) > 0) {

		if(gfud->max_len != -1 && (gfud->len + len) > gfud->max_len) {
			oul_http_fetch_url_error(gfud, _("Error reading from %s: response too long (%d bytes limit)"),
						    gfud->website.address, gfud->max_len);
			return;
		}

		/* If we've filled up our buffer, make it bigger */
		if((gfud->len + len) >= gfud->data_len) {
			while((gfud->len + len) >= gfud->data_len)
				gfud->data_len += sizeof(buf);

			gfud->webdata = g_realloc(gfud->webdata, gfud->data_len);
		}

		data_cursor = gfud->webdata + gfud->len;

		gfud->len += len;

		memcpy(data_cursor, buf, len);

		gfud->webdata[gfud->len] = '\0';

		if(!gfud->got_headers) {
			char *tmp;

			/* See if we've reached the end of the headers yet */
			if((tmp = strstr(gfud->webdata, "\r\n\r\n"))) {
				char * new_data;
				guint header_len = (tmp + 4 - gfud->webdata);
				size_t content_len;

				oul_debug_misc("util", "Response headers: '%.*s'\n",
					header_len, gfud->webdata);

				/* See if we can find a redirect. */
				if(parse_redirect(gfud->webdata, header_len, gfud))
					return;

				gfud->got_headers = TRUE;

				/* No redirect. See if we can find a content length. */
				content_len = parse_content_len(gfud->webdata, header_len);

				if(content_len == 0) {
					/* We'll stick with an initial 8192 */
					content_len = 8192;
				} else {
					gfud->has_explicit_data_len = TRUE;
				}


				/* If we're returning the headers too, we don't need to clean them out */
				if(gfud->include_headers) {
					gfud->data_len = content_len + header_len;
					gfud->webdata = g_realloc(gfud->webdata, gfud->data_len);
				} else {
					size_t body_len = 0;

					if(gfud->len > (header_len + 1))
						body_len = (gfud->len - header_len);

					content_len = MAX(content_len, body_len);

					new_data = g_try_malloc(content_len);
					if(new_data == NULL) {
						oul_debug_error("util",
								"Failed to allocate %" G_GSIZE_FORMAT " bytes: %s\n",
								content_len, g_strerror(errno));
						oul_http_fetch_url_error(gfud,
								_("Unable to allocate enough memory to hold "
								  "the contents from %s.  The web server may "
								  "be trying something malicious."),
								gfud->website.address);

						return;
					}

					/* We may have read part of the body when reading the headers, don't lose it */
					if(body_len > 0) {
						tmp += 4;
						memcpy(new_data, tmp, body_len);
					}

					/* Out with the old... */
					g_free(gfud->webdata);

					/* In with the new. */
					gfud->len = body_len;
					gfud->data_len = content_len;
					gfud->webdata = new_data;
				}
			}
		}

		if(gfud->has_explicit_data_len && gfud->len >= gfud->data_len) {
			got_eof = TRUE;
			break;
		}
	}

	if(len < 0) {
		if(errno == EAGAIN) {
			return;
		} else {
			oul_http_fetch_url_error(gfud, _("Error reading from %s: %s"),
					gfud->website.address, g_strerror(errno));
			return;
		}
	}

	if((len == 0) || got_eof) {
		gfud->webdata = g_realloc(gfud->webdata, gfud->len + 1);
		gfud->webdata[gfud->len] = '\0';

		gfud->callback(gfud, gfud->user_data, gfud->webdata, gfud->len, NULL);
		oul_http_fetch_url_cancel(gfud);
	}
}

static void
url_fetch_send_cb(gpointer data, gint source, OulInputCondition cond)
{
	OulHttpFetchUrlData *gfud;
	int len, total_len;

	gfud = data;

	total_len = strlen(gfud->request);

	len = write(gfud->fd, gfud->request + gfud->request_written,
			total_len - gfud->request_written);

	if (len < 0 && errno == EAGAIN)
		return;
	else if (len < 0) {
		oul_http_fetch_url_error(gfud, _("Error writing to %s: %s"),
				gfud->website.address, g_strerror(errno));
		return;
	}
	gfud->request_written += len;

	if (gfud->request_written < total_len)
		return;

	/* We're done writing our request, now start reading the response */
	oul_input_remove(gfud->inpa);
	gfud->inpa = oul_input_add(gfud->fd, OUL_INPUT_READ, url_fetch_recv_cb,
		gfud);
}

static void
url_fetch_connect_cb(gpointer url_data, gint source, const gchar *error_message)
{
	OulHttpFetchUrlData *gfud;

	gfud = url_data;
	gfud->connect_data = NULL;

	if (source == -1)
	{
		oul_http_fetch_url_error(gfud, _("Unable to connect to %s: %s"),
				(gfud->website.address ? gfud->website.address : ""), error_message);
		return;
	}

	gfud->fd = source;

	if (!gfud->request) {
		if (gfud->user_agent) {
			/* Host header is not forbidden in HTTP/1.0 requests, and HTTP/1.1
			 * clients must know how to handle the "chunked" transfer encoding.
			 * Oul doesn't know how to handle "chunked", so should always send
			 * the Host header regardless, to get around some observed problems
			 */
			gfud->request = g_strdup_printf(
				"GET %s%s HTTP/%s\r\n"
				"Connection: close\r\n"
				"User-Agent: %s\r\n"
				"Accept: */*\r\n"
				"Host: %s\r\n\r\n",
				(gfud->full ? "" : "/"),
				(gfud->full ? (gfud->url ? gfud->url : "") : (gfud->website.page ? gfud->website.page : "")),
				(gfud->http11 ? "1.1" : "1.0"),
				(gfud->user_agent ? gfud->user_agent : ""),
				(gfud->website.address ? gfud->website.address : ""));
		} else {
			gfud->request = g_strdup_printf(
				"GET %s%s HTTP/%s\r\n"
				"Connection: close\r\n"
				"Accept: */*\r\n"
				"Host: %s\r\n\r\n",
				(gfud->full ? "" : "/"),
				(gfud->full ? (gfud->url ? gfud->url : "") : (gfud->website.page ? gfud->website.page : "")),
				(gfud->http11 ? "1.1" : "1.0"),
				(gfud->website.address ? gfud->website.address : ""));
		}
	}

	oul_debug_misc("util", "Request:\n%s\n", gfud->request);

	gfud->inpa = oul_input_add(source, OUL_INPUT_WRITE,
								url_fetch_send_cb, gfud);
	url_fetch_send_cb(gfud, source, OUL_INPUT_WRITE);
}

OulHttpFetchUrlData *
oul_http_fetch_url_request(const char *url, gboolean full,
		const char *user_agent, gboolean http11,
		const char *request, gboolean include_headers,
		OulHttpFetchUrlCallback callback, void *user_data)
{
	return oul_http_fetch_url_request_len(url, full, user_agent, http11, 
						request, include_headers, -1,
						callback, user_data);
}

OulHttpFetchUrlData *
oul_http_fetch_url_request_len(const char *url, gboolean full,
		const char *user_agent, gboolean http11,
		const char *request, gboolean include_headers, gssize max_len,
		OulHttpFetchUrlCallback callback, void *user_data)
{
	OulHttpFetchUrlData *gfud;

	g_return_val_if_fail(url      != NULL, NULL);
	g_return_val_if_fail(callback != NULL, NULL);

	oul_debug_info("util",
			 "requested to fetch (%s), full=%d, user_agent=(%s), http11=%d\n",
			 url, full, user_agent?user_agent:"(null)", http11);

	gfud = g_new0(OulHttpFetchUrlData, 1);

	gfud->callback = callback;
	gfud->user_data  = user_data;
	gfud->url = g_strdup(url);
	gfud->user_agent = g_strdup(user_agent);
	gfud->http11 = http11;
	gfud->full = full;
	gfud->request = g_strdup(request);
	gfud->include_headers = include_headers;
	gfud->fd = -1;
	gfud->max_len = max_len;

	oul_url_parse(url, &gfud->website.address, &gfud->website.port,
				   &gfud->website.page, &gfud->website.user, &gfud->website.passwd);

	gfud->connect_data = oul_proxy_connect(NULL, gfud->website.address, gfud->website.port,
			url_fetch_connect_cb, gfud);

	if (gfud->connect_data == NULL)
	{
		oul_http_fetch_url_error(gfud, _("Unable to connect to %s"),
				gfud->website.address);
		return NULL;
	}

	return gfud;
}

void
oul_http_fetch_url_cancel(OulHttpFetchUrlData *gfud)
{
	if (gfud->connect_data != NULL)
		oul_proxy_connect_cancel(gfud->connect_data);

	if (gfud->inpa > 0)
		oul_input_remove(gfud->inpa);

	if (gfud->fd >= 0)
		close(gfud->fd);

	g_free(gfud->website.user);
	g_free(gfud->website.passwd);
	g_free(gfud->website.address);
	g_free(gfud->website.page);
	g_free(gfud->url);
	g_free(gfud->user_agent);
	g_free(gfud->request);
	g_free(gfud->webdata);

	g_free(gfud);
}




const char *
oul_url_decode(const char *str)
{
	static char buf[BUF_LEN];
	guint i, j = 0;
	char *bum;
	char hex[3];

	g_return_val_if_fail(str != NULL, NULL);

	/*
	 * XXX - This check could be removed and buf could be made
	 * dynamically allocated, but this is easier.
	 */
	if (strlen(str) >= BUF_LEN)
		return NULL;

	for (i = 0; i < strlen(str); i++) {

		if (str[i] != '%')
			buf[j++] = str[i];
		else {
			strncpy(hex, str + ++i, 2);
			hex[2] = '\0';

			/* i is pointing to the start of the number */
			i++;

			/*
			 * Now it's at the end and at the start of the for loop
			 * will be at the next character.
			 */
			buf[j++] = strtol(hex, NULL, 16);
		}
	}

	buf[j] = '\0';

	if (!g_utf8_validate(buf, -1, (const char **)&bum))
		*bum = '\0';

	return buf;
}

const char *
oul_url_encode(const char *str)
{
	const char *iter;
	static char buf[BUF_LEN];
	char utf_char[6];
	guint i, j = 0;

	g_return_val_if_fail(str != NULL, NULL);
	g_return_val_if_fail(g_utf8_validate(str, -1, NULL), NULL);

	iter = str;
	for (; *iter && j < (BUF_LEN - 1) ; iter = g_utf8_next_char(iter)) {
		gunichar c = g_utf8_get_char(iter);
		/* If the character is an ASCII character and is alphanumeric
		 * no need to escape */
		if (c < 128 && isalnum(c)) {
			buf[j++] = c;
		} else {
			int bytes = g_unichar_to_utf8(c, utf_char);
			for (i = 0; i < bytes; i++) {
				if (j > (BUF_LEN - 4))
					break;
				sprintf(buf + j, "%%%02x", utf_char[i] & 0xff);
				j += 3;
			}
		}
	}

	buf[j] = '\0';

	return buf;
}

/**************************************************************************
 * URI/URL Functions
 **************************************************************************/

/*
 * TODO: Should probably add a "gboolean *ret_ishttps" parameter that
 *       is set to TRUE if this URL is https, otherwise it is set to
 *       FALSE.  But that change will break the API.
 *
 *       This is important for Yahoo! web messenger login.  They now
 *       force https login, and if you access the web messenger login
 *       page via http then it redirects you to the https version, but
 *       oul_util_fetch_url() ignores the "https" and attempts to
 *       fetch the URL via http again, which gets redirected again.
 */
gboolean
oul_url_parse(const char *url, char **ret_host, int *ret_port,
			   char **ret_path, char **ret_user, char **ret_passwd)
{
	char scan_info[255];
	char port_str[6];
	int f;
	const char *at, *slash;
	const char *turl;
	char host[256], path[256], user[256], passwd[256];
	int port = 0;
	/* hyphen at end includes it in control set */
	static const char addr_ctrl[] = "A-Za-z0-9.-";
	static const char port_ctrl[] = "0-9";
	static const char page_ctrl[] = "A-Za-z0-9.~_/:*!@&%%?=+^-";
	static const char user_ctrl[] = "A-Za-z0-9.~_/*!&%%?=+^-";
	static const char passwd_ctrl[] = "A-Za-z0-9.~_/*!&%%?=+^-";

	g_return_val_if_fail(url != NULL, FALSE);

	if ((turl = oul_strcasestr(url, "http://")) != NULL)
	{
		turl += 7;
		url = turl;
	}
	else if ((turl = oul_strcasestr(url, "https://")) != NULL)
	{
		turl += 8;
		url = turl;
	}

	/* parse out authentication information if supplied */
	/* Only care about @ char BEFORE the first / */
	at = strchr(url, '@');
	slash = strchr(url, '/');
	if ((at != NULL) &&
			(((slash != NULL) && (strlen(at) > strlen(slash))) ||
			(slash == NULL))) {
		g_snprintf(scan_info, sizeof(scan_info),
					"%%255[%s]:%%255[%s]^@", user_ctrl, passwd_ctrl);
		f = sscanf(url, scan_info, user, passwd);

		if (f ==1 ) {
			/* No passwd, possibly just username supplied */
			g_snprintf(scan_info, sizeof(scan_info),
						"%%255[%s]^@", user_ctrl);
			f = sscanf(url, scan_info, user);
			*passwd = '\0';
		}

		url = at+1; /* move pointer after the @ char */
	} else {
		*user = '\0';
		*passwd = '\0';
	}

	g_snprintf(scan_info, sizeof(scan_info),
			   "%%255[%s]:%%5[%s]/%%255[%s]", addr_ctrl, port_ctrl, page_ctrl);

	f = sscanf(url, scan_info, host, port_str, path);

	if (f == 1)
	{
		g_snprintf(scan_info, sizeof(scan_info),
				   "%%255[%s]/%%255[%s]",
				   addr_ctrl, page_ctrl);
		f = sscanf(url, scan_info, host, path);
		g_snprintf(port_str, sizeof(port_str), "80");
	}

	if (f == 0)
		*host = '\0';

	if (f == 1)
		*path = '\0';

	sscanf(port_str, "%d", &port);

	if (ret_host != NULL) *ret_host = g_strdup(host);
	if (ret_port != NULL) *ret_port = port;
	if (ret_path != NULL) *ret_path = g_strdup(path);
	if (ret_user != NULL) *ret_user = g_strdup(user);
	if (ret_passwd != NULL) *ret_passwd = g_strdup(passwd);

	return ((*host != '\0') ? TRUE : FALSE);
}

