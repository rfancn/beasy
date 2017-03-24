/**
 * @file stun.c STUN (RFC3489) Implementation
 * @ingroup core
 */

/* oul
 *
 * STUN implementation inspired by jstun [http://jstun.javawi.de/]
 *
 * Simple Traversal of User Datagram Protocol Through Network Address Translators
 * 1. To detect whether the ip is behind the NAT
 * 2. If it is, what is the ip and port after tranlated by NAT
 * 3. If it is, what the type of the NAT
 * 
 * Please refer to: http://blogger.org.cn/blog/more.asp?name=airclh&id=18585
 */

#include "internal.h"

#include <net/if.h>
#include <sys/ioctl.h>

/* Solaris */
#if defined (__SVR4) && defined (__sun)
#include <sys/sockio.h>
#endif

#include "eventloop.h"
#include "debug.h"
#include "dnsquery.h"
#include "dnssrv.h"
#include "network.h"
#include "proxy.h"
#include "stun.h"
#include "prefs.h"

#define MSGTYPE_BINDINGREQUEST 0x0001
#define MSGTYPE_BINDINGRESPONSE 0x0101

#define ATTRIB_MAPPEDADDRESS 0x0001

struct stun_header {
	guint16 type;
	guint16 len;
	guint32 transid[4];
};

struct stun_attrib {
	guint16 type;
	guint16 len;
};

#ifdef NOTYET
struct stun_change {
	struct stun_header hdr;
	struct stun_attrib attrib;
	char value[4];
};
#endif

struct stun_conn {
	int fd;
	struct sockaddr_in addr;
	int test;
	int retry;
	guint incb;
	guint timeout;
	struct stun_header *packet;
	size_t packetsize;
};

static OulStunNatDiscovery nattype = {
	OUL_STUN_STATUS_UNDISCOVERED,
	OUL_STUN_NAT_TYPE_PUBLIC_IP,
	"\0", NULL, 0};

static GSList *callbacks = NULL;

static void close_stun_conn(struct stun_conn *sc) {

	if (sc->incb)
		oul_input_remove(sc->incb);

	if (sc->timeout)
		oul_timeout_remove(sc->timeout);

	if (sc->fd)
		close(sc->fd);

	g_free(sc);
}

static void do_callbacks(void) {
	while(callbacks) {
		StunCallback cb = callbacks->data;
		if(cb)
			cb(&nattype);
		callbacks = g_slist_remove(callbacks, cb);
	}
}

static gboolean timeoutfunc(gpointer data) {
	struct stun_conn *sc = data;
	if(sc->retry >= 2) {
		oul_debug_info("stun", "request timed out, giving up.\n");
		if(sc->test == 2)
			nattype.type = OUL_STUN_NAT_TYPE_SYMMETRIC;

		/* set unknown */
		nattype.status = OUL_STUN_STATUS_UNKNOWN;

		nattype.lookup_time = time(NULL);

		/* callbacks */
		do_callbacks();

		/* we don't need to remove the timeout (returning FALSE) */
		sc->timeout = 0;
		close_stun_conn(sc);

		return FALSE;
	}
	oul_debug_info("stun", "request timed out, retrying.\n");
	sc->retry++;
	sendto(sc->fd, sc->packet, sc->packetsize, 0,
		(struct sockaddr *)&(sc->addr), sizeof(struct sockaddr_in));
	return TRUE;
}

#ifdef NOTYET
static void do_test2(struct stun_conn *sc) {
	struct stun_change data;
	data.hdr.type = htons(0x0001);
	data.hdr.len = 0;
	data.hdr.transid[0] = rand();
	data.hdr.transid[1] = ntohl(((int)'g' << 24) + ((int)'a' << 16) + ((int)'i' << 8) + (int)'m');
	data.hdr.transid[2] = rand();
	data.hdr.transid[3] = rand();
	data.attrib.type = htons(0x003);
	data.attrib.len = htons(4);
	data.value[3] = 6;
	sc->packet = (struct stun_header*)&data;
	sc->packetsize = sizeof(struct stun_change);
	sc->retry = 0;
	sc->test = 2;
	sendto(sc->fd, sc->packet, sc->packetsize, 0, (struct sockaddr *)&(sc->addr), sizeof(struct sockaddr_in));
	sc->timeout = oul_timeout_add(500, (GSourceFunc) timeoutfunc, sc);
}
#endif

