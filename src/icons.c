/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * icons.c  This file is part of Woofer GTK
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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

// Woofer core includes
/*< none >*/

// Module includes
#include "icons.h"

// Dependency includes
/*< none >*/

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This provides some functions to loopup graphical icons to use in the
 * application windows.  Both themed icons (fetched from the system files) as
 * well as static images pre-compiled into the executables are supported.
 *
 * Since this module only contains utilities for other modules, all of
 * these "utilities" are part of the normal module functions and
 * constructors, destructors, etc are left out.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */
/* DEFINES END */

/* CUSTOM TYPES BEGIN */
/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */
/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */
/* GLOBAL VARIABLES END */

/* MODULE FUNCTIONS BEGIN */

// Unref returned value
GdkPixbuf *
icons_get_themed_image(const gchar *icon_name)
{
	GError *error = NULL;
	GtkIconTheme *icon_theme;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail(icon_name != NULL, NULL);

	icon_theme = gtk_icon_theme_get_default();
	pixbuf = gtk_icon_theme_load_icon(icon_theme, icon_name, 16, 0, &error);

	if (error != NULL)
	{
		g_warning("Couldn’t load icon: %s", error->message);
		g_error_free (error);

		return NULL;
	}
	else
	{
		return pixbuf;
	}
}

// Unref returned value
GdkPixbuf *
icons_get_static_image(const gchar *resource_path)
{
	GdkPixbuf *image;
	GError *err = NULL;

	g_return_val_if_fail(resource_path != NULL, NULL);

	image = gdk_pixbuf_new_from_resource(resource_path, &err);

	if (err != NULL)
	{
		g_warning("Could not get resource image %s: %s", resource_path, err->message);
		g_error_free(err);

		if (image != NULL)
		{
			g_free(image);
			image = NULL;
		}
	}

	return image;
}

/* MODULE FUNCTIONS END */

/* END OF FILE */
