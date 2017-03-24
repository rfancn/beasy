/**
 * @file nat-pmp.c NAT-PMP Implementation
 * @ingroup core
 */

/* NAT Port Mapping Protocol (NAT-PMP) is an Internet Engineering Task Force Internet Draft, 
 * introduced by Apple Computer as an alternative to the more common
 * Internet Gateway Device (IGD) Standardized Device Control Protocol 
 * implemented in many network address translation (NAT) routers
 */

#include "nat-pmp.h"
#include "internal.h"
#include "debug.h"
#include "signals.h"
#include "network.h"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

/* We will need sysctl() and NET_RT_DUMP, both of which are not present
 * on all platforms, to continue. */
#if defined(HAVE_SYS_SYSCTL_H) && defined(NET_RT_DUMP)

#include <sys/types.h>
#include <net/route.h>

#define PMP_DEBUG	1

typedef struct {
	guint8	version;
	guint8 opcode;
} OulPmpIpRequest;

typedef struct {
	guint8		version;
	guint8		opcode; /* 128 + n */
	guint16		resultcode;
	guint32		epoch;
	guint32		address;
} OulPmpIpResponse;

typedef struct {
	guint8		version;
	guint8		opcode;
	char		reserved[2];
	guint16		privateport;
	guint16		publicport;
	guint32		lifetime;
} OulPmpMapRequest;

struct _OulPmpMapResponse {
	guint8		version;
	guint8		opcode;
	guint16		resultcode;
	guint32		epoch;
	guint16		privateport;
	guint16		publicport;
	guint32		lifetime;
};

typedef struct _OulPmpMapResponse OulPmpMapResponse;

typedef enum {
	OUL_PMP_STATUS_UNDISCOVERED = -1,
	OUL_PMP_STATUS_UNABLE_TO_DISCOVER,
	OUL_PMP_STATUS_DISCOVERING,
	OUL_PMP_STATUS_DISCOVERED
} OulUPnPStatus;

typedef struct {
	OulUPnPStatus status;
	gchar *publicip;
} OulPmpInfo;

static OulPmpInfo pmp_info = {OUL_PMP_STATUS_UNDISCOVERED, NULL};

/*
 *	Thanks to R. Matthew Emerson for the fixes on this
 */

#define PMP_MAP_OPCODE_UDP	1
#define PMP_MAP_OPCODE_TCP	2

#define PMP_VERSION			0
#define PMP_PORT			5351
#define PMP_TIMEOUT			250000	/* 250000 useconds */

/* alignment constraint for routing socket */
#define ROUNDUP(a)			((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n)		(x += ROUNDUP((n)->sa_len))

static void
get_rtaddrs(int bitmask, struct sockaddr *sa, struct sockaddr *addrs[])
{
	int i;

	for (i = 0; i < RTAX_MAX; i++)
	{
		if (bitmask & (1 << i)) 
		{
			addrs[i] = sa;
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
			sa = (struct sockaddr *)(ROUNDUP(sa->sa_len) + (char *)sa);
#else
			if (sa->sa_family == AF_INET)
				sa = (struct sockaddr*)(sizeof(struct sockaddr_in) + (char *)sa);
#ifdef AF_INET6
			else if (sa->sa_family == AF_INET6)
				sa = (struct sockaddr*)(sizeof(struct sockaddr_in6) + (char *)sa);
#endif
#endif
		} 
		else
		{
			addrs[i] = NULL;
		}
	}
}

static int
is_default_route(struct sockaddr *sa, struct sockaddr *mask)
{
    struct sockaddr_in *sin;

    if (sa->sa_family != AF_INET)
		return 0;

    sin = (struct sockaddr_in *)sa;
    if ((sin->sin_addr.s_addr == INADDR_ANY) &&
		mask &&
		(ntohl(((struct sockaddr_in *)mask)->sin_addr.s_addr) == 0L ||
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
		 mask->sa_len == 0
#else
		0
#endif
		))
		return 1;
    else
		return 0;
}