static void reply_cb(gpointer data, gint source, OulInputCondition cond) {
	struct stun_conn *sc = data;
	char buffer[65536];
	char *tmp;
	int len;
	struct in_addr in;
	struct stun_attrib *attrib;
	struct stun_header *hdr;
	struct ifconf ifc;
	struct ifreq *ifr;
	struct sockaddr_in *sinptr;

	len = recv(source, buffer, sizeof(buffer) - 1, 0);
	if (!len) {
		oul_debug_info("stun", "unable to read stun response\n");
		return;
	}
	buffer[len] = '\0';

	if (len < sizeof(struct stun_header)) {
		oul_debug_info("stun", "got invalid response\n");
		return;
	}

	hdr = (struct stun_header*) buffer;
	if (len != (ntohs(hdr->len) + sizeof(struct stun_header))) {
		oul_debug_info("stun", "got incomplete response\n");
		return;
	}

	/* wrong transaction */
	if(hdr->transid[0] != sc->packet->transid[0]
			|| hdr->transid[1] != sc->packet->transid[1]
			|| hdr->transid[2] != sc->packet->transid[2]
			|| hdr->transid[3] != sc->packet->transid[3]) {
		oul_debug_info("stun", "got wrong transid\n");
		return;
	}

	if(sc->test==1) {
		if (hdr->type != MSGTYPE_BINDINGRESPONSE) {
			oul_debug_info("stun",
				"Expected Binding Response, got %d\n",
				hdr->type);
			return;
		}

		tmp = buffer + sizeof(struct stun_header);
		while((buffer + len) > (tmp + sizeof(struct stun_attrib))) {
			attrib = (struct stun_attrib*) tmp;
			tmp += sizeof(struct stun_attrib);

			if (!((buffer + len) > (tmp + ntohs(attrib->len))))
				break;

			if(attrib->type == htons(ATTRIB_MAPPEDADDRESS)
					&& ntohs(attrib->len) == 8) {
				char *ip;
				/* Skip the first unused byte,
				 * the family(1 byte), and the port(2 bytes);
				 * then read the 4 byte IPv4 address */
				memcpy(&in.s_addr, tmp + 4, 4);
				ip = inet_ntoa(in);
				if(ip)
					strcpy(nattype.publicip, ip);
			}

			tmp += ntohs(attrib->len);
		}
		oul_debug_info("stun", "got public ip %s\n", nattype.publicip);
		nattype.status = OUL_STUN_STATUS_DISCOVERED;
		nattype.type = OUL_STUN_NAT_TYPE_UNKNOWN_NAT;
		nattype.lookup_time = time(NULL);

		/* is it a NAT? */

		ifc.ifc_len = sizeof(buffer);
		ifc.ifc_req = (struct ifreq *) buffer;
		ioctl(source, SIOCGIFCONF, &ifc);

		tmp = buffer;
		while(tmp < buffer + ifc.ifc_len) {
			ifr = (struct ifreq *) tmp;

			tmp += sizeof(struct ifreq);

			if(ifr->ifr_addr.sa_family == AF_INET) {
				/* we only care about ipv4 interfaces */
				sinptr = (struct sockaddr_in *) &ifr->ifr_addr;
				if(sinptr->sin_addr.s_addr == in.s_addr) {
					/* no NAT */
					oul_debug_info("stun", "no nat\n");
					nattype.type = OUL_STUN_NAT_TYPE_PUBLIC_IP;
				}
			}
		}

#ifndef NOTYET
		close_stun_conn(sc);
		do_callbacks();
#else
		oul_timeout_remove(sc->timeout);
		sc->timeout = 0;

		do_test2(sc);
	} else if(sc->test == 2) {
		close_stun_conn(sc);
		nattype.type = OUL_STUN_NAT_TYPE_FULL_CONE;
		do_callbacks();
#endif
	}
}


