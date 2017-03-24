#include "internal.h"

#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "msg.h"

/*                                          send xml msg                       parse xml msg
  * Third party message source--- ----------> msg_plugin --------------> emit("msg_plugin", title, content) 
  * 
  *  Internal message source -->assemble xml msg---> save to plugin->extra---->emit("pluginname", title, content)
  */

static void
xmlnodes_print(xmlNodeSetPtr nodes)
{
    xmlNodePtr cur;
    int size;
    int i;
    
	g_return_if_fail(nodes->nodeTab[i] != NULL);

    size = (nodes) ? nodes->nodeNr : 0;
    
    oul_debug_info("beasymsg", "Result (%d nodes):\n", size);
    for(i = 0; i < size; ++i) {
		if(nodes->nodeTab[i]->type == XML_NAMESPACE_DECL) {
	    	xmlNsPtr ns;
	    
	    	ns = (xmlNsPtr)nodes->nodeTab[i];
	    	cur = (xmlNodePtr)ns->next;
			if(cur->ns) { 
				oul_debug_info("beasymsg", "= namespace \"%s\"=\"%s\" for node %s:%s\n", 
				ns->prefix, ns->href, cur->ns->href, cur->name);
			} else {
		        oul_debug_info("beasymsg", "= namespace \"%s\"=\"%s\" for node %s\n", 
			    ns->prefix, ns->href, cur->name);
		    }
		} else if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
		    cur = nodes->nodeTab[i];   	    
		    if(cur->ns) { 
	    	        oul_debug_info("beasymsg", "= element node \"%s:%s\"\n", 
						cur->ns->href, cur->name);
		    } else {
	    	        oul_debug_info("beasymsg", "= element node \"%s\"\n", cur->name);
			}
		} else {
		    cur = nodes->nodeTab[i];    
		    oul_debug_info("beasymsg", "= node \"%s\": type %d\n", cur->name, cur->type);
		}
    }
}


/**
 * execute_xpath_expression:
 * @xmlDocPtr: 		xmldoc
 * @xpathExpr:		the xpath expression for evaluation.
 * Parses input XML doc, evaluates XPath expression and prints results.
 *
 * Returns TRUE on success and FALSE value otherwise.
 */
xmlXPathObject *
xpathobj_evaluate(xmlDocPtr doc, const xmlChar* xpathExpr) {
    xmlXPathContextPtr xpathCtx; 
    xmlXPathObjectPtr xpathObj;

	g_return_if_fail(xpathExpr != NULL);
    
    /* Create xpath evaluation context */
    xpathCtx = xmlXPathNewContext(doc);
    if(xpathCtx == NULL) {
        oul_debug_error("beasymsg","Error: unable to create new XPath context\n");
		return NULL;
    }
    
    /* Evaluate xpath expression */
    xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
    xmlXPathFreeContext(xpathCtx); 
    if(xpathObj == NULL) {
        oul_debug_error("beasymsg","Error: unable to evaluate xpath expression \"%s\"\n", xpathExpr);

        return NULL;
    }

    return xpathObj;
}


static gchar *
xml_element_name_get(xmlNode * node)
{
    xmlNode *cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
			return g_strdup(cur_node->name);
        }

        xml_element_name_get(cur_node->children);
    }

	return NULL;
}

gboolean
xmlmsg_validate(xmlDocPtr doc)
{
	xmlNode *root_element = NULL;
	gchar *rootname = NULL;
	
	/*Get the root element node */
	root_element = xmlDocGetRootElement(doc);
	if(root_element == NULL){
		return FALSE;
	}

	rootname  = xml_element_name_get(root_element);
	if(rootname == NULL)
		return FALSE;

	if(strcasecmp(rootname, "beasymsg")){
		g_free(rootname);
		return FALSE;
	}

	return TRUE;
}

xmlChar *
oul_msg_get_title(xmlDocPtr doc)
{
	xmlNodeSet *nodeset;
	xmlNode *node = NULL;
	xmlXPathObject *xpathobj = NULL;
	
	xpathobj = xpathobj_evaluate(doc, "/beasymsg/title");
	if(xpathobj == NULL){
		return NULL;
	}

	nodeset = xpathobj->nodesetval;
	if(nodeset == NULL){
		xmlXPathFreeObject(xpathobj);
		return NULL;
	}

	if(nodeset->nodeNr == 0){
		xmlXPathFreeObject(xpathobj);
		return NULL;
	}
	
	node = nodeset->nodeTab[0];
	
	xmlChar *ret = xmlNodeGetContent(node);
		
	xmlXPathFreeObject(xpathobj);
	return ret;
}

