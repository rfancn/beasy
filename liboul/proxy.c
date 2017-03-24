/**
 * @file proxy.c Proxy API
 * @ingroup core
 */

/* this is a little piece of code to handle proxy connection */
/* it is intended to : 
  * 1.  first handle http proxy, using the CONNECT command
  * 2.  provide an easy way to add socks support
  * 3.  draw women to it like flies to honey 
  */

#include "internal.h"

#include "eventloop.h"
#include "debug.h"
#include "cipher.h"
#include "dnsquery.h"
#include "notify.h"
#include "prefs.h"
#include "proxy.h"
#include "ntlm.h"
#include "util.h"

struct _OulProxyConnectData {
	void *handle;
	OulProxyConnectFunction connect_cb;
	gpointer data;
	gchar *host;
	int port;
	int fd;
	guint inpa;
	OulProxyInfo *gpi;
	OulDnsQueryData *query_data;

	/**
	 * This contains alternating length/char* values.  The char*
	 * values need to be freed when removed from the linked list.
	 */
	GSList *hosts;

	/*
	 * All of the following variables are used when establishing a
	 * connection through a proxy.
	 */
	guchar *write_buffer;
	gsize write_buf_len;
	gsize written_len;
	OulInputFunction read_cb;
	guchar *read_buffer;
	gsize read_buf_len;
	gsize read_len;
};

static const char * const socks5errors[] = {
	"succeeded\n",
	"general SOCKS server failure\n",
	"connection not allowed by ruleset\n",
	"Network unreachable\n",
	"Host unreachable\n",
	"Connection refused\n",
	"TTL expired\n",
	"Command not supported\n",
	"Address type not supported\n"
};

static OulProxyInfo *global_proxy_info = NULL;

static GSList *handles = NULL;

static void try_connect(OulProxyConnectData *connect_data);

/*
 * TODO: Eventually (GObjectification) this bad boy will be removed, because it is
 *       a gross fix for a crashy problem.
 */
#define OUL_PROXY_CONNECT_DATA_IS_VALID(connect_data) g_slist_find(handles, connect_data)

/**************************************************************************
 * Proxy structure API
 **************************************************************************/
OulProxyInfo *
oul_proxy_info_new(void)
{
	return g_new0(OulProxyInfo, 1);
}

void
oul_proxy_info_destroy(OulProxyInfo *info)
{
	g_return_if_fail(info != NULL);

	g_free(info->host);
	g_free(info->username);
	g_free(info->password);

	g_free(info);
}

void
oul_proxy_info_set_type(OulProxyInfo *info, OulProxyType type)
{
	g_return_if_fail(info != NULL);

	info->type = type;
}

void
oul_proxy_info_set_host(OulProxyInfo *info, const char *host)
{
	g_return_if_fail(info != NULL);

	g_free(info->host);
	info->host = g_strdup(host);
}

void
oul_proxy_info_set_port(OulProxyInfo *info, int port)
{
	g_return_if_fail(info != NULL);

	info->port = port;
}

void
oul_proxy_info_set_username(OulProxyInfo *info, const char *username)
{
	g_return_if_fail(info != NULL);

	g_free(info->username);
	info->username = g_strdup(username);
}

void
oul_proxy_info_set_password(OulProxyInfo *info, const char *password)
{
	g_return_if_fail(info != NULL);

	g_free(info->password);
	info->password = g_strdup(password);
}

OulProxyType
oul_proxy_info_get_type(const OulProxyInfo *info)
{
	g_return_val_if_fail(info != NULL, OUL_PROXY_NONE);

	return info->type;
}

const char *
oul_proxy_info_get_host(const OulProxyInfo *info)
{
	g_return_val_if_fail(info != NULL, NULL);

	return info->host;
}

int
oul_proxy_info_get_port(const OulProxyInfo *info)
{
	g_return_val_if_fail(info != NULL, 0);

	return info->port;
}

const char *
oul_proxy_info_get_username(const OulProxyInfo *info)
{
	g_return_val_if_fail(info != NULL, NULL);

	return info->username;
}

const char *
oul_proxy_info_get_password(const OulProxyInfo *info)
{
	g_return_val_if_fail(info != NULL, NULL);

	return info->password;
}

/**************************************************************************
 * Global Proxy API
 **************************************************************************/
OulProxyInfo *
oul_global_proxy_get_info(void)
{
	return global_proxy_info;
}

static OulProxyInfo *
oul_gnome_proxy_get_info(void)
{
	static OulProxyInfo info = {0, NULL, 0, NULL, NULL};
	gboolean use_http_proxy = FALSE;
	gchar *tmp;

	tmp = g_find_program_in_path("gconftool-2");
	if (tmp == NULL)
		return oul_global_proxy_get_info();

	/* Check whether to use a proxy. */
	g_free(tmp);
	if (!g_spawn_command_line_sync("gconftool-2 -g /system/proxy/mode",
			&tmp, NULL, NULL, NULL)){
		return oul_global_proxy_get_info();
	}

	if (!strcmp(tmp, "none\n")) {
		info.type = OUL_PROXY_NONE;
		g_free(tmp);
		return &info;
	}

	if (strcmp(tmp, "manual\n")) {
		/* Unknown setting.  Fallback to using our global proxy settings. */
		g_free(tmp);
		return oul_global_proxy_get_info();
	}

	/* Free the old fields */
	if (info.host) {
		g_free(info.host);
		info.host = NULL;
	}
	if (info.username) {
		g_free(info.username);
		info.username = NULL;
	}
	if (info.password) {
		g_free(info.password);
		info.password = NULL;
	}

	/* check whether use_http_proxy set */
	g_free(tmp);
	if (!g_spawn_command_line_sync("gconftool-2 -g /system/http_proxy/use_http_proxy",
			&tmp, NULL, NULL, NULL))
		return oul_global_proxy_get_info();


	if (!strcmp(tmp, "true\n"))
		use_http_proxy = TRUE;

	/* if not set use_http_proxy, test whether socks proxy had been set */
	g_free(tmp);
	if(!g_spawn_command_line_sync("gconftool-2 -g /system/proxy/socks_host",
			&info.host, NULL, NULL, NULL))
			return oul_global_proxy_get_info();

	g_strchomp(info.host);	
	if (!use_http_proxy && info.host == '\0')
		return oul_global_proxy_get_info();

	/* if don't use http proxy, and sock proxy host not null, then it should be socks5 proxy */
	if (!use_http_proxy && *info.host != '\0') {
		info.type = OUL_PROXY_SOCKS5;
		if (!g_spawn_command_line_sync("gconftool-2 -g /system/proxy/socks_port",
				&tmp, NULL, NULL, NULL))
		{
			g_free(info.host);
			info.host = NULL;
		
			return oul_global_proxy_get_info();
		}
		info.port = atoi(tmp);
		g_free(tmp);
	} else {
		/* If we get this far then we know we're using an HTTP proxy */
		info.type = OUL_PROXY_HTTP;
		
		if(info.host) g_free(info.host);
		if (!g_spawn_command_line_sync("gconftool-2 -g /system/http_proxy/host",
					&info.host, NULL, NULL, NULL))
			return oul_global_proxy_get_info();
		g_strchomp(info.host);
		
		if (*info.host == '\0')
		{
			oul_debug_info("proxy", "Gnome proxy settings are set to "
					"'manual' but no suitable proxy server is specified.  Using "
					"Beasy's proxy settings instead.\n");
			g_free(info.host);
			info.host = NULL;
			return oul_global_proxy_get_info();
		}

		if (!g_spawn_command_line_sync("gconftool-2 -g /system/http_proxy/authentication_user",
					&info.username, NULL, NULL, NULL))
		{
			g_free(info.host);
			info.host = NULL;

			return oul_global_proxy_get_info();
		}
		g_strchomp(info.username);

		if (!g_spawn_command_line_sync("gconftool-2 -g /system/http_proxy/authentication_password",
					&info.password, NULL, NULL, NULL))
		{
			g_free(info.host);
			info.host = NULL;
			g_free(info.username);
			info.username = NULL;
			return oul_global_proxy_get_info();
		}
		g_strchomp(info.password);

		if (!g_spawn_command_line_sync("gconftool-2 -g /system/http_proxy/port",
					&tmp, NULL, NULL, NULL))
		{
			g_free(info.host);
			info.host = NULL;
			g_free(info.username);
			info.username = NULL;
			g_free(info.password);
			info.password = NULL;
			return oul_global_proxy_get_info();
		}
		info.port = atoi(tmp);
		g_free(tmp);
	}

	return &info;

}

