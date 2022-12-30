/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * interface.c  This file is part of Woofer GTK
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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

// Woofer core includes
#include <woofer/app.h>
#include <woofer/song.h>
#include <woofer/constants.h>
#include <woofer/settings.h>
#include <woofer/library.h>
#include <woofer/resources.h>
#include <woofer/utils.h>

// Module includes
#include "interface.h"

// Dependency includes
#include "about.h"
#include "config.h"
#include "icons.h"
#include "preferences.h"
#include "question_dialog.h"
#include "settings.h"
#include "utils.h"
#include "widgets/song_info.h"

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 *
 * This is the main interface module that constructs the main application window
 * and manages it's events.
 */

/*
 * Location specific notes:
 * [1] Do not check if void pointer is valid, the gtk function will
 *     check it's object type on it's own.
 * [2] Allocate the GtkTreeIter instance here, because gtk will write to this
 *     structure and this fails if it is already allocated and the same
 *     instance is used again. So re-allocate every time the loop runs.
 * [3] Using a range of 0-100 is better for consistency in the back-end
 *     implementation, but is not ideal in an interface.  A range of 0-10 is
 *     much better to represent a rating to the user.  This means that the
 *     values between front-end and back-end need to be converted.
 * [4] Hide the window first, then quit application;
 *     This makes the window disappear immediately even if the application
 *     takes a short while to quit.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */

// Minimum column width to use
#define COLUMN_MIN_WIDTH 5

/* DEFINES END */

/* CUSTOM TYPES BEGIN */

typedef struct _InterfaceDetails InterfaceDetails;

typedef enum _DialogResponse DialogResponse;
typedef enum _TreeColumns TreeColumns;
typedef enum _SongStatusIcon SongStatusIcon;

typedef void (*func_tree_update_item) (GtkTreeStore *store, GtkTreeIter *iter, WfSong *song);

enum _DialogResponse
{
	DIALOG_CANCEL,
	DIALOG_CLOSE,
	DIALOG_QUIT
};

enum _TreeColumns
{
	STATUS_COLUMN,
	URI_COLUMN,
	NAME_COLUMN,
	NUMBER_COLUMN,
	TITLE_COLUMN,
	ARTIST_COLUMN,
	ALBUM_COLUMN,
	DURATION_COLUMN,
	RATING_COLUMN,
	SCORE_COLUMN,
	PLAYCOUNT_COLUMN,
	SKIPCOUNT_COLUMN,
	LASTPLAYED_COLUMN,
	SONGOBJ_COLUMN,
	N_COLUMNS
};

enum _SongStatusIcon
{
	STATUS_ICON_INVALID,
	STATUS_ICON_NONE,
	STATUS_ICON_PLAYING,
	STATUS_ICON_PAUSED,
	STATUS_ICON_QUEUED,
	STATUS_ICON_STOP
};

struct _InterfaceDetails
{
	gboolean constructed;
	gboolean csd;
	gboolean is_fullscreen;

	WfApp *application;
	WfSong *current_song;

	GSList *selection_tools;
	GSList *playing_tools;

	GtkWindow *main_window;
	GtkWidget *window_widget;
	GtkWidget *progress;
	GtkWidget *prog_bar;
	GtkWidget *header_bar;
	GtkWidget *subtitle_box;
	GtkWidget *subtitle_label;
	GtkWidget *toolbar;
	GtkWidget *box_prev;
	GtkWidget *box_current;
	GtkWidget *box_next;
	GtkWidget *select_box;
	GtkWidget *play_pause_button;
	GtkWidget *queue_button;
	GtkWidget *status_bar;
	GtkWidget *library_label;

	GtkLabel *position_start;
	GtkLabel *position_end;
	GtkRange *position_slider;

	GtkMenuItem *menu_fullscreen;

	GtkToolItem *remove;
	GtkToolItem *queue;
	GtkToolItem *stop;
	GtkToolItem *edit_rating;

	GtkTreeView *tree_view;
	GtkTreeStore *tree_store;
	GtkTreeViewColumn *uri_column;
	GtkTreeViewColumn *filename_column;
	GtkTreeViewColumn *track_number_column;
	GtkTreeViewColumn *title_column;
	GtkTreeViewColumn *artist_column;
	GtkTreeViewColumn *album_column;
	GtkTreeViewColumn *duration_column;
	GtkTreeViewColumn *rating_column;
	GtkTreeViewColumn *score_column;
	GtkTreeViewColumn *playcount_column;
	GtkTreeViewColumn *skipcount_column;
	GtkTreeViewColumn *lastplayed_column;

	guint tree_row_activate_handler;
	guint position_updated_handler;
};

/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static void interface_construct(WfApp *app);
static void interface_set_info_labels(WidgetSongInfo *info, WfSong *song);
static void interface_set_label_previous(WfSong *song);
static void interface_set_label_current(WfSong *song);
static void interface_set_label_next(WfSong *song);
static void interface_set_song_labels(WfSong *prev, WfSong *current, WfSong *next);

