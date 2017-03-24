#ifndef _OUL_SIGNALS_H_
#define _OUL_SIGNALS_H_

#include <glib.h>
#include "value.h"

#define OUL_CALLBACK(func) ((OulCallback)func)

typedef void (*OulCallback)(void);
typedef void (*OulSignalMarshalFunc)(OulCallback cb, va_list args,
									  void *data, void **return_val);

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/** @name Signal API                                                      */
/**************************************************************************/
/*@{*/

/** The priority of a signal connected using oul_signal_connect().
 *
 *  @see oul_signal_connect_priority()
 */
#define OUL_SIGNAL_PRIORITY_DEFAULT     0
/** The largest signal priority; signals with this priority will be called
 *  <em>last</em>.  (This is highest as in numerical value, not as in order of
 *  importance.)
 *
 *  @see oul_signal_connect_priority().
 */
#define OUL_SIGNAL_PRIORITY_HIGHEST  9999
/** The smallest signal priority; signals with this priority will be called
 *  <em>first</em>.  (This is lowest as in numerical value, not as in order of
 *  importance.)
 *
 *  @see oul_signal_connect_priority().
 */
#define OUL_SIGNAL_PRIORITY_LOWEST  -9999

/**
 * Registers a signal in an instance.
 *
 * @param instance   The instance to register the signal for.
 * @param signal     The signal name.
 * @param marshal    The marshal function.
 * @param ret_value  The return value type, or NULL for no return value.
 * @param num_values The number of values to be passed to the callbacks.
 * @param ...        The values to pass to the callbacks.
 *
 * @return The signal ID local to that instance, or 0 if the signal
 *         couldn't be registered.
 *
 * @see OulValue
 */
gulong oul_signal_register(void *instance, const char *signal,
							OulSignalMarshalFunc marshal,
							OulValue *ret_value, int num_values, ...);

/**
 * Unregisters a signal in an instance.
 *
 * @param instance The instance to unregister the signal for.
 * @param signal   The signal name.
 */
void oul_signal_unregister(void *instance, const char *signal);

/**
 * Unregisters all signals in an instance.
 *
 * @param instance The instance to unregister the signal for.
 */
void oul_signals_unregister_by_instance(void *instance);

/**
 * Returns a list of value types used for a signal.
 *
 * @param instance   The instance the signal is registered to.
 * @param signal     The signal.
 * @param ret_value  The return value from the last signal handler.
 * @param num_values The returned number of values.
 * @param values     The returned list of values.
 */
void oul_signal_get_values(void *instance, const char *signal,
							OulValue **ret_value,
							int *num_values, OulValue ***values);

/**
 * Connects a signal handler to a signal for a particular object.
 *
 * Take care not to register a handler function twice. Oul will
 * not correct any mistakes for you in this area.
 *
 * @param instance The instance to connect to.
 * @param signal   The name of the signal to connect.
 * @param handle   The handle of the receiver.
 * @param func     The callback function.
 * @param data     The data to pass to the callback function.
 * @param priority The priority with which the handler should be called. Signal
 *                 handlers are called in ascending numerical order of @a
 *                 priority from #OUL_SIGNAL_PRIORITY_LOWEST to
 *                 #OUL_SIGNAL_PRIORITY_HIGHEST.
 *
 * @return The signal handler ID.
 *
 * @see oul_signal_disconnect()
 */
gulong oul_signal_connect_priority(void *instance, const char *signal,
	void *handle, OulCallback func, void *data, int priority);

/**
 * Connects a signal handler to a signal for a particular object.
 * (Its priority defaults to 0, aka #OUL_SIGNAL_PRIORITY_DEFAULT.)
 * 
 * Take care not to register a handler function twice. Oul will
 * not correct any mistakes for you in this area.
 *
 * @param instance The instance to connect to.
 * @param signal   The name of the signal to connect.
 * @param handle   The handle of the receiver.
 * @param func     The callback function.
 * @param data     The data to pass to the callback function.
 *
 * @return The signal handler ID.
 *
 * @see oul_signal_disconnect()
 */
gulong oul_signal_connect(void *instance, const char *signal,
	void *handle, OulCallback func, void *data);

/**
 * Connects a signal handler to a signal for a particular object.
 *
 * The signal handler will take a va_args of arguments, instead of
 * individual arguments.
 *
 * Take care not to register a handler function twice. Oul will
 * not correct any mistakes for you in this area.
 *
 * @param instance The instance to connect to.
 * @param signal   The name of the signal to connect.
 * @param handle   The handle of the receiver.
 * @param func     The callback function.
 * @param data     The data to pass to the callback function.
 * @param priority The priority with which the handler should be called. Signal
 *                 handlers are called in ascending numerical order of @a
 *                 priority from #OUL_SIGNAL_PRIORITY_LOWEST to
 *                 #OUL_SIGNAL_PRIORITY_HIGHEST.
 *
 * @return The signal handler ID.
 *
 * @see oul_signal_disconnect()
 */
