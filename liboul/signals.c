#include "internal.h"

#include "debug.h"
#include "signals.h"
#include "value.h"


typedef struct
{
	void *instance;

	GHashTable *signals;
	size_t signal_count;

	gulong next_signal_id;

} OulInstanceData;

typedef struct
{
	gulong id;

	OulSignalMarshalFunc marshal;

	int num_values;
	OulValue **values;
	OulValue *ret_value;

	GList *handlers;
	size_t handler_count;

	gulong next_handler_id;
} OulSignalData;

typedef struct
{
	gulong id;
	OulCallback cb;
	void *handle;
	void *data;
	gboolean use_vargs;
	int priority;

} OulSignalHandlerData;

static GHashTable *instance_table = NULL;

static void
destroy_instance_data(OulInstanceData *instance_data)
{
	g_hash_table_destroy(instance_data->signals);

	g_free(instance_data);
}

static void
destroy_signal_data(OulSignalData *signal_data)
{
	g_list_foreach(signal_data->handlers, (GFunc)g_free, NULL);
	g_list_free(signal_data->handlers);

	if (signal_data->values != NULL)
	{
		int i;

		for (i = 0; i < signal_data->num_values; i++)
			oul_value_destroy((OulValue *)signal_data->values[i]);

		g_free(signal_data->values);
	}

	if (signal_data->ret_value != NULL)
		oul_value_destroy(signal_data->ret_value);
	g_free(signal_data);
}

gulong
oul_signal_register(void *instance, const char *signal,
					 OulSignalMarshalFunc marshal,
					 OulValue *ret_value, int num_values, ...)
{
	OulInstanceData *instance_data;
	OulSignalData *signal_data;
	va_list args;

	g_return_val_if_fail(instance != NULL, 0);
	g_return_val_if_fail(signal   != NULL, 0);
	g_return_val_if_fail(marshal  != NULL, 0);

	instance_data =
		(OulInstanceData *)g_hash_table_lookup(instance_table, instance);

	if (instance_data == NULL)
	{
		instance_data = g_new0(OulInstanceData, 1);

		instance_data->instance = instance;
		instance_data->next_signal_id = 1;

		instance_data->signals =
			g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
								  (GDestroyNotify)destroy_signal_data);

		g_hash_table_insert(instance_table, instance, instance_data);
	}

	signal_data = g_new0(OulSignalData, 1);
	signal_data->id              = instance_data->next_signal_id;
	signal_data->marshal         = marshal;
	signal_data->next_handler_id = 1;
	signal_data->ret_value       = ret_value;
	signal_data->num_values      = num_values;

	if (num_values > 0)
	{
		int i;

		signal_data->values = g_new0(OulValue *, num_values);

		va_start(args, num_values);

		for (i = 0; i < num_values; i++)
			signal_data->values[i] = va_arg(args, OulValue *);

		va_end(args);
	}

	g_hash_table_insert(instance_data->signals,
						g_strdup(signal), signal_data);

	instance_data->next_signal_id++;
	instance_data->signal_count++;

	return signal_data->id;
}

void
oul_signal_unregister(void *instance, const char *signal)
{
	OulInstanceData *instance_data;

	g_return_if_fail(instance != NULL);
	g_return_if_fail(signal   != NULL);

	instance_data =
		(OulInstanceData *)g_hash_table_lookup(instance_table, instance);

	g_return_if_fail(instance_data != NULL);

	g_hash_table_remove(instance_data->signals, signal);

	instance_data->signal_count--;

	if (instance_data->signal_count == 0)
	{
		/* Unregister the instance. */
		g_hash_table_remove(instance_table, instance);
	}
}

void
oul_signals_unregister_by_instance(void *instance)
{
	gboolean found;

	g_return_if_fail(instance != NULL);

	found = g_hash_table_remove(instance_table, instance);

	/*
	 * Makes things easier (more annoying?) for developers who don't have
	 * things registering and unregistering in the right order :)
	 */
	g_return_if_fail(found);
}