static gboolean interface_close_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static void interface_destroy_cb(GtkWidget *object, gpointer user_data);
static void interface_preferences_closed_cb(const gchar *message);
static gboolean interface_key_pressed_cb(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static gboolean interface_tree_button_pressed_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static void interface_menu_quit_cb(GtkMenuItem *menuitem, gpointer user_data);
static void interface_open_items_cb(GtkWidget *widget, gpointer user_data);
static void interface_open_directory_cb(GtkWidget *widget, gpointer user_data);
static void interface_remove_items_cb(GtkWidget *widget, gpointer user_data);
static void interface_move_items_up_cb(GtkWidget *widget, gpointer user_data);
static void interface_move_items_down_cb(GtkWidget *widget, gpointer user_data);
static void interface_toggle_activate_cb(GtkCheckMenuItem *checkmenuitem, gpointer user_data);
static void interface_edit_preferences_cb(GtkWidget *widget, gpointer user_data);
static void interface_previous_cb(GtkWidget *widget, gpointer user_data);
static void interface_next_cb(GtkWidget *widget, gpointer user_data);
static void interface_toggle_incognito_cb(GtkCheckMenuItem *checkmenuitem, gpointer user_data);
static void interface_play_pause_cb(GtkWidget *widget, gpointer user_data);
static void interface_stop_cb(GtkWidget *widget, gpointer user_data);
static void interface_library_write_cb(GtkWidget *widget, gpointer user_data);
static void interface_metadata_refresh_cb(GtkWidget *widget, gpointer user_data);
static void interface_fullscreen_toggle_cb(GtkCheckMenuItem *checkmenuitem, gpointer user_data);
static void interface_toggle_toolbar_cb(GtkCheckMenuItem *checkmenuitem, gpointer user_data);
static void interface_hide_window_cb(GtkWidget *widget, gpointer user_data);
static void interface_help_about_cb(GtkWidget *widget, gpointer user_data);

static void interface_stop_after_song_cb(GtkWidget *widget, gpointer user_data);
static void interface_selection_changed_cb(GtkTreeSelection *tree_selection, gpointer user_data);
static void interface_select_all_cb(GtkWidget *widget, gpointer user_data);
static void interface_select_none_cb(GtkWidget *widget, gpointer user_data);
static void interface_toggle_queue_cb(GtkWidget *widget, gpointer user_data);
static void interface_redraw_next_cb(GtkWidget *widget, gpointer user_data);
static void interface_edit_rating_cb(GtkWidget *widget, gpointer user_data);
static void interface_scroll_to_playing_cb(GtkWidget *widget, gpointer user_data);

static void interface_dialog_stop_cb(GtkEntry *entry, gpointer user_data);
static void interface_dialog_value_changed_cb(GtkWidget *widget, gpointer user_data);

static void interface_items_are_added_cb(WfSong *song, gint item, gint total);
static void interface_update_song_info_cb(WfApp *app, WfSong *song_previous, WfSong *song_current, WfSong *song_next, gpointer user_data);
static void interface_statusbar_update_cb(WfApp *app, const gchar *message, gpointer user_data);
static void interface_playing_state_changed_cb(WfApp *app, WfAppStatus state, gdouble duration, gpointer user_data);
static void interface_playback_position_cb(WfApp *app, gdouble position, gdouble duration, gpointer user_data);
static void interface_position_slider_updated_cb(GtkRange *range, gpointer user_data);

static void interface_tree_update_song_stat_cb(GtkTreeStore *store, GtkTreeIter *iter, WfSong *song);
static void interface_tree_update_song_metadata_cb(GtkTreeStore *store, GtkTreeIter *iter, WfSong *song);
static void interface_tree_update_all_stats_cb(void);
static void interface_tree_update_all_song_icons(void);
static void interface_tree_update_song_status(GtkTreeStore *store, GtkTreeIter *iter, WfSong *song);
static void interface_tree_scroll_to_row(GtkTreePath *path);
static void interface_tree_activated_cb(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);
static void interface_drag_data_received_cb(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data);

static gboolean interface_handle_notification_cb(WfApp *app, WfSong *song, gint64 duration, gpointer user_data);

static void interface_quit_application(void);
static void interface_show_window(void);
static void interface_hide_window(void);
static void interface_present_window(void);
static void interface_toggle_fullscreen(void);
static void interface_leave_fullscreen(void);
static void interface_enter_fullscreen(void);
static void interface_show_toolbar(gboolean show);
static void interface_update_playback_position(gdouble position, gdouble duration);
static void interface_update_position_slider_marks(void);

static void interface_add_items(GSList *files, WfLibraryFileChecks checks, gboolean skip_metadata);
static void interface_update_toolbar(gint items_selected, gint items_total);
static void interface_update_library_info(gint selected, gint total);
static void interface_report_items_added(gint amount);
static void interface_tree_update_song_data(func_tree_update_item cb_func);
static gboolean interface_tree_get_iter_for_song(WfSong *song, GtkTreeIter *iter);
static WfSong * interface_tree_get_song_for_iter(GtkTreeModel *model, GtkTreeIter *iter);
static WfSong * interface_tree_get_song_for_path(GtkTreeModel *model, GtkTreePath *path);

static gboolean interface_ask_to_quit();
static gboolean interface_remove_confirm_dialog(gint amount);
static gint interface_edit_rating_dialog(GtkWindow *parent, gint amount);
static DialogResponse interface_close_confirm(GtkWindow *parent);

static void interface_progress_window_create(const gchar *description);
static void interface_progress_window_update(gdouble complete);
static void interface_progress_window_destroy(void);

static void interface_set_subtitle(gchar *subtitle);
static void interface_set_button_play(void);
static void interface_set_button_pause(void);

static gboolean interface_tree_pop_menu(GtkWidget *widget, GdkEventButton *event, gpointer subwidget);
static GtkWidget * interface_get_default_media_icon(const gchar *icon_name);
static GdkPixbuf * interface_get_pixbuf_icon(SongStatusIcon state);
static void interface_window_set_default_widget(GtkWindow *window, GtkWidget *widget);
static void interface_update_gtk_events(void);

static void interface_destruct(void);
static void interface_finalize(void);

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */

static InterfaceDetails InterfaceData =
{
	.constructed = FALSE,
	.csd = TRUE,

	// All others are %NULL
};

/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

static void
interface_construct(WfApp *app)
{
	const gchar *name;
	const gchar *icon_name;

	WfSong *song;

	GtkWidget *widget;
	GtkWidget *header_bar;
	GtkWidget *button;
	GtkWidget *separator;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *box;
	GtkWidget *image;
	GtkWidget *menu_bar;
	GtkWidget *menu;
	GtkWidget *menu_item;
	GtkWidget *toolbar;
	GtkWidget *volume_button;
	GtkWidget *info;
	GtkWidget *label;
	GtkWidget *frame;
	GtkWidget *scroll_window;
	GtkWidget *tree_view;
	GtkWidget *progress_box;
	GtkWidget *slider;
	GtkWidget *controls;
	GtkWidget *status_bar;
	GtkToolItem *tool_item;
	GtkTreeStore *tree_store;
	GtkTreeSelection *tree_select;
	GtkTreeViewColumn *column;
	GtkCellRenderer *text_renderer;
	GtkCellRenderer *pixbuf_renderer;
	GtkIconSize icon_size = GTK_ICON_SIZE_DIALOG;
	GtkStyleContext *style;
	GdkPixbuf *icon;
	GList *hide_widgets = NULL, *list;
	gdouble app_time;
	gchar *str;
	gchar *time;
	gint total_items;

	const GtkTargetEntry targets[] = { { "text/uri-list", GTK_TARGET_OTHER_APP, 0 } };

	g_info("Application activation: Constructing main window");

	// Application window
	InterfaceData.window_widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	InterfaceData.main_window = GTK_WINDOW(InterfaceData.window_widget);
	gtk_window_set_default_size(InterfaceData.main_window, INTERFACE_DEFAULT_LARGE_WIDTH, INTERFACE_DEFAULT_LARGE_HEIGHT);
	g_signal_connect(InterfaceData.window_widget, "delete-event", G_CALLBACK(interface_close_cb), NULL /* user_data */);
	g_signal_connect(InterfaceData.window_widget, "destroy", G_CALLBACK(interface_destroy_cb), NULL /* user_data */);
	g_signal_connect(InterfaceData.window_widget, "key-press-event", G_CALLBACK(interface_key_pressed_cb), NULL /* user_data */);

	name = wf_app_get_display_name();
	gtk_window_set_title(InterfaceData.main_window, name);

	icon_name = wf_app_get_icon_name();
	gtk_window_set_icon_name(InterfaceData.main_window, icon_name);

	icon = icons_get_static_image(WF_RESOURCE_ICON256_SVG);
	if (icon != NULL)
	{
		gtk_window_set_default_icon(icon);
	}

	// Volume button
	volume_button = gtk_volume_button_new();
	g_object_bind_property(app, "volume", volume_button, "value", (G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE));

	if (InterfaceData.csd)
	{
		// HeaderBar
		header_bar = gtk_header_bar_new();
		gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), name);
		gtk_header_bar_set_subtitle(GTK_HEADER_BAR(header_bar), "Initializing...");
		gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
		gtk_window_set_titlebar(InterfaceData.main_window, header_bar);
		InterfaceData.header_bar = header_bar;

		// Adding volume button
		gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), volume_button);

		// Menu button (in HeaderBar)
		button = gtk_menu_button_new();
		image = gtk_image_new_from_icon_name("open-menu-symbolic", GTK_ICON_SIZE_MENU);
		gtk_button_set_image(GTK_BUTTON(button), image);
		gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), button);

		menu = gtk_menu_new();
		gtk_menu_button_set_popup(GTK_MENU_BUTTON(button), menu);

		menu_item = gtk_menu_item_new_with_mnemonic("_Quit...");
		g_signal_connect(menu_item, "activate", G_CALLBACK(interface_menu_quit_cb), NULL /* user_data */);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		gtk_widget_show_all(menu);
	}

	// Main content box
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_add(GTK_CONTAINER(InterfaceData.window_widget), vbox);

	// Menu / Info box
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	// MenuBar
	menu_bar = gtk_menu_bar_new();
	gtk_widget_set_valign(menu_bar, GTK_ALIGN_START);
	gtk_box_pack_start(GTK_BOX(hbox), menu_bar, TRUE, TRUE, 0);

	// Player menu
	menu_item = gtk_menu_item_new_with_mnemonic("_Player");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	menu_item = gtk_menu_item_new_with_mnemonic("_Play/pause");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_play_pause_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_mnemonic("_Stop");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_stop_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_mnemonic("Skip _backward");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_previous_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_mnemonic("Skip _forward");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_next_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_check_menu_item_new_with_mnemonic("_Incognito");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_toggle_incognito_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_mnemonic("_Quit");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_menu_quit_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// Library menu
	menu_item = gtk_menu_item_new_with_mnemonic("_Library");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	menu_item = gtk_menu_item_new_with_mnemonic("_Add songs...");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_open_items_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_mnemonic("Add _directory...");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_open_directory_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_mnemonic("Re_fresh metadata");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_metadata_refresh_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_mnemonic("_Force write to disk");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_library_write_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// Song menu
	menu_item = gtk_menu_item_new_with_mnemonic("_Song");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	menu_item = gtk_menu_item_new_with_mnemonic("_Remove from library...");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_remove_items_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_mnemonic("_Toggle queue");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_toggle_queue_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_mnemonic("_Redraw next song");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_redraw_next_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_mnemonic("_Stop after playing song");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_stop_after_song_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// Edit menu
	menu_item = gtk_menu_item_new_with_mnemonic("_Edit");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	menu_item = gtk_check_menu_item_new_with_mnemonic("Can _activate");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), TRUE);
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_toggle_activate_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_mnemonic("Set _rating");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_edit_rating_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_mnemonic("_Preferences...");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_edit_preferences_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// View menu
	menu_item = gtk_menu_item_new_with_mnemonic("_View");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	menu_item = gtk_check_menu_item_new_with_mnemonic("_Fullscreen");
	g_signal_connect(menu_item, "toggled", G_CALLBACK(interface_fullscreen_toggle_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_check_menu_item_new_with_mnemonic("_Toolbar");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), TRUE);
	g_signal_connect(menu_item, "toggled", G_CALLBACK(interface_toggle_toolbar_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_menu_item_new_with_mnemonic("_Close window");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_hide_window_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// Help menu
	menu_item = gtk_menu_item_new_with_mnemonic("_Help");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);

	menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);

	menu_item = gtk_menu_item_new_with_mnemonic("_About");
	g_signal_connect(menu_item, "activate", G_CALLBACK(interface_help_about_cb), NULL /* user_data */);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// Alternative subtitle box
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(hbox), box, FALSE, TRUE, 0);
	InterfaceData.subtitle_box = box;

	// Hide after construction if client-side decoration is disabled
	if (InterfaceData.csd)
	{
		hide_widgets = g_list_append(hide_widgets, box);
	}

	separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
	gtk_box_pack_start(GTK_BOX(box), separator, FALSE, TRUE, 0);

	// No client-side decoration subtitle alternative
	info = gtk_label_new(NULL);
	gtk_widget_set_margin_start(info, 8);
	gtk_widget_set_margin_end(info, 8);
	gtk_box_pack_end(GTK_BOX(box), info, FALSE, TRUE, 0);
	InterfaceData.subtitle_label = info;

	// Tool box
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	// Toolbar
	toolbar = gtk_toolbar_new();
	gtk_box_pack_start(GTK_BOX(hbox), toolbar, FALSE, TRUE, 0);
	icon_size = GTK_ICON_SIZE_LARGE_TOOLBAR;
	InterfaceData.toolbar = toolbar;

	// Make items look linked
	style = gtk_widget_get_style_context(toolbar);
	gtk_style_context_add_class(style, GTK_STYLE_CLASS_LINKED);

	image = gtk_image_new_from_icon_name("checkbox-checked-symbolic", icon_size);
	tool_item = gtk_tool_button_new(image, "Select");
	gtk_tool_item_set_tooltip_text(tool_item, "Select all songs");
	g_signal_connect(tool_item, "clicked", G_CALLBACK(interface_select_all_cb), NULL /* user_data */);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1 /* position */);

	image = gtk_image_new_from_icon_name("checkbox-symbolic", icon_size);
	tool_item = gtk_tool_button_new(image, "Deselect");
	gtk_tool_item_set_tooltip_text(tool_item, "Deselect all songs");
	g_signal_connect(tool_item, "clicked", G_CALLBACK(interface_select_none_cb), NULL /* user_data */);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1 /* position */);
	InterfaceData.selection_tools = g_slist_append(InterfaceData.selection_tools, tool_item);

	image = gtk_image_new_from_icon_name("add", icon_size);
	tool_item = gtk_tool_button_new(image, "Add");
	gtk_tool_item_set_tooltip_text(tool_item, "Add new songs to the library");
	g_signal_connect(tool_item, "clicked", G_CALLBACK(interface_open_directory_cb), NULL /* user_data */);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1 /* position */);

	image = gtk_image_new_from_icon_name("remove", icon_size);
	tool_item = gtk_tool_button_new(image, "Remove");
	g_signal_connect(tool_item, "clicked", G_CALLBACK(interface_remove_items_cb), NULL /* user_data */);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1 /* position */);
	InterfaceData.remove = tool_item;
	InterfaceData.selection_tools = g_slist_append(InterfaceData.selection_tools, tool_item);

	image = gtk_image_new_from_icon_name("up", icon_size);
	tool_item = gtk_tool_button_new(image, "Move up");
	gtk_tool_item_set_tooltip_text(tool_item, "Move selection up in the library");
	g_signal_connect(tool_item, "clicked", G_CALLBACK(interface_move_items_up_cb), NULL /* user_data */);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1 /* position */);
	InterfaceData.selection_tools = g_slist_append(InterfaceData.selection_tools, tool_item);

	image = gtk_image_new_from_icon_name("down", icon_size);
	tool_item = gtk_tool_button_new(image, "Move down");
	gtk_tool_item_set_tooltip_text(tool_item, "Move selection down in the library");
	g_signal_connect(tool_item, "clicked", G_CALLBACK(interface_move_items_down_cb), NULL /* user_data */);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1 /* position */);
	InterfaceData.selection_tools = g_slist_append(InterfaceData.selection_tools, tool_item);

	image = gtk_image_new_from_icon_name("playlist-queue", icon_size);
	tool_item = gtk_tool_button_new(image, "Queue");
	g_signal_connect(tool_item, "clicked", G_CALLBACK(interface_toggle_queue_cb), NULL /* user_data */);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1 /* position */);
	InterfaceData.queue = tool_item;
	InterfaceData.selection_tools = g_slist_append(InterfaceData.selection_tools, tool_item);

	image = gtk_image_new_from_icon_name("media-playback-stop", icon_size);
	tool_item = gtk_tool_button_new(image, "Stop");
	gtk_tool_item_set_tooltip_text(tool_item, "Stop after song has been played");
	g_signal_connect(tool_item, "clicked", G_CALLBACK(interface_stop_after_song_cb), NULL /* user_data */);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1 /* position */);
	InterfaceData.stop = tool_item;

	image = gtk_image_new_from_icon_name(NULL, icon_size);
	tool_item = gtk_tool_button_new(image, "Rating");
	g_signal_connect(tool_item, "clicked", G_CALLBACK(interface_edit_rating_cb), NULL /* user_data */);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1 /* position */);
	InterfaceData.edit_rating = tool_item;
	InterfaceData.selection_tools = g_slist_append(InterfaceData.selection_tools, tool_item);

	image = gtk_image_new_from_icon_name(NULL, icon_size);
	tool_item = gtk_tool_button_new(image, "Scroll");
	gtk_tool_item_set_tooltip_text(tool_item, "Scroll to currently playing");
	g_signal_connect(tool_item, "clicked", G_CALLBACK(interface_scroll_to_playing_cb), NULL /* user_data */);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1 /* position */);
	InterfaceData.playing_tools = g_slist_append(InterfaceData.playing_tools, tool_item);

	// Volume button (no client-side decoration)
	if (!InterfaceData.csd)
	{
		gtk_widget_set_margin_end(volume_button, 8);
		gtk_widget_set_valign(volume_button, GTK_ALIGN_CENTER);
		gtk_box_pack_end(GTK_BOX(hbox), volume_button, FALSE, TRUE, 0);
	}

	// Song info labels
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	g_signal_connect(app, "songs-changed", G_CALLBACK(interface_update_song_info_cb), NULL /* user_data */);

	info = widget_song_info_new("Previously played:");
	gtk_box_pack_start(GTK_BOX(hbox), info, TRUE, TRUE, 0);
	InterfaceData.box_prev = info;

	info = widget_song_info_new("Currently playing:");
	gtk_box_pack_start(GTK_BOX(hbox), info, TRUE, TRUE, 0);
	InterfaceData.box_current = info;

	info = widget_song_info_new("Up next:");
	gtk_box_pack_start(GTK_BOX(hbox), info, TRUE, TRUE, 0);
	InterfaceData.box_next = info;

	// Set initial content (empty labels)
	interface_set_song_labels(NULL, NULL, NULL);

	// Tree frame
	frame = gtk_frame_new(NULL /* label */);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

	// Scroll window for tree view
	scroll_window = gtk_scrolled_window_new(NULL, NULL);
	// Settings the minimal height prevent the vertical scrollbar from getting crammed.
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_window), 140);
	gtk_container_add(GTK_CONTAINER(frame), scroll_window);

	// Tree list
	tree_view = gtk_tree_view_new();
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree_view), FALSE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(tree_view), FALSE);
	gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(tree_view), TRUE);
	InterfaceData.tree_row_activate_handler = g_signal_connect(tree_view, "row-activated", G_CALLBACK(interface_tree_activated_cb), NULL /* user_data */);
	gtk_container_add(GTK_CONTAINER(scroll_window), tree_view);
	InterfaceData.tree_view = GTK_TREE_VIEW(tree_view);

	// Right-click menu
	menu = gtk_menu_new();
	g_signal_connect(tree_view, "button-press-event", G_CALLBACK(interface_tree_button_pressed_cb), menu);
	menu_item = gtk_menu_item_new_with_mnemonic("Play now");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	// Tree selection
	tree_select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	gtk_tree_selection_set_mode(tree_select, GTK_SELECTION_MULTIPLE);

	// Tree content
	total_items = wf_song_get_count();
	tree_store = gtk_tree_store_new(N_COLUMNS,
	                                GDK_TYPE_PIXBUF, // Icon
	                                G_TYPE_STRING, // URI
	                                G_TYPE_STRING, // Filename
	                                G_TYPE_STRING, // Track number
	                                G_TYPE_STRING, // Title
	                                G_TYPE_STRING, // Artist
	                                G_TYPE_STRING, // Album
	                                G_TYPE_STRING, // Duration
	                                G_TYPE_STRING, // Rating
	                                G_TYPE_INT, // Score (rounded)
	                                G_TYPE_INT, // Play count
	                                G_TYPE_INT, // Skip count
	                                G_TYPE_STRING, // Timestamp / time since last played
	                                G_TYPE_OBJECT); // SongObj pointer
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), GTK_TREE_MODEL(tree_store));
	InterfaceData.tree_store = tree_store;

	// Only now connect to this signal so it doesn't trigger while still setting stuff up
	g_signal_connect(tree_select, "changed", G_CALLBACK(interface_selection_changed_cb), NULL /* user_data */);

	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(tree_view), targets, 1, GDK_ACTION_PRIVATE);
	g_signal_connect(tree_view, "drag-data-received", G_CALLBACK(interface_drag_data_received_cb), NULL /* user_data */);

	text_renderer = gtk_cell_renderer_text_new();
	pixbuf_renderer = gtk_cell_renderer_pixbuf_new();

	// Columns
	column = gtk_tree_view_column_new_with_attributes(NULL, pixbuf_renderer, "pixbuf", STATUS_COLUMN, NULL /* terminator */);
	gtk_tree_view_column_set_resizable(column, FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

	column = gtk_tree_view_column_new_with_attributes("Track", text_renderer, "text", NUMBER_COLUMN, NULL /* terminator */);
	gtk_tree_view_column_set_min_width(column, COLUMN_MIN_WIDTH);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	InterfaceData.track_number_column = column;

	column = gtk_tree_view_column_new_with_attributes("Filepath", text_renderer, "text", URI_COLUMN, NULL /* terminator */);
	gtk_tree_view_column_set_min_width(column, COLUMN_MIN_WIDTH);
	gtk_tree_view_column_set_fixed_width(column, 120);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_visible(column, FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	InterfaceData.uri_column = column;

	column = gtk_tree_view_column_new_with_attributes("Filename", text_renderer, "text", NAME_COLUMN, NULL /* terminator */);
	gtk_tree_view_column_set_min_width(column, COLUMN_MIN_WIDTH);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	InterfaceData.filename_column = column;

	column = gtk_tree_view_column_new_with_attributes("Title", text_renderer, "text", TITLE_COLUMN, NULL /* terminator */);
	gtk_tree_view_column_set_min_width(column, COLUMN_MIN_WIDTH);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	InterfaceData.title_column = column;

	column = gtk_tree_view_column_new_with_attributes("Artist", text_renderer, "text", ARTIST_COLUMN, NULL /* terminator */);
	gtk_tree_view_column_set_min_width(column, COLUMN_MIN_WIDTH);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	InterfaceData.artist_column = column;

	column = gtk_tree_view_column_new_with_attributes("Album", text_renderer, "text", ALBUM_COLUMN, NULL /* terminator */);
	gtk_tree_view_column_set_min_width(column, COLUMN_MIN_WIDTH);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	InterfaceData.album_column = column;

	column = gtk_tree_view_column_new_with_attributes("Duration", text_renderer, "text", DURATION_COLUMN, NULL /* terminator */);
	gtk_tree_view_column_set_min_width(column, COLUMN_MIN_WIDTH);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	InterfaceData.duration_column = column;

	column = gtk_tree_view_column_new_with_attributes("Rating", text_renderer, "text", RATING_COLUMN, NULL /* terminator */);
	gtk_tree_view_column_set_min_width(column, COLUMN_MIN_WIDTH);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	InterfaceData.rating_column = column;

	column = gtk_tree_view_column_new_with_attributes("Score", text_renderer, "text", SCORE_COLUMN, NULL /* terminator */);
	gtk_tree_view_column_set_min_width(column, COLUMN_MIN_WIDTH);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	InterfaceData.score_column = column;

	column = gtk_tree_view_column_new_with_attributes("Play count", text_renderer, "text", PLAYCOUNT_COLUMN, NULL /* terminator */);
	gtk_tree_view_column_set_min_width(column, COLUMN_MIN_WIDTH);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	InterfaceData.playcount_column = column;

	column = gtk_tree_view_column_new_with_attributes("Skip count", text_renderer, "text", SKIPCOUNT_COLUMN, NULL /* terminator */);
	gtk_tree_view_column_set_min_width(column, COLUMN_MIN_WIDTH);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	InterfaceData.skipcount_column = column;

	column = gtk_tree_view_column_new_with_attributes("Last played", text_renderer, "text", LASTPLAYED_COLUMN, NULL /* terminator */);
	gtk_tree_view_column_set_min_width(column, COLUMN_MIN_WIDTH);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	InterfaceData.lastplayed_column = column;

	// Adding tree items
	for (song = wf_song_get_first(); song != NULL; song = wf_song_get_next(song))
	{
		interface_tree_add_item(song);
	}

	// Hide columns if there is no information in them
	interface_show_hide_columns();

	// Connect to player events (run function when statistics are updated)
	wf_library_connect_event_stats_updated(interface_tree_update_all_stats_cb);

	// Progress box
	progress_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(vbox), progress_box, FALSE, TRUE, 0);

	// Progress labels
	label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(progress_box), label, FALSE, TRUE, 8);
	InterfaceData.position_start = GTK_LABEL(label);
	label = gtk_label_new(NULL);
	gtk_box_pack_end(GTK_BOX(progress_box), label, FALSE, TRUE, 8);
	InterfaceData.position_end = GTK_LABEL(label);

	// Progress slider
	slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 10.0);
	gtk_scale_set_draw_value(GTK_SCALE(slider), FALSE);
	g_signal_connect(app, "position-updated", G_CALLBACK(interface_playback_position_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(progress_box), slider, TRUE, TRUE, 0);
	InterfaceData.position_updated_handler = g_signal_connect(slider, "value-changed", G_CALLBACK(interface_position_slider_updated_cb), NULL /* user_data */);
	InterfaceData.position_slider = GTK_RANGE(slider);

	// Set initial properties
	interface_playback_position_cb(app, 0.0, 0.0, NULL /* user_data */);

	// Controls
	controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(vbox), controls, FALSE, TRUE, 0);

	button = gtk_button_new();
	image = interface_get_default_media_icon("media-skip-backward");
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_widget_set_tooltip_text(button, "Play previous song");
	g_signal_connect(button, "clicked", G_CALLBACK(interface_previous_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(controls), button, TRUE, FALSE, 0);

	button = gtk_button_new_from_icon_name(NULL, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_set_tooltip_text(button, "Play/pause the current song");
	gtk_widget_set_can_default(button, TRUE);
	g_signal_connect(button, "clicked", G_CALLBACK(interface_play_pause_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(controls), button, TRUE, FALSE, 0);
	InterfaceData.play_pause_button = button;
	interface_set_button_play();

	button = gtk_button_new();
	image = interface_get_default_media_icon("media-playback-stop");
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_widget_set_tooltip_text(button, "Stop playing");
	g_signal_connect(button, "clicked", G_CALLBACK(interface_stop_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(controls), button, TRUE, FALSE, 0);

	button = gtk_button_new();
	image = interface_get_default_media_icon("media-skip-forward");
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_widget_set_tooltip_text(button, "Play next song");
	g_signal_connect(button, "clicked", G_CALLBACK(interface_next_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(controls), button, TRUE, FALSE, 0);

	// Status area
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 18);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	// Status bar
	status_bar = gtk_label_new(NULL /* text */);
	gtk_widget_set_margin_start(status_bar, 14);
	gtk_widget_set_margin_end(status_bar, 14);
	gtk_widget_set_margin_top(status_bar, 10);
	gtk_widget_set_margin_bottom(status_bar, 10);
	gtk_widget_set_halign(status_bar, GTK_ALIGN_START);
	gtk_widget_set_valign(status_bar, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(status_bar, GTK_ALIGN_CENTER);
	gtk_label_set_ellipsize(GTK_LABEL(status_bar), PANGO_ELLIPSIZE_END);
	gtk_label_set_single_line_mode(GTK_LABEL(status_bar), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), status_bar, FALSE, TRUE, 0);
	g_signal_connect(app, "message", G_CALLBACK(interface_statusbar_update_cb), NULL /* user_data */);
	preference_dialog_connect_close(interface_preferences_closed_cb);
	InterfaceData.status_bar = status_bar;

	// Library stats
	label = gtk_label_new(NULL);
	gtk_widget_set_margin_end(label, 14);
	gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	InterfaceData.library_label = label;

	// Set play/pause button to be the default widget that gets activated on enter key press
	gtk_widget_set_can_focus(InterfaceData.play_pause_button, TRUE);
	interface_window_set_default_widget(InterfaceData.main_window, InterfaceData.play_pause_button);

	g_signal_connect(app, "state-change", G_CALLBACK(interface_playing_state_changed_cb), NULL /* user_data */);

	interface_question_dialog_set_parent(InterfaceData.main_window);

	g_debug("Constructed main window");

	// Show window and it's content
	gtk_widget_show_all(InterfaceData.window_widget);

	// Hide selected widgets (see note [1] at module description)
	for (list = hide_widgets; list != NULL; list = list->next)
	{
		widget = list->data;

		gtk_widget_hide(widget);
	}
	g_list_free(hide_widgets);

	// Deselect all tree items & set toolbar items accordingly
	gtk_tree_selection_unselect_all(tree_select);
	interface_update_toolbar(0 /* items_selected */, total_items);
	interface_update_library_info(0 /* items_selected */, total_items);

	// Show the user we're done setting up the interface
	interface_set_subtitle("Ready");
	InterfaceData.constructed = TRUE;

	// Show startup time
	app_time = wf_app_get_app_time();
	time = interface_utils_round_double_two_decimals_to_str(app_time);
	str = g_strdup_printf("Initialized in %s seconds", time);
	interface_update_status(str);
	g_free(time);
	g_free(str);

	// Start in background?
	if (wf_app_get_background_flag())
	{
		interface_hide_window();
	}
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */

static void
interface_set_info_labels(WidgetSongInfo *info, WfSong *song)
{
	const gchar *name;
	const gchar *title;
	const gchar *artist;
	const gchar *album;

	g_return_if_fail(WIDGET_IS_SONG_INFO(info));

	if (song == NULL)
	{
		widget_song_info_set_title(info, NULL);
		widget_song_info_set_artist(info, NULL);
		widget_song_info_set_album(info, NULL);
	}
	else
	{
		title = wf_song_get_title(song);

		if (title == NULL)
		{
			name = wf_song_get_name_not_empty(song);

			widget_song_info_set_title(info, name);
			widget_song_info_set_artist(info, NULL);
			widget_song_info_set_album(info, NULL);
		}
		else
		{
			artist = wf_song_get_artist(song);
			album = wf_song_get_album(song);

			widget_song_info_set_title(info, title);
			widget_song_info_set_artist(info, artist);
			widget_song_info_set_album(info, album);
		}
	}
}

static void
interface_set_label_previous(WfSong *song)
{
	gchar *tooltip;

	interface_set_info_labels(WIDGET_SONG_INFO(InterfaceData.box_prev), song);

	if (song == NULL)
	{
		gtk_widget_set_tooltip_text(InterfaceData.box_prev, "Nothing has been played yet");
	}
	else
	{
		tooltip = wf_utils_get_pretty_song_msg(song, 0 /* duration */);

		gtk_widget_set_tooltip_text(InterfaceData.box_prev, tooltip);

		g_free(tooltip);
	}
}

static void
interface_set_label_current(WfSong *song)
{
	gchar *tooltip;

	interface_set_info_labels(WIDGET_SONG_INFO(InterfaceData.box_current), song);

	if (song == NULL)
	{
		gtk_widget_set_tooltip_text(InterfaceData.box_current, "Nothing is currently playing");
	}
	else
	{
		tooltip = wf_utils_get_pretty_song_msg(song, 0 /* duration */);

		gtk_widget_set_tooltip_text(InterfaceData.box_current, tooltip);

		g_free(tooltip);
	}
}

static void
interface_set_label_next(WfSong *song)
{
	gchar *tooltip;

	interface_set_info_labels(WIDGET_SONG_INFO(InterfaceData.box_next), song);

	if (song != NULL)
	{
		tooltip = wf_utils_get_pretty_song_msg(song, 0 /* duration */);

		gtk_widget_set_tooltip_text(InterfaceData.box_next, tooltip);

		g_free(tooltip);
	}
}

static void
interface_set_song_labels(WfSong *prev, WfSong *current, WfSong *next)
{
	interface_set_label_previous(prev);
	interface_set_label_current(current);
	interface_set_label_next(next);
}

gboolean
interface_window_is_present(void)
{
	return (InterfaceData.main_window != NULL && InterfaceData.main_window);
}

GtkWindow *
interface_get_parent_window(void)
{
	return InterfaceData.main_window;
}

gboolean
interface_is_active(void)
{
	return gtk_window_is_active(InterfaceData.main_window);
}

gboolean
interface_is_visible(void)
{
	return gtk_widget_is_visible(InterfaceData.window_widget);
}

// Enable/Disable client-side-decoration for the GtkWindow
void
interface_set_use_csd(gboolean use_csd)
{
	InterfaceData.csd = use_csd;
}

/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */

static gboolean
interface_close_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	if (interface_ask_to_quit())
	{
		// Report TRUE: event has been handled
		return TRUE;
	}
	else
	{
		/*
		 * Hide the window first, and then report back to GTK if the window
		 * should be destroyed or if the event is already handled.
		 */
		interface_hide_window();

		// Report FALSE: let event be handled by GTK
		return FALSE;
	}
}

static void
interface_destroy_cb(GtkWidget *object, gpointer user_data)
{
	interface_finalize();
}

static void
interface_preferences_closed_cb(const gchar *message)
{
	// Update statusbar in case there was a message
	if (message != NULL)
	{
		interface_update_status(message);
	}
}

static gboolean
interface_key_pressed_cb(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	gboolean handled = FALSE;

	g_return_val_if_fail(event != NULL, handled);

	if (event->type == GDK_KEY_PRESS)
	{
		switch (event->keyval)
		{
			case GDK_KEY_F11:
				handled = TRUE;
				g_info("Key press: <F11>");

				interface_toggle_fullscreen();
				break;
			case GDK_KEY_Escape:
				handled = TRUE;
				g_info("Key press: <Esc>");

				interface_leave_fullscreen();
				break;
			default:
				// Do nothing
				break;
		}
	}

	return handled;
}

static gboolean
interface_tree_button_pressed_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	gboolean handled = FALSE;

	g_return_val_if_fail(event != NULL, handled);

	if (event->type == GDK_KEY_PRESS)
	{
		switch (event->button)
		{
			case GDK_BUTTON_SECONDARY:
				g_info("Secondary button press");

				handled = interface_tree_pop_menu(widget, event, user_data);
				break;
			default:
				// Do nothing
				break;
		}
	}

	return handled;
}

static void
interface_menu_quit_cb(GtkMenuItem *menuitem, gpointer user_data)
{
	interface_quit_application();
}

static void
interface_open_items_cb(GtkWidget *widget, gpointer user_data)
{
	const gchar *options[] = { "media", "metadata", NULL };
	const gchar *options_file[] = { "audio", "all", "none", NULL };
	const gchar *options_file_labels[] = { "Allow audio files only", "Allow all media files", "Disable file checks (allow all)", NULL };

	const gchar *opt;
	gboolean allow_audio;
	gboolean allow_media;
	gboolean skip_metadata;
	WfLibraryFileChecks checks;

	gint result;
	GSList *files = NULL;
	GtkWidget *dialog;
	GtkFileFilter *filter;

	g_debug("Event open.");

	g_warn_if_fail(GTK_IS_WINDOW(InterfaceData.main_window));

	dialog = gtk_file_chooser_dialog_new("Open one or more audio files",
	                                     InterfaceData.main_window, GTK_FILE_CHOOSER_ACTION_OPEN,
	                                     "Cancel", GTK_RESPONSE_CANCEL,
	                                     "Open", GTK_RESPONSE_OK,
	                                     NULL);

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Audio files");
	gtk_file_filter_add_mime_type(filter, "audio/*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "All files");
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	// Add some options
	gtk_file_chooser_add_choice(GTK_FILE_CHOOSER(dialog), options[0],
	                            NULL /* label */, options_file, options_file_labels);
	gtk_file_chooser_set_choice(GTK_FILE_CHOOSER(dialog), options[0],
	                            options_file[0]);
	gtk_file_chooser_add_choice(GTK_FILE_CHOOSER(dialog), options[1],
	                            "Disable metadata check", NULL /* options */, NULL /* option_labels */);

	g_debug("Running dialog...");
	result = gtk_dialog_run(GTK_DIALOG(dialog));

	if (result == GTK_RESPONSE_OK)
	{
		g_debug("Processing files to library...");

		// Get files
		files = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));

		// Get option for file checks
		opt = gtk_file_chooser_get_choice(GTK_FILE_CHOOSER(dialog), options[0]);
		allow_audio = (g_strcmp0(opt, options_file[0]) == 0);
		allow_media = (g_strcmp0(opt, options_file[1]) == 0);

		if (allow_audio)
		{
			checks = WF_LIBRARY_CHECK_AUDIO;
		}
		else if (allow_media)
		{
			checks = WF_LIBRARY_CHECK_MEDIA;
		}
		else
		{
			checks = WF_LIBRARY_CHECK_NONE;
		}

		// Get option for metadata
		opt = gtk_file_chooser_get_choice(GTK_FILE_CHOOSER(dialog), options[1]);
		skip_metadata = (g_strcmp0(opt, "true") == 0);

		// Destroy the chooser, so the user knows file selection is over
		gtk_widget_destroy(dialog);
		interface_update_gtk_events();

		// Try adding items
		interface_add_items(files, checks, skip_metadata);

		// This list is no longer needed
		g_slist_free_full(files, g_free);
	}
	else
	{
		gtk_widget_destroy(dialog);
	}
}

static void
interface_open_directory_cb(GtkWidget *widget, gpointer user_data)
{
	const gchar *options[] = { "media", "metadata", NULL };
	const gchar *options_file[] = { "audio", "all", NULL };
	const gchar *options_file_labels[] = { "Allow audio files only", "Allow all media files", NULL };

	const gchar *opt;
	gboolean allow_audio;
	gboolean allow_media;
	gboolean skip_metadata;
	WfLibraryFileChecks checks;

	gint result;
	GSList *files = NULL;
	GtkWidget *dialog;

	g_debug("Event open.");

	g_warn_if_fail(GTK_IS_WINDOW(InterfaceData.main_window));

	dialog = gtk_file_chooser_dialog_new("Select one or multiple directorys containing audio files",
	                                     InterfaceData.main_window, GTK_FILE_CHOOSER_ACTION_OPEN,
	                                     "Cancel", GTK_RESPONSE_CANCEL,
	                                     "Select", GTK_RESPONSE_OK,
	                                     NULL);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	gtk_file_chooser_set_action(GTK_FILE_CHOOSER(dialog), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

	// Add some options
	gtk_file_chooser_add_choice(GTK_FILE_CHOOSER(dialog), options[0],
	                            NULL /* label */, options_file, options_file_labels);
	gtk_file_chooser_set_choice(GTK_FILE_CHOOSER(dialog), options[0],
	                            options_file[0]);
	gtk_file_chooser_add_choice(GTK_FILE_CHOOSER(dialog), options[1],
	                            "Disable metadata check", NULL /* options */, NULL /* option_labels */);

	g_debug("Running directory dialog...");
	result = gtk_dialog_run(GTK_DIALOG(dialog));

	if (result == GTK_RESPONSE_OK)
	{
		g_debug("Processing folders to library...");

		// Get files
		files = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));

		// Get option for file checks
		opt = gtk_file_chooser_get_choice(GTK_FILE_CHOOSER(dialog), options[0]);
		allow_audio = (g_strcmp0(opt, options_file[0]) == 0);
		allow_media = (g_strcmp0(opt, options_file[1]) == 0);

		if (allow_audio)
		{
			checks = WF_LIBRARY_CHECK_AUDIO;
		}
		else if (allow_media)
		{
			checks = WF_LIBRARY_CHECK_MEDIA;
		}
		else
		{
			checks = WF_LIBRARY_CHECK_NONE;
		}

		// Get option for metadata
		opt = gtk_file_chooser_get_choice(GTK_FILE_CHOOSER(dialog), options[1]);
		skip_metadata = (g_strcmp0(opt, "true") == 0);

		// Destroy the chooser, so the user knows file selection is over
		gtk_widget_destroy(dialog);
		interface_update_gtk_events();

		// Try adding items
		interface_add_items(files, checks, skip_metadata);

		// This list is no longer needed
		g_slist_free_full(files, g_free);
	}
	else
	{
		gtk_widget_destroy(dialog);
	}
}

