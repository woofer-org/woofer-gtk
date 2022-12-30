/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * preferences.h  This file is part of Woofer GTK
 * Copyright (C) 2021, 2022  Quico Augustijn
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

#ifndef __PREFERENCES__
#define __PREFERENCES__

/* INCLUDES BEGIN */

#include <gtk/gtk.h>

/* INCLUDES END */

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */

typedef void (*func_report_close) (const gchar *msg);

/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */
/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */

gboolean preference_dialog_is_visible(void);

void preference_dialog_connect_close(func_report_close cb_func);

/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

void preference_dialog_activate(GtkWindow *parent_window);
void preference_dialog_hide(void);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */
/* DESTRUCTOR PROTOTYPES END */

#endif /* __PREFERENCES__ */

/* END OF FILE */
