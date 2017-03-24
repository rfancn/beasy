#ifndef _OUL_STATUS_H_
#define _OUL_STATUS_H_

typedef struct _OulStatusType      OulStatusType;
typedef struct _OulStatusAttr      OulStatusAttr;
typedef struct _OulPresence        OulPresence;
typedef struct _OulStatus          OulStatus;

/**
 * A primitive defining the basic structure of a status type.
 */
/*
 * If you add a value to this enum, make sure you update
 * the status_primitive_map array in status.c and the special-cases for idle
 * and offline-messagable just below it.
 */
typedef enum
{
	OUL_STATUS_UNSET = 0,
	OUL_STATUS_OFFLINE,
	OUL_STATUS_ONLINE,
	OUL_STATUS_BUSY,
	OUL_STATUS_UNREAD_MSG,
	OUL_STATUS_NUM_PRIMITIVES
} OulStatusPrimitive;

#include "value.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/** @name OulStatusPrimitive API                                         */
/**************************************************************************/
/*@{*/

/**
 * Lookup the id of a primitive status type based on the type.  This
 * ID is a unique plain-text name of the status, without spaces.
 *
 * @param type A primitive status type.
 *
 * @return The unique ID for this type.
 */
const char *oul_primitive_get_id_from_type(OulStatusPrimitive type);

/**
 * Lookup the name of a primitive status type based on the type.  This
 * name is the plain-English name of the status type.  It is usually one
 * or two words.
 *
 * @param type A primitive status type.
 *
 * @return The name of this type, suitable for users to see.
 */
const char *oul_primitive_get_name_from_type(OulStatusPrimitive type);

/**
 * Lookup the value of a primitive status type based on the id.  The
 * ID is a unique plain-text name of the status, without spaces.
 *
 * @param id The unique ID of a primitive status type.
 *
 * @return The OulStatusPrimitive value.
 */
OulStatusPrimitive oul_primitive_get_type_from_id(const char *id);

/*@}*/

/**************************************************************************/
/** @name OulStatusType API                                              */
/**************************************************************************/
/*@{*/

/**
 * Creates a new status type.
 *
 * @param primitive     The primitive status type.
 * @param id            The ID of the status type, or @c NULL to use the id of
 *                      the primitive status type.
 * @param name          The name presented to the user, or @c NULL to use the
 *                      name of the primitive status type.
 * @param saveable      TRUE if the information set for this status by the
 *                      user can be saved for future sessions.
 * @param user_settable TRUE if this is a status the user can manually set.
 * @param independent   TRUE if this is an independent (non-exclusive)
 *                      status type.
 *
 * @return A new status type.
 */
OulStatusType *oul_status_type_new_full(OulStatusPrimitive primitive,
										  const char *id, const char *name,
										  gboolean saveable,
										  gboolean user_settable,
										  gboolean independent);

/**
 * Creates a new status type with some default values (not
 * savable and not independent).
 *
 * @param primitive     The primitive status type.
 * @param id            The ID of the status type, or @c NULL to use the id of
 *                      the primitive status type.
 * @param name          The name presented to the user, or @c NULL to use the
 *                      name of the primitive status type.
 * @param user_settable TRUE if this is a status the user can manually set.
 *
 * @return A new status type.
 */
OulStatusType *oul_status_type_new(OulStatusPrimitive primitive,
									 const char *id, const char *name,
									 gboolean user_settable);

/**
 * Creates a new status type with attributes.
 *
 * @param primitive     The primitive status type.
 * @param id            The ID of the status type, or @c NULL to use the id of
 *                      the primitive status type.
 * @param name          The name presented to the user, or @c NULL to use the
 *                      name of the primitive status type.
 * @param saveable      TRUE if the information set for this status by the
 *                      user can be saved for future sessions.
 * @param user_settable TRUE if this is a status the user can manually set.
 * @param independent   TRUE if this is an independent (non-exclusive)
 *                      status type.
 * @param attr_id       The ID of the first attribute.
 * @param attr_name     The name of the first attribute.
 * @param attr_value    The value type of the first attribute attribute.
 * @param ...           Additional attribute information.
 *
 * @return A new status type.
 */
