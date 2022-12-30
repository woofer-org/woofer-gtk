/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * about.c  This file is part of Woofer GTK
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
 * This file extends the functionallity of interface.c; Only to be used by
 * interface modules.
 */

/* INCLUDES BEGIN */

// Library includes
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

// Woofer core includes
#include <woofer/authors.h>
#include <woofer/constants.h>

// Module includes
#include "about.h"

// Dependency includes
#include "config.h"

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This is the about window that shows some information about the application
 * and authors.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */

/*
 * Make clear what version we use (see:
 * https://www.gnu.org/licenses/identify-licenses-clearly.html
 * for more details)
 */
#ifndef GTK_LICENSE_GPL_3_0_OR_LATER
#  define GTK_LICENSE_GPL_3_0_OR_LATER GTK_LICENSE_GPL_3_0
#endif

/* DEFINES END */

/* CUSTOM TYPES BEGIN */

typedef struct _AboutDetails AboutDetails;

struct _AboutDetails
{
	gboolean constructed;
	GtkDialog *dialog;
};

/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static void about_dialog_construct(GtkWindow *parent_window);

static void about_dialog_destroy_cb(GtkWidget *object, gpointer user_data);

static void about_dialog_run(GtkDialog *dialog);

static void about_dialog_destruct();

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */

static AboutDetails AboutData =
{
	.constructed = FALSE,

	// All others are %NULL
};

/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

static void
about_dialog_construct(GtkWindow *parent_window)
{
	const gchar *win_title = "About " WF_NAME;

	GtkAboutDialog *dialog_about;
	GtkWindow *dialog_window;
	GtkWidget *dialog_widget;
	GtkWidget *header_bar;

	g_debug("Constructing about dialog...");

	g_warn_if_fail(GTK_IS_WINDOW(parent_window));

	// Create types
	dialog_widget = gtk_about_dialog_new();
	dialog_window = GTK_WINDOW(dialog_widget);
	dialog_about = GTK_ABOUT_DIALOG(dialog_widget);

	// Set window options
	gtk_window_set_transient_for(dialog_window, parent_window);
	gtk_window_set_destroy_with_parent(dialog_window, TRUE);
	gtk_window_set_modal(dialog_window, TRUE);
	g_signal_connect(dialog_window, "destroy", G_CALLBACK(about_dialog_destroy_cb), NULL /* user_data */);

	// Fill dialog with information
	gtk_about_dialog_set_program_name(dialog_about, WF_DISPLAY_NAME);
	gtk_about_dialog_set_version(dialog_about, "v" INTERFACE_VERSION " (API " WF_VERSION ")");
	gtk_about_dialog_set_copyright(dialog_about, WF_COPYRIGHT);
	gtk_about_dialog_set_comments(dialog_about, WF_DESCRIPTION);
	gtk_about_dialog_set_wrap_license(dialog_about, TRUE);
	gtk_about_dialog_set_license_type(dialog_about, GTK_LICENSE_GPL_3_0_OR_LATER);
	gtk_about_dialog_set_website(dialog_about, WF_WEBSITE);

	// Hack on the HeaderBar (if set)
	header_bar = gtk_window_get_titlebar(dialog_window);
	if (header_bar != NULL && GTK_IS_HEADER_BAR(header_bar))
	{
		// Using a real GtkHeaderBar
		gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), win_title);
	}
	else
	{
		// Just set (or override) the window title and hope it works
		gtk_window_set_title(dialog_window, win_title);
	}

	// Add information to container
	AboutData.constructed = TRUE;
	AboutData.dialog = GTK_DIALOG(dialog_about);
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */
/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */

static void
about_dialog_destroy_cb(GtkWidget *object, gpointer user_data)
{
	about_dialog_destruct();
}

/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

void
about_dialog_activate(GtkWindow *parent_window)
{
	if (!AboutData.constructed)
	{
		about_dialog_construct(parent_window);
	}

	about_dialog_run(AboutData.dialog);
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

static void
about_dialog_run(GtkDialog *dialog)
{
	GtkWidget *widget;

	g_return_if_fail(GTK_IS_DIALOG(dialog));

	// Run dialog
	widget = GTK_WIDGET(dialog);
	gtk_widget_show_all(widget);
	gtk_dialog_run(dialog);
	gtk_widget_hide(widget);
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

/*
 * Since gtk_window_set_destroy_with_parent() has been set during construction,
 * no widget destructors are needed when the application is about to quit.
 */

static void
about_dialog_destruct(void)
{
	// Reset all
	AboutData = (AboutDetails) { 0 };

	// Explicitly set @constructed to %FALSE
	AboutData.constructed = FALSE;
}

/* DESTRUCTORS END */

/* END OF FILE */