void
oul_signal_get_values(void *instance, const char *signal,
					   OulValue **ret_value,
					   int *num_values, OulValue ***values)
{
	OulInstanceData *instance_data;
	OulSignalData *signal_data;

	g_return_if_fail(instance   != NULL);
	g_return_if_fail(signal     != NULL);
	g_return_if_fail(num_values != NULL);
	g_return_if_fail(values     != NULL);

	/* Get the instance data */
	instance_data =
		(OulInstanceData *)g_hash_table_lookup(instance_table, instance);

	g_return_if_fail(instance_data != NULL);

	/* Get the signal data */
	signal_data =
		(OulSignalData *)g_hash_table_lookup(instance_data->signals, signal);

	g_return_if_fail(signal_data != NULL);

	*num_values = signal_data->num_values;
	*values     = signal_data->values;

	if (ret_value != NULL)
		*ret_value = signal_data->ret_value;
}

static gint handler_priority(void * a, void * b) {
	OulSignalHandlerData *ah = (OulSignalHandlerData*)a;
	OulSignalHandlerData *bh = (OulSignalHandlerData*)b;
	if (ah->priority > bh->priority) return 1;
	if (ah->priority < bh->priority) return -1;
	return 0;
}

static gulong
signal_connect_common(void *instance, const char *signal, void *handle,
					  OulCallback func, void *data, int priority, gboolean use_vargs)
{
	OulInstanceData *instance_data;
	OulSignalData *signal_data;
	OulSignalHandlerData *handler_data;

	g_return_val_if_fail(instance != NULL, 0);
	g_return_val_if_fail(signal   != NULL, 0);
	g_return_val_if_fail(handle   != NULL, 0);
	g_return_val_if_fail(func     != NULL, 0);

	/* Get the instance data */
	instance_data =
		(OulInstanceData *)g_hash_table_lookup(instance_table, instance);

	if (instance_data == NULL)
	{
		oul_debug_warning("signals", "Something tried to register a callback "
				"for the '%s' signal, but we do not have any signals "
				"registered with the given handle\n", signal);
		g_return_val_if_reached(0);
	}

	/* Get the signal data */
	signal_data =
		(OulSignalData *)g_hash_table_lookup(instance_data->signals, signal);

	if (signal_data == NULL)
	{
		oul_debug(OUL_DEBUG_ERROR, "signals",
				   "Signal data for %s not found!\n", signal);
		return 0;
	}

	/* Create the signal handler data */
	handler_data = g_new0(OulSignalHandlerData, 1);
	handler_data->id        = signal_data->next_handler_id;
	handler_data->cb        = func;
	handler_data->handle    = handle;
	handler_data->data      = data;
	handler_data->use_vargs = use_vargs;
	handler_data->priority = priority;

	signal_data->handlers = g_list_insert_sorted(signal_data->handlers, handler_data, (GCompareFunc)handler_priority);
	signal_data->handler_count++;
	signal_data->next_handler_id++;

	return handler_data->id;
}

gulong
oul_signal_connect_priority(void *instance, const char *signal, void *handle,
					OulCallback func, void *data, int priority)
{
	return signal_connect_common(instance, signal, handle, func, data, priority, FALSE);
}

gulong
oul_signal_connect(void *instance, const char *signal, void *handle,
					OulCallback func, void *data)
{
	return signal_connect_common(instance, signal, handle, func, data, OUL_SIGNAL_PRIORITY_DEFAULT, FALSE);
}

gulong
oul_signal_connect_priority_vargs(void *instance, const char *signal, void *handle,
						  OulCallback func, void *data, int priority)
{
	return signal_connect_common(instance, signal, handle, func, data, priority, TRUE);
}

gulong
oul_signal_connect_vargs(void *instance, const char *signal, void *handle,
						  OulCallback func, void *data)
{
	return signal_connect_common(instance, signal, handle, func, data, OUL_SIGNAL_PRIORITY_DEFAULT, TRUE);
}

