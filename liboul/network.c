/**
 * @file network.c Network Implementation
 * @ingroup core
 */

#include "internal.h"

#include <arpa/nameser.h>
#include <resolv.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>

/* Solaris */
#if defined (__SVR4) && defined (__sun)
#include <sys/sockio.h>
#endif

#include "eventloop.h"
#include "signals.h"
#include "debug.h"
#include "network.h"
#include "prefs.h"
#include "stun.h"
#include "nat-pmp.h"
#include "upnp.h"
#include "nat-pmp.h"

/*
 * Calling sizeof(struct ifreq) isn't always correct on
 * Mac OS X (and maybe others).
 */
#ifdef _SIZEOF_ADDR_IFREQ
#  define HX_SIZE_OF_IFREQ(a) _SIZEOF_ADDR_IFREQ(a)
#else
#  define HX_SIZE_OF_IFREQ(a) sizeof(a)
#endif

struct _OulNetworkListenData {
	int listenfd;
	int socket_type;
	gboolean retry;
	gboolean adding;
	OulNetworkListenCallback cb;
	gpointer cb_data;
	UPnPMappingAddRemove *mapping_data;
};

const unsigned char *
oul_network_ip_atoi(const char *ip)
{
	static unsigned char ret[4];
	gchar *delimiter = ".";
	gchar **split;
	int i;

	g_return_val_if_fail(ip != NULL, NULL);

	split = g_strsplit(ip, delimiter, 4);
	for (i = 0; split[i] != NULL; i++)
		ret[i] = atoi(split[i]);
	g_strfreev(split);

	/* i should always be 4 */
	if (i != 4)
		return NULL;

	return ret;
}

void
oul_network_set_public_ip(const char *ip)
{
	g_return_if_fail(ip != NULL);

	/* XXX - Ensure the IP address is valid */

	oul_prefs_set_string("/oul/network/public_ip", ip);
}

const char *
oul_network_get_public_ip(void)
{
	return oul_prefs_get_string("/oul/network/public_ip");
}

const char *
oul_network_get_local_system_ip(int fd)
{
	char buffer[1024];
	static char ip[16];
	char *tmp;
	struct ifconf ifc;
	struct ifreq *ifr;
	struct sockaddr_in *sinptr;
	guint32 lhost = htonl(127 * 256 * 256 * 256 + 1);
	long unsigned int add;
	int source = fd;

	if (fd < 0)
		source = socket(PF_INET,SOCK_STREAM, 0);

	ifc.ifc_len = sizeof(buffer);
	ifc.ifc_req = (struct ifreq *)buffer;
	ioctl(source, SIOCGIFCONF, &ifc);

	if (fd < 0)
		close(source);

	tmp = buffer;
	while (tmp < buffer + ifc.ifc_len)
	{
		ifr = (struct ifreq *)tmp;
		tmp += HX_SIZE_OF_IFREQ(*ifr);

		if (ifr->ifr_addr.sa_family == AF_INET)
		{
			sinptr = (struct sockaddr_in *)&ifr->ifr_addr;
			if (sinptr->sin_addr.s_addr != lhost)
			{
				add = ntohl(sinptr->sin_addr.s_addr);
				g_snprintf(ip, 16, "%lu.%lu.%lu.%lu",
					((add >> 24) & 255),
					((add >> 16) & 255),
					((add >> 8) & 255),
					add & 255);

				return ip;
			}
		}
	}

	return "0.0.0.0";
}

const char *
oul_network_get_my_ip(int fd)
{
	const char *ip = NULL;
	OulStunNatDiscovery *stun;

	/* Check if the user specified an IP manually */
	if (!oul_prefs_get_bool("/oul/network/auto_ip")) {
		ip = oul_network_get_public_ip();
		/* Make sure the IP address entered by the user is valid */
		if ((ip != NULL) && (oul_network_ip_atoi(ip) != NULL))
			return ip;
	} else {
		/* Check if STUN discovery was already done */
		stun = oul_stun_discover(NULL);
		if ((stun != NULL) && (stun->status == OUL_STUN_STATUS_DISCOVERED))
			return stun->publicip;

		/* Attempt to get the IP from a NAT device using UPnP */
		ip = oul_upnp_get_public_ip();
		if (ip != NULL)
			return ip;

		/* Attempt to get the IP from a NAT device using NAT-PMP */
		ip = oul_pmp_get_public_ip();
		if (ip != NULL)
			return ip;
	}

	/* Just fetch the IP of the local system */
	return oul_network_get_local_system_ip(fd);
}


