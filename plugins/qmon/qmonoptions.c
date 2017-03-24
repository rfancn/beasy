#include "internal.h"
#include "qmonoptions.h"

static void
qmon_options_get(QmonOptions *options)
{
	g_return_if_fail(options != NULL);

	options->username = oul_prefs_get_string(PLUGIN_QMON_USERNAME);
	if(options->username)
		options->username = g_strstrip(options->username);

	options->password = oul_prefs_get_string(PLUGIN_QMON_PASSWORD);
		if(options->password)
			options->password = g_strstrip(options->password);
	
	options->target = oul_prefs_get_int(PLUGIN_QMON_TARGET);
	switch(options->target){
		case QMON_TARGET_SRLIST:
			options->params.srlist = oul_prefs_get_string_list(PLUGIN_QMON_SRLIST);
			break;
		case QMON_TARGET_ANALYST:
			options->params.analyst = oul_prefs_get_string(PLUGIN_QMON_ANALYST);
			if(options->params.analyst)
				options->params.analyst = g_strstrip(options->params.analyst);
			
			break;
		case QMON_TARGET_CTC:
		default:
			break;
	}

	options->interval  = oul_prefs_get_int(PLUGIN_QMON_INTERVAL);
	
	options->selection = oul_prefs_get_string(PLUGIN_QMON_SELECTION);
	if(options->selection)
		options->selection = g_strstrip(options->selection);

	
}

QmonOptions *
qmon_options_new()
{
	QmonOptions *options = NULL;
	
	options = g_new0(QmonOptions, 1);

	qmon_options_get(options);

	return options;		
}

void
qmon_options_refresh(QmonOptions *options)
{
	qmon_options_get(options);
}

void
qmon_options_destroy(QmonOptions *options)
{
	g_return_if_fail(options != NULL);

	if(options->selection){
		g_free(options->selection);	
		options->selection = NULL;
	}

	switch(options->target){
		case QMON_TARGET_SRLIST:
			g_list_foreach(options->params.srlist, (GFunc)g_free, NULL);
			g_list_free(options->params.srlist);
			options->params.srlist = NULL;
			break;
		case QMON_TARGET_ANALYST:
			g_free(options->params.analyst);
			options->params.analyst = NULL;
			break;
		case QMON_TARGET_CTC:
		default:
			break;
	}

	g_free(options);
	options = NULL;
}

gboolean
qmon_options_validate(QmonOptions *options)
{
	if(options->username == NULL || strlen(options->username) == 0){
		oul_debug_error("qmonoptions", "Username should not be empty.\n");
		return FALSE;
	}

	if(options->password == NULL || strlen(options->password) == 0){
		oul_debug_error("qmonoptions", "Password should not be empty.\n");
		return FALSE;
	}

	if(options->selection == NULL || strlen(options->selection) == 0){
		oul_debug_error("qmonoptions", "Selection should not be empty.\n");
		return FALSE;
	}
	
	if(options->interval == 0){
		oul_debug_error("qmonoptions", "interval should not be 0.\n");
		return FALSE;
	}

	if(options->target == QMON_TARGET_ANALYST){
		if(options->params.analyst == NULL || strlen(options->params.analyst) == 0){
			oul_debug_error("qmonoptions", "Analyst should not be empty.\n");
			return FALSE;	
		}
	}

	return TRUE;
}


