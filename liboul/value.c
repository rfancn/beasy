#include "internal.h"
#include "value.h"

void
oul_value_destroy(OulValue *value)
{
	g_return_if_fail(value != NULL);

	if (oul_value_get_type(value) == OUL_TYPE_BOXED)
	{
		g_free(value->u.specific_type);
	}
	else if (oul_value_get_type(value) == OUL_TYPE_STRING)
	{
		g_free(value->data.string_data);
	}

	g_free(value);
}

OulType
oul_value_get_type(const OulValue *value)
{
	g_return_val_if_fail(value != NULL, OUL_TYPE_UNKNOWN);

	return value->type;
}

OulValue *
oul_value_new(OulType type, ...)
{
	OulValue *value;
	va_list args;

	g_return_val_if_fail(type != OUL_TYPE_UNKNOWN, NULL);

	value = g_new0(OulValue, 1);

	value->type = type;

	va_start(args, type);

	if (type == OUL_TYPE_SUBTYPE)
		value->u.subtype = va_arg(args, int);
	else if (type == OUL_TYPE_BOXED)
		value->u.specific_type = g_strdup(va_arg(args, char *));

	va_end(args);

	return value;
}

OulValue *
oul_value_dup(const OulValue *value)
{
    OulValue *new_value;
    OulType type;

    g_return_val_if_fail(value != NULL, NULL);

    type = oul_value_get_type(value);

    if (type == OUL_TYPE_SUBTYPE)
    {
        new_value = oul_value_new(OUL_TYPE_SUBTYPE,
                                   oul_value_get_subtype(value));
    }
    else if (type == OUL_TYPE_BOXED)
    {
        new_value = oul_value_new(OUL_TYPE_BOXED,
                                   oul_value_get_specific_type(value));
    }
    else
        new_value = oul_value_new(type);

    new_value->flags = value->flags;

    switch (type)
    {
        case OUL_TYPE_CHAR:
            //oul_value_set_char(new_value, oul_value_get_char(value));
            break;

        case OUL_TYPE_UCHAR:
            //oul_value_set_uchar(new_value, oul_value_get_uchar(value));
            break;

        case OUL_TYPE_BOOLEAN:
            //oul_value_set_boolean(new_value, oul_value_get_boolean(value));
            break;

        case OUL_TYPE_SHORT:
            //oul_value_set_short(new_value, oul_value_get_short(value));
            break;

        case OUL_TYPE_USHORT:
            //oul_value_set_ushort(new_value, oul_value_get_ushort(value));
            break;

        case OUL_TYPE_INT:
            //oul_value_set_int(new_value, oul_value_get_int(value));
            break;

        case OUL_TYPE_UINT:
            //oul_value_set_uint(new_value, oul_value_get_uint(value));
            break;

        case OUL_TYPE_LONG:
            //oul_value_set_long(new_value, oul_value_get_long(value));
            break;

        case OUL_TYPE_ULONG:
            //oul_value_set_ulong(new_value, oul_value_get_ulong(value));
            break;

        case OUL_TYPE_INT64:
            //oul_value_set_int64(new_value, oul_value_get_int64(value));
            break;

        case OUL_TYPE_UINT64:
            //oul_value_set_uint64(new_value, oul_value_get_uint64(value));
            break;

        case OUL_TYPE_STRING:
            //oul_value_set_string(new_value, oul_value_get_string(value));
            break;

        case OUL_TYPE_OBJECT:
            //oul_value_set_object(new_value, oul_value_get_object(value));
            break;

        case OUL_TYPE_POINTER:
            //oul_value_set_pointer(new_value, oul_value_get_pointer(value));
            break;

        case OUL_TYPE_ENUM:
            //oul_value_set_enum(new_value, oul_value_get_enum(value));
            break;

        case OUL_TYPE_BOXED:
            //oul_value_set_boxed(new_value, oul_value_get_boxed(value));
            break;

        default:
            break;
    }

    return new_value;
}

const char *
oul_value_get_specific_type(const OulValue *value)
{
    g_return_val_if_fail(value != NULL, NULL);
    g_return_val_if_fail(oul_value_get_type(value) == OUL_TYPE_BOXED, NULL);

    return value->u.specific_type;
}

const char*
oul_value_get_string(const OulValue *value)
{
    g_return_val_if_fail(value != NULL, NULL);
    
    return value->data.string_data;
}

unsigned int
oul_value_get_subtype(const OulValue *value)
{
    g_return_val_if_fail(value != NULL, 0);
    g_return_val_if_fail(oul_value_get_type(value) == OUL_TYPE_SUBTYPE, 0);

    return value->u.subtype;
}

void
oul_value_set_boolean(OulValue *value, gboolean data)
{
    g_return_if_fail(value != NULL);

    value->data.boolean_data = data;
}