OulMsgContentType
oul_msg_content_get_type(xmlDocPtr doc)
{
	xmlNodeSet *nodeset;
	xmlNode *node = NULL;
	xmlXPathObject *xpathobj = NULL;

	xpathobj = xpathobj_evaluate(doc, "/beasymsg/content");
	if(xpathobj == NULL){
		return OULMSG_CTYPE_INVALID;
	}

	nodeset = xpathobj->nodesetval;
	if(nodeset == NULL){
		xmlXPathFreeObject(xpathobj);
		return OULMSG_CTYPE_INVALID;
	}

	if(nodeset->nodeNr == 0){
		xmlXPathFreeObject(xpathobj);
		return OULMSG_CTYPE_INVALID;
	}

	node = nodeset->nodeTab[0];
	xmlChar *type = xmlGetProp(node, "type");
	xmlXPathFreeObject(xpathobj);
	if(type == NULL)
		return OULMSG_CTYPE_INVALID;

	OulMsgContentType content_type;
	if(!strcasecmp(type, "text")){
		content_type = OULMSG_CTYPE_TEXT;
	}else if(!strcasecmp(type, "table")){
		content_type = OULMSG_CTYPE_TABLE;
	}else if(!strcasecmp(type, "markup")){
		content_type = OULMSG_CTYPE_MARKUP;
	}else{
		content_type = OULMSG_CTYPE_INVALID;
	}

	return content_type;
}

xmlChar *
oul_msg_content_get_text(xmlDocPtr doc)
{
	xmlNodeSet *nodeset;
	xmlNode *node = NULL;
	xmlXPathObject *xpathobj = NULL;

	xpathobj = xpathobj_evaluate(doc, "/beasymsg/content/text");
	if(xpathobj == NULL){
		return NULL;
	}

	nodeset = xpathobj->nodesetval;
	if(nodeset == NULL){
		xmlXPathFreeObject(xpathobj);
		return NULL;
	}

	if(nodeset->nodeNr == 0){
		xmlXPathFreeObject(xpathobj);
		return NULL;
	}

	node = nodeset->nodeTab[0];
	xmlChar *text = xmlNodeGetContent(node);

	xmlXPathFreeObject(xpathobj);
		
	return text;
}

static void
oul_msg_tbheaders_destroy(gpointer data, gpointer user_data)
{
	g_return_if_fail(data != NULL);
	
	OulMsgTbHeader *tbheader = (OulMsgTbHeader *)data;

	if(tbheader->name){
		g_free(tbheader->name);
	}
}

static void
oul_msg_tbrows_destroy(gpointer data, gpointer user_data)
{
	g_return_if_fail(data != NULL);

	OulMsgTbRow *row = (OulMsgTbRow *)data;

	for(; row->cells; row->cells = g_list_next(row->cells)){
			OulMsgTbCell *cell = (OulMsgTbCell *)row->cells->data;

			if(cell->content)
				g_free(cell->content);

			if(cell->url)
				g_free(cell->url);

	}

	g_list_free(row->cells);
	
}

OulMsgTable *
oul_msg_content_get_table(xmlDocPtr doc)
{
	xmlNodeSet *nodeset;
	xmlNode *node = NULL, *child_node = NULL;
	xmlXPathObject *xpathobj = NULL;

	/* get headers */
	xpathobj = xpathobj_evaluate(doc, "/beasymsg/content/header");
	if(xpathobj == NULL){
		return NULL;
	}

	nodeset = xpathobj->nodesetval;
	if(nodeset == NULL){
		xmlXPathFreeObject(xpathobj);
		return NULL;
	}

	if(nodeset->nodeNr == 0){
		xmlXPathFreeObject(xpathobj);
		return NULL;
	}

	OulMsgTable *table = g_new0(OulMsgTable, 1);
	
	node = nodeset->nodeTab[0];
	int i = 0;
	for(child_node = node->children; child_node; child_node = child_node->next){
		OulMsgTbHeader *header = g_new0(OulMsgTbHeader, 1);
		
		header->idx = i++;
		header->name = xmlNodeGetContent(child_node);

		table->headers = g_list_append(table->headers, header);
	}

	/* get rows */
	xpathobj = xpathobj_evaluate(doc, "/beasymsg/content/row");
	if(xpathobj == NULL){
		table->rows = NULL;
		return table;
	}

	nodeset = xpathobj->nodesetval;
	if(nodeset == NULL){
		xmlXPathFreeObject(xpathobj);
		
		table->rows = NULL;
		return table;
	}

	if(nodeset->nodeNr == 0){
		xmlXPathFreeObject(xpathobj);

		table->rows = NULL;
		return table;
	}

	for(i=0; i < nodeset->nodeNr; i++){
		OulMsgTbRow *row = g_new0(OulMsgTbRow, 1);

		node = nodeset->nodeTab[i];
		for(child_node = node->children; child_node; child_node = child_node->next){
			OulMsgTbCell *cell = g_new0(OulMsgTbCell, 1);

			gchar *tmp = xmlGetProp(child_node, "col");
			if(tmp){
				cell->col = atoi(tmp);
				g_free(tmp);
			}
			
			cell->content = xmlNodeGetContent(child_node);
			cell->url = xmlGetProp(child_node, "url");

			row->cells = g_list_append(row->cells, cell);
		}

		table->rows = g_list_append(table->rows, row);
	}

	return table;
}

