/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * question_dialog.h  This file is part of Woofer GTK
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

#ifndef __QUESTION_DIALOG__
#define __QUESTION_DIALOG__

/* INCLUDES BEGIN */

#include <glib.h>
#include <gtk/gtk.h>

/* INCLUDES END */

/* DEFINES BEGIN */
/* DEFINES END */

/* MODULE TYPES BEGIN */
/* MODULE TYPES END */

/* CONSTRUCTOR PROTOTYPES BEGIN */
/* CONSTRUCTOR PROTOTYPES END */

/* GETTER/SETTER PROTOTYPES BEGIN */

void interface_question_dialog_set_parent(GtkWindow *parent);

/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */
/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

gboolean interface_question_dialog_run(const gchar *message);

/* FUNCTION PROTOTYPES END */

/* UTILITY PROTOTYPES BEGIN */
/* UTILITY PROTOTYPES END */

/* DESTRUCTOR PROTOTYPES BEGIN */
/* DESTRUCTOR PROTOTYPES END */

#endif /* __QUESTION_DIALOG__ */

/* END OF FILE */
