/**
 * @file proxy.h Proxy API
 * @ingroup core
 */

#ifndef _OUL_PROXY_H_
#define _OUL_PROXY_H_

#include <glib.h>
#include "eventloop.h"

/**
 * A type of proxy connection.
 */
typedef enum
{
	OUL_PROXY_USE_GLOBAL = -1,  /**< Use the global proxy information. */
	OUL_PROXY_NONE = 0,         /**< No proxy.                         */
	OUL_PROXY_HTTP,             /**< HTTP proxy.                       */
	OUL_PROXY_SOCKS4,           /**< SOCKS 4 proxy.                    */
	OUL_PROXY_SOCKS5,           /**< SOCKS 5 proxy.                    */
	OUL_PROXY_USE_ENVVAR        /**< Use environmental settings.       */

} OulProxyType;

/**
 * Information on proxy settings.
 */
typedef struct
{
	OulProxyType type;   /**< The proxy type.  */

	char *host;           /**< The host.        */
	int   port;           /**< The port number. */
	char *username;       /**< The username.    */
	char *password;       /**< The password.    */

} OulProxyInfo;

typedef struct _OulProxyConnectData OulProxyConnectData;

typedef void (*OulProxyConnectFunction)(gpointer data, gint source, const gchar *error_message);


#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/** @name Proxy structure API                                             */
/**************************************************************************/
/*@{*/

/**
 * Creates a proxy information structure.
 *
 * @return The proxy information structure.
 */
OulProxyInfo *oul_proxy_info_new(void);

/**
 * Destroys a proxy information structure.
 *
 * @param info The proxy information structure to destroy.
 */
void oul_proxy_info_destroy(OulProxyInfo *info);

/**
 * Sets the type of proxy.
 *
 * @param info The proxy information.
 * @param type The proxy type.
 */
void oul_proxy_info_set_type(OulProxyInfo *info, OulProxyType type);

/**
 * Sets the proxy host.
 *
 * @param info The proxy information.
 * @param host The host.
 */
void oul_proxy_info_set_host(OulProxyInfo *info, const char *host);

/**
 * Sets the proxy port.
 *
 * @param info The proxy information.
 * @param port The port.
 */
void oul_proxy_info_set_port(OulProxyInfo *info, int port);

/**
 * Sets the proxy username.
 *
 * @param info     The proxy information.
 * @param username The username.
 */
void oul_proxy_info_set_username(OulProxyInfo *info, const char *username);

/**
 * Sets the proxy password.
 *
 * @param info     The proxy information.
 * @param password The password.
 */
void oul_proxy_info_set_password(OulProxyInfo *info, const char *password);

/**
 * Returns the proxy's type.
 *
 * @param info The proxy information.
 *
 * @return The type.
 */
OulProxyType oul_proxy_info_get_type(const OulProxyInfo *info);

/**
 * Returns the proxy's host.
 *
 * @param info The proxy information.
 *
 * @return The host.
 */
const char *oul_proxy_info_get_host(const OulProxyInfo *info);

/**
 * Returns the proxy's port.
 *
 * @param info The proxy information.
 *
 * @return The port.
 */
int oul_proxy_info_get_port(const OulProxyInfo *info);

/**
 * Returns the proxy's username.
 *
 * @param info The proxy information.
 *
 * @return The username.
 */
const char *oul_proxy_info_get_username(const OulProxyInfo *info);

/**
 * Returns the proxy's password.
 *
 * @param info The proxy information.
 *
 * @return The password.
 */
const char *oul_proxy_info_get_password(const OulProxyInfo *info);

/*@}*/

/**************************************************************************/
/** @name Global Proxy API                                                */
/**************************************************************************/
/*@{*/

/**
 * Returns oul's global proxy information.
 *
 * @return The global proxy information.
 */
OulProxyInfo *oul_global_proxy_get_info(void);

/*@}*/

/**************************************************************************/
/** @name Proxy API                                                       */
/**************************************************************************/
/*@{*/

/**
 * Returns the proxy subsystem handle.
 *
 * @return The proxy subsystem handle.
 */
void *oul_proxy_get_handle(void);

/**
 * Initializes the proxy subsystem.
 */
void oul_proxy_init(void);

/**
 * Uninitializes the proxy subsystem.
 */
void oul_proxy_uninit(void);

/**
 * Makes a connection to the specified host and port.  Note that this
 * function name can be misleading--although it is called "proxy
 * connect," it is used for establishing any outgoing TCP connection,
 * whether through a proxy or not.
 *
 * @param handle     A handle that should be associated with this
 *                   connection attempt.  The handle can be used
 *                   to cancel the connection attempt using the
 *                   oul_proxy_connect_cancel_with_handle()
 *                   function.
 * @param host       The destination host.
 * @param port       The destination port.
 * @param connect_cb The function to call when the connection is
 *                   established.  If the connection failed then
 *                   fd will be -1 and error message will be set
 *                   to something descriptive (hopefully).
 * @param data       User-defined data.
 *
 * @return NULL if there was an error, or a reference to an
 *         opaque data structure that can be used to cancel
 *         the pending connection, if needed.
 */
OulProxyConnectData *oul_proxy_connect(void *handle,
			const char *host, int port,
			OulProxyConnectFunction connect_cb, gpointer data);

/**
 * Makes a connection through a SOCKS5 proxy.
 *
 * @param handle     A handle that should be associated with this
 *                   connection attempt.  The handle can be used
 *                   to cancel the connection attempt using the
 *                   oul_proxy_connect_cancel_with_handle()
 *                   function.
 * @param gpi        The OulProxyInfo specifying the proxy settings
 * @param host       The destination host.
 * @param port       The destination port.
 * @param connect_cb The function to call when the connection is
 *                   established.  If the connection failed then
 *                   fd will be -1 and error message will be set
 *                   to something descriptive (hopefully).
 * @param data       User-defined data.
 *
 * @return NULL if there was an error, or a reference to an
 *         opaque data structure that can be used to cancel
 *         the pending connection, if needed.
 */
OulProxyConnectData *oul_proxy_connect_socks5(void *handle,
			OulProxyInfo *gpi,
			const char *host, int port,
			OulProxyConnectFunction connect_cb, gpointer data);

/**
 * Cancel an in-progress connection attempt.  This should be called
 * by the PRPL if the user disables an account while it is still
 * performing the initial sign on.  Or when establishing a file
 * transfer, if we attempt to connect to a remote user but they
 * are behind a firewall then the PRPL can cancel the connection
 * attempt early rather than just letting the OS's TCP/IP stack
 * time-out the connection.
 */
void oul_proxy_connect_cancel(OulProxyConnectData *connect_data);

/*
 * Closes all proxy connections registered with the specified handle.
 *
 * @param handle The handle.
 */
void oul_proxy_connect_cancel_with_handle(void *handle);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _OUL_PROXY_H_ */
