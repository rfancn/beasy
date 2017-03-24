/**
 * @file dnssrv.h
 */

#ifndef _OUL_DNSSRV_H
#define _OUL_DNSSRV_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _OulSrvResponse OulSrvResponse;
typedef struct _OulSrvQueryData OulSrvQueryData;

struct _OulSrvResponse {
	char hostname[256];
	int port;
	int weight;
	int pref;
};

typedef void (*OulSrvCallback)(OulSrvResponse *resp, int results, gpointer data);

/**
 * Queries an SRV record.
 *
 * @param protocol Name of the protocol (e.g. "sip")
 * @param transport Name of the transport ("tcp" or "udp")
 * @param domain Domain name to query (e.g. "blubb.com")
 * @param cb A callback which will be called with the results
 * @param extradata Extra data to be passed to the callback
 */
OulSrvQueryData *oul_srv_resolve(const char *protocol, const char *transport, const char *domain, OulSrvCallback cb, gpointer extradata);

/**
 * Cancel an SRV DNS query.
 *
 * @param query_data The request to cancel.
 */
void oul_srv_cancel(OulSrvQueryData *query_data);

#ifdef __cplusplus
}
#endif

#endif /* _OUL_DNSSRV_H */