static void
interface_remove_items_cb(GtkWidget *widget, gpointer user_data)
{
	const gchar *name, *amount_str;
	WfSong *song;
	gint amount, count = 0;
	gchar *string;
	GList *rows, *l;
	GSList *refs = NULL, *sl;
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GtkTreeRowReference *row_ref;

	g_debug("Event remove from list");

	view = InterfaceData.tree_view;

	selection = gtk_tree_view_get_selection(view);
	amount = gtk_tree_selection_count_selected_rows(selection);

	if (amount <= 0)
	{
		g_info("Nothing selected");
		interface_update_status("Nothing is selected");
		return;
	}

	// Confirm dialog
	if (!interface_remove_confirm_dialog(amount))
	{
		g_info("Action canceled by user");
		return;
	}

	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	// First convert all paths into rowreferences so they don't get invalid if items get removed
	for (l = rows; l != NULL; l = l->next)
	{
		path = l->data;

		if (path == NULL)
		{
			continue;
		}
		else
		{
			// Convert the path into a row reference and add it to the list
			row_ref = gtk_tree_row_reference_new(model, path);
			refs = g_slist_append(refs, row_ref);
		}
	}

	// Free the list containing the selection, we don't need it since we have the row references
	g_list_free_full(rows, (GDestroyNotify) gtk_tree_path_free);

	// Run through all rowreferences and remove them from the tree and the *struct from the library
	for (sl = refs; sl != NULL; sl = sl->next)
	{
		row_ref = sl->data;

		if (row_ref == NULL)
		{
			continue;
		}

		// Allocate here (see note [2] at module description)
		GtkTreeIter iter;

		// Get song
		path = gtk_tree_row_reference_get_path(row_ref);

		if (gtk_tree_model_get_iter(model, &iter, path))
		{
			// Get the song from the tree
			song = interface_tree_get_song_for_iter(model, &iter);
			g_warn_if_fail(WF_IS_SONG(song));

			// Remove from the tree
			gtk_tree_store_remove(InterfaceData.tree_store, &iter);

			if (song != NULL)
			{
				// Copy the song name temporarily to use in a message after the song and it's name are freed
				name = wf_song_get_name(song);
				string = g_strdup(name);

				// Remove the item
				wf_library_remove_song(song);

				g_object_unref(song);
				g_free(string);
				g_debug("Successfully removed %s", string);
				count++;
			}
		}
		else
		{
			g_warning("Song is not valid before removal");
		}

		gtk_tree_path_free(path);
	}

	// Write the library file
	wf_library_write(FALSE);

	interface_show_hide_columns();

	amount_str = wf_utils_string_to_single_multiple(count, "item", "items");
	string = g_strdup_printf("Removed %d %s from the library", count, amount_str);
	interface_update_status(string);
	g_free(string);

	g_slist_free_full(refs, (GDestroyNotify) gtk_tree_row_reference_free);
}

