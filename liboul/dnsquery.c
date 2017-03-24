/**
 * @file dnsquery.c DNS query API
 * @ingroup core
 */

#include "internal.h"

#include "dnsquery.h"

#include "notify.h"
#include "prefs.h"
#include "util.h"
#include "eventloop.h"
#include "debug.h"



/**************************************************************************
 * DNS query API
 **************************************************************************/

static OulDnsQueryUiOps *dns_query_ui_ops = NULL;

typedef struct _OulDnsQueryResolverProcess OulDnsQueryResolverProcess;

struct _OulDnsQueryData {
	char *hostname;
	int port;
	OulDnsQueryConnectFunction callback;
	gpointer data;
	guint timeout;
};

static void
oul_dnsquery_resolved(OulDnsQueryData *query_data, GSList *hosts)
{
	oul_debug_info("dnsquery", "IP resolved for %s\n", query_data->hostname);
	if (query_data->callback != NULL)
		query_data->callback(hosts, query_data->data, NULL);
	else
	{
		/*
		 * Callback is a required parameter, but it can get set to
		 * NULL if we cancel a thread-based DNS lookup.  So we need
		 * to free hosts.
		 */
		while (hosts != NULL)
		{
			hosts = g_slist_remove(hosts, hosts->data);
			g_free(hosts->data);
			hosts = g_slist_remove(hosts, hosts->data);
		}
	}

	oul_dnsquery_destroy(query_data);
}

static void
oul_dnsquery_failed(OulDnsQueryData *query_data, const gchar *error_message)
{
	oul_debug_info("dnsquery", "%s\n", error_message);
	if (query_data->callback != NULL)
		query_data->callback(NULL, query_data->data, error_message);
	oul_dnsquery_destroy(query_data);
}

static gboolean
oul_dnsquery_ui_resolve(OulDnsQueryData *query_data)
{
	OulDnsQueryUiOps *ops = oul_dnsquery_get_ui_ops();

	if (ops && ops->resolve_host)
	{
		if (ops->resolve_host(query_data, oul_dnsquery_resolved, oul_dnsquery_failed))
			return TRUE;
	}

	return FALSE;
}

/*
 * We weren't able to do anything fancier above, so use the
 * fail-safe name resolution code, which is blocking.
 */

static gboolean
resolve_host(gpointer data)
{
	OulDnsQueryData *query_data;
	struct sockaddr_in sin;
	GSList *hosts = NULL;

	query_data = data;
	query_data->timeout = 0;

	if (oul_dnsquery_ui_resolve(query_data))
	{
		/* The UI is handling the resolve; we're done */
		return FALSE;
	}

	if (!inet_aton(query_data->hostname, &sin.sin_addr)) {
		struct hostent *hp;
		if(!(hp = gethostbyname(query_data->hostname))) {
			char message[1024];
			g_snprintf(message, sizeof(message), _("Error resolving %s: %d"),
					query_data->hostname, h_errno);
			oul_dnsquery_failed(query_data, message);
			return FALSE;
		}
		memset(&sin, 0, sizeof(struct sockaddr_in));
		memcpy(&sin.sin_addr.s_addr, hp->h_addr, hp->h_length);
		sin.sin_family = hp->h_addrtype;
	} else
		sin.sin_family = AF_INET;
	sin.sin_port = htons(query_data->port);

	hosts = g_slist_append(hosts, GINT_TO_POINTER(sizeof(sin)));
	hosts = g_slist_append(hosts, g_memdup(&sin, sizeof(sin)));

	oul_dnsquery_resolved(query_data, hosts);

	return FALSE;
}

OulDnsQueryData *
oul_dnsquery_a(const char *hostname, int port,
				OulDnsQueryConnectFunction callback, gpointer data)
{
	OulDnsQueryData *query_data;

	g_return_val_if_fail(hostname != NULL, NULL);
	g_return_val_if_fail(port	  != 0, NULL);
	g_return_val_if_fail(callback != NULL, NULL);

	query_data = g_new(OulDnsQueryData, 1);
	query_data->hostname = g_strdup(hostname);
	g_strstrip(query_data->hostname);
	query_data->port = port;
	query_data->callback = callback;
	query_data->data = data;

	if (strlen(query_data->hostname) == 0)
	{
		oul_dnsquery_destroy(query_data);
		g_return_val_if_reached(NULL);
	}

	/* Don't call the callback before returning */
	query_data->timeout = oul_timeout_add(0, resolve_host, query_data);

	return query_data;
}

void
oul_dnsquery_destroy(OulDnsQueryData *query_data)
{
	OulDnsQueryUiOps *ops = oul_dnsquery_get_ui_ops();

	if (ops && ops->destroy)
		ops->destroy(query_data);

	if (query_data->timeout > 0)
		oul_timeout_remove(query_data->timeout);

	g_free(query_data->hostname);
	g_free(query_data);
}

char *
oul_dnsquery_get_host(OulDnsQueryData *query_data)
{
	g_return_val_if_fail(query_data != NULL, NULL);

	return query_data->hostname;
}

unsigned short
oul_dnsquery_get_port(OulDnsQueryData *query_data)
{
	g_return_val_if_fail(query_data != NULL, 0);

	return query_data->port;
}

void
oul_dnsquery_set_ui_ops(OulDnsQueryUiOps *ops)
{
	dns_query_ui_ops = ops;
}

OulDnsQueryUiOps *
oul_dnsquery_get_ui_ops(void)
{
	/* It is perfectly acceptable for dns_query_ui_ops to be NULL; this just
	 * means that the default platform-specific implementation will be used.
	 */
	return dns_query_ui_ops;
}

void
oul_dnsquery_init(void)
{
}

void
oul_dnsquery_uninit(void)
{
#if defined(OUL_DNSQUERY_USE_FORK)
	while (free_dns_children != NULL)
	{
		oul_dnsquery_resolver_destroy(free_dns_children->data);
		free_dns_children = g_slist_remove(free_dns_children, free_dns_children->data);
	}
#endif
}