/**************************************************************************
 * Proxy API
 **************************************************************************/

/**
 * Whoever calls this needs to have called
 * oul_proxy_connect_data_disconnect() beforehand.
 */
static void
oul_proxy_connect_data_destroy(OulProxyConnectData *connect_data)
{
	handles = g_slist_remove(handles, connect_data);

	if (connect_data->query_data != NULL)
		oul_dnsquery_destroy(connect_data->query_data);

	while (connect_data->hosts != NULL)
	{
		/* Discard the length... */
		connect_data->hosts = g_slist_remove(connect_data->hosts, connect_data->hosts->data);
		/* Free the address... */
		g_free(connect_data->hosts->data);
		connect_data->hosts = g_slist_remove(connect_data->hosts, connect_data->hosts->data);
	}

	g_free(connect_data->host);
	g_free(connect_data);
}

/**
 * Free all information dealing with a connection attempt and
 * reset the connect_data to prepare for it to try to connect
 * to another IP address.
 *
 * If an error message is passed in, then we know the connection
 * attempt failed.  If the connection attempt failed and
 * connect_data->hosts is not empty then we try the next IP address.
 * If the connection attempt failed and we have no more hosts
 * try try then we call the callback with the given error message,
 * then destroy the connect_data.
 *
 * @param error_message An error message explaining why the connection
 *        failed.  This will be passed to the callback function
 *        specified in the call to oul_proxy_connect().  If the
 *        connection was successful then pass in null.
 */
static void
oul_proxy_connect_data_disconnect(OulProxyConnectData *connect_data, const gchar *error_message)
{
	if (connect_data->inpa > 0)
	{
		oul_input_remove(connect_data->inpa);
		connect_data->inpa = 0;
	}

	if (connect_data->fd >= 0)
	{
		close(connect_data->fd);
		connect_data->fd = -1;
	}

	g_free(connect_data->write_buffer);
	connect_data->write_buffer = NULL;

	g_free(connect_data->read_buffer);
	connect_data->read_buffer = NULL;

	if (error_message != NULL)
	{
		oul_debug_info("proxy", "Connection attempt failed: %s\n",
				error_message);
		if (connect_data->hosts != NULL)
			try_connect(connect_data);
		else
		{
			/* Everything failed!  Tell the originator of the request. */
			connect_data->connect_cb(connect_data->data, -1, error_message);
			oul_proxy_connect_data_destroy(connect_data);
		}
	}
}

/**
 * This calls oul_proxy_connect_data_disconnect(), but it lets you
 * specify the error_message using a printf()-like syntax.
 */
static void
oul_proxy_connect_data_disconnect_formatted(OulProxyConnectData *connect_data, const char *format, ...)
{
	va_list args;
	gchar *tmp;

	va_start(args, format);
	tmp = g_strdup_vprintf(format, args);
	va_end(args);

	oul_proxy_connect_data_disconnect(connect_data, tmp);
	g_free(tmp);
}

static void
oul_proxy_connect_data_connected(OulProxyConnectData *connect_data)
{
	connect_data->connect_cb(connect_data->data, connect_data->fd, NULL);

	/*
	 * We've passed the file descriptor to the protocol, so it's no longer
	 * our responsibility, and we should be careful not to free it when
	 * we destroy the connect_data.
	 */
	connect_data->fd = -1;

	oul_proxy_connect_data_disconnect(connect_data, NULL);
	oul_proxy_connect_data_destroy(connect_data);
}

static void
socket_ready_cb(gpointer data, gint source, OulInputCondition cond)
{
	OulProxyConnectData *connect_data = data;
	int error = 0;
	int ret;

	/* If the socket-connected message had already been triggered when connect_data
 	 * was destroyed via oul_proxy_connect_cancel(), we may get here with a freed connect_data.
 	 */
	if (!OUL_PROXY_CONNECT_DATA_IS_VALID(connect_data))
		return;

	oul_debug_info("proxy", "Connected to %s:%d.\n",
					connect_data->host, connect_data->port);

	/*
	 * oul_input_get_error after a non-blocking connect returns -1 if something is
	 * really messed up (bad descriptor, usually). Otherwise, it returns 0 and
	 * error holds what connect would have returned if it blocked until now.
	 * Thus, error == 0 is success, error == EINPROGRESS means "try again",
	 * and anything else is a real error.
	 *
	 * (error == EINPROGRESS can happen after a select because the kernel can
	 * be overly optimistic sometimes. select is just a hint that you might be
	 * able to do something.)
	 */
	ret = oul_input_get_error(connect_data->fd, &error);

	if (ret == 0 && error == EINPROGRESS) {
		/* No worries - we'll be called again later */
		/* TODO: Does this ever happen? */
		oul_debug_info("proxy", "(ret == 0 && error == EINPROGRESS)\n");
		return;
	}

	if (ret != 0 || error != 0) {
		if (ret != 0)
			error = errno;
		oul_debug_info("proxy", "Error connecting to %s:%d (%s).\n",
						connect_data->host, connect_data->port, g_strerror(error));

		oul_proxy_connect_data_disconnect(connect_data, g_strerror(error));
		return;
	}

	oul_proxy_connect_data_connected(connect_data);
}

static gboolean
clean_connect(gpointer data)
{
	oul_proxy_connect_data_connected(data);

	return FALSE;
}

static void
proxy_connect_none(OulProxyConnectData *connect_data, struct sockaddr *addr, socklen_t addrlen)
{
	int flags;

	oul_debug_info("proxy", "Connecting to %s:%d with no proxy\n",
			connect_data->host, connect_data->port);

	connect_data->fd = socket(addr->sa_family, SOCK_STREAM, 0);
	if (connect_data->fd < 0)
	{
		oul_proxy_connect_data_disconnect_formatted(connect_data,
				_("Unable to create socket:\n%s"), g_strerror(errno));
		return;
	}

	flags = fcntl(connect_data->fd, F_GETFL);
	fcntl(connect_data->fd, F_SETFL, flags | O_NONBLOCK);
	fcntl(connect_data->fd, F_SETFD, FD_CLOEXEC);

	if (connect(connect_data->fd, addr, addrlen) != 0)
	{
		if ((errno == EINPROGRESS) || (errno == EINTR))
		{
			oul_debug_info("proxy", "Connection in progress\n");
			connect_data->inpa = oul_input_add(connect_data->fd,
					OUL_INPUT_WRITE, socket_ready_cb, connect_data);
		}
		else
		{
			oul_proxy_connect_data_disconnect(connect_data, g_strerror(errno));
		}
	}
	else
	{
		/*
		 * The connection happened IMMEDIATELY... strange, but whatever.
		 */
		int error = ETIMEDOUT;
		int ret;

		oul_debug_info("proxy", "Connected immediately.\n");

		ret = oul_input_get_error(connect_data->fd, &error);
		if ((ret != 0) || (error != 0))
		{
			if (ret != 0)
				error = errno;
			oul_proxy_connect_data_disconnect(connect_data, g_strerror(error));
			return;
		}

		/*
		 * We want to call the "connected" callback eventually, but we
		 * don't want to call it before we return, just in case.
		 */
		oul_timeout_add(10, clean_connect, connect_data);
	}
}

/**
 * This is a utility function used by the HTTP, SOCKS4 and SOCKS5
 * connect functions.  It writes data from a buffer to a socket.
 * When all the data is written it sets up a watcher to read a
 * response and call a specified function.
 */
static void
proxy_do_write(gpointer data, gint source, OulInputCondition cond)
{
	OulProxyConnectData *connect_data;
	const guchar *request;
	gsize request_len;
	int ret;

	connect_data = data;
	request = connect_data->write_buffer + connect_data->written_len;
	request_len = connect_data->write_buf_len - connect_data->written_len;

	ret = write(connect_data->fd, request, request_len);
	if (ret <= 0)
	{
		if (errno == EAGAIN)
			/* No worries */
			return;

		/* Error! */
		oul_proxy_connect_data_disconnect(connect_data, g_strerror(errno));
		return;
	}
	if (ret < request_len) {
		connect_data->written_len += ret;
		return;
	}

	/* We're done writing data!  Wait for a response. */
	g_free(connect_data->write_buffer);
	connect_data->write_buffer = NULL;
	oul_input_remove(connect_data->inpa);
	connect_data->inpa = oul_input_add(connect_data->fd,
			OUL_INPUT_READ, connect_data->read_cb, connect_data);
}