static void
interface_move_items_up_cb(GtkWidget *widget, gpointer user_data)
{
	WfSong *song;
	WfSong *song_prev;
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeIter iter_prev;
	GList *rows, *list;

	view = InterfaceData.tree_view;

	selection = gtk_tree_view_get_selection(view);

	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	if (rows == NULL)
	{
		interface_update_status("Nothing is selected");
	}

	for (list = rows; list != NULL; list = list->next)
	{
		// Get node
		path = list->data;
		gtk_tree_model_get_iter(model, &iter, path);
		iter_prev = iter;

		// Get previous
		if (!gtk_tree_model_iter_previous(model, &iter_prev))
		{
			continue;
		}

		// Get songs
		song = interface_tree_get_song_for_iter(model, &iter);
		song_prev = interface_tree_get_song_for_iter(model, &iter_prev);

		// Move in library
		wf_library_move_before(song, song_prev);

		// Move in tree
		gtk_tree_store_move_before(GTK_TREE_STORE(model), &iter, &iter_prev);

		g_object_unref(song);
		g_object_unref(song_prev);
	}
}

static void
interface_move_items_down_cb(GtkWidget *widget, gpointer user_data)
{
	WfSong *song;
	WfSong *song_next;
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeIter iter_next;
	GList *rows, *list;

	view = InterfaceData.tree_view;

	selection = gtk_tree_view_get_selection(view);

	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	if (rows == NULL)
	{
		interface_update_status("Nothing is selected");
	}

	for (list = rows; list != NULL; list = list->next)
	{
		// Get node
		path = list->data;
		gtk_tree_model_get_iter(model, &iter, path);
		iter_next = iter;

		// Get next
		if (!gtk_tree_model_iter_next(model, &iter_next))
		{
			continue;
		}

		// Get songs
		song = interface_tree_get_song_for_iter(model, &iter);
		song_next = interface_tree_get_song_for_iter(model, &iter_next);

		// Move in library
		wf_library_move_after(song, song_next);

		// Move in tree
		gtk_tree_store_move_after(GTK_TREE_STORE(model), &iter, &iter_next);

		g_object_unref(song);
		g_object_unref(song_next);
	}
}