void
oul_signal_disconnect(void *instance, const char *signal,
					   void *handle, OulCallback func)
{
	OulInstanceData *instance_data;
	OulSignalData *signal_data;
	OulSignalHandlerData *handler_data;
	GList *l;
	gboolean found = FALSE;

	g_return_if_fail(instance != NULL);
	g_return_if_fail(signal   != NULL);
	g_return_if_fail(handle   != NULL);
	g_return_if_fail(func     != NULL);

	/* Get the instance data */
	instance_data =
		(OulInstanceData *)g_hash_table_lookup(instance_table, instance);

	g_return_if_fail(instance_data != NULL);

	/* Get the signal data */
	signal_data =
		(OulSignalData *)g_hash_table_lookup(instance_data->signals, signal);

	if (signal_data == NULL)
	{
		oul_debug(OUL_DEBUG_ERROR, "signals",
				   "Signal data for %s not found!\n", signal);
		return;
	}

	/* Find the handler data. */
	for (l = signal_data->handlers; l != NULL; l = l->next)
	{
		handler_data = (OulSignalHandlerData *)l->data;

		if (handler_data->handle == handle && handler_data->cb == func)
		{
			g_free(handler_data);

			signal_data->handlers = g_list_remove(signal_data->handlers,
												  handler_data);
			signal_data->handler_count--;

			found = TRUE;

			break;
		}
	}

	/* See note somewhere about this actually helping developers.. */
	g_return_if_fail(found);
}

/*
 * TODO: Make this all more efficient by storing a list of handlers, keyed
 *       to a handle.
 */
static void
disconnect_handle_from_signals(const char *signal,
							   OulSignalData *signal_data, void *handle)
{
	GList *l, *l_next;
	OulSignalHandlerData *handler_data;

	for (l = signal_data->handlers; l != NULL; l = l_next)
	{
		handler_data = (OulSignalHandlerData *)l->data;
		l_next = l->next;

		if (handler_data->handle == handle)
		{
			g_free(handler_data);

			signal_data->handler_count--;
			signal_data->handlers = g_list_remove(signal_data->handlers,
												  handler_data);
		}
	}
}

static void
disconnect_handle_from_instance(void *instance,
								OulInstanceData *instance_data,
								void *handle)
{
	g_hash_table_foreach(instance_data->signals,
						 (GHFunc)disconnect_handle_from_signals, handle);
}

void
oul_signals_disconnect_by_handle(void *handle)
{
	g_return_if_fail(handle != NULL);

	g_hash_table_foreach(instance_table,
						 (GHFunc)disconnect_handle_from_instance, handle);
}

void
oul_signal_emit(void *instance, const char *signal, ...)
{
	va_list args;

	g_return_if_fail(instance != NULL);
	g_return_if_fail(signal   != NULL);

	va_start(args, signal);
	oul_signal_emit_vargs(instance, signal, args);
	va_end(args);
}

void
oul_signal_emit_vargs(void *instance, const char *signal, va_list args)
{
	OulInstanceData *instance_data;
	OulSignalData *signal_data;
	OulSignalHandlerData *handler_data;
	GList *l, *l_next;
	va_list tmp;

	g_return_if_fail(instance != NULL);
	g_return_if_fail(signal   != NULL);

	instance_data =
		(OulInstanceData *)g_hash_table_lookup(instance_table, instance);

	g_return_if_fail(instance_data != NULL);

	signal_data =
		(OulSignalData *)g_hash_table_lookup(instance_data->signals, signal);

	if (signal_data == NULL)
	{
		oul_debug(OUL_DEBUG_ERROR, "signals",
				   "Signal data for %s not found!\n", signal);
		return;
	}

	for (l = signal_data->handlers; l != NULL; l = l_next)
	{
		l_next = l->next;

		handler_data = (OulSignalHandlerData *)l->data;

		/* This is necessary because a va_list may only be
		 * evaluated once */
		G_VA_COPY(tmp, args);

		if (handler_data->use_vargs)
		{
			((void (*)(va_list, void *))handler_data->cb)(tmp,
														  handler_data->data);
		}
		else
		{
			signal_data->marshal(handler_data->cb, tmp,
								 handler_data->data, NULL);
		}

		va_end(tmp);
	}

#ifdef HAVE_DBUS
	oul_dbus_signal_emit_Oul(signal, signal_data->num_values, 
				   signal_data->values, args);
#endif	/* HAVE_DBUS */

}

