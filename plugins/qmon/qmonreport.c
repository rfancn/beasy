#include "internal.h"

#include "signals.h"
#include "notify.h"
#include "qmonreport.h"
#include "qmonxml.h"

static GString *
report_htmltable_get(GString *httpbody)
{
	gchar *table_begin = NULL, *table_end = NULL;
	GString *htmltable = NULL;

	g_return_if_fail(httpbody != NULL);

	/* get the SR HTML table */
	if((table_begin = strstr(httpbody->str, QMON_HTMLTABLE_BEGIN)) != NULL){
		if((table_end = strstr(table_begin, QMON_HTMLTABLE_END)) != NULL){
			if(table_end > table_begin){
				htmltable = g_string_new("<HTML><BODY>");
				htmltable = g_string_append_len(htmltable, table_begin, table_end - table_begin);
			}
		}
	}

	if(!htmltable || htmltable->len <= 0){
		oul_debug_error("qmonreport", "can not get the SR table from the html body\n");

		return htmltable;
	}

	
	/* for some reason, we need to revise the HTML to make XPATH parse ok */
	oul_strreplace(htmltable->str,"&", "&amp;");
	htmltable->len = strlen(htmltable->str);

	htmltable = g_string_append(htmltable, "</TABLE></BODY></HTML>");

	return htmltable;
}

/* Get the SR list via xpath parsing */
static GList *
report_htmltable_parse(GString *htmltable)
{
	gint sr_count 	= 0;
	GList *sr_list 	= NULL;
	
	LIBXML_TEST_VERSION
	
	htmlDocPtr doc = xml_parse_html_from_memory(htmltable->str, htmltable->len);
	if (doc == NULL ) {
		oul_debug_error("qmonplugin", "html parse error.\n");

		xmlCleanupParser();
		
		return NULL;
	}
	
	xmlXPathObjectPtr xpathobj = xml_xpath_evaluate(doc, QMON_HTMLTABLE_XPATH_EXPR);
	if(!xpathobj){
		oul_debug_info("qmonreport", "can not evaluate the xpath.\n");
		
		xmlFreeDoc(doc);
		xmlCleanupParser();
		
		return NULL;
	}

	xmlNodeSetPtr nodeset = xpathobj->nodesetval;
	if(xmlXPathNodeSetIsEmpty(nodeset)){
    	oul_debug_info("qmonreport", "xml nodeset is empty.\n");
	
    	xmlXPathFreeObject(xpathobj);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		
    	return NULL;
  	}

	/* if SR list exist */
	sr_count = nodeset->nodeNr/QMON_HTMLTABLE_COLUMN_NUM;
	if(sr_count == 0){
		oul_debug_info("qmonplugin", "no valid SRs can be found.\n");

		xmlXPathFreeObject(xpathobj);
		xmlFreeDoc(doc);
		xmlCleanupParser();

		return NULL;
	}

	/* parse the table and fill to the SR list */
	int i, j;
	xmlNodePtr cur_node = NULL;
	xmlChar	*tmp = NULL;
	for(i = 0; i < sr_count; i++){
		SR *sr = g_new0(SR, 1);
		
		for(j = 0; j < QMON_HTMLTABLE_COLUMN_NUM; j++){

			cur_node = nodeset->nodeTab[i * QMON_HTMLTABLE_COLUMN_NUM + j];
			if(!cur_node){
				oul_debug_info("qmonplugin", "invalid html elements.\n");

				g_free(sr);
				sr = NULL;
				
				break;
			}

            /* iteration <td> element to the last text element */
            while(cur_node->xmlChildrenNode){
                cur_node = cur_node->xmlChildrenNode;
            }

			tmp = xmlNodeListGetString(doc, cur_node, 1);
			switch(j){
				case INDEX_SR_NUMBER:
					sr->number = g_strdup(tmp);
					break;
				case INDEX_SR_SERVERITY:
					sr->severity = g_strdup(tmp);
					break;
				case INDEX_SR_STATUS:
					sr->status = g_strdup(tmp);
					break;
				case INDEX_SR_ANALYST:
					sr->analyst = g_strdup(tmp);
					break;
				case INDEX_SR_SUBJECT:
					sr->subject = g_strdup(tmp);
					break;
				default:
					break;
			}
		}

		sr_list = g_list_append(sr_list, sr);
	}

	g_free(tmp);
	xmlXPathFreeObject(xpathobj);
	xmlFreeDoc(doc);
	xmlCleanupParser();

	return sr_list;
}