#define HTTP_GOODSTRING "HTTP/1.0 200"
#define HTTP_GOODSTRING2 "HTTP/1.1 200"

/**
 * We're using an HTTP proxy for a non-port 80 tunnel.  Read the
 * response to the CONNECT request.
 */
static void
http_canread(gpointer data, gint source, OulInputCondition cond)
{
	int len, headers_len, status = 0;
	gboolean error;
	OulProxyConnectData *connect_data = data;
	char *p;
	gsize max_read;

	if (connect_data->read_buffer == NULL) {
		connect_data->read_buf_len = 8192;
		connect_data->read_buffer = g_malloc(connect_data->read_buf_len);
		connect_data->read_len = 0;
	}

	p = (char *)connect_data->read_buffer + connect_data->read_len;
	max_read = connect_data->read_buf_len - connect_data->read_len - 1;

	len = read(connect_data->fd, p, max_read);

	if (len == 0) {
		oul_proxy_connect_data_disconnect(connect_data,
				_("Server closed the connection."));
		return;
	}

	if (len < 0) {
		if (errno == EAGAIN)
			/* No worries */
			return;

		/* Error! */
		oul_proxy_connect_data_disconnect_formatted(connect_data,
				_("Lost connection with server:\n%s"), g_strerror(errno));
		return;
	}

	connect_data->read_len += len;
	p[len] = '\0';

	p = g_strstr_len((const gchar *)connect_data->read_buffer,
			connect_data->read_len, "\r\n\r\n");
	if (p != NULL) {
		*p = '\0';
		headers_len = (p - (char *)connect_data->read_buffer) + 4;
	} else if(len == max_read)
		headers_len = len;
	else
		return;

	error = strncmp((const char *)connect_data->read_buffer, "HTTP/", 5) != 0;
	if (!error) {
		int major;
		p = (char *)connect_data->read_buffer + 5;
		major = strtol(p, &p, 10);
		error = (major == 0) || (*p != '.');
		if(!error) {
			int minor;
			p++;
			minor = strtol(p, &p, 10);
			error = (*p != ' ');
			if(!error) {
				p++;
				status = strtol(p, &p, 10);
				error = (*p != ' ');
			}
		}
	}

	/* Read the contents */
	p = g_strrstr((const gchar *)connect_data->read_buffer, "Content-Length: ");
	if (p != NULL) {
		gchar *tmp;
		int len = 0;
		char tmpc;
		p += strlen("Content-Length: ");
		tmp = strchr(p, '\r');
		if(tmp)
			*tmp = '\0';
		len = atoi(p);
		if(tmp)
			*tmp = '\r';

		/* Compensate for what has already been read */
		len -= connect_data->read_len - headers_len;
		/* I'm assuming that we're doing this to prevent the server from
		   complaining / breaking since we don't read the whole page */
		while (len--) {
			/* TODO: deal with EAGAIN (and other errors) better */
			if (read(connect_data->fd, &tmpc, 1) < 0 && errno != EAGAIN)
				break;
		}
	}

	if (error) {
		oul_proxy_connect_data_disconnect_formatted(connect_data,
				_("Unable to parse response from HTTP proxy: %s\n"),
				connect_data->read_buffer);
		return;
	}
	else if (status != 200) {
		oul_debug_error("proxy",
				"Proxy server replied with:\n%s\n",
				connect_data->read_buffer);

		if (status == 407 /* Proxy Auth */) {
			const char *header;
			gchar *request;

			header = g_strrstr((const gchar *)connect_data->read_buffer,
					"Proxy-Authenticate: NTLM");
			if (header != NULL) {
				const char *header_end = header + strlen("Proxy-Authenticate: NTLM");
				const char *domain = oul_proxy_info_get_username(connect_data->gpi);
				char *username = NULL, hostname[256];
				gchar *response;
				int ret;

				ret = gethostname(hostname, sizeof(hostname));
				hostname[sizeof(hostname) - 1] = '\0';
				if (ret < 0 || hostname[0] == '\0') {
					oul_debug_warning("proxy", "gethostname() failed -- is your hostname set?");
					strcpy(hostname, "localhost");
				}

				if (domain != NULL)
					username = (char*) strchr(domain, '\\');
				if (username == NULL) {
					oul_proxy_connect_data_disconnect_formatted(connect_data,
							_("HTTP proxy connection error %d"), status);
					return;
				}
				*username = '\0';

				/* Is there a message? */
				if (*header_end == ' ') {
					/* Check for Type-2 */
					char *tmp = (char*) header;
					guint8 *nonce;

					header_end++;
					username++;
					while(*tmp != '\r' && *tmp != '\0') tmp++;
					*tmp = '\0';
					nonce = oul_ntlm_parse_type2(header_end, NULL);
					response = oul_ntlm_gen_type3(username,
						(gchar*) oul_proxy_info_get_password(connect_data->gpi),
						hostname,
						domain, nonce, NULL);
					username--;
				} else /* Empty message */
					response = oul_ntlm_gen_type1(hostname, domain);

				*username = '\\';

				request = g_strdup_printf(
					"CONNECT %s:%d HTTP/1.1\r\n"
					"Host: %s:%d\r\n"
					"Proxy-Authorization: NTLM %s\r\n"
					"Proxy-Connection: Keep-Alive\r\n\r\n",
					connect_data->host, connect_data->port,
					connect_data->host, connect_data->port,
					response);

				g_free(response);

			} else if((header = g_strrstr((const char *)connect_data->read_buffer, "Proxy-Authenticate: Basic"))) {
				gchar *t1, *t2;

				t1 = g_strdup_printf("%s:%s",
					oul_proxy_info_get_username(connect_data->gpi),
					oul_proxy_info_get_password(connect_data->gpi) ?
					oul_proxy_info_get_password(connect_data->gpi) : "");
				t2 = oul_base64_encode((const guchar *)t1, strlen(t1));
				g_free(t1);

				request = g_strdup_printf(
					"CONNECT %s:%d HTTP/1.1\r\n"
					"Host: %s:%d\r\n"
					"Proxy-Authorization: Basic %s\r\n",
					connect_data->host, connect_data->port,
					connect_data->host, connect_data->port,
					t2);

				g_free(t2);

			} else {
				oul_proxy_connect_data_disconnect_formatted(connect_data,
						_("HTTP proxy connection error %d"), status);
				return;
			}

			oul_input_remove(connect_data->inpa);
			g_free(connect_data->read_buffer);
			connect_data->read_buffer = NULL;

			connect_data->write_buffer = (guchar *)request;
			connect_data->write_buf_len = strlen(request);
			connect_data->written_len = 0;

			connect_data->read_cb = http_canread;

			connect_data->inpa = oul_input_add(connect_data->fd,
				OUL_INPUT_WRITE, proxy_do_write, connect_data);

			proxy_do_write(connect_data, connect_data->fd, cond);

			return;
		}

		if (status == 403) {
			/* Forbidden */
			oul_proxy_connect_data_disconnect_formatted(connect_data,
					_("Access denied: HTTP proxy server forbids port %d tunneling."),
					connect_data->port);
		} else {
			oul_proxy_connect_data_disconnect_formatted(connect_data,
					_("HTTP proxy connection error %d"), status);
		}
	} else {
		oul_input_remove(connect_data->inpa);
		connect_data->inpa = 0;
		g_free(connect_data->read_buffer);
		connect_data->read_buffer = NULL;
		oul_debug_info("proxy", "HTTP proxy connection established\n");
		oul_proxy_connect_data_connected(connect_data);
		return;
	}
}