static void
oul_network_set_upnp_port_mapping_cb(gboolean success, gpointer data)
{
	OulNetworkListenData *listen_data;

	listen_data = data;
	/* TODO: Once we're keeping track of upnp requests... */
	/* listen_data->pnp_data = NULL; */

	if (!success) {
		oul_debug_info("network", "Couldn't create UPnP mapping\n");
		if (listen_data->retry) {
			listen_data->retry = FALSE;
			listen_data->adding = FALSE;
			listen_data->mapping_data = oul_upnp_remove_port_mapping(
						oul_network_get_port_from_fd(listen_data->listenfd),
						(listen_data->socket_type == SOCK_STREAM) ? "TCP" : "UDP",
						oul_network_set_upnp_port_mapping_cb, listen_data);
			return;
		}
	} else if (!listen_data->adding) {
		/* We've tried successfully to remove the port mapping.
		 * Try to add it again */
		listen_data->adding = TRUE;
		listen_data->mapping_data = oul_upnp_set_port_mapping(
					oul_network_get_port_from_fd(listen_data->listenfd),
					(listen_data->socket_type == SOCK_STREAM) ? "TCP" : "UDP",
					oul_network_set_upnp_port_mapping_cb, listen_data);
		return;
	}

	if (listen_data->cb)
		listen_data->cb(listen_data->listenfd, listen_data->cb_data);

	/* Clear the UPnP mapping data, since it's complete and oul_netweork_listen_cancel() will try to cancel
	 * it otherwise. */
	listen_data->mapping_data = NULL;
	oul_network_listen_cancel(listen_data);
}

static gboolean
oul_network_finish_pmp_map_cb(gpointer data)
{
	OulNetworkListenData *listen_data;

	listen_data = data;

	if (listen_data->cb)
		listen_data->cb(listen_data->listenfd, listen_data->cb_data);

	oul_network_listen_cancel(listen_data);

	return FALSE;
}

static gboolean listen_map_external = TRUE;
void oul_network_listen_map_external(gboolean map_external)
{
	listen_map_external = map_external;
}

static OulNetworkListenData *
oul_network_do_listen(unsigned short port, int socket_type, OulNetworkListenCallback cb, gpointer cb_data)
{
	int listenfd = -1;
	int flags;
	const int on = 1;
	OulNetworkListenData *listen_data;
	unsigned short actual_port;
#ifdef HAVE_GETADDRINFO
	int errnum;
	struct addrinfo hints, *res, *next;
	char serv[6];

	/*
	 * Get a list of addresses on this machine.
	 */
	snprintf(serv, sizeof(serv), "%hu", port);
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = socket_type;
	errnum = getaddrinfo(NULL /* any IP */, serv, &hints, &res);
	if (errnum != 0) {
		oul_debug_warning("network", "getaddrinfo: %s\n", oul_gai_strerror(errnum));
		if (errnum == EAI_SYSTEM)
			oul_debug_warning("network", "getaddrinfo: system error: %s\n", g_strerror(errno));

		return NULL;
	}

	/*
	 * Go through the list of addresses and attempt to listen on
	 * one of them.
	 * XXX - Try IPv6 addresses first?
	 */
	for (next = res; next != NULL; next = next->ai_next) {
		listenfd = socket(next->ai_family, next->ai_socktype, next->ai_protocol);
		if (listenfd < 0)
			continue;
		if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0)
			oul_debug_warning("network", "setsockopt: %s\n", g_strerror(errno));
		if (bind(listenfd, next->ai_addr, next->ai_addrlen) == 0)
			break; /* success */
		/* XXX - It is unclear to me (datallah) whether we need to be
		   using a new socket each time */
		close(listenfd);
	}

	freeaddrinfo(res);

	if (next == NULL)
		return NULL;
#else
	struct sockaddr_in sockin;

	if ((listenfd = socket(AF_INET, socket_type, 0)) < 0) {
		oul_debug_warning("network", "socket: %s\n", g_strerror(errno));
		return NULL;
	}

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0)
		oul_debug_warning("network", "setsockopt: %s\n", g_strerror(errno));

	memset(&sockin, 0, sizeof(struct sockaddr_in));
	sockin.sin_family = PF_INET;
	sockin.sin_port = htons(port);

	if (bind(listenfd, (struct sockaddr *)&sockin, sizeof(struct sockaddr_in)) != 0) {
		oul_debug_warning("network", "bind: %s\n", g_strerror(errno));
		close(listenfd);
		return NULL;
	}
