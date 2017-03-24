#ifndef __BEASY_PLUGIN_QMONXML_H
#define	__BEASY_PLUGIN_QMONXML_H

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

xmlDocPtr 			xml_parse_html_from_memory(const char *buffer, size_t buf_len);
xmlXPathObjectPtr 	xml_xpath_evaluate(xmlDocPtr doc, xmlChar *xpath_expr);
xmlDocPtr 			xml_parse_html_from_file(char *docname);


#endif

