/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * widgets/action_list_row.c  This file is part of Woofer GTK
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
 * This module implements a GObject type derived from GtkListBoxRow.
 */

// Library includes
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

// Woofer core includes
/*< none >*/

// Module includes
#include "widgets/action_list_row.h"

// Dependency includes
/*< none >*/

// Resource includes
/*< none >*/

// Private data structure that gets automatically allocated by GObject
struct _WidgetActionListRowPrivate
{
	GtkBox *content;
	GtkBox *title_box;
	GtkLabel *title;
	GtkLabel *subtitle;
	GtkBox *widget_box;
	GtkWidget *child;
	GtkWidget *activatable_widget;
};

/*
 * Declare / Define derivable GObject type items (including it's own private
 * structure) here manually because of a couple of reasons:
 * 1) For a programmer that reads this code but does not know GObject
 *    completely, it is still relatively understandable.
 * 2) It is difficult to debug when the compiler issues a warning because
 *    a macro expects something different; the code the compiler outputs does
 *    not correspond to the actual code, thus it is up to the programmer to
 *    find where the warning is coming from.
 * 3) The macros provide no flexibility; all functions or macros are required
 *    to be named as defined by GObject.
 */

/* BEGIN OF DEFINES (based on G_DEFINE_TYPE_WITH_PRIVATE) */

// Function prototypes
static void widget_action_list_row_subtitle_shown(GtkWidget *widget, gpointer user_data);
static void widget_action_list_row_init(WidgetActionListRow *self);
static void widget_action_list_row_class_init(WidgetActionListRowClass *klass);
static GType widget_action_list_row_get_type_once(void);

// Statically allocated variables
static gpointer widget_action_list_row_parent_class = NULL;
static gint widget_action_list_row_private_offset;

// Intern class initialization; GObject magic, it just works, don't bother
static void
widget_action_list_row_class_intern_init(gpointer klass)
{
	widget_action_list_row_parent_class = g_type_class_peek_parent(klass);

	if (widget_action_list_row_private_offset != 0)
	{
		g_type_class_adjust_private_offset(klass, &widget_action_list_row_private_offset);
	}

	widget_action_list_row_class_init((WidgetActionListRowClass *) klass);
}

// Get the private structure of this GObject instance
static gpointer
widget_action_list_row_get_instance_private(WidgetActionListRow *self)
{
	return G_STRUCT_MEMBER_P(self, widget_action_list_row_private_offset);
}

// Make sure the GObject type is registered and return the GType value
static GType
widget_action_list_row_get_type_once(void)
{
	GType g_define_type_id = g_type_register_static_simple(GTK_TYPE_LIST_BOX_ROW,
	                                                       g_intern_static_string("WidgetActionListRow"),
	                                                       sizeof(WidgetActionListRowClass),
	                                                       (GClassInitFunc)(void (*) (void)) widget_action_list_row_class_intern_init,
	                                                       sizeof(WidgetActionListRow),
	                                                       (GInstanceInitFunc)(void (*) (void)) widget_action_list_row_init,
	                                                       (GTypeFlags) 0);
	{
		{
			widget_action_list_row_private_offset = g_type_add_instance_private(g_define_type_id,
			                                                                    sizeof(WidgetActionListRowPrivate));
		}
	}
	return g_define_type_id;
}

// Get this GObjects GType
GType
widget_action_list_row_get_type(void)
{
	static gsize static_g_define_type_id = 0;

	if (g_once_init_enter(&static_g_define_type_id))
	{
		GType g_define_type_id = widget_action_list_row_get_type_once();
		g_once_init_leave(&static_g_define_type_id, g_define_type_id);
	}

	return static_g_define_type_id;
}

/* END OF DEFINES */

// Function that gets called on allocation of a GObject instance to initialize the class
static void
widget_action_list_row_class_init(WidgetActionListRowClass *class)
{
	// Nothing to initialize
}