#endif

	if (socket_type == SOCK_STREAM && listen(listenfd, 4) != 0) {
		oul_debug_warning("network", "listen: %s\n", g_strerror(errno));
		close(listenfd);
		return NULL;
	}

	flags = fcntl(listenfd, F_GETFL);
	fcntl(listenfd, F_SETFL, flags | O_NONBLOCK);
	fcntl(listenfd, F_SETFD, FD_CLOEXEC);

	actual_port = oul_network_get_port_from_fd(listenfd);

	oul_debug_info("network", "Listening on port: %hu\n", actual_port);

	listen_data = g_new0(OulNetworkListenData, 1);
	listen_data->listenfd = listenfd;
	listen_data->adding = TRUE;
	listen_data->retry = TRUE;
	listen_data->cb = cb;
	listen_data->cb_data = cb_data;
	listen_data->socket_type = socket_type;

	if (!listen_map_external || !oul_prefs_get_bool("/oul/network/map_ports"))
	{
		oul_debug_info("network", "Skipping external port mapping.\n");
		/* The pmp_map_cb does what we want to do */
		oul_timeout_add(0, oul_network_finish_pmp_map_cb, listen_data);
	}
	/* Attempt a NAT-PMP Mapping, which will return immediately */
	else if (oul_pmp_create_map(((socket_type == SOCK_STREAM) ? OUL_PMP_TYPE_TCP : OUL_PMP_TYPE_UDP),
							  actual_port, actual_port, OUL_PMP_LIFETIME))
	{
		oul_debug_info("network", "Created NAT-PMP mapping on port %i\n", actual_port);
		/* We want to return listen_data now, and on the next run loop trigger the cb and destroy listen_data */
		oul_timeout_add(0, oul_network_finish_pmp_map_cb, listen_data);
	}
	else
	{
		/* Attempt a UPnP Mapping */
		listen_data->mapping_data = oul_upnp_set_port_mapping(
						 actual_port,
						 (socket_type == SOCK_STREAM) ? "TCP" : "UDP",
						 oul_network_set_upnp_port_mapping_cb, listen_data);
	}

	return listen_data;
}

OulNetworkListenData *
oul_network_listen(unsigned short port, int socket_type,
		OulNetworkListenCallback cb, gpointer cb_data)
{
	g_return_val_if_fail(port != 0, NULL);

	return oul_network_do_listen(port, socket_type, cb, cb_data);
}

OulNetworkListenData *
oul_network_listen_range(unsigned short start, unsigned short end,
		int socket_type, OulNetworkListenCallback cb, gpointer cb_data)
{
	OulNetworkListenData *ret = NULL;

	if (oul_prefs_get_bool("/oul/network/ports_range_use")) {
		start = oul_prefs_get_int("/oul/network/ports_range_start");
		end = oul_prefs_get_int("/oul/network/ports_range_end");
	} else {
		if (end < start)
			end = start;
	}

	for (; start <= end; start++) {
		ret = oul_network_do_listen(start, socket_type, cb, cb_data);
		if (ret != NULL)
			break;
	}

	return ret;
}

void oul_network_listen_cancel(OulNetworkListenData *listen_data)
{
	if (listen_data->mapping_data != NULL)
		oul_upnp_cancel_port_mapping(listen_data->mapping_data);

	g_free(listen_data);
}

unsigned short
oul_network_get_port_from_fd(int fd)
{
	struct sockaddr_in addr;
	socklen_t len;

	g_return_val_if_fail(fd >= 0, 0);

	len = sizeof(addr);
	if (getsockname(fd, (struct sockaddr *) &addr, &len) == -1) {
		oul_debug_warning("network", "getsockname: %s\n", g_strerror(errno));
		return 0;
	}

	return ntohs(addr.sin_port);
}

gboolean
oul_network_is_available(void)
{
	return TRUE;
}

void *
oul_network_get_handle(void)
{
	static int handle;

	return &handle;
}

void
oul_network_init(void)
{
	oul_prefs_add_none  ("/oul/network");
	oul_prefs_add_bool  ("/oul/network/auto_ip", TRUE);
	oul_prefs_add_string("/oul/network/public_ip", "");
	oul_prefs_add_bool  ("/oul/network/map_ports", TRUE);
	oul_prefs_add_bool  ("/oul/network/ports_range_use", FALSE);
	oul_prefs_add_int   ("/oul/network/ports_range_start", 1024);
	oul_prefs_add_int   ("/oul/network/ports_range_end", 2048);
	oul_prefs_add_string("/oul/network/stun_server", "");

	if(oul_prefs_get_bool("/oul/network/map_ports") || oul_prefs_get_bool("/oul/network/auto_ip"))
		oul_upnp_discover(NULL, NULL);

	oul_signal_register(oul_network_get_handle(), "network-configuration-changed",
						   oul_marshal_VOID, NULL, 0);

	oul_pmp_init();
	oul_upnp_init();
}

void
oul_network_uninit(void)
{
	oul_signal_unregister(oul_network_get_handle(), "network-configuration-changed");
}