static void
http_start_connect_tunneling(OulProxyConnectData *connect_data) {
	GString *request;
	int ret;

	oul_debug_info("proxy", "Using CONNECT tunneling for %s:%d\n",
		connect_data->host, connect_data->port);

	request = g_string_sized_new(4096);
	g_string_append_printf(request,
			"CONNECT %s:%d HTTP/1.1\r\nHost: %s:%d\r\n",
			connect_data->host, connect_data->port,
			connect_data->host, connect_data->port);

	if (oul_proxy_info_get_username(connect_data->gpi) != NULL)
	{
		char *t1, *t2, *ntlm_type1;
		char hostname[256];

		ret = gethostname(hostname, sizeof(hostname));
		hostname[sizeof(hostname) - 1] = '\0';
		if (ret < 0 || hostname[0] == '\0') {
			oul_debug_warning("proxy", "gethostname() failed -- is your hostname set?");
			strcpy(hostname, "localhost");
		}

		t1 = g_strdup_printf("%s:%s",
			oul_proxy_info_get_username(connect_data->gpi),
			oul_proxy_info_get_password(connect_data->gpi) ?
				oul_proxy_info_get_password(connect_data->gpi) : "");
		t2 = oul_base64_encode((const guchar *)t1, strlen(t1));
		g_free(t1);

		ntlm_type1 = oul_ntlm_gen_type1(hostname, "");

		g_string_append_printf(request,
			"Proxy-Authorization: Basic %s\r\n"
			"Proxy-Authorization: NTLM %s\r\n"
			"Proxy-Connection: Keep-Alive\r\n",
			t2, ntlm_type1);
		g_free(ntlm_type1);
		g_free(t2);
	}

	g_string_append(request, "\r\n");

	connect_data->write_buf_len = request->len;
	connect_data->write_buffer = (guchar *)g_string_free(request, FALSE);
	connect_data->written_len = 0;
	connect_data->read_cb = http_canread;

	connect_data->inpa = oul_input_add(connect_data->fd,
			OUL_INPUT_WRITE, proxy_do_write, connect_data);
	proxy_do_write(connect_data, connect_data->fd, OUL_INPUT_WRITE);
}

static void
http_canwrite(gpointer data, gint source, OulInputCondition cond) {
	OulProxyConnectData *connect_data = data;
	int ret, error = ETIMEDOUT;

	oul_debug_info("proxy", "Connected to %s:%d.\n",
		connect_data->host, connect_data->port);

	if (connect_data->inpa > 0)	{
		oul_input_remove(connect_data->inpa);
		connect_data->inpa = 0;
	}

	ret = oul_input_get_error(connect_data->fd, &error);
	if (ret != 0 || error != 0) {
		if (ret != 0)
			error = errno;
		oul_proxy_connect_data_disconnect(connect_data, g_strerror(error));
		return;
	}

	if (connect_data->port == 80) {
		/*
		 * If we're trying to connect to something running on
		 * port 80 then we assume the traffic using this
		 * connection is going to be HTTP traffic.  If it's
		 * not then this will fail (uglily).  But it's good
		 * to avoid using the CONNECT method because it's
		 * not always allowed.
		 */
		oul_debug_info("proxy", "HTTP proxy connection established\n");
		oul_proxy_connect_data_connected(connect_data);
	} else {
		http_start_connect_tunneling(connect_data);
	}

}

static void
proxy_connect_http(OulProxyConnectData *connect_data, struct sockaddr *addr, socklen_t addrlen)
{
	int flags;

	oul_debug_info("proxy",
			   "Connecting to %s:%d via %s:%d using HTTP\n",
			   connect_data->host, connect_data->port,
			   (oul_proxy_info_get_host(connect_data->gpi) ? oul_proxy_info_get_host(connect_data->gpi) : "(null)"),
			   oul_proxy_info_get_port(connect_data->gpi));

	connect_data->fd = socket(addr->sa_family, SOCK_STREAM, 0);
	if (connect_data->fd < 0)
	{
		oul_proxy_connect_data_disconnect_formatted(connect_data,
				_("Unable to create socket:\n%s"), g_strerror(errno));
		return;
	}

	flags = fcntl(connect_data->fd, F_GETFL);
	fcntl(connect_data->fd, F_SETFL, flags | O_NONBLOCK);
	fcntl(connect_data->fd, F_SETFD, FD_CLOEXEC);

	if (connect(connect_data->fd, addr, addrlen) != 0) {
		if (errno == EINPROGRESS || errno == EINTR) {
			oul_debug_info("proxy", "Connection in progress\n");

			connect_data->inpa = oul_input_add(connect_data->fd,
					OUL_INPUT_WRITE, http_canwrite, connect_data);
		} else
			oul_proxy_connect_data_disconnect(connect_data, g_strerror(errno));
	} else {
		oul_debug_info("proxy", "Connected immediately.\n");

		http_canwrite(connect_data, connect_data->fd, OUL_INPUT_WRITE);
	}
}

static void
s4_canread(gpointer data, gint source, OulInputCondition cond)
{
	OulProxyConnectData *connect_data = data;
	guchar *buf;
	int len, max_read;

	/* This is really not going to block under normal circumstances, but to
	 * be correct, we deal with the unlikely scenario */

	if (connect_data->read_buffer == NULL) {
		connect_data->read_buf_len = 12;
		connect_data->read_buffer = g_malloc(connect_data->read_buf_len);
		connect_data->read_len = 0;
	}

	buf = connect_data->read_buffer + connect_data->read_len;
	max_read = connect_data->read_buf_len - connect_data->read_len;

	len = read(connect_data->fd, buf, max_read);

	if ((len < 0 && errno == EAGAIN) || (len > 0 && len + connect_data->read_len < 4))
		return;
	else if (len + connect_data->read_len >= 4) {
		if (connect_data->read_buffer[1] == 90) {
			oul_proxy_connect_data_connected(connect_data);
			return;
		}
	}

	oul_proxy_connect_data_disconnect(connect_data, g_strerror(errno));
}

static void
s4_canwrite(gpointer data, gint source, OulInputCondition cond)
{
	unsigned char packet[9];
	struct hostent *hp;
	OulProxyConnectData *connect_data = data;
	int error = ETIMEDOUT;
	int ret;

	oul_debug_info("socks4 proxy", "Connected.\n");

	if (connect_data->inpa > 0)
	{
		oul_input_remove(connect_data->inpa);
		connect_data->inpa = 0;
	}

	ret = oul_input_get_error(connect_data->fd, &error);
	if ((ret != 0) || (error != 0))
	{
		if (ret != 0)
			error = errno;
		oul_proxy_connect_data_disconnect(connect_data, g_strerror(error));
		return;
	}

	/*
	 * The socks4 spec doesn't include support for doing host name
	 * lookups by the proxy.  Some socks4 servers do this via
	 * extensions to the protocol.  Since we don't know if a
	 * server supports this, it would need to be implemented
	 * with an option, or some detection mechanism - in the
	 * meantime, stick with plain old SOCKS4.
	 */
	/* TODO: Use oul_dnsquery_a() */
	hp = gethostbyname(connect_data->host);
	if (hp == NULL) {
		oul_proxy_connect_data_disconnect_formatted(connect_data,
				_("Error resolving %s"), connect_data->host);
		return;
	}

	packet[0] = 4;
	packet[1] = 1;
	packet[2] = connect_data->port >> 8;
	packet[3] = connect_data->port & 0xff;
	packet[4] = (unsigned char)(hp->h_addr_list[0])[0];
	packet[5] = (unsigned char)(hp->h_addr_list[0])[1];
	packet[6] = (unsigned char)(hp->h_addr_list[0])[2];
	packet[7] = (unsigned char)(hp->h_addr_list[0])[3];
	packet[8] = 0;

	connect_data->write_buffer = g_memdup(packet, sizeof(packet));
	connect_data->write_buf_len = sizeof(packet);
	connect_data->written_len = 0;
	connect_data->read_cb = s4_canread;

	connect_data->inpa = oul_input_add(connect_data->fd, OUL_INPUT_WRITE, proxy_do_write, connect_data);

	proxy_do_write(connect_data, connect_data->fd, cond);
}

static void
proxy_connect_socks4(OulProxyConnectData *connect_data, struct sockaddr *addr, socklen_t addrlen)
{
	int flags;

	oul_debug_info("proxy",
			   "Connecting to %s:%d via %s:%d using SOCKS4\n",
			   connect_data->host, connect_data->port,
			   oul_proxy_info_get_host(connect_data->gpi),
			   oul_proxy_info_get_port(connect_data->gpi));

	connect_data->fd = socket(addr->sa_family, SOCK_STREAM, 0);
	if (connect_data->fd < 0)
	{
		oul_proxy_connect_data_disconnect_formatted(connect_data,
				_("Unable to create socket:\n%s"), g_strerror(errno));
		return;
	}

	flags = fcntl(connect_data->fd, F_GETFL);
	fcntl(connect_data->fd, F_SETFL, flags | O_NONBLOCK);
	fcntl(connect_data->fd, F_SETFD, FD_CLOEXEC);

	if (connect(connect_data->fd, addr, addrlen) != 0)
	{
		if ((errno == EINPROGRESS) || (errno == EINTR))
		{
			oul_debug_info("proxy", "Connection in progress.\n");
			connect_data->inpa = oul_input_add(connect_data->fd,
					OUL_INPUT_WRITE, s4_canwrite, connect_data);
		}
		else
		{
			oul_proxy_connect_data_disconnect(connect_data, g_strerror(errno));
		}
	}
	else
	{
		oul_debug_info("proxy", "Connected immediately.\n");

		s4_canwrite(connect_data, connect_data->fd, OUL_INPUT_WRITE);
	}
}