// Function that gets called on allocation of a GObject instance to initialize any other object stuff
static void
widget_action_list_row_init(WidgetActionListRow *row)
{
	// In fact, we create and pack all subwidgets here

	WidgetActionListRowPrivate *priv;

	// Use our private container to store pointer to used widgets
	row->priv = priv = widget_action_list_row_get_instance_private(row);

	// Use this to have the title left and the widget on the right
	//priv->content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));
	// Use this to have the title first and the widget below that
	priv->content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 12));

	// Parameters for the main content box
	gtk_container_set_border_width(GTK_CONTAINER(priv->content), 6);

	// Title & subtitle box (Left-top of the row)
	priv->title_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
	priv->title = GTK_LABEL(gtk_label_new(NULL /* text */));
	gtk_label_set_xalign(priv->title, 0.0);
	priv->subtitle = GTK_LABEL(gtk_label_new(NULL /* text */));
	gtk_label_set_xalign(priv->subtitle, 0.0);
	gtk_label_set_line_wrap(priv->subtitle, TRUE);
	g_signal_connect(priv->subtitle, "show", G_CALLBACK(widget_action_list_row_subtitle_shown), NULL /* user_data */);

	// Box for the custom widget
	priv->widget_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 8));
	gtk_widget_set_halign(GTK_WIDGET(priv->widget_box), GTK_ALIGN_START);
	priv->child = NULL; // To be provided by caller
	priv->activatable_widget = NULL;

	// Set GtkListBoxRow options
	gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
	gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(row), FALSE);

	// Now pack the widgets in the right order
	gtk_container_add(GTK_CONTAINER(row), GTK_WIDGET(priv->content));
	gtk_box_pack_start(priv->content, GTK_WIDGET(priv->title_box), TRUE, TRUE, 0);
	gtk_box_pack_end(priv->content, GTK_WIDGET(priv->widget_box), FALSE, FALSE, 0);
	gtk_box_pack_start(priv->title_box, GTK_WIDGET(priv->title), FALSE, TRUE, 0);
	gtk_box_pack_end(priv->title_box, GTK_WIDGET(priv->subtitle), FALSE, TRUE, 0);
}

// If the application calls "show_all" on the window, then hide the label again if it is empty
static void
widget_action_list_row_subtitle_shown(GtkWidget *widget, gpointer user_data)
{
	const gchar *text;
	GtkLabel *label;

	g_return_if_fail(GTK_IS_LABEL(widget));

	label = GTK_LABEL(widget);

	text = gtk_label_get_text(label);

	if (text == NULL || text[0] == '\0')
	{
		gtk_widget_hide(widget);
	}
}

// Creates a new ActionListRow widget; based on GtkListBoxRow
GtkWidget *
widget_action_list_row_new(const gchar *title, const gchar *subtitle)
{
	WidgetActionListRow *row;
	GObject *widget;

	widget = g_object_new(WIDGET_TYPE_ACTION_LIST_ROW, NULL);
	row = WIDGET_ACTION_LIST_ROW(widget);

	widget_action_list_row_set_title(row, title);
	widget_action_list_row_set_subtitle(row, subtitle);

	return GTK_WIDGET(row);
}

void
widget_action_list_row_set_child_widget(WidgetActionListRow *row, GtkWidget *widget)
{
	WidgetActionListRowPrivate *priv;
	gboolean child_is_activatable;

	g_return_if_fail(row != NULL);
	g_return_if_fail(row->priv != NULL);

	priv = row->priv;

	child_is_activatable = (priv->child == priv->activatable_widget); // compare pointer

	// Remove old child
	if (priv->child != NULL )
	{
		gtk_container_remove(GTK_CONTAINER(priv->widget_box), priv->child);

		g_object_unref(priv->child);
	}

	// Add new child
	priv->child = widget;
	gtk_container_add(GTK_CONTAINER(priv->widget_box), priv->child);

	// If the activatable widget was not set manual, set it to the new widget
	if (child_is_activatable)
	{
		priv->activatable_widget = widget;
	}
}

