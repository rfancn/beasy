#ifndef _OUL_SAVEDSTATUSES_H_
#define _OUL_SAVEDSTATUSES_H_

/**
 * Saved statuses don't really interact much with the rest of Oul.  It
 * could really be a plugin.  It's just a list of away states.  When
 * a user chooses one of the saved states, their Oul plugins are set
 * to the settings of that state.
 *
 * In the savedstatus API, there is the concept of a 'transient'
 * saved status.  A transient saved status is one that is not
 * permanent.  Oul will removed it automatically if it isn't
 * used for a period of time.  Transient saved statuses don't
 * have titles and they don't show up in the list of saved
 * statuses.  In fact, if a saved status does not have a title
 * then it is transient.  If it does have a title, then it is not
 * transient.
 *
 * What good is a transient status, you ask?  They can be used to
 * keep track of the user's 5 most recently used statuses, for
 * example.  Basically if they just set a message on the fly,
 * we'll cache it for them in case they want to use it again.  If
 * they don't use it again, we'll just delete it.
 */

/*
 * TODO: Hmm.  We should probably just be saving BeasyPresences.  That's
 *       something we should look into once the status box gets fleshed
 *       out more.
 */

#include "plugin.h"
#include "status.h"

typedef struct _OulSavedStatus     OulSavedStatus;
typedef struct _OulSavedStatusSub  OulSavedStatusSub;

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/** @name Saved status subsystem                                          */
/**************************************************************************/
/*@{*/

/**
 * Create a new saved status.  This will add the saved status to the
 * list of saved statuses and writes the revised list to status.xml.
 *
 * @param title     The title of the saved status.  This must be
 *                  unique.  Or, if you want to create a transient
 *                  saved status, then pass in NULL.
 * @param type      The type of saved status.
 *
 * @return The newly created saved status, or NULL if the title you
 *         used was already taken.
 */
OulSavedStatus *oul_savedstatus_new(const char *title,
									  OulStatusPrimitive type);

/**
 * Set the title for the given saved status.
 *
 * @param status  The saved status.
 * @param title   The title of the saved status.
 */
void oul_savedstatus_set_title(OulSavedStatus *status,
								const char *title);

/**
 * Set the type for the given saved status.
 *
 * @param status  The saved status.
 * @param type    The type of saved status.
 */
void oul_savedstatus_set_type(OulSavedStatus *status,
							   OulStatusPrimitive type);

/**
 * Set the message for the given saved status.
 *
 * @param status  The saved status.
 * @param message The message, or NULL if you want to unset the
 *                message for this status.
 */
void oul_savedstatus_set_message(OulSavedStatus *status,
								  const char *message);

/**
 * Delete a saved status.  This removes the saved status from the list
 * of saved statuses, and writes the revised list to status.xml.
 *
 * @param title The title of the saved status.
 *
 * @return TRUE if the status was successfully deleted.  FALSE if the
 *         status could not be deleted because no saved status exists
 *         with the given title.
 */
gboolean oul_savedstatus_delete(const char *title);

/**
 * Delete a saved status.  This removes the saved status from the list
 * of saved statuses, and writes the revised list to status.xml.
 *
 * @param saved_status the status to delete, the pointer is invalid after
 *        the call
 *
 */
void oul_savedstatus_delete_by_status(OulSavedStatus *saved_status);

/**
 * Returns all saved statuses.
 *
 * @constreturn A list of saved statuses.
 */
GList *oul_savedstatuses_get_all(void);

/**
 * Returns the n most popular saved statuses.  "Popularity" is
 * determined by when the last time a saved_status was used and
 * how many times it has been used. Transient statuses without
 * messages are not included in the list.
 *
 * @param how_many The maximum number of saved statuses
 *                 to return, or '0' to get all saved
 *                 statuses sorted by popularity.
 * @return A linked list containing at most how_many
 *         OulSavedStatuses.  This list should be
 *         g_list_free'd by the caller (but the
 *         OulSavedStatuses must not be free'd).
 */
GList *oul_savedstatuses_get_popular(unsigned int how_many);

/**
 * Returns the currently selected saved status. 
 * it returns oul_savedstatus_get_default().
 *
 * @return A pointer to the in-use OulSavedStatus.
 *         This function never returns NULL.
 */
OulSavedStatus *oul_savedstatus_get_current(void);

/**
 * Returns the default saved status that is used when our
 * accounts are not idle-away.
 *
 * @return A pointer to the in-use OulSavedStatus.
 *         This function never returns NULL.
 */
OulSavedStatus *oul_savedstatus_get_default(void);

/**
 * Returns the status to be used when oul is starting up
 *
 * @return A pointer to the startup OulSavedStatus.
 *         This function never returns NULL.
 */
OulSavedStatus *oul_savedstatus_get_startup(void);

