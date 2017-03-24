#ifndef _OUL_EVENTLOOP_H
#define _OUL_EVENTLOOP_H

#ifdef __cplusplus
extern "C"{
#endif

typedef enum{
    OUL_INPUT_READ = 1 << 0,
    OUL_INPUT_WRITE = 1 << 1
}OulInputCondition;

typedef void (*OulInputFunction)(gpointer, gint, OulInputCondition);

typedef struct _OulEventLoopUiOps OulEventLoopUiOps;


struct _OulEventLoopUiOps
{
	guint (*timeout_add)(guint interval, GSourceFunc function, gpointer data);
	
    gboolean (*timeout_remove)(guint handle);
	
    guint (*input_add)(int fd, OulInputCondition cond,
	                   OulInputFunction func, gpointer user_data);

	gboolean (*input_remove)(guint handle);
	
	int (*input_get_error)(int fd, int *error);

	guint (*timeout_add_seconds)(guint interval, GSourceFunc function,
	                             gpointer data);

	void (*_oul_reserved2)(void);
	void (*_oul_reserved3)(void);
	void (*_oul_reserved4)(void);
};

guint                   oul_timeout_add(guint interval, GSourceFunc function, gpointer data);
guint                   oul_timeout_add_seconds(guint interval, GSourceFunc function, gpointer data);
gboolean                oul_timeout_remove(guint handle);

/**
 * Adds an input handler.
 *
 * @param fd        The input file descriptor.
 * @param cond      The condition type.
 * @param func      The callback function for data.
 * @param user_data User-specified data.
 *
 * @return The resulting handle (will be greater than 0).
 * @see g_io_add_watch_full
 */
guint
oul_input_add(int source, OulInputCondition condition, OulInputFunction func, gpointer user_data);

gboolean                oul_input_remove(guint handle);
int                     oul_input_get_error(int fd, int *error);

void                    oul_eventloop_set_ui_ops(OulEventLoopUiOps *ops);
OulEventLoopUiOps*		oul_eventloop_get_ui_ops(void);

#ifdef __cplusplus
}
#endif

#endif
