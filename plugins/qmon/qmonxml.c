#include "internal.h"

#include "qmonxml.h"

xmlDocPtr xml_parse_html_from_file(char *docname) {
  xmlDocPtr doc;

  doc = htmlReadFile(docname, "UTF-8", HTML_PARSE_NOERROR);

  if (doc == NULL ) {
    oul_debug_error("qmonxml","HTML Document parse failed.\n");
    return NULL;
  }

  return doc;
}

xmlDocPtr
xml_parse_html_from_memory(const char *buffer, size_t buf_len) {
  xmlDocPtr doc = NULL;

  doc = htmlReadMemory(buffer, buf_len, NULL, "UTF-8", HTML_PARSE_NOERROR);

  if (doc == NULL ) {
    oul_debug_error("qmonxml","HTML Document parse failed.\n");
	
    return NULL;
  }

  return doc;
}

xmlXPathObjectPtr
xml_xpath_evaluate(xmlDocPtr doc, xmlChar *xpath_expr){
  xmlXPathContextPtr context = NULL;
  xmlXPathObjectPtr  object	 = NULL;

  context = xmlXPathNewContext(doc);
  if (context == NULL) {
    oul_debug_error("qmonxml", "allocate xmlXPathNewContext error.\n");
    return NULL;
  }

  object = xmlXPathEvalExpression(xpath_expr, context);
  xmlXPathFreeContext(context);
  if (object == NULL) {
    oul_debug_error("qmonxml", "evaluate the XPATH expression failed.\n");
	
    return NULL;
  }



  return object;
}