/**
 * Finds a saved status with the specified title.
 *
 * @param title The name of the saved status.
 *
 * @return The saved status if found, or NULL.
 */
OulSavedStatus *oul_savedstatus_find(const char *title);

/**
 * Finds a saved status with the specified creation time.
 *
 * @param creation_time The timestamp when the saved
 *        status was created.
 *
 * @return The saved status if found, or NULL.
 */
OulSavedStatus *oul_savedstatus_find_by_creation_time(time_t creation_time);

/**
 * Finds a saved status with the specified primitive and message.
 *
 * @param type The OulStatusPrimitive for the status you're trying
 *        to find.
 * @param message The message for the status you're trying
 *        to find.
 *
 * @return The saved status if found, or NULL.
 */
OulSavedStatus *oul_savedstatus_find_transient_by_type_and_message(OulStatusPrimitive type, const char *message);

/**
 * Determines if a given saved status is "transient."
 * A transient saved status is one that was not
 * explicitly added by the user.  Transient statuses
 * are automatically removed if they are not used
 * for a period of time.
 *
 * A transient saved statuses is automatically
 * created by the status box when the user sets himself
 * to one of the generic primitive statuses.  The reason
 * we need to save this status information is so we can
 * restore it when Beasy restarts.
 *
 * @param saved_status The saved status.
 *
 * @return TRUE if the saved status is transient.
 */
gboolean oul_savedstatus_is_transient(const OulSavedStatus *saved_status);

/**
 * Return the name of a given saved status.
 *
 * @param saved_status The saved status.
 *
 * @return The title.  This value may be a static buffer which may
 *         be overwritten on subsequent calls to this function.  If
 *         you need a reference to the title for prolonged use then
 *         you should make a copy of it.
 */
const char *oul_savedstatus_get_title(const OulSavedStatus *saved_status);

/**
 * Return the type of a given saved status.
 *
 * @param saved_status The saved status.
 *
 * @return The name.
 */
OulStatusPrimitive oul_savedstatus_get_type(const OulSavedStatus *saved_status);

/**
 * Return the default message of a given saved status.
 *
 * @param saved_status The saved status.
 *
 * @return The message.  This will return NULL if the saved
 *         status does not have a message.  This will
 *         contain the normal markup that is created by
 *         Beasy's IMHTML (basically HTML markup).
 */
const char *oul_savedstatus_get_message(const OulSavedStatus *saved_status);

/**
 * Return the time in seconds-since-the-epoch when this
 * saved status was created.  Note: For any status created
 * by Beasy 1.5.0 or older this value will be invalid and
 * very small (close to 0).  This is because Beasy 1.5.0
 * and older did not record the timestamp when the status
 * was created.
 *
 * However, this value is guaranteed to be a unique
 * identifier for the given saved status.
 *
 * @param saved_status The saved status.
 *
 * @return The timestamp when this saved status was created.
 */
time_t oul_savedstatus_get_creation_time(const OulSavedStatus *saved_status);

/**
 * Determine if a given saved status has "substatuses,"
 * or if it is a simple status (the same for all
 * accounts).
 *
 * @param saved_status The saved status.
 *
 * @return TRUE if the saved_status has substatuses.
 *         FALSE otherwise.
 */
gboolean oul_savedstatus_has_substatuses(const OulSavedStatus *saved_status);

OulSavedStatusSub*  oul_savedstatus_get_substatus(const OulSavedStatus *saved_status, const OulPlugin *plugin);

/**
 * Get the status type of a given substatus.
 *
 * @param substatus The substatus.
 *
 * @return The status type.
 */
const OulStatusType *oul_savedstatus_substatus_get_type(const OulSavedStatusSub *substatus);

/**
 * Get the message of a given substatus.
 *
 * @param substatus The substatus.
 *
 * @return The message of the substatus, or NULL if this substatus does
 *         not have a message.
 */
const char *oul_savedstatus_substatus_get_message(const OulSavedStatusSub *substatus);

/**
 * Sets the statuses for all your accounts to those specified
 * by the given saved_status.  This function calls
 * oul_savedstatus_activate_for_account() for all your accounts.
 *
 * @param saved_status The status you want to set your accounts to.
 */
void oul_savedstatus_activate(OulSavedStatus *saved_status);

/**
 * Get the handle for the status subsystem.
 *
 * @return the handle to the status subsystem
 */
void *oul_savedstatuses_get_handle(void);

/**
 * Initializes the status subsystem.
 */
void oul_savedstatuses_init(void);

/**
 * Uninitializes the status subsystem.
 */
void oul_savedstatuses_uninit(void);

void oul_savedstatus_activate_for_plugin(const OulSavedStatus *saved_status, OulPlugin *plugin);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _OUL_SAVEDSTATUSES_H_ */
