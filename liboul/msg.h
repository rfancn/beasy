#ifndef __OUL_MSG_H
#define __OUL_MSG_H

#include <glib.h>

/*
  * There are two kinds msg
  * 1. plain text message
  * 2. markup text message
  * 3. table message
  */
typedef enum{
	OULMSG_CTYPE_INVALID = -1,
	OULMSG_CTYPE_TEXT = 0,
	OULMSG_CTYPE_MARKUP,
	OULMSG_CTYPE_TABLE,
	OULMSG_CTYPE_NUM
}OulMsgContentType;

typedef struct _OulMsgTbHeader{
	gint	idx;
	gchar 	*name;
}OulMsgTbHeader;

typedef struct _OulMsgTbRow{
	GList	*cells;
}OulMsgTbRow;

typedef struct _OulMsgTbCell{
	gint	col;
	gchar	*content;
	gchar	*url;
}OulMsgTbCell;

typedef struct _OulMsgTable{
	GList	*headers;
	GList	*rows;
}OulMsgTable;

typedef struct _OulMsg{
	gchar				*title;

	OulMsgContentType	ctype;

	union{
		OulMsgTable		*table;
		gchar			*text;
	}content;
	
}OulMsg;

OulMsg *			xmlmsg_to_oulmsg(const char *xmlmsg, int xmlmsg_len);

#endif
