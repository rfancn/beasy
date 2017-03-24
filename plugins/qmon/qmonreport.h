#ifndef __PLUGIN_QMON_REPORT_H
#define __PLUGIN_QMON_REPORT_H

#include <glib.h>
#include "qmon.h"

#define QMON_HTMLTABLE_XPATH_EXPR		"/html/body/table/tr[position()>1]/td"
#define	QMON_HTMLTABLE_COLUMN_NUM		18

#define	QMON_HTMLTABLE_BEGIN			"<TABLE BORDER=0 CELLSPACING=1 CELLPADDING=2 BGCOLOR=#CDCE9C>"
#define	QMON_HTMLTABLE_END				"</TABLE>"

#define	QMON_SR_URL						"http://qmon.oraclecorp.com:7777/qmon3/quickpicks.pl?t=t&q=%s"

#define INDEX_SR_NUMBER		1
#define	INDEX_SR_SERVERITY	5
#define	INDEX_SR_STATUS		6
#define	INDEX_SR_ANALYST	7
#define	INDEX_SR_SUBJECT	13

typedef struct _BeasyMsgHeader{
	gchar	*idx;
	gchar 	*name;
	gchar	*width;
}BeasyMsgHeader;

typedef struct _SR{
	gchar	*number;
	gchar	*severity;
	gchar	*status;
	gchar	*analyst;
	gchar	*subject;
}SR;

void	qmon_report(QmonMonitor *monitor);


#endif