static gboolean
s5_ensure_buffer_length(OulProxyConnectData *connect_data, int len)
{
	if(connect_data->read_len < len) {
		if(connect_data->read_buf_len < len) {
			/* it's not just that we haven't read enough, it's that we haven't tried to read enough yet */
			oul_debug_info("s5", "reallocing from %" G_GSIZE_FORMAT
					" to %d\n", connect_data->read_buf_len, len);
			connect_data->read_buf_len = len;
			connect_data->read_buffer = g_realloc(connect_data->read_buffer, connect_data->read_buf_len);
		}
		return FALSE;
	}

	return TRUE;
}

static void
s5_canread_again(gpointer data, gint source, OulInputCondition cond)
{
	guchar *dest, *buf;
	OulProxyConnectData *connect_data = data;
	int len;

	if (connect_data->read_buffer == NULL) {
		connect_data->read_buf_len = 5;
		connect_data->read_buffer = g_malloc(connect_data->read_buf_len);
		connect_data->read_len = 0;
	}

	dest = connect_data->read_buffer + connect_data->read_len;
	buf = connect_data->read_buffer;

	len = read(connect_data->fd, dest, (connect_data->read_buf_len - connect_data->read_len));

	if (len == 0)
	{
		oul_proxy_connect_data_disconnect(connect_data,
				_("Server closed the connection."));
		return;
	}

	if (len < 0)
	{
		if (errno == EAGAIN)
			/* No worries */
			return;

		/* Error! */
		oul_proxy_connect_data_disconnect_formatted(connect_data,
				_("Lost connection with server:\n%s"), g_strerror(errno));
		return;
	}

	connect_data->read_len += len;

	if(connect_data->read_len < 4)
		return;

	if ((buf[0] != 0x05) || (buf[1] != 0x00)) {
		if ((buf[0] == 0x05) && (buf[1] < 0x09)) {
			oul_debug_error("socks5 proxy", socks5errors[buf[1]]);
			oul_proxy_connect_data_disconnect(connect_data,
					socks5errors[buf[1]]);
		} else {
			oul_debug_error("socks5 proxy", "Bad data.\n");
			oul_proxy_connect_data_disconnect(connect_data,
					_("Received invalid data on connection with server."));
		}
		return;
	}

	/* Skip past BND.ADDR */
	switch(buf[3]) {
		case 0x01: /* the address is a version-4 IP address, with a length of 4 octets */
			if(!s5_ensure_buffer_length(connect_data, 4 + 4))
				return;
			buf += 4 + 4;
			break;
		case 0x03: /* the address field contains a fully-qualified domain name.  The first
					  octet of the address field contains the number of octets of name that
					  follow, there is no terminating NUL octet. */
			if(!s5_ensure_buffer_length(connect_data, 4 + 1))
				return;
			buf += 4;
			if(!s5_ensure_buffer_length(connect_data, 4 + 1 + buf[0]))
				return;
			buf += buf[0] + 1;
			break;
		case 0x04: /* the address is a version-6 IP address, with a length of 16 octets */
			if(!s5_ensure_buffer_length(connect_data, 4 + 16))
				return;
			buf += 4 + 16;
			break;
		default:
			oul_debug_error("socks5 proxy", "Invalid ATYP received (0x%X)\n", buf[3]);
			oul_proxy_connect_data_disconnect(connect_data,
					_("Received invalid data on connection with server."));
			return;
	}

	/* Skip past BND.PORT */
	if(!s5_ensure_buffer_length(connect_data, (buf - connect_data->read_buffer) + 2))
		return;

	oul_proxy_connect_data_connected(connect_data);
}

static void
s5_sendconnect(gpointer data, int source)
{
	OulProxyConnectData *connect_data = data;
	int hlen = strlen(connect_data->host);
	connect_data->write_buf_len = 5 + hlen + 2;
	connect_data->write_buffer = g_malloc(connect_data->write_buf_len);
	connect_data->written_len = 0;

	connect_data->write_buffer[0] = 0x05;
	connect_data->write_buffer[1] = 0x01;		/* CONNECT */
	connect_data->write_buffer[2] = 0x00;		/* reserved */
	connect_data->write_buffer[3] = 0x03;		/* address type -- host name */
	connect_data->write_buffer[4] = hlen;
	memcpy(connect_data->write_buffer + 5, connect_data->host, hlen);
	connect_data->write_buffer[5 + hlen] = connect_data->port >> 8;
	connect_data->write_buffer[5 + hlen + 1] = connect_data->port & 0xff;

	connect_data->read_cb = s5_canread_again;

	connect_data->inpa = oul_input_add(connect_data->fd, OUL_INPUT_WRITE, proxy_do_write, connect_data);
	proxy_do_write(connect_data, connect_data->fd, OUL_INPUT_WRITE);
}

static void
s5_readauth(gpointer data, gint source, OulInputCondition cond)
{
	OulProxyConnectData *connect_data = data;
	int len;

	if (connect_data->read_buffer == NULL) {
		connect_data->read_buf_len = 2;
		connect_data->read_buffer = g_malloc(connect_data->read_buf_len);
		connect_data->read_len = 0;
	}

	oul_debug_info("socks5 proxy", "Got auth response.\n");

	len = read(connect_data->fd, connect_data->read_buffer + connect_data->read_len,
		connect_data->read_buf_len - connect_data->read_len);

	if (len == 0)
	{
		oul_proxy_connect_data_disconnect(connect_data,
				_("Server closed the connection."));
		return;
	}

	if (len < 0)
	{
		if (errno == EAGAIN)
			/* No worries */
			return;

		/* Error! */
		oul_proxy_connect_data_disconnect_formatted(connect_data,
				_("Lost connection with server:\n%s"), g_strerror(errno));
		return;
	}

	connect_data->read_len += len;
	if (connect_data->read_len < 2)
		return;

	oul_input_remove(connect_data->inpa);
	connect_data->inpa = 0;

	if ((connect_data->read_buffer[0] != 0x01) || (connect_data->read_buffer[1] != 0x00)) {
		oul_proxy_connect_data_disconnect(connect_data,
				_("Received invalid data on connection with server."));
		return;
	}

	g_free(connect_data->read_buffer);
	connect_data->read_buffer = NULL;

	s5_sendconnect(connect_data, connect_data->fd);
}

static void
hmacmd5_chap(const unsigned char * challenge, int challen, const char * passwd, unsigned char * response)
{
	OulCipher *cipher;
	OulCipherContext *ctx;
	int i;
	unsigned char Kxoripad[65];
	unsigned char Kxoropad[65];
	int pwlen;

	cipher = oul_ciphers_find_cipher("md5");
	ctx = oul_cipher_context_new(cipher, NULL);

	memset(Kxoripad,0,sizeof(Kxoripad));
	memset(Kxoropad,0,sizeof(Kxoropad));

	pwlen=strlen(passwd);
	if (pwlen>64) {
		oul_cipher_context_append(ctx, (const guchar *)passwd, strlen(passwd));
		oul_cipher_context_digest(ctx, sizeof(Kxoripad), Kxoripad, NULL);
		pwlen=16;
	} else {
		memcpy(Kxoripad, passwd, pwlen);
	}
	memcpy(Kxoropad,Kxoripad,pwlen);

	for (i=0;i<64;i++) {
		Kxoripad[i]^=0x36;
		Kxoropad[i]^=0x5c;
	}

	oul_cipher_context_reset(ctx, NULL);
	oul_cipher_context_append(ctx, Kxoripad, 64);
	oul_cipher_context_append(ctx, challenge, challen);
	oul_cipher_context_digest(ctx, sizeof(Kxoripad), Kxoripad, NULL);

	oul_cipher_context_reset(ctx, NULL);
	oul_cipher_context_append(ctx, Kxoropad, 64);
	oul_cipher_context_append(ctx, Kxoripad, 16);
	oul_cipher_context_digest(ctx, 16, response, NULL);

	oul_cipher_context_destroy(ctx);
}