void *
oul_signal_emit_return_1(void *instance, const char *signal, ...)
{
	void *ret_val;
	va_list args;

	g_return_val_if_fail(instance != NULL, NULL);
	g_return_val_if_fail(signal   != NULL, NULL);

	va_start(args, signal);
	ret_val = oul_signal_emit_vargs_return_1(instance, signal, args);
	va_end(args);

	return ret_val;
}

void *
oul_signal_emit_vargs_return_1(void *instance, const char *signal,
								va_list args)
{
	OulInstanceData *instance_data;
	OulSignalData *signal_data;
	OulSignalHandlerData *handler_data;
	GList *l, *l_next;
	va_list tmp;

	g_return_val_if_fail(instance != NULL, NULL);
	g_return_val_if_fail(signal   != NULL, NULL);

	instance_data =
		(OulInstanceData *)g_hash_table_lookup(instance_table, instance);

	g_return_val_if_fail(instance_data != NULL, NULL);

	signal_data =
		(OulSignalData *)g_hash_table_lookup(instance_data->signals, signal);

	if (signal_data == NULL)
	{
		oul_debug(OUL_DEBUG_ERROR, "signals",
				   "Signal data for %s not found!\n", signal);
		return 0;
	}

#ifdef HAVE_DBUS
	G_VA_COPY(tmp, args);
	oul_dbus_signal_emit_Oul(signal, signal_data->num_values, 
				   signal_data->values, tmp);
	va_end(tmp);
#endif	/* HAVE_DBUS */

	for (l = signal_data->handlers; l != NULL; l = l_next)
	{
		void *ret_val = NULL;

		l_next = l->next;

		handler_data = (OulSignalHandlerData *)l->data;

		G_VA_COPY(tmp, args);
		if (handler_data->use_vargs)
		{
			ret_val = ((void *(*)(va_list, void *))handler_data->cb)(
				tmp, handler_data->data);
		}
		else
		{
			signal_data->marshal(handler_data->cb, tmp,
								 handler_data->data, &ret_val);
		}
		va_end(tmp);

		if (ret_val != NULL)
			return ret_val;
	}

	return NULL;
}

void
oul_signals_init()
{
	g_return_if_fail(instance_table == NULL);

	instance_table =
		g_hash_table_new_full(g_direct_hash, g_direct_equal,
							  NULL, (GDestroyNotify)destroy_instance_data);
}

void
oul_signals_uninit()
{
	g_return_if_fail(instance_table != NULL);

	g_hash_table_destroy(instance_table);
	instance_table = NULL;
}

/**************************************************************************
 * Marshallers
 **************************************************************************/
void
oul_marshal_VOID(OulCallback cb, va_list args, void *data,
				  void **return_val)
{
	((void (*)(void *))cb)(data);
}

void
oul_marshal_VOID__INT(OulCallback cb, va_list args, void *data,
					   void **return_val)
{
	gint arg1 = va_arg(args, gint);

	((void (*)(gint, void *))cb)(arg1, data);
}

void
oul_marshal_VOID__INT_INT(OulCallback cb, va_list args, void *data,
						   void **return_val)
{
	gint arg1 = va_arg(args, gint);
	gint arg2 = va_arg(args, gint);

	((void (*)(gint, gint, void *))cb)(arg1, arg2, data);
}

void
oul_marshal_VOID__POINTER(OulCallback cb, va_list args, void *data,
						   void **return_val)
{
	void *arg1 = va_arg(args, void *);

	((void (*)(void *, void *))cb)(arg1, data);
}

void
oul_marshal_VOID__POINTER_UINT(OulCallback cb, va_list args,
										void *data, void **return_val)
{
	void *arg1 = va_arg(args, void *);
	guint arg2 = va_arg(args, guint);

	((void (*)(void *, guint, void *))cb)(arg1, arg2, data);
}

void oul_marshal_VOID__POINTER_INT_INT(OulCallback cb, va_list args,
                                        void *data, void **return_val)
{
	void *arg1 = va_arg(args, void *);
	gint arg2 = va_arg(args, gint);
	gint arg3 = va_arg(args, gint);

	((void (*)(void *, gint, gint, void *))cb)(arg1, arg2, arg3, data);
}

