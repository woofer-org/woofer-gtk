/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * question_dialog.c  This file is part of Woofer GTK
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

/*
 * This file extends the functionallity of interface.c; Intented to be used
 * by non-interface modules.
 */

/* INCLUDES BEGIN */

// Library includes
#include <glib.h>
#include <gtk/gtk.h>

// Woofer core includes
/*< none >*/

// Module includes
#include "question_dialog.h"

// Dependency includes
/*< none >*/

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This provides a simple and flexible dialog with a question for the user, to
 * which he can respond with either yes or no.
 * TODO TODO TODO
 * Question dialog is an interface extension intended to be used by
 * non-interface modules to ask the user (via the graphical interface) if a
 * particular task should really be performed.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */
/* DEFINES END */

/* CUSTOM TYPES BEGIN */
/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static gboolean interface_question_dialog_new(GtkWindow *parent_window, const gchar *msg);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */

static GtkWindow *ParentWindow = NULL;

/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

static gboolean
interface_question_dialog_new(GtkWindow *parent_window, const gchar *msg)
{
	GtkDialog *dialog;
	GtkWindow *dialog_window;
	GtkWidget *dialog_widget, *content, *box, *icon, *label;
	gint value;

	dialog_widget = gtk_dialog_new();
	dialog_window = GTK_WINDOW(dialog_widget);
	dialog = GTK_DIALOG(dialog_widget);

	gtk_window_set_title(dialog_window, "Question");
	gtk_window_set_transient_for(dialog_window, parent_window);
	gtk_window_set_modal(dialog_window, TRUE);
	gtk_window_set_destroy_with_parent(dialog_window, TRUE);
	gtk_window_set_resizable(dialog_window, FALSE);

	gtk_dialog_add_button(dialog, "_Yes", GTK_RESPONSE_YES);
	gtk_dialog_add_button(dialog, "_No", GTK_RESPONSE_NO);

	// Set dialog properties
	content = gtk_dialog_get_content_area(dialog);
	gtk_container_set_border_width(GTK_CONTAINER(content), 12);
	gtk_box_set_spacing(GTK_BOX(content), 18);
	gtk_window_set_modal(dialog_window, TRUE);
	gtk_dialog_set_default_response(dialog, GTK_RESPONSE_NO);

	// Create a container for the icon and the text
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(content), box, FALSE, TRUE, 0);

	// Create the icon
	icon = gtk_image_new_from_icon_name("dialog-question-symbolic", GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(box), icon, FALSE, TRUE, 8);

	// Create a label and add it to the container
	label = gtk_label_new(msg);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 4);

	// Show and run
	gtk_widget_show_all(box);
	value = gtk_dialog_run(dialog);

	gtk_widget_destroy(dialog_widget);

	return (value == GTK_RESPONSE_YES);
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */

void
interface_question_dialog_set_parent(GtkWindow *parent)
{
	g_return_if_fail(GTK_IS_WINDOW(parent));

	ParentWindow = parent;
}

/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */
/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

gboolean
interface_question_dialog_run(const gchar *message)
{
	g_return_val_if_fail(message != NULL, FALSE);

	if (ParentWindow == NULL)
	{
		g_warning("No parent window present during construction of the question dialog");
	}

	return interface_question_dialog_new(ParentWindow, message);
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */
/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */
/* DESTRUCTORS END */

/* END OF FILE */
