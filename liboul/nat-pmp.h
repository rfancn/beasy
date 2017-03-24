/**
 * @file nat-pmp.h NAT-PMP Implementation
 * @ingroup core
 */

#ifndef _OUL_NAT_PMP_H
#define _OUL_NAT_PMP_H

#include <glib.h>

#define OUL_PMP_LIFETIME	3600	/* 3600 seconds */

typedef enum {
	OUL_PMP_TYPE_UDP,
	OUL_PMP_TYPE_TCP
} OulPmpType;

/**
 * Initialize nat-pmp
 */
void oul_pmp_init(void);

/**
 *
 */
char *oul_pmp_get_public_ip(void);

/**
 * Remove the NAT-PMP mapping for a specified type on a specified port
 *
 * @param type The OulPmpType
 * @param privateport The private port on which we are listening locally
 * @param publicport The public port on which we are expecting a response
 * @param lifetime The lifetime of the mapping. It is recommended that this be OUL_PMP_LIFETIME.
 *
 * @returns TRUE if succesful; FALSE if unsuccessful
 */
gboolean oul_pmp_create_map(OulPmpType type, unsigned short privateport, unsigned short publicport, int lifetime);

/**
 * Remove the NAT-PMP mapping for a specified type on a specified port
 *
 * @param type The OulPmpType
 * @param privateport The private port on which the mapping was previously made
 *
 * @returns TRUE if succesful; FALSE if unsuccessful
 */
gboolean oul_pmp_destroy_map(OulPmpType type, unsigned short privateport);

#endif