void oul_marshal_VOID__POINTER_INT_POINTER(OulCallback cb, va_list args,
                                              void *data, void **return_val)
{
	void *arg1 = va_arg(args, void *);
	gint arg2 = va_arg(args, gint);
	void *arg3 = va_arg(args, void *);

	((void (*)(void *, gint, void *, void *))cb)(arg1, arg2, arg3, data);
}

void
oul_marshal_VOID__POINTER_POINTER(OulCallback cb, va_list args,
								   void *data, void **return_val)
{
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);

	((void (*)(void *, void *, void *))cb)(arg1, arg2, data);
}

void
oul_marshal_VOID__POINTER_POINTER_UINT(OulCallback cb, va_list args,
										void *data, void **return_val)
{
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	guint arg3 = va_arg(args, guint);

	((void (*)(void *, void *, guint, void *))cb)(arg1, arg2, arg3, data);
}

void
oul_marshal_VOID__POINTER_POINTER_UINT_UINT(OulCallback cb, va_list args,
										     void *data, void **return_val)
{
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	guint arg3 = va_arg(args, guint);
	guint arg4 = va_arg(args, guint);

	((void (*)(void *, void *, guint, guint, void *))cb)(arg1, arg2, arg3, arg4, data);
}

void
oul_marshal_VOID__POINTER_POINTER_POINTER(OulCallback cb, va_list args,
										   void *data, void **return_val)
{
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	void *arg3 = va_arg(args, void *);

	((void (*)(void *, void *, void *, void *))cb)(arg1, arg2, arg3, data);
}

void
oul_marshal_VOID__POINTER_POINTER_POINTER_POINTER(OulCallback cb,
												   va_list args,
												   void *data,
												   void **return_val)
{
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	void *arg3 = va_arg(args, void *);
	void *arg4 = va_arg(args, void *);

	((void (*)(void *, void *, void *, void *, void *))cb)(arg1, arg2, arg3, arg4, data);
}

void
oul_marshal_VOID__POINTER_POINTER_POINTER_POINTER_POINTER(OulCallback cb,
														   va_list args,
														   void *data,
														   void **return_val)
{
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	void *arg3 = va_arg(args, void *);
	void *arg4 = va_arg(args, void *);
	void *arg5 = va_arg(args, void *);

	((void (*)(void *, void *, void *, void *, void *, void *))cb)(arg1, arg2, arg3, arg4, arg5, data);
}

void
oul_marshal_VOID__POINTER_POINTER_POINTER_UINT(OulCallback cb,
												   va_list args,
												   void *data,
												   void **return_val)
{
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	void *arg3 = va_arg(args, void *);
	guint arg4 = va_arg(args, guint);

	((void (*)(void *, void *, void *, guint, void *))cb)(arg1, arg2, arg3, arg4, data);
}

void
oul_marshal_VOID__POINTER_POINTER_POINTER_POINTER_UINT(OulCallback cb,
													    va_list args,
													    void *data,
													    void **return_val)
{
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	void *arg3 = va_arg(args, void *);
	void *arg4 = va_arg(args, void *);
	guint arg5 = va_arg(args, guint);

	((void (*)(void *, void *, void *, void *, guint, void *))cb)(arg1, arg2, arg3, arg4, arg5, data);
}

void
oul_marshal_VOID__POINTER_POINTER_POINTER_UINT_UINT(OulCallback cb,
													 va_list args,
													 void *data,
													 void **return_val)
{
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	void *arg3 = va_arg(args, void *);
	guint arg4 = va_arg(args, guint);
	guint arg5 = va_arg(args, guint);

	((void (*)(void *, void *, void *, guint, guint, void *))cb)(
			arg1, arg2, arg3, arg4, arg5, data);
}

void
oul_marshal_INT__INT(OulCallback cb, va_list args, void *data,
					  void **return_val)
{
	gint ret_val;
	gint arg1 = va_arg(args, gint);

	ret_val = ((gint (*)(gint, void *))cb)(arg1, data);

	if (return_val != NULL)
		*return_val = GINT_TO_POINTER(ret_val);
}

