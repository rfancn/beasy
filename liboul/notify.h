#ifndef _OUL_NOTIFY_H_
#define _OUL_NOTIFY_H_


#include "msg.h"

/**
 * Notification close callbacks.
 */
typedef void  (*OulNotifyCloseCallback) (gpointer user_data);


/**
 * Notification types.
 */
typedef enum
{
	OUL_NOTIFY_MESSAGE = 0,   /**< Message notification.         */
	OUL_NOTIFY_EMAIL,         /**< Single e-mail notification.   */
	OUL_NOTIFY_EMAILS,        /**< Multiple e-mail notification. */
	OUL_NOTIFY_FORMATTED,     /**< Formatted text.               */
	OUL_NOTIFY_TABLE, 			/**< Buddy search results.         */
	OUL_NOTIFY_URI            /**< URI notification or display.  */

} OulNotifyType;


/**
 * Notification message types.
 */
typedef enum
{
	OUL_NOTIFY_MSG_ERROR   = 0, /**< Error notification.       */
	OUL_NOTIFY_MSG_WARNING,     /**< Warning notification.     */
	OUL_NOTIFY_MSG_INFO         /**< Information notification. */

} OulNotifyMsgType;

/**
 * Notification UI operations.
 */
typedef struct
{
	void	(*init)(void);
	void	(*uninit)(void);
	
	void *	(*notify_message)(OulNotifyMsgType type, const char *title,
	                        const char *primary, const char *secondary);

	void *	(*notify_formatted)(const char *title, const char *primary,
	                          const char *secondary, const char *text, int width, int height);
	
	void *	(*notify_table)(const char *title, const OulMsg *msg);

	void *	(*notify_uri)(const char *uri);

	void 	(*close_notify)(OulNotifyType type, void *ui_handle);

	void (*_oul_reserved1)(void);
	void (*_oul_reserved2)(void);
} OulNotifyUiOps;

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/** @name Notification API                                                */
/**************************************************************************/
/*@{*/

/**
 * Displays a notification message to the user.
 *
 * @param handle    The plugin or connection handle.
 * @param type      The notification type.
 * @param title     The title of the message.
 * @param primary   The main point of the message.
 * @param secondary The secondary information.
 * @param cb        The callback to call when the user closes
 *                  the notification.
 * @param user_data The data to pass to the callback.
 *
 * @return A UI-specific handle.
 */
void *oul_notify_message(void *handle, OulNotifyMsgType type,
						  const char *title, const char *primary,
						  const char *secondary, OulNotifyCloseCallback cb,
						  gpointer user_data);

/**
 * Displays a notification with formatted text.
 *
 * The text is essentially a stripped-down format of HTML, the same that
 * IMs may send.
 *
 * @param handle    The plugin or connection handle.
 * @param title     The title of the message.
 * @param primary   The main point of the message.
 * @param secondary The secondary information.
 * @param text      The formatted text.
 * @param cb        The callback to call when the user closes
 *                  the notification.
 * @param user_data The data to pass to the callback.
 *
 * @return A UI-specific handle.
 */
void *oul_notify_formatted(void *handle, const char *title,
							const char *primary, const char *secondary, const char *text, 
							int width, int height, OulNotifyCloseCallback cb, gpointer user_data);

void *
oul_notify_table(void *handle, const char *title, OulMsg *msg, OulNotifyCloseCallback cb, gpointer user_data);



/**
 * Opens a URI or somehow presents it to the user.
 *
 * @param handle The plugin or connection handle.
 * @param uri    The URI to display or go to.
 *
 * @return A UI-specific handle, if any. This may only be presented if
 *         the UI code displays a dialog instead of a webpage, or something
 *         similar.
 */
void *oul_notify_uri(void *handle, const char *uri);

/**
 * Closes a notification.
 *
 * This should be used only by the UI operation functions and part of the
 * core.
 *
 * @param type      The notification type.
 * @param ui_handle The notification UI handle.
 */
void oul_notify_close(OulNotifyType type, void *ui_handle);

/**
 * Closes all notifications registered with the specified handle.
 *
 * @param handle The handle.
 */
void oul_notify_close_with_handle(void *handle);

/**
 * A wrapper for oul_notify_message that displays an information message.
 */
#define oul_notify_info(handle, title, primary, secondary) \
	oul_notify_message((handle), OUL_NOTIFY_MSG_INFO, (title), \
						(primary), (secondary), NULL, NULL)

/**
 * A wrapper for oul_notify_message that displays a warning message.
 */
#define oul_notify_warning(handle, title, primary, secondary) \
	oul_notify_message((handle), OUL_NOTIFY_MSG_WARNING, (title), \
						(primary), (secondary), NULL, NULL)

/**
 * A wrapper for oul_notify_message that displays an error message.
 */
#define oul_notify_error(handle, title, primary, secondary) \
	oul_notify_message((handle), OUL_NOTIFY_MSG_ERROR, (title), \
						(primary), (secondary), NULL, NULL)

/*@}*/

/**************************************************************************/
/** @name UI Registration Functions                                       */
/**************************************************************************/
/*@{*/

/**
 * Sets the UI operations structure to be used when displaying a
 * notification.
 *
 * @param ops The UI operations structure.
 */
void oul_notify_set_ui_ops(OulNotifyUiOps *ops);

/**
 * Returns the UI operations structure to be used when displaying a
 * notification.
 *
 * @return The UI operations structure.
 */
OulNotifyUiOps *oul_notify_get_ui_ops(void);

/*@}*/


void 				oul_notify_init(void);
void 				oul_notify_uninit(void);
void *				oul_notify_get_handle(void);


#ifdef __cplusplus
}
#endif

#endif /* _OUL_NOTIFY_H_ */