static void
interface_toggle_activate_cb(GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
	gboolean active;

	g_debug("Toggle activate");

	active = gtk_check_menu_item_get_active(checkmenuitem);

	if (active)
	{
		g_signal_handler_unblock(InterfaceData.tree_view, InterfaceData.tree_row_activate_handler);
	}
	else
	{
		g_signal_handler_block(InterfaceData.tree_view, InterfaceData.tree_row_activate_handler);
	}
}

static void
interface_edit_preferences_cb(GtkWidget *widget, gpointer user_data)
{
	preference_dialog_activate(InterfaceData.main_window);
}

static void
interface_previous_cb(GtkWidget *widget, gpointer user_data)
{
	g_debug("Event previous.");

	wf_app_previous();
}

static void
interface_next_cb(GtkWidget *widget, gpointer user_data)
{
	g_debug("Event skip.");

	wf_app_next();
}

static void
interface_toggle_incognito_cb(GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
	gboolean active;

	g_debug("Toggle incognito");

	active = gtk_check_menu_item_get_active(checkmenuitem);

	wf_app_set_incognito(active);
}

static void
interface_play_pause_cb(GtkWidget *widget, gpointer user_data)
{
	g_debug("Event play/pause.");

	wf_app_play_pause();
}

static void
interface_stop_cb(GtkWidget *widget, gpointer user_data)
{
	g_debug("Event stop.");

	wf_app_stop();
}

static void
interface_library_write_cb(GtkWidget *widget, gpointer user_data)
{
	g_debug("Event library write.");

	if (wf_library_write(TRUE))
	{
		interface_update_status("Successfully written library to disk");
	}
	else
	{
		interface_update_status("Failed to write library");
	}
}

static void
interface_metadata_refresh_cb(GtkWidget *widget, gpointer user_data)
{
	gint amount;

	g_debug("Event refresh metadata.");

	interface_update_status("Refreshing metadata...");

	amount = wf_library_update_metadata();

	if (amount > 0)
	{
		g_debug("%d items have been updated, refreshing interface...", amount);

		interface_tree_update_song_data(interface_tree_update_song_metadata_cb);

		interface_show_hide_columns();
	}

	interface_update_status("Metadata refreshed");
}

static void
interface_fullscreen_toggle_cb(GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
	g_debug("Event fullscreen");

	interface_toggle_fullscreen();
}

static void
interface_toggle_toolbar_cb(GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
	gboolean active;

	g_debug("Toggle toolbar");

	active = gtk_check_menu_item_get_active(checkmenuitem);

	interface_show_toolbar(active);
}

static void
interface_hide_window_cb(GtkWidget *widget, gpointer user_data)
{
	interface_hide_window();
}

static void interface_help_about_cb(GtkWidget *widget, gpointer user_data)
{
	g_debug("Event about dialog.");

	about_dialog_activate(InterfaceData.main_window);
}

static void
interface_stop_after_song_cb(GtkWidget *widget, gpointer user_data)
{
	WfSong *song;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreePath *path;
	GList *rows;
	GList *l;

	g_debug("Event stop after song.");

	selection = gtk_tree_view_get_selection(InterfaceData.tree_view);
	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	if (rows == NULL)
	{
		// Nothing selected, toggle stop on current song
		wf_app_toggle_stop(NULL /* song */);

		interface_update_status("Stopping playback after current song");
		return;
	}

	for (l = rows; l != NULL; l = l->next)
	{
		path = l->data;

		song = interface_tree_get_song_for_path(model, path);

		wf_app_toggle_stop(song);

		g_object_unref(song);
	}

	interface_update_status("Toggled stop flag for current selection");

	g_list_free_full(rows, (GDestroyNotify) gtk_tree_path_free);
}

static void
interface_selection_changed_cb(GtkTreeSelection *tree_selection, gpointer user_data)
{
	gint selected, total;

	g_debug("Selection changed");

	selected = gtk_tree_selection_count_selected_rows(tree_selection);
	total = wf_song_get_count();

	interface_update_toolbar(selected, total);
	interface_update_library_info(selected, total);
}

static void
interface_select_all_cb(GtkWidget *widget, gpointer user_data)
{
	GtkTreeSelection *treeSelection;

	g_debug("Selecting all");

	treeSelection = gtk_tree_view_get_selection(InterfaceData.tree_view);

	gtk_tree_selection_select_all(treeSelection);
}

static void
interface_select_none_cb(GtkWidget *widget, gpointer user_data)
{
	GtkTreeSelection *treeSelection;

	g_debug("Selecting none");

	treeSelection = gtk_tree_view_get_selection(InterfaceData.tree_view);

	gtk_tree_selection_unselect_all(treeSelection);
}

static void
interface_toggle_queue_cb(GtkWidget *widget, gpointer user_data)
{
	WfSong *song;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreePath *path;
	GList *rows;
	GList *l;

	g_debug("Event toggle queue.");

	selection = gtk_tree_view_get_selection(InterfaceData.tree_view);
	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	if (rows == NULL)
	{
		interface_update_status("Nothing selected");
		return;
	}

	for (l = rows; l != NULL; l = l->next)
	{
		path = l->data;

		song = interface_tree_get_song_for_path(model, path);

		wf_app_toggle_queue(song);

		g_object_unref(song);
	}

	interface_update_status("Toggled current selected songs in queue");

	g_list_free_full(rows, (GDestroyNotify) gtk_tree_path_free);
}

static void
interface_redraw_next_cb(GtkWidget *widget, gpointer user_data)
{
	wf_app_redraw_next_song();
}

static void
interface_edit_rating_cb(GtkWidget *widget, gpointer user_data)
{
	WfSong *song;
	gint amount, rating, altered = 0;
	gchar *str;
	GList *rows, *l;
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;

	g_debug("Event edit rating");

	view = InterfaceData.tree_view;

	selection = gtk_tree_view_get_selection(view);
	amount = gtk_tree_selection_count_selected_rows(selection);

	if (amount <= 0)
	{
		g_info("Nothing selected");
		interface_update_status("Nothing is selected");
		return;
	}
	else
	{
		rows = gtk_tree_selection_get_selected_rows(selection, &model);
	}

	// Get value from user
	rating = interface_edit_rating_dialog(InterfaceData.main_window, amount);

	// Scale ratings to back-end range 0-100 (see note [3] at module description)
	rating *= 10;

	if (rating >= 0)
	{
		// Go over each selected item
		for (l = rows; l != NULL; l = l->next)
		{
			path = l->data;

			if ( path == NULL )
			{
				continue;
			}

			// Allocate here (see note [2] at module description)
			GtkTreeIter iter;

			if (!gtk_tree_model_get_iter(model, &iter, path))
			{
				g_warning("Invalid iter while gettings items for queue");
				continue;
			}

			// Get the song from the tree
			song = interface_tree_get_song_for_iter(model, &iter);

			// Update song
			wf_song_set_rating(song, rating);

			// Update interface
			interface_tree_update_song_stat_cb(InterfaceData.tree_store, &iter, song);

			g_object_unref(song);
			altered++;
		}
	}

	if (altered > 0)
	{
		wf_library_write(TRUE);

		str = g_strdup_printf("Update rating of %d %s", altered, wf_utils_string_to_single_multiple(altered, "item", "items"));
		interface_update_status(str);
		g_free(str);
	}
	else
	{
		interface_update_status("No ratings updated");
	}

	g_list_free_full(rows, (GDestroyNotify) gtk_tree_path_free);
}

static void
interface_scroll_to_playing_cb(GtkWidget *widget, gpointer user_data)
{
	WfSong *song;
	GtkTreePath *path;
	GtkTreeIter iter = { 0 };

	song = InterfaceData.current_song;

	if (song == NULL)
	{
		g_info("Nothing currently playing");
		return;
	}

	// Get the matching row
	if (!interface_tree_get_iter_for_song(song, &iter))
	{
		g_warning("Could not get row to select");
		return;
	}

	// Get a matching path
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(InterfaceData.tree_store), &iter);

	// Scroll to it
	interface_tree_scroll_to_row(path);
}

static void
interface_dialog_stop_cb(GtkEntry *entry, gpointer user_data)
{
	const GtkResponseType response = GTK_RESPONSE_ACCEPT;
	GtkDialog *dialog = user_data;

	g_return_if_fail(GTK_IS_DIALOG(dialog));

	gtk_dialog_response(dialog, response);
}

static void
interface_dialog_value_changed_cb(GtkWidget *widget, gpointer user_data)
{
	GtkWidget *other_widget = user_data;
	gdouble value;

	g_return_if_fail(GTK_IS_WIDGET(widget));
	g_return_if_fail(GTK_IS_WIDGET(other_widget));

	if (GTK_IS_RANGE(widget))
	{
		value = gtk_range_get_value(GTK_RANGE(widget));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(other_widget), value);
	}
	else if (GTK_IS_SPIN_BUTTON(widget))
	{
		value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
		gtk_range_set_value(GTK_RANGE(other_widget), value);
	}
}

static void
interface_items_are_added_cb(WfSong *song, gint item, gint total)
{
	gdouble complete = 0.0;

	if (song == NULL)
	{
		interface_progress_window_update(0.0);
	}
	else
	{
		if (item > 0 && total > 0)
		{
			complete = (gfloat) item / (gfloat) total * 100.0;
		}

		interface_tree_add_item(song);
		interface_progress_window_update(complete);
	}
}

