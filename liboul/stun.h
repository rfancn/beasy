/**
 * @file stun.h STUN API
 * @ingroup core
 */

#ifndef _OUL_STUN_H_
#define _OUL_STUN_H_

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/** @name STUN API                                                        */
/**************************************************************************/
/*@{*/

typedef struct _OulStunNatDiscovery OulStunNatDiscovery;

typedef enum {
	OUL_STUN_STATUS_UNDISCOVERED = -1,
	OUL_STUN_STATUS_UNKNOWN, /* no STUN server reachable */
	OUL_STUN_STATUS_DISCOVERING,
	OUL_STUN_STATUS_DISCOVERED
} OulStunStatus;

typedef enum {
	OUL_STUN_NAT_TYPE_PUBLIC_IP,
	OUL_STUN_NAT_TYPE_UNKNOWN_NAT,
	OUL_STUN_NAT_TYPE_FULL_CONE,
	OUL_STUN_NAT_TYPE_RESTRICTED_CONE,
	OUL_STUN_NAT_TYPE_PORT_RESTRICTED_CONE,
	OUL_STUN_NAT_TYPE_SYMMETRIC
} OulStunNatType;

struct _OulStunNatDiscovery {
	OulStunStatus status;
	OulStunNatType type;
	char publicip[16];
	char *servername;
	time_t lookup_time;
};

typedef void (*StunCallback) (OulStunNatDiscovery *);

/**
 * Starts a NAT discovery. It returns a OulStunNatDiscovery if the discovery
 * is already done. Otherwise the callback is called when the discovery is over
 * and NULL is returned.
 *
 * @param cb The callback to call when the STUN discovery is finished if the
 *           discovery would block.  If the discovery is done, this is NOT
 *           called.
 *
 * @return a OulStunNatDiscovery which includes the public IP and the type
 *         of NAT or NULL is discovery would block
 */
OulStunNatDiscovery *oul_stun_discover(StunCallback cb);

void oul_stun_init(void);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _OUL_STUN_H_ */
