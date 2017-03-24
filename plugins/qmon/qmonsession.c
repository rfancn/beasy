#include <curl/curl.h>
#include <curl/easy.h>


#include "internal.h"
#include "qmonpref.h"
#include "qmonsession.h"

GString *session_httpheader = NULL;

static size_t
session_httpbody_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t real_size = size * nmemb;

	return real_size;
}

static size_t
session_httpheader_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t real_size = size * nmemb;

	if(session_httpheader == NULL)
		session_httpheader = g_string_new("");
	
	session_httpheader = g_string_append_len(session_httpheader, ptr, real_size);

	return real_size;
}

static gboolean
session_httpheader_parse(QmonSession *session)
{
	gchar *p1 = NULL, *p2 = NULL;

	oul_debug_info("qmonsession", "qmon session header parsing...\n");

	if(session_httpheader == NULL){
		oul_debug_error("qmonsession", "qmon http header is not exist.\n");
		return FALSE;
	}
		
	if((p1 = strstr(session_httpheader->str, QMON_TIER2UNPW_PREFIX)) != NULL){
		p1 += strlen(QMON_TIER2UNPW_PREFIX);
		if((p2 = strstr(p1, ";")) != NULL){
			if(p2 > p1){
				session->tier2unpw = g_strndup(p1, p2-p1);
			}
		}
	}

	if(session->tier2unpw == NULL){
		oul_debug_error("qmonsession", "Can not find tier2unpw.\n");
		g_string_free(session_httpheader, TRUE);
		return FALSE;
	}

	p1 = p2 = NULL;
	if((p1 = strstr(session_httpheader->str, QMON_AWUSER_PREFIX)) != NULL){
		p1 += strlen(QMON_AWUSER_PREFIX);
		if((p2 = strstr(p1, ";")) != NULL){
			if(p2 > p1){
				session->aw_user = g_strndup(p1, p2-p1);
			}
		}
	}

	if(session->aw_user== NULL){
		oul_debug_error("qmonsession", "Can not find aw_user.\n");
		g_string_free(session_httpheader, TRUE);
		return FALSE;
	}

	return TRUE;
}

static gboolean
qmon_login(QmonSession *session, QmonOptions *options)
{
	CURL *curl_handle = NULL;
	CURLcode curl_ret;
	gchar error_msg[CURL_ERROR_SIZE];
	gboolean ret = FALSE;
	gchar *post_content = NULL;

	curl_handle = curl_easy_init();

#if 1
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
#endif

	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, &error_msg);
	curl_easy_setopt(curl_handle, CURLOPT_URL, "http://qmon.oraclecorp.com/qmon3/qmon.pl");
	curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);

	post_content = g_strdup_printf(QMON_LOGIN_REQUEST, options->username, options->password);

	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_content);
	curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, session_httpheader_cb);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, session_httpbody_cb);

	/*As curl will not copy the postcontent, preserved until the transfer finishes*/
	curl_ret = curl_easy_perform(curl_handle);
	if(curl_ret != CURLE_OK){
		oul_debug_info("qmonsession", "qmon session http failed: [%d]%s\n", curl_ret, error_msg);
		ret = FALSE;
	}else{
		oul_debug_info("qmonsession", "qmon session http ok.\n");
		ret = TRUE;
	}

	curl_easy_cleanup(curl_handle);
	g_free(post_content);

	return ret;
}

QmonSession *
qmon_session_new(QmonOptions *options)
{
	
	QmonSession *session = NULL;

	session = g_new0(QmonSession, 1);

	session->aw_user 	= NULL;
	session->tier2unpw 	= NULL;

	/* clear session_httpheader before we login */
	if(session_httpheader){
		g_string_free(session_httpheader, TRUE);
		session_httpheader = NULL;
	}
		
	if(!qmon_login(session, options)){
		g_free(session);
		return NULL;
	}

	if(!session_httpheader_parse(session)){
		g_free(session);
		return NULL;
	}
		
	return session;
}

void
qmon_session_destroy(QmonSession *session)
{
	if(session->aw_user){
		g_free(session->aw_user);
		session->aw_user = NULL;
	}

	if(session->tier2unpw){
		g_free(session->tier2unpw);
		session->tier2unpw = NULL;
	}

	g_free(session);
	session = NULL;
}

