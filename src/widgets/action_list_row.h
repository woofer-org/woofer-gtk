/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * widgets/action_list_row.h  This file is part of Woofer GTK
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

#ifndef __WIDGETS_ACTION_LIST_ROW__
#define __WIDGETS_ACTION_LIST_ROW__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#define WIDGET_TYPE_ACTION_LIST_ROW (widget_action_list_row_get_type())
#define WIDGET_ACTION_LIST_ROW(ptr) G_TYPE_CHECK_INSTANCE_CAST(ptr, widget_action_list_row_get_type(), WidgetActionListRow)
#define WIDGET_ACTION_LIST_ROW_CLASS(ptr) G_TYPE_CHECK_CLASS_CAST(ptr, widget_action_list_row_get_type(), WidgetActionListRowClass)
#define WIDGET_IS_ACTION_LIST_ROW(ptr) G_TYPE_CHECK_INSTANCE_TYPE(ptr, widget_action_list_row_get_type())
#define WIDGET_IS_ACTION_LIST_ROW_CLASS(ptr) G_TYPE_CHECK_CLASS_TYPE(ptr, widget_action_list_row_get_type())
#define WIDGET_ACTION_LIST_ROW_GET_CLASS(ptr) G_TYPE_INSTANCE_GET_CLASS(ptr, widget_action_list_row_get_type(), WidgetActionListRowClass)

typedef struct _WidgetActionListRow WidgetActionListRow;
typedef struct _WidgetActionListRowClass WidgetActionListRowClass;
typedef struct _WidgetActionListRowPrivate WidgetActionListRowPrivate;

//_GLIB_DEFINE_AUTOPTR_CHAINUP(WidgetActionListRow, GtkListBoxRow);
//G_DEFINE_AUTOPTR_CLEANUP_FUNC(WidgetActionListRowClass, g_type_class_unref);

struct _WidgetActionListRow
{
	/*< public >*/
	GtkListBoxRow parent_instance;

	/*< private >*/
	WidgetActionListRowPrivate *priv;
};

struct _WidgetActionListRowClass
{
	GtkListBoxRowClass parent_class;
};

GType widget_action_list_row_get_type(void);

GtkWidget * widget_action_list_row_new(const gchar *title, const gchar *subtitle);

void widget_action_list_row_set_child_widget(WidgetActionListRow *row, GtkWidget *widget);

GtkWidget * widget_action_list_row_get_child_widget(WidgetActionListRow *row);

void widget_action_list_row_set_activatable_widget(WidgetActionListRow *row, GtkWidget *widget);

GtkWidget * widget_action_list_row_get_activatable_widget(WidgetActionListRow *row);

void widget_action_list_row_set_title(WidgetActionListRow *row, const gchar *title);

void widget_action_list_row_set_subtitle(WidgetActionListRow *row, const gchar *subtitle);

void widget_action_list_row_activate_child(WidgetActionListRow *row);

#endif /* __WIDGETS_ACTION_LIST_ROW__ */

/* END OF FILE */
