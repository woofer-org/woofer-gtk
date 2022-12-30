/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * widgets/song_info.c  This file is part of Woofer GTK
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

/*
 * This module implements a GObject type derived from GtkBin.
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "widgets/song_info.h"

#define WIDGET_SONG_INFO_NAME "WidgetSongInfo"

// Per widget private structure
struct _WidgetSongInfoPrivate
{
	const gchar *title_text;
	const gchar *artist_text;
	const gchar *album_text;

	GtkBox *main_box;
	GtkBox *title_box;
	GtkBox *artist_box;
	GtkBox *album_box;
	GtkLabel *name;
	GtkLabel *title;
	GtkLabel *artist;
	GtkLabel *artist_prefix;
	GtkLabel *album;
	GtkLabel *album_prefix;
};

static void widget_song_info_init(GTypeInstance *instance, gpointer g_class);

// Widget's type information
static const GTypeInfo SongInfoType =
{
	.class_size = sizeof(GtkBinClass),
	.instance_size = sizeof(WidgetSongInfo),
	.instance_init = widget_song_info_init,
};

// Get widget's registered type
GType
widget_song_info_get_type(void)
{
	const gchar *name;
	static GType type;
	GType new_type;

	if (g_once_init_enter(&type))
	{
		name = g_intern_static_string(WIDGET_SONG_INFO_NAME);

		new_type = g_type_register_static(GTK_TYPE_BIN, name, &SongInfoType, (GTypeFlags) 0);

		g_once_init_leave(&type, new_type);
	}

	return type;
}

GtkWidget *
widget_song_info_new(const gchar *name)
{
	WidgetSongInfo *widget;

	widget = WIDGET_SONG_INFO(g_object_new(WIDGET_TYPE_SONG_INFO, NULL));

	widget_song_info_set_name(widget, name);

	return GTK_WIDGET(widget);
}

// Init widget instance
static void
widget_song_info_init(GTypeInstance *instance, gpointer g_class)
{
	WidgetSongInfo *widget = WIDGET_SONG_INFO(instance);
	WidgetSongInfoPrivate *priv;

	// Allocate private structure
	widget->priv = priv = g_slice_alloc0(sizeof(WidgetSongInfoPrivate));

	// Create widgets
	priv->main_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
	priv->title_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
	priv->artist_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
	priv->album_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
	priv->name = GTK_LABEL(gtk_label_new(NULL));
	priv->title = GTK_LABEL(gtk_label_new(NULL));
	priv->artist = GTK_LABEL(gtk_label_new(NULL));
	priv->album = GTK_LABEL(gtk_label_new(NULL));
	gtk_label_set_ellipsize(priv->title, PANGO_ELLIPSIZE_END);
	gtk_label_set_ellipsize(priv->artist, PANGO_ELLIPSIZE_END);
	gtk_label_set_ellipsize(priv->album, PANGO_ELLIPSIZE_END);

	// Add static labels
	priv->artist_prefix = GTK_LABEL(gtk_label_new(NULL));
	gtk_label_set_use_markup(priv->artist_prefix, TRUE);
	gtk_widget_set_halign(GTK_WIDGET(priv->artist_prefix), GTK_ALIGN_END);
	gtk_box_pack_start(priv->artist_box, GTK_WIDGET(priv->artist_prefix), TRUE, TRUE, 0);
	priv->album_prefix = GTK_LABEL(gtk_label_new(NULL));
	gtk_label_set_use_markup(priv->album_prefix, TRUE);
	gtk_widget_set_halign(GTK_WIDGET(priv->album_prefix), GTK_ALIGN_END);
	gtk_box_pack_start(priv->album_box, GTK_WIDGET(priv->album_prefix), TRUE, TRUE, 0);

	// Pack widgets
	gtk_container_add(GTK_CONTAINER(widget), GTK_WIDGET(priv->main_box));
	gtk_box_pack_start(priv->main_box, GTK_WIDGET(priv->name), FALSE, TRUE, 0);
	gtk_box_pack_start(priv->main_box, GTK_WIDGET(priv->title_box), FALSE, TRUE, 0);
	gtk_box_pack_start(priv->main_box, GTK_WIDGET(priv->artist_box), FALSE, TRUE, 0);
	gtk_box_pack_start(priv->main_box, GTK_WIDGET(priv->album_box), FALSE, TRUE, 0);
	gtk_box_set_center_widget(priv->title_box, GTK_WIDGET(priv->title));
	gtk_box_set_center_widget(priv->artist_box, GTK_WIDGET(priv->artist));
	gtk_box_set_center_widget(priv->album_box, GTK_WIDGET(priv->album));
}

void
widget_song_info_set_name(WidgetSongInfo *widget, const gchar *name)
{
	gchar *text;

	g_return_if_fail(WIDGET_IS_SONG_INFO(widget));

	gtk_label_set_label(widget->priv->name, name);

	if (name == NULL)
	{
		gtk_label_set_markup(widget->priv->name, "<i></i>");
	}
	else
	{
		text = g_markup_printf_escaped("<i>%s</i>", name);

		gtk_label_set_markup(widget->priv->name, text);

		g_free(text);
	}
}

void
widget_song_info_set_title(WidgetSongInfo *widget, const gchar *title)
{
	gchar *text;

	g_return_if_fail(WIDGET_IS_SONG_INFO(widget));

	widget->priv->title_text = title;

	if (title == NULL)
	{
		gtk_label_set_markup(widget->priv->title, "<b></b>");
	}
	else
	{
		text = g_markup_printf_escaped("<b>%s</b>", title);

		gtk_label_set_markup(widget->priv->title, text);

		g_free(text);
	}
}

void
widget_song_info_set_artist(WidgetSongInfo *widget, const gchar *artist)
{
	g_return_if_fail(WIDGET_IS_SONG_INFO(widget));

	widget->priv->artist_text = artist;

	gtk_label_set_label(widget->priv->artist, artist);

	if (artist == NULL)
	{
		gtk_label_set_markup(widget->priv->artist_prefix, NULL);
	}
	else
	{
		gtk_label_set_markup(widget->priv->artist_prefix, "<small>by</small> ");
	}
}

void
widget_song_info_set_album(WidgetSongInfo *widget, const gchar *album)
{
	g_return_if_fail(WIDGET_IS_SONG_INFO(widget));

	widget->priv->album_text = album;

	gtk_label_set_label(widget->priv->album, album);

	if (album == NULL)
	{
		gtk_label_set_markup(widget->priv->album_prefix, NULL);
	}
	else
	{
		gtk_label_set_markup(widget->priv->album_prefix, "<small>on</small> ");
	}
}

/* END OF FILE */