/*!
 * The return sockaddr_in must be g_free()'d when no longer needed
 */
static struct sockaddr_in *
default_gw()
{
	int mib[6];
    size_t needed;
    char *buf, *next, *lim;
    struct rt_msghdr *rtm;
    struct sockaddr *sa;
	struct sockaddr_in *sin = NULL;
	gboolean found = FALSE;

    mib[0] = CTL_NET;
    mib[1] = PF_ROUTE; /* entire routing table or a subset of it */
    mib[2] = 0; /* protocol number - always 0 */
    mib[3] = 0; /* address family - 0 for all addres families */
    mib[4] = NET_RT_DUMP;
    mib[5] = 0;

	/* Determine the buffer side needed to get the full routing table */
    if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) 
	{
		oul_debug_warning("nat-pmp", "sysctl: net.route.0.0.dump estimate\n");
		return NULL;
    }

    if (!(buf = malloc(needed)))
	{
		oul_debug_warning("nat-pmp", "Failed to malloc %i\n", needed);
		return NULL;
    }

	/* Read the routing table into buf */
    if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) 
	{
		oul_debug_warning("nat-pmp", "sysctl: net.route.0.0.dump\n");
		return NULL;
    }

    lim = buf + needed;

    for (next = buf; next < lim; next += rtm->rtm_msglen) 
	{
		rtm = (struct rt_msghdr *)next;
		sa = (struct sockaddr *)(rtm + 1);
		
		if (sa->sa_family == AF_INET) 
		{
			sin = (struct sockaddr_in*) sa;

			if ((rtm->rtm_flags & RTF_GATEWAY) && sin->sin_addr.s_addr == INADDR_ANY)
			{
				/* We found the default route. Now get the destination address and netmask. */
	            struct sockaddr *rti_info[RTAX_MAX];
				struct sockaddr addr, mask;

				get_rtaddrs(rtm->rtm_addrs, sa, rti_info);
				memset(&addr, 0, sizeof(addr));

				if (rtm->rtm_addrs & RTA_DST)
					memcpy(&addr, rti_info[RTAX_DST], sizeof(addr));

				memset(&mask, 0, sizeof(mask));

				if (rtm->rtm_addrs & RTA_NETMASK)
					memcpy(&mask, rti_info[RTAX_NETMASK], sizeof(mask));

				if (rtm->rtm_addrs & RTA_GATEWAY &&
					is_default_route(&addr, &mask)) 
				{
					if (rti_info[RTAX_GATEWAY]) {
						struct sockaddr_in *rti_sin = (struct sockaddr_in *)rti_info[RTAX_GATEWAY];
						sin = g_new0(struct sockaddr_in, 1);
						sin->sin_family = rti_sin->sin_family;
						sin->sin_port = rti_sin->sin_port;
						sin->sin_addr.s_addr = rti_sin->sin_addr.s_addr;
						memcpy(sin, rti_info[RTAX_GATEWAY], sizeof(struct sockaddr_in));

						oul_debug_info("nat-pmp", "Found a default gateway\n");
						found = TRUE;
						break;
					}
				}
			}
		}
    }

	return (found ? sin : NULL);
}

/*!
 *	oul_pmp_get_public_ip() will return the publicly facing IP address of the 
 *	default NAT gateway. The function will return NULL if:
 *		- The gateway doesn't support NAT-PMP
 *		- The gateway errors in some other spectacular fashion
 */
