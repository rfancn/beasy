#include "internal.h"

#include "signals.h"
#include "prefs.h"
#include "notify.h"
#include "msg.h"

static OulNotifyUiOps	*notify_ui_ops 	= NULL;
static GList 			*handles 		= NULL;


typedef struct
{
	OulNotifyType type;
	void *handle;
	void *ui_handle;
	OulNotifyCloseCallback cb;
	gpointer cb_user_data;
} OulNotifyInfo;

void *
oul_notify_message(void *handle, OulNotifyMsgType type,
					const char *title, const char *primary,
					const char *secondary, OulNotifyCloseCallback cb, gpointer user_data)
{
	OulNotifyUiOps *ops;

	g_return_val_if_fail(primary != NULL, NULL);

	ops = oul_notify_get_ui_ops();

	if (ops != NULL && ops->notify_message != NULL) {
		void *ui_handle = ops->notify_message(type, title, primary, secondary);
		if (ui_handle != NULL) {

			OulNotifyInfo *info = g_new0(OulNotifyInfo, 1);
			info->type = OUL_NOTIFY_MESSAGE;
			info->handle = handle;
			info->ui_handle = ui_handle;
			info->cb = cb;
			info->cb_user_data = user_data;

			handles = g_list_append(handles, info);

			return info->ui_handle;
		}

	}

	if (cb != NULL)
		cb(user_data);

	return NULL;
}

void *
oul_notify_formatted(void *handle, const char *title, const char *primary,
					  const char *secondary, const char *text, int width, int height,
					  OulNotifyCloseCallback cb, gpointer user_data)
{
	OulNotifyUiOps *ops;

	g_return_val_if_fail(primary != NULL, NULL);

	ops = oul_notify_get_ui_ops();

	if (ops != NULL && ops->notify_formatted != NULL) {
		void *ui_handle = ops->notify_formatted(title, primary, secondary, text, width, height);

		if (ui_handle != NULL) {

			OulNotifyInfo *info = g_new0(OulNotifyInfo, 1);
			info->type = OUL_NOTIFY_FORMATTED;
			info->handle = handle;
			info->ui_handle = ui_handle;
			info->cb = cb;
			info->cb_user_data = user_data;

			handles = g_list_append(handles, info);

			return info->ui_handle;
		}
	}

	if (cb != NULL)
		cb(user_data);
	return NULL;
}

void *
oul_notify_table(void *handle, const char *title, OulMsg *msg, 
					OulNotifyCloseCallback cb, gpointer user_data)
{
	OulNotifyUiOps *ops;

	g_return_val_if_fail(msg != NULL, NULL);

	ops = oul_notify_get_ui_ops();
	
	if (ops != NULL && ops->notify_table != NULL) {
		void *ui_handle = ops->notify_table(title, msg);

		if (ui_handle != NULL) {

			OulNotifyInfo *info = g_new0(OulNotifyInfo, 1);
			info->type = OUL_NOTIFY_FORMATTED;
			info->handle = handle;
			info->ui_handle = ui_handle;
			info->cb = cb;
			info->cb_user_data = user_data;

			handles = g_list_append(handles, info);

			return info->ui_handle;
		}
	}

	if (cb != NULL)
		cb(user_data);
	
	return NULL;
	
}

void *
oul_notify_uri(void *handle, const char *uri)
{
	OulNotifyUiOps *ops;

	g_return_val_if_fail(uri != NULL, NULL);

	ops = oul_notify_get_ui_ops();

	if (ops != NULL && ops->notify_uri != NULL) {

		void *ui_handle = ops->notify_uri(uri);

		if (ui_handle != NULL) {

			OulNotifyInfo *info = g_new0(OulNotifyInfo, 1);
			info->type = OUL_NOTIFY_URI;
			info->handle = handle;
			info->ui_handle = ui_handle;

			handles = g_list_append(handles, info);

			return info->ui_handle;
		}
	}

	return NULL;
}

void
oul_notify_close(OulNotifyType type, void *ui_handle)
{
	GList *l;
	OulNotifyUiOps *ops;

	g_return_if_fail(ui_handle != NULL);

	ops = oul_notify_get_ui_ops();

	for (l = handles; l != NULL; l = l->next) {
		OulNotifyInfo *info = l->data;

		if (info->ui_handle == ui_handle) {
			handles = g_list_remove(handles, info);

			if (ops != NULL && ops->close_notify != NULL)
				ops->close_notify(info->type, ui_handle);

			if (info->cb != NULL)
				info->cb(info->cb_user_data);

			g_free(info);

			break;
		}
	}
}

void
oul_notify_close_with_handle(void *handle)
{
	GList *l, *prev = NULL;
	OulNotifyUiOps *ops;

	g_return_if_fail(handle != NULL);

	ops = oul_notify_get_ui_ops();

	for (l = handles; l != NULL; l = prev ? prev->next : handles) {
		OulNotifyInfo *info = l->data;

		if (info->handle == handle) {
			handles = g_list_remove(handles, info);

			if (ops != NULL && ops->close_notify != NULL)
				ops->close_notify(info->type, info->ui_handle);

			if (info->cb != NULL)
				info->cb(info->cb_user_data);

			g_free(info);
		} else
			prev = l;
	}
}

void
oul_notify_set_ui_ops(OulNotifyUiOps *ops)
{
	notify_ui_ops = ops;
}

OulNotifyUiOps *
oul_notify_get_ui_ops(void)
{
	return notify_ui_ops;
}

void *
oul_notify_get_handle(void)
{
	static int handle;

	return &handle;
}


void
oul_notify_init(void)
{
	void *handle = oul_notify_get_handle();

	/* marshal function params: sender name,  title,  message */
	oul_signal_register(handle, "notify-info", 
						oul_marshal_VOID__POINTER_POINTER_POINTER, NULL, 3,
						oul_value_new(OUL_TYPE_STRING), 
						oul_value_new(OUL_TYPE_STRING), oul_value_new(OUL_TYPE_STRING));

	oul_signal_register(handle, "notify-warn", 
						oul_marshal_VOID__POINTER_POINTER_POINTER, NULL, 3,
						oul_value_new(OUL_TYPE_STRING), 
						oul_value_new(OUL_TYPE_STRING), oul_value_new(OUL_TYPE_STRING));

	oul_signal_register(handle, "notify-error", 
						oul_marshal_VOID__POINTER_POINTER_POINTER, NULL, 3,
						oul_value_new(OUL_TYPE_STRING), 
						oul_value_new(OUL_TYPE_STRING), oul_value_new(OUL_TYPE_STRING));

	oul_signal_register(handle, "notify-fatal", 
						oul_marshal_VOID__POINTER_POINTER_POINTER, NULL, 3,
						oul_value_new(OUL_TYPE_STRING), 
						oul_value_new(OUL_TYPE_STRING), oul_value_new(OUL_TYPE_STRING));
}

void
oul_notify_uninit(void)
{
	oul_signals_unregister_by_instance(oul_notify_get_handle());
}