static void hbn_listen_cb(int fd, gpointer data) {
	GSList *hosts = data;
	struct stun_conn *sc;
	static struct stun_header hdr_data;
	int ret;

	if(fd < 0) {
		nattype.status = OUL_STUN_STATUS_UNKNOWN;
		nattype.lookup_time = time(NULL);
		do_callbacks();
		return;
	}

	sc = g_new0(struct stun_conn, 1);
	sc->fd = fd;

	sc->addr.sin_family = AF_INET;
	sc->addr.sin_port = htons(oul_network_get_port_from_fd(fd));
	sc->addr.sin_addr.s_addr = INADDR_ANY;

	sc->incb = oul_input_add(fd, OUL_INPUT_READ, reply_cb, sc);

	ret = GPOINTER_TO_INT(hosts->data);
	hosts = g_slist_remove(hosts, hosts->data);
	memcpy(&(sc->addr), hosts->data, sizeof(struct sockaddr_in));
	g_free(hosts->data);
	hosts = g_slist_remove(hosts, hosts->data);
	while(hosts) {
		hosts = g_slist_remove(hosts, hosts->data);
		g_free(hosts->data);
		hosts = g_slist_remove(hosts, hosts->data);
	}

	hdr_data.type = htons(MSGTYPE_BINDINGREQUEST);
	hdr_data.len = 0;
	hdr_data.transid[0] = rand();
	hdr_data.transid[1] = ntohl(((int)'g' << 24) + ((int)'a' << 16) + ((int)'i' << 8) + (int)'m');
	hdr_data.transid[2] = rand();
	hdr_data.transid[3] = rand();

	if(sendto(sc->fd, &hdr_data, sizeof(struct stun_header), 0,
			(struct sockaddr *)&(sc->addr),
			sizeof(struct sockaddr_in)) < sizeof(struct stun_header)) {
		nattype.status = OUL_STUN_STATUS_UNKNOWN;
		nattype.lookup_time = time(NULL);
		do_callbacks();
		close_stun_conn(sc);
		return;
	}
	sc->test = 1;
	sc->packet = &hdr_data;
	sc->packetsize = sizeof(struct stun_header);
	sc->timeout = oul_timeout_add(500, (GSourceFunc) timeoutfunc, sc);
}

static void hbn_cb(GSList *hosts, gpointer data, const char *error_message) {

	if(!hosts || !hosts->data) {
		nattype.status = OUL_STUN_STATUS_UNDISCOVERED;
		nattype.lookup_time = time(NULL);
		do_callbacks();
		return;
	}

	if (!oul_network_listen_range(12108, 12208, SOCK_DGRAM, hbn_listen_cb, hosts)) {
		nattype.status = OUL_STUN_STATUS_UNKNOWN;
		nattype.lookup_time = time(NULL);
		do_callbacks();
		return;
	}


}

static void do_test1(OulSrvResponse *resp, int results, gpointer sdata) {
	const char *servername = sdata;
	int port = 3478;

	if(results) {
		servername = resp[0].hostname;
		port = resp[0].port;
	}
	oul_debug_info("stun", "got %d SRV responses, server: %s, port: %d\n",
		results, servername, port);

	oul_dnsquery_a(servername, port, hbn_cb, NULL);
	g_free(resp);
}

static gboolean call_callback(gpointer data) {
	StunCallback cb = data;
	cb(&nattype);
	return FALSE;
}

OulStunNatDiscovery *oul_stun_discover(StunCallback cb) {
	const char *servername = oul_prefs_get_string("/oul/network/stun_server");

	oul_debug_info("stun", "using server %s\n", servername);

	if(nattype.status == OUL_STUN_STATUS_DISCOVERING) {
		if(cb)
			callbacks = g_slist_append(callbacks, cb);
		return &nattype;
	}

	if(nattype.status != OUL_STUN_STATUS_UNDISCOVERED) {
		gboolean use_cached_result = TRUE;

		/** Deal with the server name having changed since we did the
		    lookup */
		if (servername && strlen(servername) > 1
				&& ((nattype.servername
					&& strcmp(servername, nattype.servername))
				|| !nattype.servername)) {
			use_cached_result = FALSE;
		}

		/* If we don't have a successful status and it has been 5
		   minutes since we last did a lookup, redo the lookup */
		if (nattype.status != OUL_STUN_STATUS_DISCOVERED
				&& (time(NULL) - nattype.lookup_time) > 300) {
			use_cached_result = FALSE;
		}

		if (use_cached_result) {
			if(cb)
				oul_timeout_add(10, call_callback, cb);
			return &nattype;
		}
	}

	if(!servername || (strlen(servername) < 2)) {
		nattype.status = OUL_STUN_STATUS_UNKNOWN;
		nattype.lookup_time = time(NULL);
		if(cb)
			oul_timeout_add(10, call_callback, cb);
		return &nattype;
	}

	nattype.status = OUL_STUN_STATUS_DISCOVERING;
	nattype.publicip[0] = '\0';
	g_free(nattype.servername);
	nattype.servername = g_strdup(servername);

	callbacks = g_slist_append(callbacks, cb);
	oul_srv_resolve("stun", "udp", servername, do_test1,
		(gpointer) servername);

	return &nattype;
}

void oul_stun_init() {
	oul_prefs_add_string("/oul/network/stun_server", "");
}

