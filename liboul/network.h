/**
 * @file network.h Network API
 * @ingroup core
 */

#ifndef _OUL_NETWORK_H_
#define _OUL_NETWORK_H_

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/** @name Network API                                                     */
/**************************************************************************/
/*@{*/

typedef struct _OulNetworkListenData OulNetworkListenData;

typedef void (*OulNetworkListenCallback) (int listenfd, gpointer data);

/**
 * Converts a dot-decimal IP address to an array of unsigned
 * chars.  For example, converts 192.168.0.1 to a 4 byte
 * array containing 192, 168, 0 and 1.
 *
 * @param ip An IP address in dot-decimal notiation.
 * @return An array of 4 bytes containing an IP addresses
 *         equivalent to the given parameter, or NULL if
 *         the given IP address is invalid.  This value
 *         is statically allocated and should not be
 *         freed.
 */
const unsigned char *oul_network_ip_atoi(const char *ip);

/**
 * Sets the IP address of the local system in preferences.  This
 * is the IP address that should be used for incoming connections
 * (file transfer, direct IM, etc.) and should therefore be
 * publicly accessible.
 *
 * @param ip The local IP address.
 */
void oul_network_set_public_ip(const char *ip);

/**
 * Returns the IP address of the local system set in preferences.
 *
 * This returns the value set via oul_network_set_public_ip().
 * You probably want to use oul_network_get_my_ip() instead.
 *
 * @return The local IP address set in preferences.
 */
const char *oul_network_get_public_ip(void);

/**
 * Returns the IP address of the local system.
 *
 * You probably want to use oul_network_get_my_ip() instead.
 *
 * @note The returned string is a pointer to a static buffer. If this
 *       function is called twice, it may be important to make a copy
 *       of the returned string.
 *
 * @param fd The fd to use to help figure out the IP, or else -1.
 * @return The local IP address.
 */
const char *oul_network_get_local_system_ip(int fd);

/**
 * Returns the IP address that should be used anywhere a
 * public IP addresses is needed (listening for an incoming
 * file transfer, etc).
 *
 * If the user has manually specified an IP address via
 * preferences, then this IP is returned.  Otherwise the
 * IP address returned by oul_network_get_local_system_ip()
 * is returned.
 *
 * @note The returned string is a pointer to a static buffer. If this
 *       function is called twice, it may be important to make a copy
 *       of the returned string.
 *
 * @param fd The fd to use to help figure out the IP, or -1.
 * @return The local IP address to be used.
 */
const char *oul_network_get_my_ip(int fd);

#ifndef OUL_DISABLE_DEPRECATED
/**
 * Should calls to oul_network_listen() and oul_network_listen_range()
 * map the port externally using NAT-PMP or UPnP?
 * The default value is TRUE
 *
 * @param map_external Should the open port be mapped externally?
 * @deprecated In 3.0.0 a boolean will be added to the above functions to
 *             perform the same function.
 * @since 2.3.0
 */
void oul_network_listen_map_external(gboolean map_external);
#endif

/**
 * Attempts to open a listening port ONLY on the specified port number.
 * You probably want to use oul_network_listen_range() instead of this.
 * This function is useful, for example, if you wanted to write a telnet
 * server as a Oul plugin, and you HAD to listen on port 23.  Why anyone
 * would want to do that is beyond me.
 *
 * This opens a listening port. The caller will want to set up a watcher
 * of type OUL_INPUT_READ on the fd returned in cb. It will probably call
 * accept in the watcher callback, and then possibly remove the watcher and close
 * the listening socket, and add a new watcher on the new socket accept
 * returned.
 *
 * @param port The port number to bind to.  Must be greater than 0.
 * @param socket_type The type of socket to open for listening.
 *   This will be either SOCK_STREAM for TCP or SOCK_DGRAM for UDP.
 * @param cb The callback to be invoked when the port to listen on is available.
 *           The file descriptor of the listening socket will be specified in
 *           this callback, or -1 if no socket could be established.
 * @param cb_data extra data to be returned when cb is called
 *
 * @return A pointer to a data structure that can be used to cancel
 *         the pending listener, or NULL if unable to obtain a local
 *         socket to listen on.
 */
OulNetworkListenData *oul_network_listen(unsigned short port,
		int socket_type, OulNetworkListenCallback cb, gpointer cb_data);

/**
 * Opens a listening port selected from a range of ports.  The range of
 * ports used is chosen in the following manner:
 * If a range is specified in preferences, these values are used.
 * If a non-0 values are passed to the function as parameters, these
 * values are used.
 * Otherwise a port is chosen at random by the operating system.
 *
 * This opens a listening port. The caller will want to set up a watcher
 * of type OUL_INPUT_READ on the fd returned in cb. It will probably call
 * accept in the watcher callback, and then possibly remove the watcher and close
 * the listening socket, and add a new watcher on the new socket accept
 * returned.
 *
 * @param start The port number to bind to, or 0 to pick a random port.
 *              Users are allowed to override this arg in prefs.
 * @param end The highest possible port in the range of ports to listen on,
 *            or 0 to pick a random port.  Users are allowed to override this
 *            arg in prefs.
 * @param socket_type The type of socket to open for listening.
 *   This will be either SOCK_STREAM for TCP or SOCK_DGRAM for UDP.
 * @param cb The callback to be invoked when the port to listen on is available.
 *           The file descriptor of the listening socket will be specified in
 *           this callback, or -1 if no socket could be established.
 * @param cb_data extra data to be returned when cb is called
 *
 * @return A pointer to a data structure that can be used to cancel
 *         the pending listener, or NULL if unable to obtain a local
 *         socket to listen on.
 */
OulNetworkListenData *oul_network_listen_range(unsigned short start,
		unsigned short end, int socket_type,
		OulNetworkListenCallback cb, gpointer cb_data);

/**
 * This can be used to cancel any in-progress listener connection
 * by passing in the return value from either oul_network_listen()
 * or oul_network_listen_range().
 *
 * @param listen_data This listener attempt will be canceled and
 *        the struct will be freed.
 */
void oul_network_listen_cancel(OulNetworkListenData *listen_data);

/**
 * Gets a port number from a file descriptor.
 *
 * @param fd The file descriptor. This should be a tcp socket. The current
 *           implementation probably dies on anything but IPv4. Perhaps this
 *           possible bug will inspire new and valuable contributors to Oul.
 * @return The port number, in host byte order.
 */
unsigned short oul_network_get_port_from_fd(int fd);

/**
 * Detects if there is an available Internet connection. Note that this call
 * could block for the amount of time specified in inet_detect_timeout, so
 * using it in a UI thread may cause uncomfortableness
 *
 * @return TRUE if the Internet is available
 */
gboolean oul_network_is_available(void);

/**
 * Get the handle for the network system
 *
 * @return the handle to the network system
 */
void *oul_network_get_handle(void);

/**
 * Initializes the network subsystem.
 */
void oul_network_init(void);

/**
 * Shuts down the network subsystem.
 */
void oul_network_uninit(void);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _OUL_NETWORK_H_ */
