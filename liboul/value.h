#ifndef _OUL_VALUE_H
#define _OUL_VALUE_H

/**
 * Specific value types.
 */
typedef enum
{
	OUL_TYPE_UNKNOWN = 0,  /**< Unknown type.                     */
	OUL_TYPE_SUBTYPE,      /**< Subtype.                          */
	OUL_TYPE_CHAR,         /**< Character.                        */
	OUL_TYPE_UCHAR,        /**< Unsigned character.               */
	OUL_TYPE_BOOLEAN,      /**< Boolean.                          */
	OUL_TYPE_SHORT,        /**< Short integer.                    */
	OUL_TYPE_USHORT,       /**< Unsigned short integer.           */
	OUL_TYPE_INT,          /**< Integer.                          */
	OUL_TYPE_UINT,         /**< Unsigned integer.                 */
	OUL_TYPE_LONG,         /**< Long integer.                     */
	OUL_TYPE_ULONG,        /**< Unsigned long integer.            */
	OUL_TYPE_INT64,        /**< 64-bit integer.                   */
	OUL_TYPE_UINT64,       /**< 64-bit unsigned integer.          */
	OUL_TYPE_STRING,       /**< String.                           */
	OUL_TYPE_OBJECT,       /**< Object pointer.                   */
	OUL_TYPE_POINTER,      /**< Generic pointer.                  */
	OUL_TYPE_ENUM,         /**< Enum.                             */
	OUL_TYPE_BOXED         /**< Boxed pointer with specific type. */

} OulType;


/**
 * Oul -specific subtype values.
 */
typedef enum
{
    OUL_SUBTYPE_UNKNOWN = 0,
    OUL_SUBTYPE_PLUGIN,
    OUL_SUBTYPE_CIPHER,
    OUL_SUBTYPE_STATUS,
    OUL_SUBTYPE_LOG,
    OUL_SUBTYPE_XFER,
    OUL_SUBTYPE_SAVEDSTATUS,
    OUL_SUBTYPE_XMLNODE,
    OUL_SUBTYPE_STORED_IMAGE
} OulSubType;


/**
 * A wrapper for a type, subtype, and specific type of value.
 */
typedef struct
{
	OulType type;
	unsigned short flags;

	union
	{
		char char_data;
		unsigned char uchar_data;
		gboolean boolean_data;
		short short_data;
		unsigned short ushort_data;
		int int_data;
		unsigned int uint_data;
		long long_data;
		unsigned long ulong_data;
		gint64 int64_data;
		guint64 uint64_data;
		char *string_data;
		void *object_data;
		void *pointer_data;
		int enum_data;
		void *boxed_data;

	} data;

	union
	{
		unsigned int subtype;
		char *specific_type;

	} u;

} OulValue;

void            oul_value_destroy(OulValue *value);
OulType         oul_value_get_type(const OulValue *value);
OulValue*       oul_value_new(OulType type, ...); 
OulValue*       oul_value_dup(const OulValue *value);
const char*     oul_value_get_string(const OulValue *value);
const char*     oul_value_get_specific_type(const OulValue *value);

unsigned int    oul_value_get_subtype(const OulValue *value);
void            oul_value_set_boolean(OulValue *value, gboolean data);


#endif

