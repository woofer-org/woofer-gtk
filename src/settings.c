/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * settings.c  This file is part of Woofer GTK
 * Copyright (C) 2022  Quico Augustijn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed "as is" in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  If your
 * computer no longer boots, divides by 0 or explodes, you are the only
 * one responsible.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <https://www.gnu.org/licenses/gpl-3.0.html>.
 */

/* INCLUDES BEGIN */

// Library includes
#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

// Woofer core includes
#include <woofer/constants.h>
#include <woofer/settings.h>

// Module includes
#include "settings.h"

// Dependency includes
/*< none >*/

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This provides the settings layer between the interface code and the settings
 * mechanism of the back-end.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */

// Notification setting values
#define NOTIFICATIONS_STR_NEVER "Never"
#define NOTIFICATIONS_STR_HIDDEN_ONLY "HiddenOnly"
#define NOTIFICATIONS_STR_UNFOCUSED_ONLY "UnfocusedOnly"
#define NOTIFICATIONS_STR_ALWAYS "Always"
#define NOTIFICATIONS_DEFAULT (NOTIFICATIONS_NEVER)

/* DEFINES END */

/* CUSTOM TYPES BEGIN */

typedef struct _InterfaceSettings InterfaceSettings;

struct _InterfaceSettings
{
	guint32 setting_notifications;
	guint32 setting_last_played_timestamp;
};

/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static const gchar * interface_settings_get_notifications_str(NotificationSetting notifications);
static NotificationSetting interface_settings_get_notifications_enum(const gchar *str);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */

static InterfaceSettings InterfaceSettingsData = { 0 };

/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

void
interface_settings_init(void)
{
	const gchar *default_value;
	guint32 id;

	default_value = interface_settings_get_notifications_str(NOTIFICATIONS_HIDDEN_ONLY);

	id = wf_settings_dynamic_register_str("Notifications", NULL /* group */, default_value);
	InterfaceSettingsData.setting_notifications = id;

	id = wf_settings_dynamic_register_bool("LastPlayedTimestamp", NULL /* group */, FALSE);
	InterfaceSettingsData.setting_last_played_timestamp = id;
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */
/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */
/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

NotificationSetting
interface_settings_get_notification(void)
{
	const gchar *value = wf_settings_dynamic_get_str_by_id(InterfaceSettingsData.setting_notifications);

	return interface_settings_get_notifications_enum(value);
}

void
interface_settings_set_notification(NotificationSetting notifications)
{
	const gchar *value = interface_settings_get_notifications_str(notifications);

	g_return_if_fail(value != NULL);

	wf_settings_dynamic_set_str_by_id(InterfaceSettingsData.setting_notifications, value);
}

gboolean
interface_settings_get_last_played_timestamp(void)
{
	return wf_settings_dynamic_get_bool_by_id(InterfaceSettingsData.setting_last_played_timestamp);
}

void
interface_settings_set_last_played_timestamp(gboolean last_played_timestamp)
{
	wf_settings_dynamic_set_bool_by_id(InterfaceSettingsData.setting_last_played_timestamp, last_played_timestamp);
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

const gchar *
interface_settings_get_notifications_pretty_str(NotificationSetting notifications)
{
	switch (notifications)
	{
		case NOTIFICATIONS_UNDEFINED:
		case NOTIFICATIONS_DEFINED:
			g_message("Invalid notification setting");
			break;
		case NOTIFICATIONS_NEVER:
			return "Never";
		case NOTIFICATIONS_HIDDEN_ONLY:
			return "Hidden only";
		case NOTIFICATIONS_UNFOCUSED_ONLY:
			return "Unfocused only";
		case NOTIFICATIONS_ALWAYS:
			return "Always";
	}

	return NULL;
}

static const gchar *
interface_settings_get_notifications_str(NotificationSetting notifications)
{
	switch (notifications)
	{
		case NOTIFICATIONS_UNDEFINED:
		case NOTIFICATIONS_DEFINED:
			g_message("Invalid notification setting");
			break;
		case NOTIFICATIONS_NEVER:
			return NOTIFICATIONS_STR_NEVER;
		case NOTIFICATIONS_HIDDEN_ONLY:
			return NOTIFICATIONS_STR_HIDDEN_ONLY;
		case NOTIFICATIONS_UNFOCUSED_ONLY:
			return NOTIFICATIONS_STR_UNFOCUSED_ONLY;
		case NOTIFICATIONS_ALWAYS:
			return NOTIFICATIONS_STR_ALWAYS;
	}

	return NULL;
}

static NotificationSetting
interface_settings_get_notifications_enum(const gchar *str)
{
	if (g_strcmp0(NOTIFICATIONS_STR_NEVER, str) == 0)
	{
		return NOTIFICATIONS_NEVER;
	}
	if (g_strcmp0(NOTIFICATIONS_STR_HIDDEN_ONLY, str) == 0)
	{
		return NOTIFICATIONS_HIDDEN_ONLY;
	}
	if (g_strcmp0(NOTIFICATIONS_STR_UNFOCUSED_ONLY, str) == 0)
	{
		return NOTIFICATIONS_UNFOCUSED_ONLY;
	}
	if (g_strcmp0(NOTIFICATIONS_STR_ALWAYS, str) == 0)
	{
		return NOTIFICATIONS_ALWAYS;
	}

	return NOTIFICATIONS_DEFAULT;
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

void
interface_settings_finalize(void)
{
	InterfaceSettingsData = (InterfaceSettings) { 0 };
}

/* DESTRUCTORS END */

/* END OF FILE */
