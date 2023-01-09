/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * main.c  This file is part of Woofer GTK GTK
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

/* INCLUDES BEGIN */

// Library includes
#include <glib.h>
#include <gio/gio.h>

// Woofer core includes
#include <woofer/app.h>

// Dependency includes
#include "interface.h"

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This provides the starting point of the application.  It creates the back-end
 * application object, sets the command-line options, connects to signals and
 * runs the main application.  The run call returns the exit code after the
 * complete application has finalized.
 *
 * Please note that the module main, unlike the rest of the application, does
 * not have a matching header file.
 */

/* DESCRIPTION END */

/* GLOBAL VARIABLES BEGIN */

static gboolean NoCsd = FALSE;

static const GOptionEntry Options[] =
{
	// Interface options
	{
		"no-csd", '\0', G_OPTION_FLAG_NONE,
		G_OPTION_ARG_NONE, &NoCsd,
		"Do not use client-side decoration",
		NULL
	},

	// Terminator
	{ NULL }
};

/* GLOBAL VARIABLES END */

/* CALLBACK FUNCTIONS BEGIN */

static void
startup(GApplication *app, gpointer user_data)
{
	interface_startup(app);
}

static void
activate(GApplication *app, gpointer user_data)
{
	// This option should have been set by option parsing in #GApplication
	interface_set_use_csd(!NoCsd);

	// Now activate
	interface_activate(app);
}

static void
shutdown(GApplication *app, gpointer user_data)
{
	interface_shutdown(app);
}

/* CALLBACK FUNCTIONS END */

/* ENTRY FUNCTION BEGIN */

// Starting point of everything
int
main(int argc, char *argv[])
{
	WfApp *wf_app;
	GApplication *g_app;
	int status;

	wf_app = (WfApp *) g_object_new(WF_TYPE_APP, NULL);
	g_app = G_APPLICATION(wf_app);

	wf_app_set_desktop_entry("woofer-gtk");

	g_application_add_option_group(g_app, gtk_get_option_group(TRUE));
	g_application_add_main_option_entries(g_app, Options);

	g_signal_connect(g_app, "startup", G_CALLBACK(startup), NULL /* user_data */);
	g_signal_connect(g_app, "activate", G_CALLBACK(activate), NULL /* user_data */);
	g_signal_connect(g_app, "shutdown", G_CALLBACK(shutdown), NULL /* user_data */);

	status = g_application_run(g_app, (gint) argc, (gchar **) argv);

	g_object_unref(wf_app);

	return status;
}

/* ENTRY FUNCTION END */

/* END OF FILE */