static void
sr_free(gpointer data, gpointer user_data)
{
	g_return_if_fail(data != NULL);

	SR *sr = (SR *)data;
	if(sr->number) { g_free(sr->number); sr->number = NULL; }
	if(sr->severity){ g_free(sr->severity); sr->severity = NULL; }
	if(sr->status) { g_free(sr->status); sr->status = NULL; }
	if(sr->analyst){ g_free(sr->analyst); sr->analyst = NULL; }
	if(sr->subject){ g_free(sr->subject); sr->subject = NULL; }
}

GList *
sr_filter_by_srlist(GList *plist, GList *srlist)
{
	SR *sr, *new_sr = NULL;
	GList *p1 = NULL, *p2 = NULL, *ret_list = NULL;
	gchar *sr_number; 

	g_return_val_if_fail(plist != NULL, NULL);
	g_return_val_if_fail(srlist != NULL, NULL);

	if(g_list_length(srlist) == 0){
		oul_debug_info("qmonreport", "srlist is empty.\n");
		return g_list_copy(plist);
	}

	p1 = plist;
	for(; p1; p1 = g_list_next(p1)){
			sr = (SR *)p1->data;

			p2 = srlist;
			for(; p2; p2=g_list_next(p2)){
				sr_number = (gchar *)p2->data;

				if(!strcasecmp(sr->number, sr_number)){
					
					oul_debug_info("qmonreport", "matched %s, %s\n", sr->number ,sr_number);

					new_sr = g_new0(SR, 1);

					new_sr->analyst 	= g_strdup(sr->analyst);
					new_sr->number  	= g_strdup(sr->number);
					new_sr->severity 	= g_strdup(sr->severity);
					new_sr->status 		= g_strdup(sr->status);
					new_sr->subject 	= g_strdup(sr->subject);

					ret_list = g_list_append(ret_list, new_sr);
				}
			}
			
	}

	return ret_list;
}

GList *
sr_filter_by_analyst(GList *plist, gchar *analyst)
{
	SR *sr, *new_sr = NULL;
	GList *tmp = NULL, *sr_list = NULL;

	g_return_val_if_fail(plist != NULL, NULL);
	g_return_val_if_fail(analyst != NULL, NULL);

	if(strlen(analyst) == 0){
		oul_debug_info("qmonplugin", "analyst name is empty.\n");
		return NULL;
	}

	tmp = plist;
	for(; tmp; tmp = g_list_next(tmp)){
			sr = (SR *)tmp->data;

			if(!strcasecmp(analyst, sr->analyst)){
				oul_debug_info("qmonplugin", "matched %s, %s\n", sr->number ,sr->analyst);
				
				new_sr = g_new0(SR, 1);

				new_sr->analyst 	= g_strdup(sr->analyst);
				new_sr->number  	= g_strdup(sr->number);
				new_sr->severity 	= g_strdup(sr->severity);
				new_sr->status 		= g_strdup(sr->status);
				new_sr->subject 	= g_strdup(sr->subject);

				sr_list = g_list_append(sr_list, new_sr);
			}
	}

	return sr_list;
}

static GString *
srlist_to_srdetails(GList *plist)
{
	xmlDocPtr doc;
	xmlNodePtr root_node, tmp_node, source_node, content_node, header_node, row_node;
	xmlChar *buf;
	int i, buf_len;
	SR *sr = NULL;

	if(g_list_length(plist) == 0){
		oul_debug_info("qmonreport", "SR list is empty.\n");
		return NULL;
	}

	BeasyMsgHeader beasy_msg_header[5]={
		"0", "Number", 		"5",	
		"1", "Severity", 	"5",
		"2", "Status", 		"5",
		"3", "Analyst", 	"5",
		"4", "Subject", 	"30"
	};

	doc = xmlNewDoc(BAD_CAST "1.0");
	root_node = xmlNewNode(NULL, BAD_CAST "beasymsg");
	xmlNewProp(root_node, "ver", "0.1");
	xmlDocSetRootElement(doc, root_node);

	/* create title */
	xmlNewChild(root_node, NULL, "title", "SR Details");
	
	/* create content */
	content_node = xmlNewNode(NULL, "content");
	xmlNewProp(content_node, "type", "table");
	xmlAddChild(root_node, content_node);
	
	/* create header */
	header_node = xmlNewChild(content_node, NULL, "header", NULL);
	for(i=0; i < (sizeof(beasy_msg_header)/sizeof(struct _BeasyMsgHeader)); i++){
		tmp_node = xmlNewChild(header_node, NULL, "col", beasy_msg_header[i].name);
		xmlNewProp(tmp_node, "idx", beasy_msg_header[i].idx);
	}

	/* create row */
	GList *tmp = plist;
	for(; tmp; tmp = g_list_next(tmp)){
		sr = (SR *)tmp->data;

		row_node = xmlNewNode(NULL, "row");
		xmlAddChild(content_node, row_node);

		tmp_node = xmlNewChild(row_node, NULL, "cell", sr->number);
		xmlNewProp(tmp_node, "col", beasy_msg_header[0].idx);
		gchar *url = g_strdup_printf(QMON_SR_URL, sr->number);
		xmlNewProp(tmp_node, "url", url);
		g_free(url);
	
		tmp_node = xmlNewChild(row_node, NULL, "cell", sr->severity);
		xmlNewProp(tmp_node, "col", beasy_msg_header[1].idx);

		tmp_node = xmlNewChild(row_node, NULL, "cell", sr->status);
		xmlNewProp(tmp_node, "col", beasy_msg_header[2].idx);

		tmp_node = xmlNewChild(row_node, NULL, "cell", sr->analyst);
		xmlNewProp(tmp_node, "col", beasy_msg_header[3].idx);

		tmp_node = xmlNewChild(row_node, NULL, "cell", sr->subject);
		xmlNewProp(tmp_node, "col", beasy_msg_header[4].idx);
		
	}

	/* Dump the document to a buffer */
 	xmlDocDumpMemoryEnc(doc, &buf, &buf_len, "UTF-8");

	GString *ret = g_string_new("");
	ret = g_string_append_len(ret, buf, buf_len);

	/*free the document */
    if(doc)
		xmlFreeDoc(doc);

    if(buf){
		xmlFree(buf);
		buf = NULL;
	}
		
	return ret;
}
	
