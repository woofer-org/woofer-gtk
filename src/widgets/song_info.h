/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * widgets/song_info.h  This file is part of Woofer GTK
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

#ifndef __WIDGETS_SONG_INFO__
#define __WIDGETS_SONG_INFO__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#define WIDGET_TYPE_SONG_INFO (widget_song_info_get_type())
#define WIDGET_SONG_INFO(ptr) G_TYPE_CHECK_INSTANCE_CAST(ptr, widget_song_info_get_type(), WidgetSongInfo)
#define WIDGET_IS_SONG_INFO(ptr) G_TYPE_CHECK_INSTANCE_TYPE(ptr, widget_song_info_get_type())

typedef struct _WidgetSongInfo WidgetSongInfo;
typedef struct _WidgetSongInfoPrivate WidgetSongInfoPrivate;

struct _WidgetSongInfo
{
	/*< public >*/
	GtkBin parent_instance;

	/*< private >*/
	WidgetSongInfoPrivate *priv;
};

GType widget_song_info_get_type(void);
GtkWidget * widget_song_info_new(const gchar *name);

void widget_song_info_set_name(WidgetSongInfo *widget, const gchar *name);
void widget_song_info_set_title(WidgetSongInfo *widget, const gchar *title);
void widget_song_info_set_artist(WidgetSongInfo *widget, const gchar *artist);
void widget_song_info_set_album(WidgetSongInfo *widget, const gchar *album);

#endif /* __WIDGETS_SONG_INFO__ */

/* END OF FILE */
