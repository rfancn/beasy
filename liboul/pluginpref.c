#include "internal.h"

#include "pluginpref.h"

#include "debug.h"
#include "prefs.h"

struct _OulPluginPrefFrame
{
	GList *prefs;
};

struct _OulPluginPref
{
	char *name;
	char *label;

	OulPluginPrefType type;

	int min;
	int max;
	GList *choices;
	unsigned int max_length;
	gboolean masked;
	OulStringFormatType format;
};

OulPluginPrefFrame *
oul_plugin_pref_frame_new()
{
	OulPluginPrefFrame *frame;

	frame = g_new0(OulPluginPrefFrame, 1);

	return frame;
}

void
oul_plugin_pref_frame_destroy(OulPluginPrefFrame *frame)
{
	g_return_if_fail(frame != NULL);

	g_list_foreach(frame->prefs, (GFunc)oul_plugin_pref_destroy, NULL);
	g_list_free(frame->prefs);
	g_free(frame);
}

void
oul_plugin_pref_frame_add(OulPluginPrefFrame *frame, OulPluginPref *pref)
{
	g_return_if_fail(frame != NULL);
	g_return_if_fail(pref  != NULL);

	frame->prefs = g_list_append(frame->prefs, pref);
}

GList *
oul_plugin_pref_frame_get_prefs(OulPluginPrefFrame *frame)
{
	g_return_val_if_fail(frame        != NULL, NULL);
	g_return_val_if_fail(frame->prefs != NULL, NULL);

	return frame->prefs;
}

OulPluginPref *
oul_plugin_pref_new()
{
	OulPluginPref *pref;

	pref = g_new0(OulPluginPref, 1);

	return pref;
}

OulPluginPref *
oul_plugin_pref_new_with_name(const char *name)
{
	OulPluginPref *pref;

	g_return_val_if_fail(name != NULL, NULL);

	pref = g_new0(OulPluginPref, 1);
	pref->name = g_strdup(name);

	return pref;
}

OulPluginPref *
oul_plugin_pref_new_with_label(const char *label)
{
	OulPluginPref *pref;

	g_return_val_if_fail(label != NULL, NULL);

	pref = g_new0(OulPluginPref, 1);
	pref->label = g_strdup(label);

	return pref;
}

OulPluginPref *
oul_plugin_pref_new_with_name_and_label(const char *name, const char *label)
{
	OulPluginPref *pref;

	g_return_val_if_fail(name  != NULL, NULL);
	g_return_val_if_fail(label != NULL, NULL);

	pref = g_new0(OulPluginPref, 1);
	pref->name = g_strdup(name);
	pref->label = g_strdup(label);

	return pref;
}

void
oul_plugin_pref_destroy(OulPluginPref *pref)
{
	g_return_if_fail(pref != NULL);

	g_free(pref->name);
	g_free(pref->label);
	g_list_free(pref->choices);
	g_free(pref);
}

void
oul_plugin_pref_set_name(OulPluginPref *pref, const char *name)
{
	g_return_if_fail(pref != NULL);
	g_return_if_fail(name != NULL);

	g_free(pref->name);
	pref->name = g_strdup(name);
}

const char *
oul_plugin_pref_get_name(OulPluginPref *pref)
{
	g_return_val_if_fail(pref != NULL, NULL);

	return pref->name;
}

void
oul_plugin_pref_set_label(OulPluginPref *pref, const char *label)
{
	g_return_if_fail(pref  != NULL);
	g_return_if_fail(label != NULL);

	g_free(pref->label);
	pref->label = g_strdup(label);
}

const char *
oul_plugin_pref_get_label(OulPluginPref *pref)
{
	g_return_val_if_fail(pref != NULL, NULL);

	return pref->label;
}

void
oul_plugin_pref_set_bounds(OulPluginPref *pref, int min, int max)
{
	int tmp;

	g_return_if_fail(pref       != NULL);
	g_return_if_fail(pref->name != NULL);

	if (oul_prefs_get_type(pref->name) != OUL_PREF_INT)
	{
		oul_debug_info("pluginpref",
				"oul_plugin_pref_set_bounds: %s is not an integer pref\n",
				pref->name);
		return;
	}

	if (min > max)
	{
		tmp = min;
		min = max;
		max = tmp;
	}

	pref->min = min;
	pref->max = max;
}

void oul_plugin_pref_get_bounds(OulPluginPref *pref, int *min, int *max)
{
	g_return_if_fail(pref       != NULL);
	g_return_if_fail(pref->name != NULL);

	if (oul_prefs_get_type(pref->name) != OUL_PREF_INT)
	{
		oul_debug(OUL_DEBUG_INFO, "pluginpref",
				"oul_plugin_pref_get_bounds: %s is not an integer pref\n",
				pref->name);
		return;
	}

	*min = pref->min;
	*max = pref->max;
}

void
oul_plugin_pref_set_type(OulPluginPref *pref, OulPluginPrefType type)
{
	g_return_if_fail(pref != NULL);

	pref->type = type;
}

OulPluginPrefType
oul_plugin_pref_get_type(OulPluginPref *pref)
{
	g_return_val_if_fail(pref != NULL, OUL_PLUGIN_PREF_NONE);

	return pref->type;
}

void
oul_plugin_pref_add_choice(OulPluginPref *pref, const char *label, gpointer choice)
{
	g_return_if_fail(pref  != NULL);
	g_return_if_fail(label != NULL);
	g_return_if_fail(choice || oul_prefs_get_type(pref->name) == OUL_PREF_INT);

	pref->choices = g_list_append(pref->choices, (gpointer)label);
	pref->choices = g_list_append(pref->choices, choice);
}

GList *
oul_plugin_pref_get_choices(OulPluginPref *pref)
{
	g_return_val_if_fail(pref != NULL, NULL);

	return pref->choices;
}

void
oul_plugin_pref_set_max_length(OulPluginPref *pref, unsigned int max_length)
{
	g_return_if_fail(pref != NULL);

	pref->max_length = max_length;
}

unsigned int
oul_plugin_pref_get_max_length(OulPluginPref *pref)
{
	g_return_val_if_fail(pref != NULL, 0);

	return pref->max_length;
}

void
oul_plugin_pref_set_masked(OulPluginPref *pref, gboolean masked)
{
	g_return_if_fail(pref != NULL);

	pref->masked = masked;
}

gboolean
oul_plugin_pref_get_masked(OulPluginPref *pref)
{
	g_return_val_if_fail(pref != NULL, FALSE);

	return pref->masked;
}

void
oul_plugin_pref_set_format_type(OulPluginPref *pref, OulStringFormatType format)
{
	g_return_if_fail(pref != NULL);
	g_return_if_fail(pref->type == OUL_PLUGIN_PREF_STRING_FORMAT);

	pref->format = format;
}

OulStringFormatType
oul_plugin_pref_get_format_type(OulPluginPref *pref)
{
	g_return_val_if_fail(pref != NULL, 0);

	if (pref->type != OUL_PLUGIN_PREF_STRING_FORMAT)
		return OUL_STRING_FORMAT_TYPE_NONE;

	return pref->format;
}