GtkWidget *
widget_action_list_row_get_child_widget(WidgetActionListRow *row)
{
	WidgetActionListRowPrivate *priv;

	g_return_val_if_fail(WIDGET_IS_ACTION_LIST_ROW(row), NULL);
	g_return_val_if_fail(row->priv != NULL, NULL);

	priv = row->priv;

	return priv->child;
}

void
widget_action_list_row_set_activatable_widget(WidgetActionListRow *row, GtkWidget *widget)
{
	WidgetActionListRowPrivate *priv;

	g_return_if_fail(GTK_IS_WIDGET(widget));
	g_return_if_fail(WIDGET_IS_ACTION_LIST_ROW(row));
	g_return_if_fail(row->priv != NULL);

	priv = row->priv;

	if (widget == NULL)
	{
		priv->activatable_widget = priv->child;
	}
	else
	{
		priv->activatable_widget = widget;
	}
}

GtkWidget *
widget_action_list_row_get_activatable_widget(WidgetActionListRow *row)
{
	WidgetActionListRowPrivate *priv;

	g_return_val_if_fail(WIDGET_IS_ACTION_LIST_ROW(row), NULL);
	g_return_val_if_fail(row->priv != NULL, NULL);

	priv = row->priv;

	return priv->activatable_widget;
}

void
widget_action_list_row_set_title(WidgetActionListRow *row, const gchar *title)
{
	WidgetActionListRowPrivate *priv;
	gchar *str;

	g_return_if_fail(WIDGET_IS_ACTION_LIST_ROW(row));
	g_return_if_fail(row->priv != NULL);

    priv = row->priv;

	// Put title with formatting into one string
	str = g_markup_printf_escaped("<b>%s</b>", title);

	// Set label parameters
	gtk_label_set_markup(priv->title, str);

	g_free(str);
}

void
widget_action_list_row_set_subtitle(WidgetActionListRow *row, const gchar *subtitle)
{
	WidgetActionListRowPrivate *priv;
	gchar *str;

	g_return_if_fail(WIDGET_IS_ACTION_LIST_ROW(row));
	g_return_if_fail(row->priv != NULL);

	priv = row->priv;

	// If no subtitle, set it to empty
	if (subtitle == NULL)
	{
		subtitle = "";
	}

	// Put subtitle with formatting into one string
	str = g_markup_printf_escaped("<span font-size=\"smaller\" font-weight=\"thin\">%s</span>", subtitle);

	// Set label parameters
	gtk_label_set_markup(priv->subtitle, str);

	g_free(str);
}

// Activate the "activatable child" widget.
// This is supposed to be called when the container GtkListBox has an activated row.
void
widget_action_list_row_activate_child(WidgetActionListRow *row)
{
	WidgetActionListRowPrivate *priv;
	GtkWidget *widget;
	gboolean active;

	g_return_if_fail(WIDGET_IS_ACTION_LIST_ROW(row));
	g_return_if_fail(row->priv != NULL);

	priv = row->priv;
	widget = priv->activatable_widget;

	g_return_if_fail(widget != NULL);

	if (GTK_IS_BUTTON(widget))
	{
		gtk_button_clicked(GTK_BUTTON(widget));
	}
	else if (GTK_IS_TOGGLE_BUTTON(widget))
	{
		gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(widget));
	}
	else if (GTK_IS_SWITCH(widget))
	{
		active = gtk_switch_get_active(GTK_SWITCH(widget));
		gtk_switch_set_active(GTK_SWITCH(widget), !active); // Invert state
	}
	else if (GTK_IS_COMBO_BOX(widget))
	{
		gtk_combo_box_popup(GTK_COMBO_BOX(widget));
	}
	else
	{
		gtk_widget_grab_focus(priv->activatable_widget);
	}
}

/* END OF FILE */