char *
oul_pmp_get_public_ip()
{
	struct sockaddr_in addr, *gateway, *publicsockaddr = NULL;
	struct timeval req_timeout;
	socklen_t len;

	OulPmpIpRequest req;
	OulPmpIpResponse resp;
	int sendfd;
	
	if (pmp_info.status == OUL_PMP_STATUS_UNABLE_TO_DISCOVER)
		return NULL;
	
	if ((pmp_info.status == OUL_PMP_STATUS_DISCOVERED) && (pmp_info.publicip != NULL))
	{
#ifdef PMP_DEBUG
		oul_debug_info("nat-pmp", "Returning cached publicip %s\n",pmp_info.publicip);
#endif
		return pmp_info.publicip;
	}

	gateway = default_gw();

	if (!gateway)
	{
		oul_debug_info("nat-pmp", "Cannot request public IP from a NULL gateway!\n");
		/* If we get a NULL gateway, don't try again next time */
		pmp_info.status = OUL_PMP_STATUS_UNABLE_TO_DISCOVER;
		return NULL;
	}

	/* Default port for NAT-PMP is 5351 */
	if (gateway->sin_port != PMP_PORT)
		gateway->sin_port = htons(PMP_PORT);

	req_timeout.tv_sec = 0;
	req_timeout.tv_usec = PMP_TIMEOUT;

	sendfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	/* Clean out both req and resp structures */
	memset(&req, 0, sizeof(OulPmpIpRequest));
	memset(&resp, 0, sizeof(OulPmpIpResponse));
	req.version = 0;
	req.opcode = 0;

	/* The NAT-PMP spec says we should attempt to contact the gateway 9 times, doubling the time we wait each time.
	 * Even starting with a timeout of 0.1 seconds, that means that we have a total waiting of 204.6 seconds.
	 * With the recommended timeout of 0.25 seconds, we're talking 511.5 seconds (8.5 minutes).
	 * 
	 * This seems really silly... if this were nonblocking, a couple retries might be in order, but it's not at present.
	 */
#ifdef PMP_DEBUG
	oul_debug_info("nat-pmp", "Attempting to retrieve the public ip address for the NAT device at: %s\n", inet_ntoa(gateway->sin_addr));
	oul_debug_info("nat-pmp", "\tTimeout: %ds %dus\n", req_timeout.tv_sec, req_timeout.tv_usec);
#endif

	/* TODO: Non-blocking! */
	
	if (sendto(sendfd, &req, sizeof(req), 0, (struct sockaddr *)(gateway), sizeof(struct sockaddr)) < 0)
	{
		oul_debug_info("nat-pmp", "There was an error sending the NAT-PMP public IP request! (%s)\n", g_strerror(errno));
		g_free(gateway);
		pmp_info.status = OUL_PMP_STATUS_UNABLE_TO_DISCOVER;
		return NULL;
	}

	if (setsockopt(sendfd, SOL_SOCKET, SO_RCVTIMEO, &req_timeout, sizeof(req_timeout)) < 0)
	{
		oul_debug_info("nat-pmp", "There was an error setting the socket's options! (%s)\n", g_strerror(errno));
		g_free(gateway);
		pmp_info.status = OUL_PMP_STATUS_UNABLE_TO_DISCOVER;
		return NULL;
	}

	/* TODO: Non-blocking! */
	len = sizeof(struct sockaddr_in);
	if (recvfrom(sendfd, &resp, sizeof(OulPmpIpResponse), 0, (struct sockaddr *)(&addr), &len) < 0)
	{
		if (errno != EAGAIN)
		{
			oul_debug_info("nat-pmp", "There was an error receiving the response from the NAT-PMP device! (%s)\n", g_strerror(errno));
			g_free(gateway);
			pmp_info.status = OUL_PMP_STATUS_UNABLE_TO_DISCOVER;
			return NULL;
		}
	}

	if (addr.sin_addr.s_addr == gateway->sin_addr.s_addr)
		publicsockaddr = &addr;
	else
	{
		oul_debug_info("nat-pmp", "Response was not received from our gateway! Instead from: %s\n", inet_ntoa(addr.sin_addr));
		g_free(gateway);

		pmp_info.status = OUL_PMP_STATUS_UNABLE_TO_DISCOVER;
		return NULL;
	}

	if (!publicsockaddr) {
		g_free(gateway);
		
		pmp_info.status = OUL_PMP_STATUS_UNABLE_TO_DISCOVER;
		return NULL;
	}

#ifdef PMP_DEBUG
	oul_debug_info("nat-pmp", "Response received from NAT-PMP device:\n");
	oul_debug_info("nat-pmp", "version: %d\n", resp.version);
	oul_debug_info("nat-pmp", "opcode: %d\n", resp.opcode);
	oul_debug_info("nat-pmp", "resultcode: %d\n", ntohs(resp.resultcode));
	oul_debug_info("nat-pmp", "epoch: %d\n", ntohl(resp.epoch));
	struct in_addr in;
	in.s_addr = resp.address;
	oul_debug_info("nat-pmp", "address: %s\n", inet_ntoa(in));
#endif

	publicsockaddr->sin_addr.s_addr = resp.address;

	g_free(gateway);

	g_free(pmp_info.publicip);
	pmp_info.publicip = g_strdup(inet_ntoa(publicsockaddr->sin_addr));
	pmp_info.status = OUL_PMP_STATUS_DISCOVERED;

	return inet_ntoa(publicsockaddr->sin_addr);
}