static void
s5_readchap(gpointer data, gint source, OulInputCondition cond)
{
	guchar *cmdbuf, *buf;
	OulProxyConnectData *connect_data = data;
	int len, navas, currentav;

	oul_debug(OUL_DEBUG_INFO, "socks5 proxy", "Got CHAP response.\n");

	if (connect_data->read_buffer == NULL) {
		/* A big enough butfer to read the message header (2 bytes) and at least one complete attribute and value (1 + 1 + 255). */
		connect_data->read_buf_len = 259;
		connect_data->read_buffer = g_malloc(connect_data->read_buf_len);
		connect_data->read_len = 0;
	}

	if (connect_data->read_buf_len - connect_data->read_len == 0) {
		/*If the stuff below is right, this shouldn't be possible. */
		oul_debug_error("socks5 proxy", "This is about to suck because the read buffer is full (shouldn't happen).\n");
	}

	len = read(connect_data->fd, connect_data->read_buffer + connect_data->read_len,
		connect_data->read_buf_len - connect_data->read_len);

	if (len == 0)
	{
		oul_proxy_connect_data_disconnect(connect_data,
				_("Server closed the connection."));
		return;
	}

	if (len < 0)
	{
		if (errno == EAGAIN)
			/* No worries */
			return;

		/* Error! */
		oul_proxy_connect_data_disconnect_formatted(connect_data,
				_("Lost connection with server:\n%s"), g_strerror(errno));
		return;
	}

	connect_data->read_len += len;
	if (connect_data->read_len < 2)
		return;

	cmdbuf = connect_data->read_buffer;

	if (*cmdbuf != 0x01) {
		oul_proxy_connect_data_disconnect(connect_data,
				_("Received invalid data on connection with server."));
		return;
	}
	cmdbuf++;

	navas = *cmdbuf;
	cmdbuf++;

	for (currentav = 0; currentav < navas; currentav++) {

		len = connect_data->read_len - (cmdbuf - connect_data->read_buffer);
		/* We don't have enough data to even know how long the next attribute is,
		 * or we don't have the full length of the next attribute. */
		if (len < 2 || len < (cmdbuf[1] + 2)) {
			/* Clear out the attributes that have been read - decrease the attribute count */
			connect_data->read_buffer[1] = navas - currentav;
			/* Move the unprocessed data into the first attribute position */
			memmove((connect_data->read_buffer + 2), cmdbuf, len);
			/* Decrease the read count accordingly */
			connect_data->read_len = len + 2;
			return;
		}

		buf = cmdbuf + 2;

		if (cmdbuf[1] == 0) {
			oul_debug_error("socks5 proxy", "Attribute %x Value length of 0; ignoring.\n", cmdbuf[0]);
			cmdbuf = buf;
			continue;
		}

		switch (cmdbuf[0]) {
			case 0x00:
				oul_debug_info("socks5 proxy", "Received STATUS of %x\n", buf[0]);
				/* Did auth work? */
				if (buf[0] == 0x00) {
					oul_input_remove(connect_data->inpa);
					connect_data->inpa = 0;
					g_free(connect_data->read_buffer);
					connect_data->read_buffer = NULL;
					/* Success */
					s5_sendconnect(connect_data, connect_data->fd);
				} else {
					/* Failure */
					oul_debug_warning("proxy",
						"socks5 CHAP authentication "
						"failed.  Disconnecting...");
					oul_proxy_connect_data_disconnect(connect_data,
							_("Authentication failed"));
				}
				return;
			case 0x03:
				oul_debug_info("socks5 proxy", "Received CHALLENGE\n");
				/* Server wants our credentials */

				connect_data->write_buf_len = 16 + 4;
				connect_data->write_buffer = g_malloc(connect_data->write_buf_len);
				connect_data->written_len = 0;

				hmacmd5_chap(buf, cmdbuf[1],
					oul_proxy_info_get_password(connect_data->gpi),
					connect_data->write_buffer + 4);
				connect_data->write_buffer[0] = 0x01;
				connect_data->write_buffer[1] = 0x01;
				connect_data->write_buffer[2] = 0x04;
				connect_data->write_buffer[3] = 0x10;

				oul_input_remove(connect_data->inpa);
				g_free(connect_data->read_buffer);
				connect_data->read_buffer = NULL;

				connect_data->read_cb = s5_readchap;

				connect_data->inpa = oul_input_add(connect_data->fd,
					OUL_INPUT_WRITE, proxy_do_write, connect_data);

				proxy_do_write(connect_data, connect_data->fd, OUL_INPUT_WRITE);
				return;
			case 0x11:
				oul_debug_info("socks5 proxy", "Received ALGORIGTHMS of %x\n", buf[0]);
				/* Server wants to select an algorithm */
				if (buf[0] != 0x85) {
					/* Only currently support HMAC-MD5 */
					oul_debug_warning("proxy",
						"Server tried to select an "
						"algorithm that we did not advertise "
						"as supporting.  This is a violation "
						"of the socks5 CHAP specification.  "
						"Disconnecting...");
					oul_proxy_connect_data_disconnect(connect_data,
							_("Received invalid data on connection with server."));
					return;
				}
				break;
			default:
				oul_debug_info("socks5 proxy", "Received unused command %x, length=%d\n", cmdbuf[0], cmdbuf[1]);
		}
		cmdbuf = buf + cmdbuf[1];
	}

	/* Fell through.  We ran out of CHAP events to process, but haven't
	 * succeeded or failed authentication - there may be more to come.
	 * If this is the case, come straight back here. */

	/* We've processed all the available attributes, so get ready for a whole new message */
 	g_free(connect_data->read_buffer);
	connect_data->read_buffer = NULL;
}

static void
s5_canread(gpointer data, gint source, OulInputCondition cond)
{
	OulProxyConnectData *connect_data = data;
	int len;

	if (connect_data->read_buffer == NULL) {
		connect_data->read_buf_len = 2;
		connect_data->read_buffer = g_malloc(connect_data->read_buf_len);
		connect_data->read_len = 0;
	}

	oul_debug_info("socks5 proxy", "Able to read.\n");

	len = read(connect_data->fd, connect_data->read_buffer + connect_data->read_len,
		connect_data->read_buf_len - connect_data->read_len);

	if (len == 0)
	{
		oul_proxy_connect_data_disconnect(connect_data,
				_("Server closed the connection."));
		return;
	}

	if (len < 0)
	{
		if (errno == EAGAIN)
			/* No worries */
			return;

		/* Error! */
		oul_proxy_connect_data_disconnect_formatted(connect_data,
				_("Lost connection with server:\n%s"), g_strerror(errno));
		return;
	}

	connect_data->read_len += len;
	if (connect_data->read_len < 2)
		return;

	oul_input_remove(connect_data->inpa);
	connect_data->inpa = 0;

	if ((connect_data->read_buffer[0] != 0x05) || (connect_data->read_buffer[1] == 0xff)) {
		oul_proxy_connect_data_disconnect(connect_data,
				_("Received invalid data on connection with server."));
		return;
	}

	if (connect_data->read_buffer[1] == 0x02) {
		gsize i, j;
		const char *u, *p;

		u = oul_proxy_info_get_username(connect_data->gpi);
		p = oul_proxy_info_get_password(connect_data->gpi);

		i = (u == NULL) ? 0 : strlen(u);
		j = (p == NULL) ? 0 : strlen(p);

		connect_data->write_buf_len = 1 + 1 + i + 1 + j;
		connect_data->write_buffer = g_malloc(connect_data->write_buf_len);
		connect_data->written_len = 0;

		connect_data->write_buffer[0] = 0x01;	/* version 1 */
		connect_data->write_buffer[1] = i;
		if (u != NULL)
			memcpy(connect_data->write_buffer + 2, u, i);
		connect_data->write_buffer[2 + i] = j;
		if (p != NULL)
			memcpy(connect_data->write_buffer + 2 + i + 1, p, j);

		g_free(connect_data->read_buffer);
		connect_data->read_buffer = NULL;

		connect_data->read_cb = s5_readauth;

		connect_data->inpa = oul_input_add(connect_data->fd, OUL_INPUT_WRITE,
			proxy_do_write, connect_data);

		proxy_do_write(connect_data, connect_data->fd, OUL_INPUT_WRITE);

		return;
	} else if (connect_data->read_buffer[1] == 0x03) {
		gsize userlen;
		userlen = strlen(oul_proxy_info_get_username(connect_data->gpi));

		connect_data->write_buf_len = 7 + userlen;
		connect_data->write_buffer = g_malloc(connect_data->write_buf_len);
		connect_data->written_len = 0;

		connect_data->write_buffer[0] = 0x01;
		connect_data->write_buffer[1] = 0x02;
		connect_data->write_buffer[2] = 0x11;
		connect_data->write_buffer[3] = 0x01;
		connect_data->write_buffer[4] = 0x85;
		connect_data->write_buffer[5] = 0x02;
		connect_data->write_buffer[6] = userlen;
		memcpy(connect_data->write_buffer + 7,
			oul_proxy_info_get_username(connect_data->gpi), userlen);

		g_free(connect_data->read_buffer);
		connect_data->read_buffer = NULL;

		connect_data->read_cb = s5_readchap;

		connect_data->inpa = oul_input_add(connect_data->fd, OUL_INPUT_WRITE,
			proxy_do_write, connect_data);

		proxy_do_write(connect_data, connect_data->fd, OUL_INPUT_WRITE);

		return;
	} else {
		g_free(connect_data->read_buffer);
		connect_data->read_buffer = NULL;

		s5_sendconnect(connect_data, connect_data->fd);
	}
}