void
oul_marshal_INT__INT_INT(OulCallback cb, va_list args, void *data,
						  void **return_val)
{
	gint ret_val;
	gint arg1 = va_arg(args, gint);
	gint arg2 = va_arg(args, gint);

	ret_val = ((gint (*)(gint, gint, void *))cb)(arg1, arg2, data);

	if (return_val != NULL)
		*return_val = GINT_TO_POINTER(ret_val);
}

void
oul_marshal_INT__POINTER_POINTER(OulCallback cb, va_list args, void *data,
                                      void **return_val)
{
	gint ret_val;
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);

	ret_val = ((gint (*)(void *, void *, void *))cb)(arg1, arg2, data);

	if (return_val != NULL)
		*return_val = GINT_TO_POINTER(ret_val);
}

void
oul_marshal_INT__POINTER_POINTER_POINTER_POINTER_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val)
{
	gint ret_val;
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	void *arg3 = va_arg(args, void *);
	void *arg4 = va_arg(args, void *);
	void *arg5 = va_arg(args, void *);

	ret_val =
		((gint (*)(void *, void *, void *, void *, void *, void *))cb)(
			arg1, arg2, arg3, arg4, arg5, data);

	if (return_val != NULL)
		*return_val = GINT_TO_POINTER(ret_val);
}

void
oul_marshal_BOOLEAN__POINTER(OulCallback cb, va_list args, void *data,
							  void **return_val)
{
	gboolean ret_val;
	void *arg1 = va_arg(args, void *);

	ret_val = ((gboolean (*)(void *, void *))cb)(arg1, data);

	if (return_val != NULL)
		*return_val = GINT_TO_POINTER(ret_val);
}

void
oul_marshal_BOOLEAN__POINTER_POINTER(OulCallback cb, va_list args,
									  void *data, void **return_val)
{
	gboolean ret_val;
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);

	ret_val = ((gboolean (*)(void *, void *, void *))cb)(arg1, arg2, data);

	if (return_val != NULL)
		*return_val = GINT_TO_POINTER(ret_val);
}

void
oul_marshal_BOOLEAN__POINTER_POINTER_POINTER(OulCallback cb, va_list args,
											  void *data, void **return_val)
{
	gboolean ret_val;
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	void *arg3 = va_arg(args, void *);

	ret_val = ((gboolean (*)(void *, void *, void *, void *))cb)(arg1, arg2,
																 arg3, data);

	if (return_val != NULL)
		*return_val = GINT_TO_POINTER(ret_val);
}

void
oul_marshal_BOOLEAN__POINTER_POINTER_UINT(OulCallback cb,
												   va_list args,
												   void *data,
												   void **return_val)
{
	gboolean ret_val;
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	guint arg3 = va_arg(args, guint);

	ret_val = ((gboolean (*)(void *, void *, guint, void *))cb)(
			arg1, arg2, arg3, data);

	if (return_val != NULL)
		*return_val = GINT_TO_POINTER(ret_val);
}

void
oul_marshal_BOOLEAN__POINTER_POINTER_POINTER_UINT(OulCallback cb,
												   va_list args,
												   void *data,
												   void **return_val)
{
	gboolean ret_val;
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	void *arg3 = va_arg(args, void *);
	guint arg4 = va_arg(args, guint);

	ret_val = ((gboolean (*)(void *, void *, void *, guint, void *))cb)(
			arg1, arg2, arg3, arg4, data);

	if (return_val != NULL)
		*return_val = GINT_TO_POINTER(ret_val);
}

void
oul_marshal_BOOLEAN__POINTER_POINTER_POINTER_POINTER(OulCallback cb,
													  va_list args,
													  void *data,
													  void **return_val)
{
	gboolean ret_val;
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	void *arg3 = va_arg(args, void *);
	void *arg4 = va_arg(args, void *);

	ret_val = ((gboolean (*)(void *, void *, void *, void *, void *))cb)(
			arg1, arg2, arg3, arg4, data);

	if (return_val != NULL)
		*return_val = GINT_TO_POINTER(ret_val);
}