static void
interface_update_song_info_cb(WfApp *app, WfSong *song_previous, WfSong *song_current, WfSong *song_next, gpointer user_data)
{
	InterfaceData.current_song = song_current;

	interface_set_song_labels(song_previous, song_current, song_next);

	interface_tree_update_all_song_icons();
}

static void
interface_statusbar_update_cb(WfApp *app, const gchar *message, gpointer user_data)
{
	interface_update_status(message);
}

static void
interface_playing_state_changed_cb(WfApp *app, WfAppStatus state, gdouble duration, gpointer user_data)
{
	GtkWidget *widget;
	GSList *l;
	gchar *msg;
	gboolean playing;

	switch (state)
	{
		case WF_APP_INIT:
		case WF_APP_READY:
			msg = "Ready";
			break;
		case WF_APP_PLAYING:
			msg = "Playing";
			break;
		case WF_APP_PAUSED:
			msg = "Paused";
			break;
		case WF_APP_STOPPED: // Actually means there's an error
			msg = "Stopped";
			break;
		default:
			msg = NULL;
			break;
	}

	interface_set_subtitle(msg);

	playing = (state == WF_APP_PLAYING || state == WF_APP_PAUSED);

	// Change toolbar sensitivity (see note [1] at module description)
	for (l = InterfaceData.playing_tools; l != NULL; l = l->next)
	{
		widget = GTK_WIDGET(l->data);

		gtk_widget_set_sensitive(widget, playing);
	}

	// Set right button icon
	if (state == WF_APP_PLAYING)
	{
		interface_set_button_pause();
	}
	else
	{
		interface_set_button_play();
	}

	// Update playback threshold on slider (if changed)
	interface_update_position_slider_marks();
}

static void
interface_playback_position_cb(WfApp *app, gdouble position, gdouble duration, gpointer user_data)
{
	interface_update_playback_position(position, duration);
}

static void
interface_position_slider_updated_cb(GtkRange *range, gpointer user_data)
{
	gdouble value;

	g_return_if_fail(GTK_IS_RANGE(range));

	value = gtk_range_get_value(range);

	wf_app_set_playback_percentage(value);
}

static void
interface_tree_update_song_stat_cb(GtkTreeStore *store, GtkTreeIter *iter, WfSong *song)
{
	gint rating;
	gchar *last_played;
	gchar *str;

	g_return_if_fail(store != NULL);
	g_return_if_fail(iter != NULL);
	g_return_if_fail(song != NULL);

	if (interface_settings_get_last_played_timestamp()) // If bool is %TRUE
	{
		last_played = wf_song_get_played_on_as_string(song);
	}
	else
	{
		last_played = wf_song_get_last_played_as_string(song);
	}

	// Get rating
	rating = wf_song_get_rating(song);

	// Scale and round ratings to range 0-10 (see note [3] at module description)
	rating = (rating + 5) / 10;

	// Represent the rating as a string (and hide it when zero)
	str = (rating == 0) ? NULL : g_strdup_printf("%d", rating);

	// Set the values
	gtk_tree_store_set(store, iter,
	                   RATING_COLUMN, str,
	                   SCORE_COLUMN, wf_utils_round(wf_song_get_score(song)), // Round the float so it shows the right score in the interface
	                   PLAYCOUNT_COLUMN, wf_song_get_play_count(song),
	                   SKIPCOUNT_COLUMN, wf_song_get_skip_count(song),
	                   LASTPLAYED_COLUMN, last_played,
	                   -1);

	g_free(str);
	g_free(last_played);
}

static void
interface_tree_update_song_metadata_cb(GtkTreeStore *store, GtkTreeIter *iter, WfSong *song)
{
	gint track;
	gchar *duration;
	gchar *str = NULL;

	g_return_if_fail(store != NULL);
	g_return_if_fail(iter != NULL);
	g_return_if_fail(song != NULL);

	track = wf_song_get_track_number(song);
	str = (track > 0) ? g_strdup_printf("%d", track) : NULL;
	duration = wf_song_get_duration_string(song);

	/*
	 * Do not fill the column with useless zeros if track numbers
	 * aren't set for at least some of the songs.
	 * The rest of the data are strings and therefore
	 * are not set (and appear empty) if the pointer is NULL.
	 */
	gtk_tree_store_set(InterfaceData.tree_store, iter,
	                   NUMBER_COLUMN, str,
	                   TITLE_COLUMN, wf_song_get_title(song),
	                   ARTIST_COLUMN, wf_song_get_artist(song),
	                   ALBUM_COLUMN, wf_song_get_album(song),
	                   DURATION_COLUMN, duration,
	                   -1);

	g_free(str);
	g_free(duration);
}

static void
interface_tree_update_all_stats_cb(void)
{
	// Update stats of all items
	g_info("Updating song statistics in interface");

	interface_tree_update_song_data(interface_tree_update_song_stat_cb);
}

static void
interface_tree_update_all_song_icons(void)
{
	WfSong *song;
	GtkTreeIter iter;
	GtkTreeStore *store = InterfaceData.tree_store;
	GtkTreeModel *model = GTK_TREE_MODEL(store);

	if (wf_song_get_count() <= 0)
	{
		// No items present; nothing to update
		return;
	}
	else if (!gtk_tree_model_get_iter_first(model, &iter))
	{
		g_warning("Could not get first row from tree");
		return;
	}

	// Update for each iter
	do
	{
		// Get the song from the iter
		song = interface_tree_get_song_for_iter(model, &iter);

		// Now update the icon
		interface_tree_update_song_status(store, &iter, song);

		g_object_unref(song);
	} while (gtk_tree_model_iter_next(model, &iter));
}

static void
interface_tree_update_song_status(GtkTreeStore *store, GtkTreeIter *iter, WfSong *song)
{
	SongStatusIcon status = STATUS_ICON_NONE;
	GdkPixbuf *icon;

	g_return_if_fail(store != NULL);
	g_return_if_fail(iter != NULL);
	g_return_if_fail(song != NULL);

	if (wf_song_get_queued(song))
	{
		status = STATUS_ICON_QUEUED;
	}
	else if (wf_song_get_stop_flag(song))
	{
		status = STATUS_ICON_STOP;
	}
	else
	{
		switch (wf_song_get_status(song))
		{
			case WF_SONG_AVAILABLE:
				break;
			case WF_SONG_PLAYING:
				status = STATUS_ICON_PLAYING;
				break;
			default:
				status = STATUS_ICON_INVALID;
		}
	}

	icon = interface_get_pixbuf_icon(status);

	// Set the values
	gtk_tree_store_set(store, iter,
	                   URI_COLUMN, wf_song_get_uri(song),
	                   NAME_COLUMN, wf_song_get_name(song),
	                   STATUS_COLUMN, icon,
	                   SONGOBJ_COLUMN, song,
	                   -1);

	g_clear_object(&icon);
}

static void
interface_tree_scroll_to_row(GtkTreePath *path)
{
	GtkTreeSelection *selection;

	g_return_if_fail(path != NULL);

	// Scroll to item
	gtk_tree_view_scroll_to_cell(InterfaceData.tree_view, path, NULL /* column */, TRUE, 0.5, 0.0);

	// Get current selection
	selection = gtk_tree_view_get_selection(InterfaceData.tree_view);

	// Deselect selection
	gtk_tree_selection_unselect_all(selection);

	// Only select one item
	gtk_tree_selection_select_range(selection, path /* start */, path /* end */);
}

static void
interface_tree_activated_cb(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	// Row activated, check item, get song, play song

	WfSong *song;
	GtkTreeModel *model;

	g_debug("Getting activated row");

	model = gtk_tree_view_get_model(view);

	song = interface_tree_get_song_for_path(model, path);

	wf_app_open(song);

	g_object_unref(song);
}

static void interface_drag_data_received_cb(GtkWidget *widget,
                                            GdkDragContext *context,
                                            gint x,
                                            gint y,
                                            GtkSelectionData *data,
                                            guint info,
                                            guint time,
                                            gpointer user_data)
{
	gchar **files;
	gint amount;

	g_info("Drag & drop data received");

	files = gtk_selection_data_get_uris(data);

	if (files == NULL)
	{
		gtk_drag_finish(context, FALSE, FALSE, time);
	}
	else
	{
		gtk_drag_finish(context, TRUE, FALSE, time);

		// Create progress window
		interface_progress_window_create("Adding new items. Standy by...");

		// Add items
		amount = wf_library_add_strv(files, interface_items_are_added_cb, 0, FALSE);

		// Progress done
		interface_progress_window_destroy();

		g_strfreev(files);

		interface_report_items_added(amount);
	}
}

static gboolean
interface_handle_notification_cb(WfApp *app, WfSong *song, gint64 duration, gpointer user_data)
{
	NotificationSetting setting;

	if (song == NULL)
	{
		// Back-end should handle removal of notification
		return FALSE;
	}
	else
	{
		setting = interface_settings_get_notification();

		if (setting == NOTIFICATIONS_ALWAYS ||
		    (setting == NOTIFICATIONS_UNFOCUSED_ONLY && !interface_is_active()) ||
		    (setting == NOTIFICATIONS_HIDDEN_ONLY && !interface_is_visible()))
		{
			// Back-end should handle sending notifications
			return FALSE;
		}
	}

	// Notifications "handled": do not propagate the event further
	return TRUE;
}

/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

gboolean
interface_activate(GApplication *app)
{
	WfApp *wf_app = WF_APP(app);

	g_return_val_if_fail(WF_IS_APP(app), FALSE);

	InterfaceData.application = wf_app;

	if (interface_window_is_present())
	{
		g_info("Application activation: Showing main window");
		interface_present_window();

		return FALSE;
	}
	else
	{
		interface_construct(wf_app);

		return TRUE;
	}
}

void
interface_startup(GApplication *app)
{
	gtk_init(NULL /* argc */, NULL /* argv */);

	g_signal_connect(app, "notification", G_CALLBACK(wf_app_default_notification_handler), NULL /* user_data */);
	g_signal_connect(app, "player-notification", G_CALLBACK(interface_handle_notification_cb), NULL /* user_data */);

	interface_settings_init();
}

void
interface_shutdown(GApplication *app)
{
	interface_destruct();
}

static void
interface_quit_application(void)
{
	if (!interface_ask_to_quit())
	{
		// Hide the window first (see note [4] at module description)
		interface_hide_window();

		g_debug("Quitting...");
		interface_destruct();
	}
}

static void
interface_show_window(void)
{
	gtk_widget_show(InterfaceData.window_widget);
}

static void
interface_hide_window(void)
{
	// Hide windows
	preference_dialog_hide();
	gtk_widget_hide(InterfaceData.window_widget);

	// Update GTK
	interface_update_gtk_events();
}

// Force focus steal; Only use when the user expects it
static void
interface_present_window(void)
{
	interface_show_window();

	gtk_window_present(InterfaceData.main_window);
	gdk_notify_startup_complete();
}

static void
interface_toggle_fullscreen(void)
{
	if (InterfaceData.is_fullscreen)
	{
		interface_leave_fullscreen();
	}
	else
	{
		interface_enter_fullscreen();
	}
}

static void
interface_leave_fullscreen(void)
{
	InterfaceData.is_fullscreen = FALSE;

	// Enter fullscreen
	gtk_window_unfullscreen(InterfaceData.main_window);

	// Hide alternative subtitle as the headerbar shows automatically (if in use)
	if (InterfaceData.header_bar != NULL)
	{
		gtk_widget_hide(InterfaceData.subtitle_box);
	}
}

static void
interface_enter_fullscreen(void)
{
	InterfaceData.is_fullscreen = TRUE;

	// Enter fullscreen
	gtk_window_fullscreen(InterfaceData.main_window);

	// Use alternative subtitle as the headerbar hides automatically
	gtk_widget_show(InterfaceData.subtitle_box);
}

static void
interface_show_toolbar(gboolean show)
{
	g_return_if_fail(InterfaceData.toolbar != NULL);

	if (show)
	{
		gtk_widget_show(InterfaceData.toolbar);
	}
	else
	{
		gtk_widget_hide(InterfaceData.toolbar);
	}
}