OulStatusType *oul_status_type_new_with_attrs(OulStatusPrimitive primitive,
												const char *id,
												const char *name,
												gboolean saveable,
												gboolean user_settable,
												gboolean independent,
												const char *attr_id,
												const char *attr_name,
												OulValue *attr_value, ...) G_GNUC_NULL_TERMINATED;

/**
 * Destroys a status type.
 *
 * @param status_type The status type to destroy.
 */
void oul_status_type_destroy(OulStatusType *status_type);

/**
 * Sets a status type's primary attribute.
 *
 * The value for the primary attribute is used as the description for
 * the particular status type. An example is an away message. The message
 * would be the primary attribute.
 *
 * @param status_type The status type.
 * @param attr_id     The ID of the primary attribute.
 */
void oul_status_type_set_primary_attr(OulStatusType *status_type,
									   const char *attr_id);

/**
 * Adds an attribute to a status type.
 *
 * @param status_type The status type to add the attribute to.
 * @param id          The ID of the attribute.
 * @param name        The name presented to the user.
 * @param value       The value type of this attribute.
 */
void oul_status_type_add_attr(OulStatusType *status_type, const char *id,
							   const char *name, OulValue *value);

/**
 * Adds multiple attributes to a status type.
 *
 * @param status_type The status type to add the attribute to.
 * @param id          The ID of the first attribute.
 * @param name        The description of the first attribute.
 * @param value       The value type of the first attribute attribute.
 * @param ...         Additional attribute information.
 */
void oul_status_type_add_attrs(OulStatusType *status_type, const char *id,
								const char *name, OulValue *value, ...) G_GNUC_NULL_TERMINATED;

/**
 * Adds multiple attributes to a status type using a va_list.
 *
 * @param status_type The status type to add the attribute to.
 * @param args        The va_list of attributes.
 */
void oul_status_type_add_attrs_vargs(OulStatusType *status_type,
									  va_list args);

/**
 * Returns the primitive type of a status type.
 *
 * @param status_type The status type.
 *
 * @return The primitive type of the status type.
 */
OulStatusPrimitive oul_status_type_get_primitive(
	const OulStatusType *status_type);

/**
 * Returns the ID of a status type.
 *
 * @param status_type The status type.
 *
 * @return The ID of the status type.
 */
const char *oul_status_type_get_id(const OulStatusType *status_type);

/**
 * Returns the name of a status type.
 *
 * @param status_type The status type.
 *
 * @return The name of the status type.
 */
const char *oul_status_type_get_name(const OulStatusType *status_type);

/**
 * Returns whether or not the status type is saveable.
 *
 * @param status_type The status type.
 *
 * @return TRUE if user-defined statuses based off this type are saveable.
 *         FALSE otherwise.
 */
gboolean oul_status_type_is_saveable(const OulStatusType *status_type);

/**
 * Returns whether or not the status type can be set or modified by the
 * user.
 *
 * @param status_type The status type.
 *
 * @return TRUE if the status type can be set or modified by the user.
 *         FALSE if it's a protocol-set setting.
 */
gboolean oul_status_type_is_user_settable(const OulStatusType *status_type);

/**
 * Returns whether or not the status type is independent.
 *
 * Independent status types are non-exclusive. If other status types on
 * the same hierarchy level are set, this one will not be affected.
 *
 * @param status_type The status type.
 *
 * @return TRUE if the status type is independent, or FALSE otherwise.
 */
gboolean oul_status_type_is_independent(const OulStatusType *status_type);

/**
 * Returns whether the status type is exclusive.
 *
 * @param status_type The status type.
 *
 * @return TRUE if the status type is exclusive, FALSE otherwise.
 */
gboolean oul_status_type_is_exclusive(const OulStatusType *status_type);

/**
 * Returns whether or not a status type is available.
 *
 * Available status types are online and possibly invisible, but not away.
 *
 * @param status_type The status type.
 *
 * @return TRUE if the status is available, or FALSE otherwise.
 */
gboolean oul_status_type_is_available(const OulStatusType *status_type);

/**
 * Returns a status type's primary attribute ID.
 *
 * @param type The status type.
 *
 * @return The primary attribute's ID.
 */
const char *oul_status_type_get_primary_attr(const OulStatusType *type);

/**
 * Returns the attribute with the specified ID.
 *
 * @param status_type The status type containing the attribute.
 * @param id          The ID of the desired attribute.
 *
 * @return The attribute, if found. NULL otherwise.
 */