gulong oul_signal_connect_priority_vargs(void *instance, const char *signal,
	void *handle, OulCallback func, void *data, int priority);

/**
 * Connects a signal handler to a signal for a particular object.
 * (Its priority defaults to 0, aka #OUL_SIGNAL_PRIORITY_DEFAULT.)
 *
 * The signal handler will take a va_args of arguments, instead of
 * individual arguments.
 *
 * Take care not to register a handler function twice. Oul will
 * not correct any mistakes for you in this area.
 *
 * @param instance The instance to connect to.
 * @param signal   The name of the signal to connect.
 * @param handle   The handle of the receiver.
 * @param func     The callback function.
 * @param data     The data to pass to the callback function.
 *
 * @return The signal handler ID.
 *
 * @see oul_signal_disconnect()
 */
gulong oul_signal_connect_vargs(void *instance, const char *signal,
	void *handle, OulCallback func, void *data);

/**
 * Disconnects a signal handler from a signal on an object.
 *
 * @param instance The instance to disconnect from.
 * @param signal   The name of the signal to disconnect.
 * @param handle   The handle of the receiver.
 * @param func     The registered function to disconnect.
 *
 * @see oul_signal_connect()
 */
void oul_signal_disconnect(void *instance, const char *signal,
							void *handle, OulCallback func);

/**
 * Removes all callbacks associated with a receiver handle.
 *
 * @param handle The receiver handle.
 */
void oul_signals_disconnect_by_handle(void *handle);

/**
 * Emits a signal.
 *
 * @param instance The instance emitting the signal.
 * @param signal   The signal being emitted.
 *
 * @see oul_signal_connect()
 * @see oul_signal_disconnect()
 */
void oul_signal_emit(void *instance, const char *signal, ...);

/**
 * Emits a signal, using a va_list of arguments.
 *
 * @param instance The instance emitting the signal.
 * @param signal   The signal being emitted.
 * @param args     The arguments list.
 *
 * @see oul_signal_connect()
 * @see oul_signal_disconnect()
 */
void oul_signal_emit_vargs(void *instance, const char *signal, va_list args);

/**
 * Emits a signal and returns the first non-NULL return value.
 *
 * Further signal handlers are NOT called after a handler returns
 * something other than NULL.
 *
 * @param instance The instance emitting the signal.
 * @param signal   The signal being emitted.
 *
 * @return The first non-NULL return value
 */
void *oul_signal_emit_return_1(void *instance, const char *signal, ...);

/**
 * Emits a signal and returns the first non-NULL return value.
 *
 * Further signal handlers are NOT called after a handler returns
 * something other than NULL.
 *
 * @param instance The instance emitting the signal.
 * @param signal   The signal being emitted.
 * @param args     The arguments list.
 *
 * @return The first non-NULL return value
 */
void *oul_signal_emit_vargs_return_1(void *instance, const char *signal,
									  va_list args);

/**
 * Initializes the signals subsystem.
 */
void oul_signals_init(void);

/**
 * Uninitializes the signals subsystem.
 */
void oul_signals_uninit(void);

/*@}*/

/**************************************************************************/
/** @name Marshal Functions                                               */
/**************************************************************************/
/*@{*/

void oul_marshal_VOID(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__INT(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__INT_INT(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__POINTER_UINT(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__POINTER_INT_INT(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__POINTER_INT_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__POINTER_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__POINTER_POINTER_UINT(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__POINTER_POINTER_UINT_UINT(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__POINTER_POINTER_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__POINTER_POINTER_POINTER_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__POINTER_POINTER_POINTER_POINTER_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__POINTER_POINTER_POINTER_UINT(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__POINTER_POINTER_POINTER_POINTER_UINT(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_VOID__POINTER_POINTER_POINTER_UINT_UINT(
		OulCallback cb, va_list args, void *data, void **return_val);

void oul_marshal_INT__INT(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_INT__INT_INT(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_INT__POINTER_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_INT__POINTER_POINTER_POINTER_POINTER_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);

void oul_marshal_BOOLEAN__POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_BOOLEAN__POINTER_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_BOOLEAN__POINTER_POINTER_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_BOOLEAN__POINTER_POINTER_UINT(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_BOOLEAN__POINTER_POINTER_POINTER_UINT(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_BOOLEAN__POINTER_POINTER_POINTER_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_BOOLEAN__POINTER_POINTER_POINTER_POINTER_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_BOOLEAN__POINTER_POINTER_POINTER_POINTER_UINT(
		OulCallback cb, va_list args, void *data, void **return_val);

void oul_marshal_BOOLEAN__INT_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);

void oul_marshal_POINTER__POINTER_INT(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_POINTER__POINTER_INT64(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_POINTER__POINTER_INT_BOOLEAN(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_POINTER__POINTER_INT64_BOOLEAN(
		OulCallback cb, va_list args, void *data, void **return_val);
void oul_marshal_POINTER__POINTER_POINTER(
		OulCallback cb, va_list args, void *data, void **return_val);
/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _OUL_SIGNALS_H_ */
