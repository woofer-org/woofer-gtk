/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * settings.h  This file is part of Woofer GTK
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

#ifndef __SETTINGS__
#define __SETTINGS__

/* INCLUDES BEGIN */

#include <glib.h>

/* INCLUDES END */

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */

typedef enum _NotificationSetting NotificationSetting;

enum _NotificationSetting
{
	NOTIFICATIONS_UNDEFINED,
	NOTIFICATIONS_NEVER,
	NOTIFICATIONS_HIDDEN_ONLY,
	NOTIFICATIONS_UNFOCUSED_ONLY,
	NOTIFICATIONS_ALWAYS,
	NOTIFICATIONS_DEFINED // Validation checker
};

/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */

void interface_settings_init(void);

/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */
/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

NotificationSetting interface_settings_get_notification(void);
void interface_settings_set_notification(NotificationSetting notifications);

gboolean interface_settings_get_last_played_timestamp(void);
void interface_settings_set_last_played_timestamp(gboolean last_played_timestamp);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */

const gchar * interface_settings_get_notifications_pretty_str(NotificationSetting notifications);

/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */
/* DESTRUCTOR PROTOTYPES END */

#endif /* __SETTINGS__ */

/* END OF FILE */
