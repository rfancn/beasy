#ifndef __PLUGIN_QMON_OPTIONS_H
#define __PLUGIN_QMON_OPTIONS_H

#include <glib.h>
#include "qmonpref.h"

typedef struct _QmonOptions
{
	gchar			*username;
	gchar			*password;
	
	int				interval;
	gchar			*selection;
	
	QmonTarget		target;
	
	union
	{
		GList *srlist;
		gchar *analyst;
	}params;
	
}QmonOptions;

QmonOptions *	qmon_options_new();
void			qmon_options_destroy(QmonOptions *options);
gboolean		qmon_options_validate(QmonOptions *options);
QmonOptions *	qmon_options_new();
void			qmon_options_refresh(QmonOptions *options);


#endif