static void
s5_canwrite(gpointer data, gint source, OulInputCondition cond)
{
	unsigned char buf[5];
	int i;
	OulProxyConnectData *connect_data = data;
	int error = ETIMEDOUT;
	int ret;

	oul_debug_info("socks5 proxy", "Connected.\n");

	if (connect_data->inpa > 0)
	{
		oul_input_remove(connect_data->inpa);
		connect_data->inpa = 0;
	}

	ret = oul_input_get_error(connect_data->fd, &error);
	if ((ret != 0) || (error != 0))
	{
		if (ret != 0)
			error = errno;
		oul_proxy_connect_data_disconnect(connect_data, g_strerror(error));
		return;
	}

	i = 0;
	buf[0] = 0x05;		/* SOCKS version 5 */

	if (oul_proxy_info_get_username(connect_data->gpi) != NULL) {
		buf[1] = 0x03;	/* three methods */
		buf[2] = 0x00;	/* no authentication */
		buf[3] = 0x03;	/* CHAP authentication */
		buf[4] = 0x02;	/* username/password authentication */
		i = 5;
	}
	else {
		buf[1] = 0x01;
		buf[2] = 0x00;
		i = 3;
	}

	connect_data->write_buf_len = i;
	connect_data->write_buffer = g_malloc(connect_data->write_buf_len);
	memcpy(connect_data->write_buffer, buf, i);

	connect_data->read_cb = s5_canread;

	connect_data->inpa = oul_input_add(connect_data->fd, OUL_INPUT_WRITE, proxy_do_write, connect_data);
	proxy_do_write(connect_data, connect_data->fd, OUL_INPUT_WRITE);
}

static void
proxy_connect_socks5(OulProxyConnectData *connect_data, struct sockaddr *addr, socklen_t addrlen)
{
	int flags;

	oul_debug_info("proxy",
			   "Connecting to %s:%d via %s:%d using SOCKS5\n",
			   connect_data->host, connect_data->port,
			   oul_proxy_info_get_host(connect_data->gpi),
			   oul_proxy_info_get_port(connect_data->gpi));

	connect_data->fd = socket(addr->sa_family, SOCK_STREAM, 0);
	if (connect_data->fd < 0)
	{
		oul_proxy_connect_data_disconnect_formatted(connect_data,
				_("Unable to create socket:\n%s"), g_strerror(errno));
		return;
	}

	flags = fcntl(connect_data->fd, F_GETFL);
	fcntl(connect_data->fd, F_SETFL, flags | O_NONBLOCK);
	fcntl(connect_data->fd, F_SETFD, FD_CLOEXEC);

	if (connect(connect_data->fd, addr, addrlen) != 0)
	{
		if ((errno == EINPROGRESS) || (errno == EINTR))
		{
			oul_debug_info("socks5 proxy", "Connection in progress\n");
			connect_data->inpa = oul_input_add(connect_data->fd,
					OUL_INPUT_WRITE, s5_canwrite, connect_data);
		}
		else
		{
			oul_proxy_connect_data_disconnect(connect_data, g_strerror(errno));
		}
	}
	else
	{
		oul_debug_info("proxy", "Connected immediately.\n");

		s5_canwrite(connect_data, connect_data->fd, OUL_INPUT_WRITE);
	}
}

/**
 * This function attempts to connect to the next IP address in the list
 * of IP addresses returned to us by oul_dnsquery_a() and attemps
 * to connect to each one.  This is called after the hostname is
 * resolved, and each time a connection attempt fails (assuming there
 * is another IP address to try).
 */
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

static void try_connect(OulProxyConnectData *connect_data)
{
	size_t addrlen;
	struct sockaddr *addr;
	char ipaddr[INET6_ADDRSTRLEN];

	addrlen = GPOINTER_TO_INT(connect_data->hosts->data);
	connect_data->hosts = g_slist_remove(connect_data->hosts, connect_data->hosts->data);
	addr = connect_data->hosts->data;
	connect_data->hosts = g_slist_remove(connect_data->hosts, connect_data->hosts->data);
#ifdef HAVE_INET_NTOP
	inet_ntop(addr->sa_family, &((struct sockaddr_in *)addr)->sin_addr,
			ipaddr, sizeof(ipaddr));
#else
	memcpy(ipaddr,inet_ntoa(((struct sockaddr_in *)addr)->sin_addr),
			sizeof(ipaddr));
#endif
	oul_debug_info("proxy", "Attempting connection to %s\n", ipaddr);

	switch (oul_proxy_info_get_type(connect_data->gpi)) {
		case OUL_PROXY_NONE:
			proxy_connect_none(connect_data, addr, addrlen);
			break;

		case OUL_PROXY_HTTP:
			proxy_connect_http(connect_data, addr, addrlen);
			break;

		case OUL_PROXY_SOCKS4:
			proxy_connect_socks4(connect_data, addr, addrlen);
			break;

		case OUL_PROXY_SOCKS5:
			proxy_connect_socks5(connect_data, addr, addrlen);
			break;

		case OUL_PROXY_USE_ENVVAR:
			proxy_connect_http(connect_data, addr, addrlen);
			break;

		default:
			break;
	}

	g_free(addr);
}

static void
connection_host_resolved(GSList *hosts, gpointer data,
						 const char *error_message)
{
	OulProxyConnectData *connect_data;

	connect_data = data;
	connect_data->query_data = NULL;

	if (error_message != NULL)
	{
		oul_proxy_connect_data_disconnect(connect_data, error_message);
		return;
	}

	if (hosts == NULL)
	{
		oul_proxy_connect_data_disconnect(connect_data, _("Could not resolve host name"));
		return;
	}

	connect_data->hosts = hosts;

	try_connect(connect_data);
}

OulProxyInfo *
oul_proxy_get_setup()
{
	OulProxyInfo *gpi = NULL;
	const gchar *tmp;

	/* This is used as a fallback so we don't overwrite the selected proxy type */
	static OulProxyInfo *tmp_none_proxy_info = NULL;
	if (!tmp_none_proxy_info) {
		tmp_none_proxy_info = oul_proxy_info_new();
		oul_proxy_info_set_type(tmp_none_proxy_info, OUL_PROXY_NONE);
	}

	if (gpi == NULL) {
		if (oul_running_gnome())
			gpi = oul_gnome_proxy_get_info();
		else
			gpi = oul_global_proxy_get_info();
	}

	if (oul_proxy_info_get_type(gpi) == OUL_PROXY_USE_ENVVAR) {
		if ((tmp = g_getenv("HTTP_PROXY")) != NULL ||
			(tmp = g_getenv("http_proxy")) != NULL ||
			(tmp = g_getenv("HTTPPROXY")) != NULL) {
			char *proxyhost, *proxyuser, *proxypasswd;
			int proxyport;

			/* http_proxy-format:
			 * export http_proxy="http://user:passwd@your.proxy.server:port/"
			 */
			if(oul_url_parse(tmp, &proxyhost, &proxyport, NULL, &proxyuser, &proxypasswd)) {
				oul_proxy_info_set_host(gpi, proxyhost);
				g_free(proxyhost);

				oul_proxy_info_set_username(gpi, proxyuser);
				g_free(proxyuser);

				oul_proxy_info_set_password(gpi, proxypasswd);
				g_free(proxypasswd);

				/* only for backward compatibility */
				if (proxyport == 80 &&
				    ((tmp = g_getenv("HTTP_PROXY_PORT")) != NULL ||
				     (tmp = g_getenv("http_proxy_port")) != NULL ||
				     (tmp = g_getenv("HTTPPROXYPORT")) != NULL))
					proxyport = atoi(tmp);

				oul_proxy_info_set_port(gpi, proxyport);

				/* XXX: Do we want to skip this step if user/password were part of url? */
				if ((tmp = g_getenv("HTTP_PROXY_USER")) != NULL ||
					(tmp = g_getenv("http_proxy_user")) != NULL ||
					(tmp = g_getenv("HTTPPROXYUSER")) != NULL)
					oul_proxy_info_set_username(gpi, tmp);

				if ((tmp = g_getenv("HTTP_PROXY_PASS")) != NULL ||
					(tmp = g_getenv("http_proxy_pass")) != NULL ||
					(tmp = g_getenv("HTTPPROXYPASS")) != NULL)
					oul_proxy_info_set_password(gpi, tmp);

			}
		} else {

			/* no proxy environment variable found, don't use a proxy */
			oul_debug_info("proxy", "No environment settings found, not using a proxy\n");
			gpi = tmp_none_proxy_info;
		}

	}

	return gpi;
}

