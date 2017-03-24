/**
 * @file dnsquery.h DNS query API
 * @ingroup core
 */

#ifndef _OUL_DNSQUERY_H_
#define _OUL_DNSQUERY_H_

#include <glib.h>
#include "eventloop.h"

typedef struct _OulDnsQueryData OulDnsQueryData;

/**
 * The "hosts" parameter is a linked list containing pairs of
 * one size_t addrlen and one struct sockaddr *addr.  It should
 * be free'd by the callback function.
 */
typedef void (*OulDnsQueryConnectFunction)(GSList *hosts, gpointer data, const char *error_message);

/**
 * Callbacks used by the UI if it handles resolving DNS
 */
typedef void  (*OulDnsQueryResolvedCallback) (OulDnsQueryData *query_data, GSList *hosts);
typedef void  (*OulDnsQueryFailedCallback) (OulDnsQueryData *query_data, const gchar *error_message);

/**
 * DNS Request UI operations;  UIs should implement this if they want to do DNS
 * lookups themselves, rather than relying on the core.
 *
 * @see @ref ui-ops
 */
typedef struct
{
	/** If implemented, the UI is responsible for DNS queries */
	gboolean (*resolve_host)(OulDnsQueryData *query_data,
	                         OulDnsQueryResolvedCallback resolved_cb,
	                         OulDnsQueryFailedCallback failed_cb);

	/** Called just before @a query_data is freed; this should cancel any
	 *  further use of @a query_data the UI would make. Unneeded if
	 *  #resolve_host is not implemented.
	 */
	void (*destroy)(OulDnsQueryData *query_data);

	void (*_oul_reserved1)(void);
	void (*_oul_reserved2)(void);
	void (*_oul_reserved3)(void);
	void (*_oul_reserved4)(void);
} OulDnsQueryUiOps;

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/** @name DNS query API                                                   */
/**************************************************************************/
/*@{*/

/**
 * Perform an asynchronous DNS query.
 *
 * @param hostname The hostname to resolve.
 * @param port     A port number which is stored in the struct sockaddr.
 * @param callback The callback function to call after resolving.
 * @param data     Extra data to pass to the callback function.
 *
 * @return NULL if there was an error, otherwise return a reference to
 *         a data structure that can be used to cancel the pending
 *         DNS query, if needed.
 */
OulDnsQueryData *oul_dnsquery_a(const char *hostname, int port, OulDnsQueryConnectFunction callback, gpointer data);

/**
 * Cancel a DNS query and destroy the associated data structure.
 *
 * @param query_data The DNS query to cancel.  This data structure
 *        is freed by this function.
 */
void oul_dnsquery_destroy(OulDnsQueryData *query_data);

/**
 * Sets the UI operations structure to be used when doing a DNS
 * resolve.  The UI operations need only be set if the UI wants to
 * handle the resolve itself; otherwise, leave it as NULL.
 *
 * @param ops The UI operations structure.
 */
void oul_dnsquery_set_ui_ops(OulDnsQueryUiOps *ops);

/**
 * Returns the UI operations structure to be used when doing a DNS
 * resolve.
 *
 * @return The UI operations structure.
 */
OulDnsQueryUiOps *oul_dnsquery_get_ui_ops(void);

/**
 * Get the host associated with a OulDnsQueryData
 *
 * @param query_data The DNS query
 * @return The host.
 */
char *oul_dnsquery_get_host(OulDnsQueryData *query_data);

/**
 * Get the port associated with a OulDnsQueryData
 *
 * @param query_data The DNS query
 * @return The port.
 */
unsigned short oul_dnsquery_get_port(OulDnsQueryData *query_data);

/**
 * Initializes the DNS query subsystem.
 */
void oul_dnsquery_init(void);

/**
 * Uninitializes the DNS query subsystem.
 */
void oul_dnsquery_uninit(void);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _OUL_DNSQUERY_H_ */