OulStatusAttr *oul_status_type_get_attr(const OulStatusType *status_type,
										  const char *id);

/**
 * Returns a list of all attributes in a status type.
 *
 * @param status_type The status type.
 *
 * @constreturn The list of attributes.
 */
GList *oul_status_type_get_attrs(const OulStatusType *status_type);

/**
 * Find the OulStatusType with the given id.
 *
 * @param status_types A list of status types.  Often account->status_types.
 * @param id The unique ID of the status type you wish to find.
 *
 * @return The status type with the given ID, or NULL if one could
 *         not be found.
 */
const OulStatusType *oul_status_type_find_with_id(GList *status_types,
													const char *id);

/*@}*/

/**************************************************************************/
/** @name OulStatusAttr API                                              */
/**************************************************************************/
/*@{*/

/**
 * Creates a new status attribute.
 *
 * @param id         The ID of the attribute.
 * @param name       The name presented to the user.
 * @param value_type The type of data contained in the attribute.
 *
 * @return A new status attribute.
 */
OulStatusAttr *oul_status_attr_new(const char *id, const char *name,
									 OulValue *value_type);

/**
 * Destroys a status attribute.
 *
 * @param attr The status attribute to destroy.
 */
void oul_status_attr_destroy(OulStatusAttr *attr);

/**
 * Returns the ID of a status attribute.
 *
 * @param attr The status attribute.
 *
 * @return The status attribute's ID.
 */
const char *oul_status_attr_get_id(const OulStatusAttr *attr);

/**
 * Returns the name of a status attribute.
 *
 * @param attr The status attribute.
 *
 * @return The status attribute's name.
 */
const char *oul_status_attr_get_name(const OulStatusAttr *attr);

/**
 * Returns the value of a status attribute.
 *
 * @param attr The status attribute.
 *
 * @return The status attribute's value.
 */
OulValue *oul_status_attr_get_value(const OulStatusAttr *attr);

/*@}*/

/**************************************************************************/
/** @name OulStatus API                                                  */
/**************************************************************************/
/*@{*/

/**
 * Creates a new status.
 *
 * @param status_type The type of status.
 * @param presence    The parent presence.
 *
 * @return The new status.
 */
OulStatus *oul_status_new(OulStatusType *status_type);
							

/**
 * Destroys a status.
 *
 * @param status The status to destroy.
 */
void oul_status_destroy(OulStatus *status);

/**
 * Sets whether or not a status is active.
 *
 * This should only be called by the account, conversation, and buddy APIs.
 *
 * @param status The status.
 * @param active The active state.
 */
void oul_status_set_active(OulStatus *status, gboolean active);

/**
 * Sets whether or not a status is active.
 *
 * This should only be called by the account, conversation, and buddy APIs.
 *
 * @param status The status.
 * @param active The active state.
 * @param args   A list of attributes to set on the status.  This list is
 *               composed of key/value pairs, where each key is a valid
 *               attribute name for this OulStatusType.  The list should
 *               be NULL terminated.
 */
void oul_status_set_active_with_attrs(OulStatus *status, gboolean active,
									   va_list args);

/**
 * Sets whether or not a status is active.
 *
 * This should only be called by the account, conversation, and buddy APIs.
 *
 * @param status The status.
 * @param active The active state.
 * @param attrs  A list of attributes to set on the status.  This list is
 *               composed of key/value pairs, where each key is a valid
 *               attribute name for this OulStatusType.  The list is
 *               not modified or freed by this function.
 */
void oul_status_set_active_with_attrs_list(OulStatus *status, gboolean active,
											GList *attrs);

/**
 * Sets the boolean value of an attribute in a status with the specified ID.
 *
 * @param status The status.
 * @param id     The attribute ID.
 * @param value  The boolean value.
 */
void oul_status_set_attr_boolean(OulStatus *status, const char *id,
								  gboolean value);

/**
 * Sets the integer value of an attribute in a status with the specified ID.
 *
 * @param status The status.
 * @param id     The attribute ID.
 * @param value  The integer value.
 */
void oul_status_set_attr_int(OulStatus *status, const char *id,
							  int value);