static void
interface_update_playback_position(gdouble position, gdouble duration)
{
	gchar str_start[] = "00:00.0";
	gchar str_end[] = "00:00  ";
	gulong size_start = 8;
	gulong size_end = 6;
	gdouble percentage;
	guint pos = (guint) position;
	guint dur = (guint) duration;
	guint decimal;
	guint seconds;
	guint minutes;

	// Block slider update signal
	g_signal_handler_block(InterfaceData.position_slider, InterfaceData.position_updated_handler);

	if (position < 0.0 || duration <= 0.0)
	{
		gtk_range_set_value(GTK_RANGE(InterfaceData.position_slider), 0.0);
		gtk_scale_clear_marks(GTK_SCALE(InterfaceData.position_slider));
		gtk_widget_set_sensitive(GTK_WIDGET(InterfaceData.position_slider), FALSE);

		gtk_label_set_text(InterfaceData.position_start, str_start);
		gtk_label_set_text(InterfaceData.position_end, str_end);
	}
	else
	{
		// Calculate numbers and fill buffer for position
		minutes = pos / 60;
		seconds = pos % 60;
		decimal = (position * 10) - (pos * 10);
		g_snprintf(str_start, size_start, "%02u:%02u.%01u", minutes, seconds, decimal);

		// Calculate numbers and fill buffer for duration
		minutes = dur / 60;
		seconds = dur % 60;
		g_snprintf(str_end, size_end, "%02u:%02u", minutes, seconds);

		// Calculate the progress percentage
		percentage = (position / duration) * 100.0;

		// Set the label text
		gtk_label_set_text(InterfaceData.position_start, str_start);
		gtk_label_set_text(InterfaceData.position_end, str_end);

		// Set slider position (seconds)
		gtk_range_set_value(InterfaceData.position_slider, percentage);

		// Set sensitivity so the user can interact
		gtk_widget_set_sensitive(GTK_WIDGET(InterfaceData.position_slider), TRUE);
	}

	// Re-enable slider update signal
	g_signal_handler_unblock(InterfaceData.position_slider, InterfaceData.position_updated_handler);
}

static void
interface_update_position_slider_marks(void)
{
	GtkStyleContext *style;
	gint min;
	gint max;

	// First clear the old marks
	gtk_scale_clear_marks(GTK_SCALE(InterfaceData.position_slider));

	// Get the positions of the marks
	min = wf_settings_static_get_double(WF_SETTING_MIN_PLAYED_FRACTION);
	max = wf_settings_static_get_double(WF_SETTING_FULL_PLAYED_FRACTION);

	// Add marks
	gtk_scale_add_mark(GTK_SCALE(InterfaceData.position_slider), min, GTK_POS_TOP, NULL);
	gtk_scale_add_mark(GTK_SCALE(InterfaceData.position_slider), max, GTK_POS_TOP, NULL);

	// Remove the pointed style of the button on the slider
	style = gtk_widget_get_style_context(GTK_WIDGET(InterfaceData.position_slider));
	gtk_style_context_remove_class(style, "marks-before");
}

void interface_tree_add_item(WfSong *song)
{
	GtkTreeIter iter = { 0 };

	g_return_if_fail(WF_IS_SONG(song));

	// Add item
	gtk_tree_store_append(InterfaceData.tree_store, &iter, NULL /* parent */);

	// Fill the row with all other information (possibly using callbacks)
	interface_tree_update_song_status(InterfaceData.tree_store, &iter, song);
	interface_tree_update_song_stat_cb(InterfaceData.tree_store, &iter, song);
	interface_tree_update_song_metadata_cb(InterfaceData.tree_store, &iter, song);
}

static void
interface_add_items(GSList *files, WfLibraryFileChecks checks, gboolean skip_metadata)
{
	gint amount = 0;

	if (files != NULL)
	{
		// Create progress window
		interface_progress_window_create("Adding new items. Standy by...");

		amount = wf_library_add_uris(files, interface_items_are_added_cb, checks, skip_metadata); // transfer full

		// Progress done
		interface_progress_window_destroy();
	}

	interface_report_items_added(amount);
}

static void
interface_update_toolbar(gint items_selected, gint items_total)
{
	GtkWidget *widget;
	GSList *l;
	gboolean clickable;

	g_info("Updated toolbar button sensitivity");

	// Change labels
	if (items_selected == 1)
	{
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(InterfaceData.remove), "Remove selected track from the library");
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(InterfaceData.queue), "Toggle selected track in the queue");
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(InterfaceData.stop), "Toggle stop flag for selected track");
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(InterfaceData.edit_rating), "Set rating for all selected track");
	}
	else
	{
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(InterfaceData.remove), "Remove selected tracks from the library");
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(InterfaceData.queue), "Toggle selected tracks in the queue");
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(InterfaceData.stop), "Toggle stop flag for selected tracks");
		gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(InterfaceData.edit_rating), "Set rating for all selected tracks");
	}

	clickable = (items_selected > 0);

	// Change toolbar sensitivity (see note [1] at module description)
	for (l = InterfaceData.selection_tools; l != NULL; l = l->next)
	{
		widget = GTK_WIDGET(l->data);

		gtk_widget_set_sensitive(widget, clickable);
	}
}

static void
interface_update_library_info(gint selected, gint total)
{
	gchar *txt = g_strdup_printf("%d/%d", selected, total);

	gtk_label_set_text(GTK_LABEL(InterfaceData.library_label), txt);

	g_free(txt);
}

static void
interface_report_items_added(gint amount)
{
	const gchar *amount_str;
	gchar *string;

	if (amount > 0)
	{
		// Update columns
		interface_show_hide_columns();

		// Report how many song were added
		amount_str = wf_utils_string_to_single_multiple(amount, "item", "items");
		string = g_strdup_printf("Added %d %s to the library", amount, amount_str);
		interface_update_status(string);
		g_free(string);
	}
	else
	{
		interface_update_status("Did not add any items");
	}
}

static void
interface_tree_update_song_data(func_tree_update_item cb_func)
{
	WfSong *song;
	GtkTreeStore *store;

	if (!InterfaceData.constructed)
	{
		return;
	}

	store = InterfaceData.tree_store;

	for (song = wf_song_get_first(); song != NULL; song = wf_song_get_next(song))
	{
		GtkTreeIter iter;

		if (interface_tree_get_iter_for_song(song, &iter))
		{
			// Give the callback function the information it needs to update the tree iter
			cb_func(store, &iter, song);
		}
	}

	g_debug("Tree store metadata is now updated");
}

// Set a GtkTreeIter for a given song. Returns %TRUE on success, %FALSE otherwise
static gboolean
interface_tree_get_iter_for_song(WfSong *song, GtkTreeIter *iter)
{
	WfSong *song_item;
	GtkTreeModel *model = GTK_TREE_MODEL(InterfaceData.tree_store);

	g_return_val_if_fail(GTK_IS_TREE_MODEL(model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	if (song == NULL)
	{
		return FALSE;
	}

	// Get first row (first iter)
	gtk_tree_model_get_iter_first(model, iter);

	// Iterate over all items
	do
	{
		// Get the song from the iter
		song_item = interface_tree_get_song_for_iter(model, iter);

		if (song == song_item)
		{
			// Found matching item, @iter is already set
			return TRUE;
		}

		g_object_unref(song_item);
	} while (gtk_tree_model_iter_next(model, iter));

	return FALSE;
}

static WfSong *
interface_tree_get_song_for_iter(GtkTreeModel *model, GtkTreeIter *iter)
{
	gpointer item = NULL;

	// Get the pointer from the tree (this will increase the reference count)
	gtk_tree_model_get(model, iter, SONGOBJ_COLUMN, &item, -1);

	return (WfSong *) item;
}

static WfSong *
interface_tree_get_song_for_path(GtkTreeModel *model, GtkTreePath *path)
{
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter(model, &iter, path))
	{
		g_warning("Invalid iter while gettings items for queue");
		return NULL;
	}

	return interface_tree_get_song_for_iter(model, &iter);
}

void
interface_show_hide_columns(void)
{
	gboolean all_have_titles, all_have_artists, empty_track_numbers, empty_titles, empty_artists, empty_albums, empty_durations;
	gint count = wf_song_get_count();

	if (count <= 0)
	{
		/*
		 * Empty library (could be first run of the application), so show the
		 * metadata columns because it looks better; Only show filename column
		 * if any track don't have metadata.
		 */
		gtk_tree_view_column_set_visible(InterfaceData.filename_column, FALSE);
		gtk_tree_view_column_set_visible(InterfaceData.track_number_column, TRUE);
		gtk_tree_view_column_set_visible(InterfaceData.title_column, TRUE);
		gtk_tree_view_column_set_visible(InterfaceData.artist_column, TRUE);
		gtk_tree_view_column_set_visible(InterfaceData.album_column, TRUE);
		gtk_tree_view_column_set_visible(InterfaceData.duration_column, TRUE);

		return;
	}

	// Get column information
	wf_library_update_column_info();

	// Get what columns or empty
	empty_track_numbers = wf_library_track_number_column_is_empty();
	empty_titles = wf_library_title_column_is_empty();
	empty_artists = wf_library_artist_column_is_empty();
	empty_albums = wf_library_album_column_is_empty();
	empty_durations = wf_library_duration_column_is_empty();

	// Get what columns or full
	all_have_titles = wf_library_title_column_is_full();
	all_have_artists = wf_library_artist_column_is_full();

	// Show columns based on the inverted values fetched in the block above
	gtk_tree_view_column_set_visible(InterfaceData.track_number_column, !empty_track_numbers);
	gtk_tree_view_column_set_visible(InterfaceData.title_column, !empty_titles);
	gtk_tree_view_column_set_visible(InterfaceData.artist_column, !empty_artists);
	gtk_tree_view_column_set_visible(InterfaceData.album_column, !empty_albums);
	gtk_tree_view_column_set_visible(InterfaceData.duration_column, !empty_durations);

	if (all_have_titles && all_have_artists) // Conditions to hide filenames
	{
		gtk_tree_view_column_set_visible(InterfaceData.filename_column, FALSE);
	}
	else
	{
		gtk_tree_view_column_set_visible(InterfaceData.filename_column, TRUE);
	}
}

void
interface_update_status(const gchar *message)
{
	gtk_label_set_text(GTK_LABEL(InterfaceData.status_bar), message);
}

static gboolean
interface_ask_to_quit(void)
{
	DialogResponse action;

	g_debug("Event quit.");

	if (InterfaceData.current_song != NULL)
	{
		// Something is playing, so ask the user to quit the application, close window, or do nothing
		action = interface_close_confirm(InterfaceData.main_window);
	}
	else
	{
		// The player is ready, so quit the application immediately
		action = DIALOG_QUIT;
	}

	if (action == DIALOG_CANCEL)
	{
		g_debug("Dialog response: cancel");

		return TRUE;
	}
	else if (action == DIALOG_CLOSE)
	{
		// Close
		interface_hide_window();

		// Returning true indicates to gtk that the delete-event has been handled so gtk doesn't do that anymore
		return TRUE;
	}

	g_debug("Interface quit confirmed");
	return FALSE;
}

static gboolean
interface_remove_confirm_dialog(gint amount)
{
	gboolean result;
	gchar *message, *msg_part;

	if (amount <= 0)
	{
		return TRUE;
	}
	else if (amount == 1)
	{
		msg_part = g_strdup("this item?");
	}
	else
	{
		msg_part = g_strdup_printf("these %d items?", amount);
	}

	message = g_strdup_printf("Removing songs from the library\n"
	                          "also removes their statistics.\n"
	                          "After deletion these cannot be recovered.\n"
	                          "\nAre you sure you want to remove %s", msg_part);
	g_free(msg_part);

	result = interface_question_dialog_run(message);

	g_free(message);

	return result;
}

// Returns: a rating (integer) entered by the user or -1 on error
static gint
interface_edit_rating_dialog(GtkWindow *parent, gint amount)
{
	const gint error = -1;
	gint value, rating;
	gchar *message;
	GtkAdjustment *adjustment;
	GtkWidget *content, *hbox, *vbox, *icon, *label, *spin_button, *slider;
	GtkWidget *dialog_widget;
	GtkWindow *dialog_window;
	GtkDialog *dialog;

	g_return_val_if_fail(amount > 0, error);
	g_warn_if_fail(GTK_IS_WINDOW(parent));

	if (amount == 1)
	{
		message = g_strdup("Enter a new rating for this song");
	}
	else
	{
		message = g_strdup_printf("Enter a new rating for these %d songs", amount);
	}

	dialog_widget = gtk_dialog_new();
	dialog_window = GTK_WINDOW(dialog_widget);
	dialog = GTK_DIALOG(dialog_widget);

	gtk_window_set_title(dialog_window, "Change song rating");
	gtk_window_set_transient_for(dialog_window, parent);
	gtk_window_set_modal(dialog_window, TRUE);
	gtk_window_set_destroy_with_parent(dialog_window, TRUE);
	gtk_window_set_resizable(dialog_window, FALSE);

	gtk_dialog_add_button(dialog, "_OK", GTK_RESPONSE_OK);
	gtk_dialog_add_button(dialog, "_Cancel", GTK_RESPONSE_CANCEL);

	// Set dialog properties
	content = gtk_dialog_get_content_area(dialog);
	gtk_container_set_border_width(GTK_CONTAINER(content), 12);
	gtk_box_set_spacing(GTK_BOX(content), 18);
	gtk_window_set_modal(dialog_window, TRUE);
	gtk_dialog_set_default_response(dialog, GTK_RESPONSE_OK);

	// Create a container for the content
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(content), hbox, FALSE, TRUE, 0);

	// Create the icon
	icon = gtk_image_new_from_icon_name("", GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, TRUE, 8);

	// Create the main content box
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 4);

	// Create a label and add it to the container
	label = gtk_label_new(message);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);

	// Create adjustment to use
	adjustment = gtk_adjustment_new(0, // Value
	                                0, // Lowest value
	                                10, // Highest value
	                                1, // Step Increment
	                                1, // Page Increment
	                                0); // Page Size

	// Create slider widget
	slider = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adjustment);
	gtk_scale_set_digits(GTK_SCALE(slider), 0);
	gtk_scale_set_draw_value(GTK_SCALE(slider), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), slider, FALSE, TRUE, 0);

	// Create adjustment to use
	adjustment = gtk_adjustment_new(0, // Value
	                                0, // Lowest value
	                                10, // Highest value
	                                1, // Step Increment
	                                2, // Page Increment
	                                0); // Page Size

	// Create the input widget
	spin_button = gtk_spin_button_new(adjustment, 1, 0);
	g_signal_connect(spin_button, "activate", G_CALLBACK(interface_dialog_stop_cb), dialog);
	gtk_box_pack_start(GTK_BOX(vbox), spin_button, FALSE, TRUE, 0);

	// Connect to value-changed events
	g_signal_connect(slider, "value-changed", G_CALLBACK(interface_dialog_value_changed_cb), spin_button);
	g_signal_connect(spin_button, "value-changed", G_CALLBACK(interface_dialog_value_changed_cb), slider);

	// Focus spin button
	gtk_window_set_focus(dialog_window, spin_button);

	// Show and run
	gtk_widget_show_all(hbox);
	value = gtk_dialog_run(dialog);

	// Check respond, destroy widget and return
	if (value == GTK_RESPONSE_OK || value == GTK_RESPONSE_ACCEPT)
	{
		rating = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_button));
	}
	else
	{
		// Return error valid even though it is not really an error
		rating = error;
	}

	gtk_widget_destroy(dialog_widget);
	g_free(message);

	// Otherwise return the new value
	return rating;
}

