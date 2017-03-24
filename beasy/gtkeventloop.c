#include <glib.h>

#include "gtkeventloop.h"
#include "eventloop.h"

#define BEASY_READ_COND (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define BEASY_WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

typedef struct _BeasyIOClosure{
    OulInputFunction function;
    guint   result;
    gpointer    data;
}BeasyIOClosure;

/****************************************************
 **** private functions *****************************
 ***************************************************/
static void
beasy_io_destroy(gpointer data)
{
    g_free(data);
}

static gboolean
beasy_io_invoke(GIOChannel *source, GIOCondition condition, gpointer data)
{
    BeasyIOClosure *closure = data;
    OulInputCondition oul_cond = 0;

    if(condition & BEASY_READ_COND)
        oul_cond |= OUL_INPUT_READ;
    if(condition & BEASY_WRITE_COND)
        oul_cond |= OUL_INPUT_WRITE;
    
    closure->function(closure->data, g_io_channel_unix_get_fd(source), oul_cond);

    return TRUE;
}

static guint
beasy_input_add(gint fd, OulInputCondition condition, 
        OulInputFunction function, gpointer data)
{
    BeasyIOClosure *closure = g_new0(BeasyIOClosure, 1);
    GIOChannel *channel;
    GIOCondition cond = 0;

    closure->function = function;
    closure->data = data;

    if(condition & OUL_INPUT_READ)
        cond |= BEASY_READ_COND;
    
    if(condition & OUL_INPUT_WRITE)
        cond |= BEASY_WRITE_COND;
    
    channel = g_io_channel_unix_new(fd);
    closure->result = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, cond,
                beasy_io_invoke, closure, beasy_io_destroy);

    g_io_channel_unref(channel);
    
    return closure->result;
}

static OulEventLoopUiOps eventloop_ops =
{
	g_timeout_add,
	g_source_remove,
	beasy_input_add,
	g_source_remove,
	NULL, /* input_get_error */
#if GLIB_CHECK_VERSION(2,14,0)
	g_timeout_add_seconds,
#else
	NULL,
#endif
	NULL,
	NULL,
	NULL
};

OulEventLoopUiOps *
beasy_eventloop_get_ui_ops(void)
{
	return &eventloop_ops;
}
