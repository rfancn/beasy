#include <curl/curl.h>
#include <curl/easy.h>

#include "internal.h"
#include "qmon.h"
#include "qmonoptions.h"
#include "notify.h"

static guint http_check_timer = 0;

static size_t
monitor_httpheader_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
	g_return_if_fail(data != NULL);
	
	QmonMonitor *monitor = (QmonMonitor *)data;

	size_t real_size = size * nmemb;

	if(monitor->httpheader == NULL){
		monitor->httpheader = g_string_new("");
	}

	monitor->httpheader = g_string_append_len(monitor->httpheader, ptr, real_size);

	return real_size;
}

static size_t
monitor_httpbody_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
	g_return_if_fail(data != NULL);
	
	QmonMonitor *monitor = (QmonMonitor *)data;

	size_t real_size = size * nmemb;

	if(monitor->httpbody == NULL)
		monitor->httpbody = g_string_new("");
	
	monitor->httpbody = g_string_append_len(monitor->httpbody, ptr, real_size);

	return real_size;
}

static void
monitor_http_cleanup(QmonMonitor *monitor)
{
	monitor->httpdone = FALSE;
	
	if(monitor->httpheader){
		g_string_free(monitor->httpheader, TRUE);
		monitor->httpheader = NULL;
	}

	if(monitor->httpbody){
		g_string_free(monitor->httpbody, TRUE);
		monitor->httpbody = NULL;
	}
}

static void *
monitor_query(gpointer data)
{
	CURL *curl_handle = NULL;
	CURLcode curl_ret;
	gchar error_msg[CURL_ERROR_SIZE];
	gchar *cookie = NULL;

	g_return_if_fail(data != NULL);
	
	QmonMonitor *monitor = (QmonMonitor *)data;

	curl_handle = curl_easy_init();

#if 1
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
#endif

	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, &error_msg);
	curl_easy_setopt(curl_handle, CURLOPT_URL, QMON_URL);
	curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);

	cookie = g_strdup_printf(QMON_COOKIE, monitor->session->aw_user, monitor->session->tier2unpw);

	curl_easy_setopt(curl_handle, CURLOPT_COOKIE, cookie);
	
	gchar *post_content = g_strdup_printf(QMON_POLL_CONTENT, monitor->options->selection);

	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_content);

	curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, monitor_httpheader_cb);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, monitor);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, monitor_httpbody_cb);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, monitor);

	curl_ret = curl_easy_perform(curl_handle);
	if(curl_ret != CURLE_OK){
		oul_debug_info("qmon", "qmon http perform failed: %s.\n", error_msg);
	}else{
		oul_debug_info("qmon", "qmon http perform ok.\n");
		monitor->httpdone = TRUE;
	}

	curl_easy_cleanup(curl_handle);
	g_free(cookie);
	g_free(post_content);

}

gboolean
monitor_http_check(QmonMonitor *monitor)
{
	if(monitor->httpdone){
		/* remove http check timer first */
		oul_timeout_remove(http_check_timer);
		http_check_timer = 0;
		
		qmon_report(monitor);
		monitor_http_cleanup(monitor);
		
		return FALSE;
	}

	return TRUE;
}

/*******************************************************
 ******public interface*************************************
 ********************************************************/


gboolean
qmon_monitor_poll(gpointer data)
{
	GThread *thread;
	GError *error;

	oul_debug_info("qmon", "qmon poll ...\n");

	QmonMonitor *monitor = (QmonMonitor *)data;
	
	/* give a chance to use the new modified options */
	qmon_options_refresh(monitor->options);
	if(!qmon_options_validate(monitor->options)){
		oul_debug_error("qmon", "invalid options, please check it.\n");
		return FALSE;
	}

	thread = g_thread_create(monitor_query, monitor, FALSE, &error);
	if(thread == NULL){
		oul_debug_error("qmon", "Error create http thread: %s\n", error);
		
		if(error)
			g_error_free(error);

		return TRUE;
	}

	/* startup the http check process */
	if(http_check_timer == 0){
		oul_debug_info("qmon", "startup the http check timer.\n");
		http_check_timer = oul_timeout_add_seconds(2, monitor_http_check, monitor);
	}

	return TRUE;
}

gboolean
qmon_monitor_start(QmonMonitor *monitor)
{
	g_return_val_if_fail(monitor != NULL, FALSE);

	oul_debug_info("qmon", "monitor starting...\n");

	if(monitor->status == QMON_STATUS_STARTED){
		oul_debug_info("qmon", "monitor already started...\n");	
		return;
	}

	monitor->timer = oul_timeout_add_seconds(monitor->options->interval * 60, 
									qmon_monitor_poll, monitor);

	monitor->status = QMON_STATUS_STARTED;
	
	return TRUE;
}

void
qmon_monitor_stop(QmonMonitor *monitor)
{
	g_return_if_fail(monitor != NULL);

	oul_debug_info("qmon", "monitor stop...\n");

	if(monitor->status == QMON_STATUS_STOPED){
		oul_debug_info("qmon", "monitor already stoped...\n");	
		return;
	}

	/* if we stop monitor, we need remove the timer handler */
	if(monitor->timer > 0){
		oul_timeout_remove(monitor->timer);
		monitor->timer = 0;
	}

	monitor->status = QMON_STATUS_STOPED;
}

QmonMonitor *
qmon_monitor_new(OulPlugin *plugin)
{
	oul_debug_info("qmon", "qmon monitor new ...\n");

	QmonMonitor *monitor = NULL;

	monitor = g_new0(QmonMonitor, 1);
		
	monitor->plugin = plugin;

	monitor->status = QMON_STATUS_STOPED;

	monitor->httpdone = FALSE;
	monitor->httpheader = NULL;
	monitor->httpbody = NULL;

	monitor->options = qmon_options_new();
	if(!monitor->options){
		g_free(monitor);
	
		return NULL;
	}

	if(!qmon_options_validate(monitor->options)){
		oul_debug_error("qmon", "Options was invalid.\n");

		return NULL;
	}

	/* if cannot get valid qmonsession, then failed */
	monitor->session = qmon_session_new(monitor->options);
	if(!monitor->session){
		qmon_options_destroy(monitor->options);
		monitor->options = NULL;
		
		g_free(monitor);

		return NULL;
	}

	return monitor;

}

void
qmon_monitor_destroy(QmonMonitor *monitor)
{
	monitor->plugin = NULL;
	
	if(monitor->timer > 0){
		oul_timeout_remove(monitor->timer);
		monitor->timer = 0;
	}
	
	qmon_session_destroy(monitor->session);
	qmon_options_destroy(monitor->options);
	
	monitor_http_cleanup(monitor);

	g_free(monitor);
	monitor = NULL;
}


