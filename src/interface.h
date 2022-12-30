/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * interface.h  This file is part of Woofer GTK
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

#ifndef __INTERFACE__
#define __INTERFACE__

/* INCLUDES BEGIN */

#include <glib.h>
#include <gtk/gtk.h>
#include <woofer/song.h>

/* INCLUDES END */

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */
/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */
/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */

GtkWindow * interface_get_parent_window(void);
gboolean interface_window_is_present(void);
gboolean interface_is_active(void);
gboolean interface_is_visible(void);
void interface_set_use_csd(gboolean use_csd);

/* GETTER/SETTER PROTOTYPES END */

/* FUNCTION PROTOTYPES BEGIN */

gboolean interface_activate(GApplication *app);
void interface_startup(GApplication *app);
void interface_shutdown(GApplication *app);

void interface_tree_add_item(WfSong *song);

void interface_show_hide_columns(void);

void interface_volume_updated(gdouble volume);

void interface_update_status(const gchar *message);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */
/* DESTRUCTOR PROTOTYPES END */

#endif /* __INTERFACE__ */

/* END OF FILE */
