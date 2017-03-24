#ifndef __PLUGIN_QMON_ACTION_H
#define __PLUGIN_QMON_ACTION_H

#include <glib.h>
#include "qmonoptions.h"

#define	QMON_TIER2UNPW_PREFIX       "Tier2unpw="
#define QMON_AWUSER_PREFIX			"AW_user="

#define QMON_LOGIN_REQUEST			"++++++++LOGIN++++++++=LOGIN&un=%s&pw=%s"

typedef struct _QmonSession
{
	gchar *aw_user;
	gchar *tier2unpw;
}QmonSession;

QmonSession *	qmon_session_new(QmonOptions *options);


#endif

