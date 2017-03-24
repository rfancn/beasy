#include "internal.h"

#include "eventloop.h"

static OulEventLoopUiOps *eventloop_ui_ops = NULL;

/**************************************************
 ******public functions****************************
 *************************************************/
guint
oul_timeout_add(guint interval, GSourceFunc function, gpointer data)
{
	OulEventLoopUiOps *ops = oul_eventloop_get_ui_ops();
	
	return ops->timeout_add(interval, function, data);
}

guint
oul_timeout_add_seconds(guint interval, GSourceFunc function, gpointer data)
{
    OulEventLoopUiOps *ops = oul_eventloop_get_ui_ops(); 

    if(ops->timeout_add_seconds)
        return ops->timeout_add_seconds(interval, function, data);
    else
        return ops->timeout_add(1000 * interval, function, data);
}

gboolean
oul_timeout_remove(guint tag)
{
	OulEventLoopUiOps *ops = oul_eventloop_get_ui_ops();

	return ops->timeout_remove(tag);
}


guint
oul_input_add(int source, OulInputCondition condition, OulInputFunction func, gpointer user_data)
{
	OulEventLoopUiOps *ops = oul_eventloop_get_ui_ops();

	return ops->input_add(source, condition, func, user_data);
}

gboolean
oul_input_remove(guint tag)
{
	OulEventLoopUiOps *ops = oul_eventloop_get_ui_ops();

	return ops->input_remove(tag);
}

int
oul_input_get_error(int fd, int *error)
{
	OulEventLoopUiOps *ops = oul_eventloop_get_ui_ops();

	if (ops->input_get_error)
	{
		int ret = ops->input_get_error(fd, error);
		errno = *error;
		return ret;
	}
	else
	{
		socklen_t len;
		len = sizeof(*error);

		return getsockopt(fd, SOL_SOCKET, SO_ERROR, error, &len);
	}
}

void
oul_eventloop_set_ui_ops(OulEventLoopUiOps *ops)
{
	eventloop_ui_ops = ops;
}

OulEventLoopUiOps *
oul_eventloop_get_ui_ops(void)
{
	g_return_val_if_fail(eventloop_ui_ops != NULL, NULL);

	return eventloop_ui_ops;
}