void
oul_msg_destroy(OulMsg *msg)
{
	g_return_if_fail(msg != NULL);

	if(msg->title)
		g_free(msg->title);

	if(msg->ctype){
		switch(msg->ctype){
			case OULMSG_CTYPE_TEXT:
			case OULMSG_CTYPE_MARKUP:
				g_free(msg->content.text);
				msg->content.text = NULL;
				break;
			case OULMSG_CTYPE_TABLE:
				g_list_foreach(msg->content.table->headers, 
							(GFunc)oul_msg_tbheaders_destroy, NULL);

				g_list_free(msg->content.table->headers);
				msg->content.table->headers = NULL;

				g_list_foreach(msg->content.table->rows, 
							(GFunc)oul_msg_tbrows_destroy, NULL);

				g_list_free(msg->content.table->rows);

				msg->content.table->rows = NULL;

				msg->content.table = NULL;
				
				break;
		}
	}
}

/* xml msg format like 
 *  <beasymsg ver="xxx">
 *		<title></title>
 *  1. if it is plain text or markup text, then it should be:
 * 		<content type = "text">
 *	 		<text></text>
 *	 	</content>
 *  2. if it is table
 *		<content type= "table" >
 *			<header>
 *				<col idx="0">xxx</col>
 *				...
 *				<col idx="n">yyy</col>
 *			</header>
 *			<row>
 *				<cell col ="0" url="xxx">yyyy</cell>
 *				...
 *				<cell col ="n" url="xxx">zzz</cell>
 *			</row>
 *			<row>...</row>
 *				...
 *			<row>...</row>
 * 		</content>
 * </beasymsg>
*/
OulMsg *
xmlmsg_to_oulmsg(const char *xmlmsg, int xmlmsg_len)
{
	xmlDocPtr	doc;
	xmlNode		*root_element;
	
	 /* this initialize the library and check potential ABI mismatches
	  * between the version it was compiled for and the actual shared
	  * library used. */
	LIBXML_TEST_VERSION

	doc = xmlReadMemory(xmlmsg, xmlmsg_len, "noname.xml", NULL, 0);
	if (doc == NULL) {
		oul_debug_error("beasymsg", "Failed to parse document\n");
		goto error_exit;
	}

#if 0
	if(!xmlmsg_validate(doc)){
		oul_debug_error("beasymsg", "xml message was invalid.\n");
		goto error_exit;
	}
#endif

	OulMsg *beasymsg = g_new0(OulMsg, 1);

	beasymsg->title = oul_msg_get_title(doc);

	beasymsg->ctype = oul_msg_content_get_type(doc);
	if(beasymsg->ctype == OULMSG_CTYPE_INVALID){
		oul_debug_error("beasymsg", "xml message was invalid.\n");
		goto error_exit;
	}

	switch(beasymsg->ctype){
		case OULMSG_CTYPE_TEXT:
		case OULMSG_CTYPE_MARKUP:
			beasymsg->content.text = oul_msg_content_get_text(doc);
			break;
		case OULMSG_CTYPE_TABLE:
			beasymsg->content.table = oul_msg_content_get_table(doc);
			break;
	}
	
	if(doc) xmlFreeDoc(doc);
	xmlCleanupParser();

	return beasymsg;

error_exit:	
	oul_msg_destroy(beasymsg);
	
	if(doc) xmlFreeDoc(doc);
	xmlCleanupParser();

	return NULL;
}

gchar *
oulmsg_to_xmlmsg(OulMsg *beasymsg)
{

}