gboolean
oul_pmp_create_map(OulPmpType type, unsigned short privateport, unsigned short publicport, int lifetime)
{
	struct sockaddr_in *gateway;
	gboolean success = TRUE;
	int sendfd;
	struct timeval req_timeout;
	OulPmpMapRequest req;
	OulPmpMapResponse *resp;

	gateway = default_gw();

	if (!gateway)
	{
		oul_debug_info("nat-pmp", "Cannot create mapping on a NULL gateway!\n");
		return FALSE;
	}

	/* Default port for NAT-PMP is 5351 */
	if (gateway->sin_port != PMP_PORT)
		gateway->sin_port = htons(PMP_PORT);

	resp = g_new0(OulPmpMapResponse, 1);

	req_timeout.tv_sec = 0;
	req_timeout.tv_usec = PMP_TIMEOUT;

	sendfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	/* Set up the req */
	memset(&req, 0, sizeof(OulPmpMapRequest));
	req.version = 0;
	req.opcode = ((type == OUL_PMP_TYPE_UDP) ? PMP_MAP_OPCODE_UDP : PMP_MAP_OPCODE_TCP);
	req.privateport = htons(privateport); /* What a difference byte ordering makes...d'oh! */
	req.publicport = htons(publicport);
	req.lifetime = htonl(lifetime);

	/* The NAT-PMP spec says we should attempt to contact the gateway 9 times, doubling the time we wait each time.
	 * Even starting with a timeout of 0.1 seconds, that means that we have a total waiting of 204.6 seconds.
	 * With the recommended timeout of 0.25 seconds, we're talking 511.5 seconds (8.5 minutes).
	 * 
	 * This seems really silly... if this were nonblocking, a couple retries might be in order, but it's not at present.
	 * XXX Make this nonblocking.
	 * XXX This code looks like the pmp_get_public_ip() code. Can it be consolidated?
	 */
#ifdef PMP_DEBUG
	oul_debug_info("nat-pmp", "Attempting to create a NAT-PMP mapping the private port %d, and the public port %d\n", privateport, publicport);
	oul_debug_info("nat-pmp", "\tTimeout: %ds %dus\n", req_timeout.tv_sec, req_timeout.tv_usec);
#endif

	/* TODO: Non-blocking! */
	success = (sendto(sendfd, &req, sizeof(req), 0, (struct sockaddr *)(gateway), sizeof(struct sockaddr)) >= 0);
	if (!success)
		oul_debug_info("nat-pmp", "There was an error sending the NAT-PMP mapping request! (%s)\n", g_strerror(errno));

	if (success)
	{
		success = (setsockopt(sendfd, SOL_SOCKET, SO_RCVTIMEO, &req_timeout, sizeof(req_timeout)) >= 0);
		if (!success)
			oul_debug_info("nat-pmp", "There was an error setting the socket's options! (%s)\n", g_strerror(errno));
	}

	if (success)
	{
		/* The original code treats EAGAIN as a reason to iterate.. but I've removed iteration. This may be a problem */
		/* TODO: Non-blocking! */
		success = ((recvfrom(sendfd, resp, sizeof(OulPmpMapResponse), 0, NULL, NULL) >= 0) ||
				   (errno == EAGAIN));
		if (!success)
			oul_debug_info("nat-pmp", "There was an error receiving the response from the NAT-PMP device! (%s)\n", g_strerror(errno));
	}

	if (success)
	{
		success = (resp->opcode == (req.opcode + 128));
		if (!success)
			oul_debug_info("nat-pmp", "The opcode for the response from the NAT device (%i) does not match the request opcode (%i + 128 = %i)!\n",
							  resp->opcode, req.opcode, req.opcode + 128);
	}

#ifdef PMP_DEBUG
	if (success)
	{
		oul_debug_info("nat-pmp", "Response received from NAT-PMP device:\n");
		oul_debug_info("nat-pmp", "version: %d\n", resp->version);
		oul_debug_info("nat-pmp", "opcode: %d\n", resp->opcode);
		oul_debug_info("nat-pmp", "resultcode: %d\n", ntohs(resp->resultcode));
		oul_debug_info("nat-pmp", "epoch: %d\n", ntohl(resp->epoch));
		oul_debug_info("nat-pmp", "privateport: %d\n", ntohs(resp->privateport));
		oul_debug_info("nat-pmp", "publicport: %d\n", ntohs(resp->publicport));
		oul_debug_info("nat-pmp", "lifetime: %d\n", ntohl(resp->lifetime));
	}
#endif

	g_free(resp);
	g_free(gateway);

	/* XXX The private port may actually differ from the one we requested, according to the spec.
	 * We don't handle that situation at present.
	 *
	 * TODO: Look at the result and verify it matches what we wanted; either return a failure if it doesn't,
	 * or change network.c to know what to do if the desired private port shifts as a result of the nat-pmp operation.
	 */
	return success;
}