/**
 * Sets the string value of an attribute in a status with the specified ID.
 *
 * @param status The status.
 * @param id     The attribute ID.
 * @param value  The string value.
 */
void oul_status_set_attr_string(OulStatus *status, const char *id,
								 const char *value);

/**
 * Returns the status's type.
 *
 * @param status The status.
 *
 * @return The status's type.
 */
OulStatusType *oul_status_get_type(const OulStatus *status);

/**
 * Returns the status's type ID.
 *
 * This is a convenience method for
 * oul_status_type_get_id(oul_status_get_type(status)).
 *
 * @param status The status.
 *
 * @return The status's ID.
 */
const char *oul_status_get_id(const OulStatus *status);

/**
 * Returns the status's name.
 *
 * This is a convenience method for
 * oul_status_type_get_name(oul_status_get_type(status)).
 *
 * @param status The status.
 *
 * @return The status's name.
 */
const char *oul_status_get_name(const OulStatus *status);

/**
 * Returns whether or not a status is independent.
 *
 * This is a convenience method for
 * oul_status_type_is_independent(oul_status_get_type(status)).
 *
 * @param status The status.
 *
 * @return TRUE if the status is independent, or FALSE otherwise.
 */
gboolean oul_status_is_independent(const OulStatus *status);

/**
 * Returns whether or not a status is exclusive.
 *
 * This is a convenience method for
 * oul_status_type_is_exclusive(oul_status_get_type(status)).
 *
 * @param status The status.
 *
 * @return TRUE if the status is exclusive, FALSE otherwise.
 */
gboolean oul_status_is_exclusive(const OulStatus *status);

/**
 * Returns whether or not a status is available.
 *
 * Available statuses are online and possibly invisible, but not away or idle.
 *
 * This is a convenience method for
 * oul_status_type_is_available(oul_status_get_type(status)).
 *
 * @param status The status.
 *
 * @return TRUE if the status is available, or FALSE otherwise.
 */
gboolean oul_status_is_available(const OulStatus *status);

/**
 * Returns the active state of a status.
 *
 * @param status The status.
 *
 * @return The active state of the status.
 */
gboolean oul_status_is_active(const OulStatus *status);

/**
 * Returns whether or not a status is considered 'online'
 *
 * @param status The status.
 *
 * @return TRUE if the status is considered online, FALSE otherwise
 */
gboolean oul_status_is_online(const OulStatus *status);

/**
 * Returns the value of an attribute in a status with the specified ID.
 *
 * @param status The status.
 * @param id     The attribute ID.
 *
 * @return The value of the attribute.
 */
OulValue *oul_status_get_attr_value(const OulStatus *status,
									  const char *id);

/**
 * Returns the boolean value of an attribute in a status with the specified ID.
 *
 * @param status The status.
 * @param id     The attribute ID.
 *
 * @return The boolean value of the attribute.
 */
gboolean oul_status_get_attr_boolean(const OulStatus *status,
									  const char *id);

/**
 * Returns the integer value of an attribute in a status with the specified ID.
 *
 * @param status The status.
 * @param id     The attribute ID.
 *
 * @return The integer value of the attribute.
 */
int oul_status_get_attr_int(const OulStatus *status, const char *id);

/**
 * Returns the string value of an attribute in a status with the specified ID.
 *
 * @param status The status.
 * @param id     The attribute ID.
 *
 * @return The string value of the attribute.
 */
const char *oul_status_get_attr_string(const OulStatus *status,
										const char *id);

/**
 * Compares two statuses for availability.
 *
 * @param status1 The first status.
 * @param status2 The second status.
 *
 * @return -1 if @a status1 is more available than @a status2.
 *          0 if @a status1 is equal to @a status2.
 *          1 if @a status2 is more available than @a status1.
 */
gint oul_status_compare(const OulStatus *status1, const OulStatus *status2);

/*@}*/

/**************************************************************************/
/** @name Status subsystem                                                */
/**************************************************************************/
/*@{*/

/**
 * Get the handle for the status subsystem.
 *
 * @return the handle to the status subsystem
 */
void *oul_status_get_handle(void);

/**
 * Initializes the status subsystem.
 */
void oul_status_init(void);

/**
 * Uninitializes the status subsystem.
 */
void oul_status_uninit(void);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* _OUL_STATUS_H_ */