static void
report_common(QmonMonitor *monitor, GList *plist, const gchar *title)
{
	GString *sr_details = NULL;

	sr_details = srlist_to_srdetails(plist);
	if(!sr_details){
		oul_debug_info("qmonreport", "Cannot get SR details.\n");
		return;
	}

	oul_debug_info("qmonreport", "srdetails:%s\n", sr_details->str);
	
	/* send the signal to GUI notifcation sub system */
	gpointer handle = oul_notify_get_handle();

	gchar *content = g_strdup_printf("There are %d SRs were updated!", g_list_length(plist));
	oul_signal_emit(handle, "notify-info", "Qmon Monitor", title, content);
	g_free(content);
	
	monitor->plugin->extra = g_strdup(sr_details->str);
	
	g_string_free(sr_details, TRUE);
}

static void
report_for_ctc(QmonMonitor *monitor, GList *plist)
{
	GString *sr_details = NULL;

	oul_debug_info("qmonreport", "report for ctc.\n");

	report_common(monitor, plist, "CTC Notification Details");
}

static void
report_for_analyst(QmonMonitor *monitor, GList *plist)
{
	GList *mylist = NULL;

	g_return_if_fail(plist != NULL);

	oul_debug_info("qmonreport", "report for analyst.\n");

	mylist = sr_filter_by_analyst(plist, monitor->options->params.analyst);
	if(!mylist){
		oul_debug_info("qmonplugin", "Can not found the updated SR for analyst.\n");
		return;
	}

	report_common(monitor, mylist, "Analyst Notification Details");

	g_list_foreach(mylist, (GFunc)sr_free, NULL);
	g_list_free(mylist);
	
}

static void
report_for_srlist(QmonMonitor *monitor, GList *plist)
{
	GList *mylist = NULL;

	g_return_if_fail(plist != NULL);

	oul_debug_info("qmonreport", "report for srlist.\n");

	mylist = sr_filter_by_srlist(plist, monitor->options->params.srlist);
	if(!mylist){
		oul_debug_info("qmonreport", "Can not found the updated SR for srlist.\n");
		return;
	}

	report_common(monitor, mylist, "SRList Notification Details");

	g_list_foreach(mylist, (GFunc)sr_free, NULL);
	g_list_free(mylist);
}

void
qmon_report(QmonMonitor *monitor)
{
	GList *srlist = NULL;
	g_return_if_fail(monitor != NULL);
	
	GString *htmltable = report_htmltable_get(monitor->httpbody);
	if(!htmltable){
		oul_debug_error("qmonreport", "Can not get valid htmltable.\n");
		return;	
	}

	srlist = report_htmltable_parse(htmltable);
	g_string_free(htmltable, TRUE);
	if(srlist == NULL){
		oul_debug_error("qmonreport", "Can not get valid sr list.\n");
		g_string_free(htmltable, TRUE);
		return;	
	}

	oul_debug_info("qmonreport", "total sr list:%d\n", g_list_length(srlist));
	switch(monitor->options->target){
		case QMON_TARGET_ANALYST:
			report_for_analyst(monitor, srlist);
			break;
		case QMON_TARGET_SRLIST:
			report_for_srlist(monitor, srlist);
			break;
		case QMON_TARGET_CTC:
		default:
			report_for_ctc(monitor, srlist);
			break;
	}

	g_list_foreach(srlist, (GFunc)sr_free, NULL);
	g_list_free(srlist);

	return;
}