OulProxyConnectData *
oul_proxy_connect(void *handle, const char *host, int port,
				   OulProxyConnectFunction connect_cb, gpointer data)
{
	const char *connecthost = host;
	int connectport = port;
	OulProxyConnectData *connect_data;

	g_return_val_if_fail(host       != NULL, NULL);
	g_return_val_if_fail(port       >  0,    NULL);
	g_return_val_if_fail(connect_cb != NULL, NULL);

	connect_data = g_new0(OulProxyConnectData, 1);
	connect_data->fd = -1;
	connect_data->handle = handle;
	connect_data->connect_cb = connect_cb;
	connect_data->data = data;
	connect_data->host = g_strdup(host);
	connect_data->port = port;
	connect_data->gpi = oul_proxy_get_setup();

	if ((oul_proxy_info_get_type(connect_data->gpi) != OUL_PROXY_NONE) &&
		(oul_proxy_info_get_host(connect_data->gpi) == NULL ||
		 oul_proxy_info_get_port(connect_data->gpi) <= 0)) {

		oul_notify_error(NULL, NULL, _("Invalid proxy settings"), _("Either the host name or port number specified for your given proxy type is invalid."));
		oul_proxy_connect_data_destroy(connect_data);
		return NULL;
	}

	switch (oul_proxy_info_get_type(connect_data->gpi))
	{
		case OUL_PROXY_NONE:
			break;

		case OUL_PROXY_HTTP:
		case OUL_PROXY_SOCKS4:
		case OUL_PROXY_SOCKS5:
		case OUL_PROXY_USE_ENVVAR:
			connecthost = oul_proxy_info_get_host(connect_data->gpi);
			connectport = oul_proxy_info_get_port(connect_data->gpi);
			break;

		default:
			oul_proxy_connect_data_destroy(connect_data);
			return NULL;
	}

	connect_data->query_data = oul_dnsquery_a(connecthost,
			connectport, connection_host_resolved, connect_data);
	if (connect_data->query_data == NULL)
	{
		oul_proxy_connect_data_destroy(connect_data);
		return NULL;
	}

	handles = g_slist_prepend(handles, connect_data);

	return connect_data;
}

/*
 * Combine some of this code with oul_proxy_connect()
 */
OulProxyConnectData *
oul_proxy_connect_socks5(void *handle, OulProxyInfo *gpi,
						  const char *host, int port,
						  OulProxyConnectFunction connect_cb,
						  gpointer data)
{
	OulProxyConnectData *connect_data;

	g_return_val_if_fail(host       != NULL, NULL);
	g_return_val_if_fail(port       >= 0,    NULL);
	g_return_val_if_fail(connect_cb != NULL, NULL);

	connect_data = g_new0(OulProxyConnectData, 1);
	connect_data->fd = -1;
	connect_data->handle = handle;
	connect_data->connect_cb = connect_cb;
	connect_data->data = data;
	connect_data->host = g_strdup(host);
	connect_data->port = port;
	connect_data->gpi = gpi;

	connect_data->query_data =
			oul_dnsquery_a(oul_proxy_info_get_host(gpi),
					oul_proxy_info_get_port(gpi),
					connection_host_resolved, connect_data);
	if (connect_data->query_data == NULL)
	{
		oul_proxy_connect_data_destroy(connect_data);
		return NULL;
	}

	handles = g_slist_prepend(handles, connect_data);

	return connect_data;
}

void
oul_proxy_connect_cancel(OulProxyConnectData *connect_data)
{
	oul_proxy_connect_data_disconnect(connect_data, NULL);
	oul_proxy_connect_data_destroy(connect_data);
}

void
oul_proxy_connect_cancel_with_handle(void *handle)
{
	GSList *l, *l_next;

	for (l = handles; l != NULL; l = l_next) {
		OulProxyConnectData *connect_data = l->data;

		l_next = l->next;

		if (connect_data->handle == handle)
			oul_proxy_connect_cancel(connect_data);
	}
}

static void
proxy_pref_cb(const char *name, OulPrefType type,
			  gconstpointer value, gpointer data)
{
	OulProxyInfo *info = oul_global_proxy_get_info();

	if (!strcmp(name, "/oul/proxy/type")) {
		int proxytype;
		const char *type = value;

		if (!strcmp(type, "none"))
			proxytype = OUL_PROXY_NONE;
		else if (!strcmp(type, "http"))
			proxytype = OUL_PROXY_HTTP;
		else if (!strcmp(type, "socks4"))
			proxytype = OUL_PROXY_SOCKS4;
		else if (!strcmp(type, "socks5"))
			proxytype = OUL_PROXY_SOCKS5;
		else if (!strcmp(type, "envvar"))
			proxytype = OUL_PROXY_USE_ENVVAR;
		else
			proxytype = -1;

		oul_proxy_info_set_type(info, proxytype);
	} else if (!strcmp(name, "/oul/proxy/host"))
		oul_proxy_info_set_host(info, value);
	else if (!strcmp(name, "/oul/proxy/port"))
		oul_proxy_info_set_port(info, GPOINTER_TO_INT(value));
	else if (!strcmp(name, "/oul/proxy/username"))
		oul_proxy_info_set_username(info, value);
	else if (!strcmp(name, "/oul/proxy/password"))
		oul_proxy_info_set_password(info, value);
}

void *
oul_proxy_get_handle()
{
	static int handle;

	return &handle;
}

void
oul_proxy_init(void)
{
	void *handle;

	/* Initialize a default proxy info struct. */
	global_proxy_info = oul_proxy_info_new();

	/* Proxy */
	oul_prefs_add_none("/oul/network");
	oul_prefs_add_none("/oul/network/proxy");
	oul_prefs_add_string("/oul/network/proxy/type", "none");
	oul_prefs_add_string("/oul/network/proxy/host", "");
	oul_prefs_add_int("/oul/network/proxy/port", 0);
	oul_prefs_add_string("/oul/network/proxy/username", "");
	oul_prefs_add_string("/oul/network/proxy/password", "");

	/* Setup callbacks for the preferences. */
	handle = oul_proxy_get_handle();

	oul_prefs_connect_callback(handle, "/oul/network/proxy/type", proxy_pref_cb, NULL);
	oul_prefs_connect_callback(handle, "/oul/network/proxy/host", proxy_pref_cb, NULL);
	oul_prefs_connect_callback(handle, "/oul/network/proxy/port", proxy_pref_cb, NULL);
	oul_prefs_connect_callback(handle, "/oul/network/proxy/username", proxy_pref_cb, NULL);
	oul_prefs_connect_callback(handle, "/oul/network/proxy/password", proxy_pref_cb, NULL);

	/* Load the initial proxy settings */
	oul_prefs_trigger_callback("/oul/network/proxy/type");
	oul_prefs_trigger_callback("/oul/network/proxy/host");
	oul_prefs_trigger_callback("/oul/network/proxy/port");
	oul_prefs_trigger_callback("/oul/network/proxy/username");
	oul_prefs_trigger_callback("/oul/network/proxy/password");
}

void
oul_proxy_uninit(void)
{
	while (handles != NULL)
	{
		oul_proxy_connect_data_disconnect(handles->data, NULL);
		oul_proxy_connect_data_destroy(handles->data);
	}
}

