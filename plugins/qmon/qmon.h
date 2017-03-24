#ifndef _BEASY_PLUGIN_QMON_H
#define _BEASY_PLUGIN_QMON_H

#include "qmonsession.h"
#include "qmonoptions.h"

#define	QMON_URL						"http://qmon.oraclecorp.com/qmon3/qmon.pl"
#define QMON_COOKIE						"AW_user=%s; Tier2unpw=%s;"

#define	QMON_POLL_CONTENT				"tab=srs&label=status&" \
										"stat_type=B&sel_type=T&.cgifields=stat_type&" \
										"sel_action=Pick&selection=%s"
typedef enum
{
	QMON_STATUS_STARTED,			/* states connected to qmon website */
	QMON_STATUS_STOPED				/* indicates one qmon poll was done */
} QmonMonitorStatus;

typedef struct _QmonMonitor
{
	OulPlugin				*plugin;
	QmonSession				*session;
	QmonOptions				*options;
	
	guint					timer;			/* timer which will invoked periodically */

	QmonMonitorStatus		status;			/* current status in QmonMonitor */

	gboolean				httpdone;		/* a flag state that http was done */
	GString 				*httpheader;	/* validated and parsed http header */
	GString					*httpbody;		/* validated and parsed http body */
	
}QmonMonitor;

QmonMonitor *	qmon_monitor_new(OulPlugin *plugin);
void			qmon_monitor_destroy(QmonMonitor *monitor);



#endif