// Runs a dialog to ask the user to keep playing and only close the window, or to fully quit the application
static DialogResponse interface_close_confirm(GtkWindow *parent)
{
	GtkDialog *dialog;
	GtkWindow *dialog_window;
	GtkWidget *dialog_widget, *content, *hbox, *vbox, *icon, *label;
	GtkWidget *default_button;
	gint response;

	if (parent == NULL)
	{
		return DIALOG_QUIT;
	}
	else
	{
		// Make sure the main window is visible
		interface_show_window();
	}

	dialog_widget = gtk_dialog_new();
	dialog_window = GTK_WINDOW(dialog_widget);
	dialog = GTK_DIALOG(dialog_widget);

	gtk_window_set_transient_for(dialog_window, GTK_WINDOW(parent));
	gtk_window_set_title(dialog_window, "Close or quit?");
	gtk_window_set_modal(dialog_window, TRUE);
    gtk_window_set_destroy_with_parent(dialog_window, TRUE);
	gtk_window_set_resizable(dialog_window, TRUE);

	gtk_dialog_add_button(dialog, "_Quit", GTK_RESPONSE_ACCEPT);
	default_button = gtk_dialog_add_button(dialog, "_Cancel", GTK_RESPONSE_REJECT);
	gtk_dialog_add_button(dialog, "_Close", GTK_RESPONSE_CLOSE);

	interface_window_set_default_widget(dialog_window, default_button);

	// Set dialog properties
	content = gtk_dialog_get_content_area(dialog);
	gtk_container_set_border_width(GTK_CONTAINER(content), 12);
	gtk_box_set_spacing(GTK_BOX(content), 18);
	gtk_dialog_set_default_response(dialog, GTK_RESPONSE_CLOSE);

	// Create a container for the icon and the text
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(content), hbox, FALSE, TRUE, 0);

	// Create the icon
	icon = gtk_image_new_from_icon_name("dialog-warning-symbolic", GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, TRUE, 8);

	// Create a container for the labels
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 4);

	// Create some labels and add them to the container
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
	                     "<span weight=\"bold\" size=\"larger\">"
	                     "Quitting the application stops the playback."
	                     "</span>\n");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 4);

	label = gtk_label_new("Do you want to close the window and continue the playback\n"
	                      "or quit the application and stop the playback?");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 4);

	gtk_widget_show_all(hbox);

	response = gtk_dialog_run(dialog);

	gtk_widget_destroy(dialog_widget);

	// Check respond, destroy widget and return
	switch (response)
	{
		case GTK_RESPONSE_ACCEPT:
			// Quit
			return DIALOG_QUIT;
		case GTK_RESPONSE_CLOSE:
			// Close
			return DIALOG_CLOSE;
		default:
			// Do nothing
			return DIALOG_CANCEL;
	}
}

static void
interface_progress_window_create(const gchar *description)
{
	GtkWidget *progress_win;
	GtkWidget *content;
	GtkWidget *vbox;
	GtkWidget *prog;

	g_return_if_fail(GTK_IS_WINDOW(InterfaceData.main_window));

	progress_win = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(progress_win), "Processing...");
	gtk_window_set_transient_for(GTK_WINDOW(progress_win), InterfaceData.main_window);
	gtk_window_set_modal(GTK_WINDOW(progress_win), TRUE);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(progress_win), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(progress_win), TRUE);
	g_signal_connect(progress_win, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL /* user_data */);

	content = gtk_dialog_get_content_area(GTK_DIALOG(progress_win));
	gtk_container_set_border_width(GTK_CONTAINER(content), 12);
	gtk_box_set_spacing(GTK_BOX(content), 18);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start(GTK_BOX(content), vbox, FALSE, TRUE, 0);

	prog = gtk_progress_bar_new();
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(prog), description);
	gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(prog), TRUE);
	gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(prog), 0.02);

	gtk_box_pack_start(GTK_BOX(vbox), prog, FALSE, TRUE, 0);

	gtk_widget_show_all(progress_win);

	interface_update_gtk_events();

	InterfaceData.progress = progress_win;
	InterfaceData.prog_bar = prog;
}

// Update progress bar in the progress window
static void
interface_progress_window_update(gdouble complete)
{
	g_return_if_fail(InterfaceData.prog_bar != NULL);

	if (complete > 0.0 && complete <= 1.0) // Within range
	{
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(InterfaceData.prog_bar), complete);
	}
	else
	{
		gtk_progress_bar_pulse(GTK_PROGRESS_BAR(InterfaceData.prog_bar));
	}

	interface_update_gtk_events();
}

static void
interface_progress_window_destroy(void)
{
	g_return_if_fail(InterfaceData.prog_bar != NULL);

	gtk_widget_destroy(InterfaceData.progress);

	InterfaceData.progress = NULL;
	InterfaceData.prog_bar = NULL;
}

static void
interface_set_subtitle(gchar *subtitle)
{
	// Always set "no client-side decoration" subtitle (even if hidden)
	gtk_label_set_label(GTK_LABEL(InterfaceData.subtitle_label), subtitle);

	// Set headerbar subtitle if present
	if (InterfaceData.header_bar != NULL)
	{
		gtk_header_bar_set_subtitle(GTK_HEADER_BAR(InterfaceData.header_bar), subtitle);
	}
}

static void
interface_set_button_play(void)
{
	GtkWidget *button, *image;

	button = InterfaceData.play_pause_button;

	if (button != NULL)
	{
		image = interface_get_default_media_icon("media-playback-start");

		gtk_button_set_image(GTK_BUTTON(button), image);
	}
}

static void
interface_set_button_pause(void)
{
	GtkWidget *button, *image;

	button = InterfaceData.play_pause_button;

	if (button != NULL)
	{
		image = interface_get_default_media_icon("media-playback-pause");

		gtk_button_set_image(GTK_BUTTON(button), image);
	}
}

static GtkWidget *
interface_get_default_media_icon(const gchar *icon_name)
{
	GtkWidget *image;

	image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);

	gtk_image_set_pixel_size(GTK_IMAGE(image), 32);

	gtk_widget_set_margin_start(image, 6);
	gtk_widget_set_margin_end(image, 6);

	return image;
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

static gboolean
interface_tree_pop_menu(GtkWidget *widget, GdkEventButton *event, gpointer subwidget)
{
	/*
	 * Check where the user has clicked and
	 * what rows are selected, then popup the menu.
	 */

	GtkTreeView *view = GTK_TREE_VIEW(widget);
	GtkTreeViewColumn *column = NULL;
	GtkTreePath *path = NULL;
	GtkMenu *menu = subwidget;

	g_return_val_if_fail(GTK_IS_TREE_VIEW(view), FALSE);
	g_return_val_if_fail(GTK_IS_MENU(menu), FALSE);
	g_return_val_if_fail(event != NULL, FALSE);

	if (!gtk_tree_view_get_path_at_pos(view, event->x, event->y,
	                                   &path, &column,
	                                   NULL /* cell_x */, NULL /* cell_y */))
	{
		// Could not find matching row
		return FALSE;
	}
	else
	{
		gtk_menu_popup_at_pointer(menu, NULL /* trigger_event */);

		return TRUE;
	}
}

static GdkPixbuf *
interface_get_pixbuf_icon(SongStatusIcon state)
{
	GdkPixbuf *icon = NULL;

	switch (state)
	{
		case STATUS_ICON_NONE:
			// Nothing to report
			break;
		case STATUS_ICON_PLAYING:
		case STATUS_ICON_PAUSED:
			icon = icons_get_themed_image("media-playback-start");
			break;
		case STATUS_ICON_QUEUED:
			icon = icons_get_themed_image("playlist-queue");
			break;
		case STATUS_ICON_STOP:
			icon = icons_get_themed_image("media-playback-stop");
			break;
		default:
			icon = icons_get_themed_image("action-unavailable");
			break;
	}

	return icon;
}

static void
interface_window_set_default_widget(GtkWindow *window, GtkWidget *widget)
{
	g_return_if_fail(GTK_IS_WINDOW(window));

	gtk_widget_grab_focus(widget);
}

static void
interface_update_gtk_events(void)
{
	while (gtk_events_pending())
	{
		gtk_main_iteration();
	}
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

static void
interface_destruct(void)
{
	if (InterfaceData.constructed)
	{
		// Make sure the toplevel window is indeed (going to be) destructed
		gtk_widget_destroy(InterfaceData.window_widget);
	}
}

static void
interface_finalize(void)
{
	// Free allocated lists
	g_slist_free(InterfaceData.selection_tools);
	g_slist_free(InterfaceData.playing_tools);

	// Reset all
	InterfaceData = (InterfaceDetails) { 0 };

	// Explicitly set @constructed to %FALSE
	InterfaceData.constructed = FALSE;

	// Just quit the application
	wf_app_quit();
}

/* DESTRUCTORS END */

/* END OF FILE */