gboolean
oul_pmp_destroy_map(OulPmpType type, unsigned short privateport)
{
	gboolean success;

	success = oul_pmp_create_map(((type == OUL_PMP_TYPE_UDP) ? PMP_MAP_OPCODE_UDP : PMP_MAP_OPCODE_TCP),
							privateport, 0, 0);
	if (!success)
		oul_debug_warning("nat-pmp", "Failed to properly destroy mapping for %s port %d!\n",
							 ((type == OUL_PMP_TYPE_UDP) ? "UDP" : "TCP"), privateport);

	return success;
}

static void
oul_pmp_network_config_changed_cb(void *data)
{
	pmp_info.status = OUL_PMP_STATUS_UNDISCOVERED;
	g_free(pmp_info.publicip);
	pmp_info.publicip = NULL;
}

static void*
oul_pmp_get_handle(void)
{
	static int handle;

	return &handle;
}

void
oul_pmp_init()
{
	oul_signal_connect(oul_network_get_handle(), "network-configuration-changed",
		  oul_pmp_get_handle(), OUL_CALLBACK(oul_pmp_network_config_changed_cb),
		  GINT_TO_POINTER(0));
}
#else /* #ifdef NET_RT_DUMP */
char *
oul_pmp_get_public_ip()
{
	return NULL;
}

gboolean
oul_pmp_create_map(OulPmpType type, unsigned short privateport, unsigned short publicport, int lifetime)
{
	return FALSE;
}

gboolean
oul_pmp_destroy_map(OulPmpType type, unsigned short privateport)
{
	return FALSE;
}

void
oul_pmp_init()
{

}
#endif /* #if !(defined(HAVE_SYS_SYCTL_H) && defined(NET_RT_DUMP)) */