void
oul_marshal_BOOLEAN__POINTER_POINTER_POINTER_POINTER_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val)
{
	gboolean ret_val;
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	void *arg3 = va_arg(args, void *);
	void *arg4 = va_arg(args, void *);
	void *arg5 = va_arg(args, void *);

	ret_val =
		((gboolean (*)(void *, void *, void *, void *, void *, void *))cb)(
			arg1, arg2, arg3, arg4, arg5, data);

	if (return_val != NULL)
		*return_val = GINT_TO_POINTER(ret_val);
}

void
oul_marshal_BOOLEAN__POINTER_POINTER_POINTER_POINTER_UINT(
		OulCallback cb, va_list args, void *data, void **return_val)
{
	gboolean ret_val;
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);
	void *arg3 = va_arg(args, void *);
	void *arg4 = va_arg(args, void *);
	guint arg5 = va_arg(args, guint);

	ret_val =
		((gboolean (*)(void *, void *, void *, void *, guint, void *))cb)(
			arg1, arg2, arg3, arg4, arg5, data);

	if (return_val != NULL)
		*return_val = GINT_TO_POINTER(ret_val);
}

void
oul_marshal_BOOLEAN__INT_POINTER(OulCallback cb, va_list args, void *data,
                                  void **return_val)
{
	gboolean ret_val;
	gint arg1 = va_arg(args, gint);
	void *arg2 = va_arg(args, void *);

	ret_val = ((gboolean (*)(gint, void *, void *))cb)(arg1, arg2, data);

	if (return_val != NULL)
		*return_val = GINT_TO_POINTER(ret_val);
}

void
oul_marshal_POINTER__POINTER_INT(
                                    OulCallback cb, va_list args, void *data,
                                    void **return_val)
{
	gpointer ret_val;
	void *arg1 = va_arg(args, void *);
	gint arg2 = va_arg(args, gint);

	ret_val = ((gpointer(*)(void *, gint, void *))cb)(arg1, arg2, data);

	if (return_val != NULL)
		*return_val = ret_val;
}

void
oul_marshal_POINTER__POINTER_INT64(
                                    OulCallback cb, va_list args, void *data,
                                    void **return_val)
{
	gpointer ret_val;
	void *arg1 = va_arg(args, void *);
	gint64 arg2 = va_arg(args, gint64);

	ret_val = ((gpointer(*)(void *, gint64, void *))cb)(arg1, arg2, data);

	if (return_val != NULL)
		*return_val = ret_val;
}

void
oul_marshal_POINTER__POINTER_INT_BOOLEAN(
                                    OulCallback cb, va_list args, void *data,
                                    void **return_val)
{
	gpointer ret_val;
	void *arg1 = va_arg(args, void *);
	gint arg2 = va_arg(args, gint);
	gboolean arg3 = va_arg(args, gboolean);

	ret_val = ((gpointer(*)(void *, gint, gboolean, void *))cb)(arg1, arg2, arg3, data);

	if (return_val != NULL)
		*return_val = ret_val;
}

void
oul_marshal_POINTER__POINTER_INT64_BOOLEAN(
                                    OulCallback cb, va_list args, void *data,
                                    void **return_val)
{
	gpointer ret_val;
	void *arg1 = va_arg(args, void *);
	gint64 arg2 = va_arg(args, gint64);
	gboolean arg3 = va_arg(args, gboolean);

	ret_val = ((gpointer(*)(void *, gint64, gboolean, void *))cb)(arg1, arg2, arg3, data);

	if (return_val != NULL)
		*return_val = ret_val;
}

void
oul_marshal_POINTER__POINTER_POINTER(OulCallback cb, va_list args, void *data,
                                      void **return_val)
{
	gpointer ret_val;
	void *arg1 = va_arg(args, void *);
	void *arg2 = va_arg(args, void *);

	ret_val = ((gpointer (*)(void *, void *, void *))cb)(arg1, arg2, data);

	if (return_val != NULL)
		*return_val = ret_val;
}
