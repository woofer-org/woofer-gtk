/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * preferences.c  This file is part of Woofer GTK
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
#include <gtk/gtk.h>

// Woofer core includes
#include <woofer/app.h>
#include <woofer/constants.h>
#include <woofer/intelligence.h>
#include <woofer/settings.h>
#include <woofer/utils.h>

// Module includes
#include "preferences.h"

// Dependency includes
#include "config.h"
#include "settings.h"
#include "widgets/action_list_row.h"

// Resource includes
/*< none >*/

/* INCLUDES END */

/* DESCRIPTION BEGIN */

/*
 * This provides the preference window, including getting and setting the
 * values.
 */

/* DESCRIPTION END */

/* DEFINES BEGIN */

// Macro to check string array length
#define SIZEOF(a) (sizeof(a)/sizeof(*a))

/* DEFINES END */

/* CUSTOM TYPES BEGIN */

typedef enum _PreferenceNotifications PreferenceNotifications;
typedef struct _PreferenceDetails PreferenceDetails;
typedef struct _PreferenceEvents PreferenceEvents;

enum _PreferenceNotifications
{
	PREF_NOT_NEVER,
	PREF_NOT_HIDDEN_ONLY,
	PREF_NOT_UNFOCUSED_ONLY,
	PREF_NOT_ALWAYS,
	PREF_NOT_DEFINED
};

struct _PreferenceEvents
{
	func_report_close close_func;
};

struct _PreferenceDetails
{
	PreferenceEvents events;

	GList *list_boxes;
	gboolean ignore_widget_updates;
	gboolean constructed;
	gchar *current_message;

	guint statusContextId;
	GtkWindow *dialog_window;
	GtkWidget *dialog_widget;
	GtkWidget *scrolled_window;
	GtkWidget *status;
	GtkWidget *apply_button;

	// Order of interface appearance
	GtkComboBox *notifications;
	GtkSpinButton *update_interval;
	GtkSwitch *prefer_play_ram;
	GtkSwitch *timestamp;
	GtkSpinButton *min_play_percentage;
	GtkSpinButton *full_play_percentage;
	GtkSpinButton *filter_recent_artists;
	GtkSpinButton *filter_recents_amount;
	GtkSpinButton *filter_recents_percentage;
	GtkCheckButton *filter_rating;
	GtkCheckButton *rating_inc_zero;
	GtkSpinButton *rating_min;
	GtkSpinButton *rating_max;
	GtkCheckButton *filter_score;
	GtkSpinButton *score_min;
	GtkSpinButton *score_max;
	GtkCheckButton *filter_playcount;
	GtkCheckButton *playcount_invert;
	GtkSpinButton *playcount_th;
	GtkCheckButton *filter_skipcount;
	GtkCheckButton *skipcount_invert;
	GtkSpinButton *skipcount_th;
	GtkCheckButton *filter_lastplayed;
	GtkCheckButton *lastplayed_invert;
	GtkSpinButton *lastplayed_th;
	GtkCheckButton *use_rating;
	GtkCheckButton *invert_rating_prop;
	GtkSpinButton *rating_multiplier;
	GtkSpinButton *rating_default;
	GtkCheckButton *use_score;
	GtkCheckButton *invert_score_prop;
	GtkSpinButton *score_multiplier;
	GtkCheckButton *use_playcount;
	GtkCheckButton *invert_playcount_prop;
	GtkSpinButton *playcount_multiplier;
	GtkCheckButton *use_skipcount;
	GtkCheckButton *invert_skipcount_prop;
	GtkSpinButton *skipcount_multiplier;
	GtkCheckButton *use_lastplayed;
	GtkCheckButton *invert_lastplayed_prop;
	GtkSpinButton *lastplayed_multiplier;
};

/* CUSTOM TYPES END */

/* FUNCTION PROTOTYPES BEGIN */

static void preference_dialog_new_list_box(GtkWidget *list_box);
static void preference_dialog_construct(GtkWindow *parent_window);

static void preference_dialog_set_message(const gchar *msg);

static gboolean preference_dialog_delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static void preference_dialog_destroy_cb(GtkWidget *object, gpointer user_data);
static void preference_dialog_widget_updated_cb(GObject *gobject, GParamSpec *pspec, gpointer user_data);
static void preference_dialog_range_max_updated_cb(GtkAdjustment *main_adjustment, gpointer user_data);
static void preference_dialog_range_min_updated_cb(GtkAdjustment *main_adjustment, gpointer user_data);
static void preference_dialog_row_activated_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data);
static gboolean preference_dialog_key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
static gboolean preference_dialog_keynav_failed_cb(GtkWidget *widget, GtkDirectionType direction, gpointer user_data);
static void preference_dialog_apply_cb(GtkWidget *button, gpointer user_data);
static void preference_dialog_close_cb(GtkWidget *button, gpointer user_data);

static void preference_dialog_emit_close(PreferenceEvents *events);
static void preference_dialog_update_status(gchar *message);
static void preference_dialog_set_apply_enabled(gboolean enable);
static void preference_dialog_update_widgets();

static GtkAdjustment * preference_dialog_adjustment_copy(GtkAdjustment *adjustment);

static NotificationSetting preference_dialog_get_notification_setting(PreferenceNotifications x);
static PreferenceNotifications preference_dialog_get_notification_preference(NotificationSetting x);
static const gchar * preference_dialog_get_notifications_str(PreferenceNotifications x);

static void preference_dialog_destruct();

/* FUNCTION PROTOTYPES END */

/* GLOBAL VARIABLES BEGIN */

static PreferenceDetails PreferenceData =
{
	.constructed = FALSE,

	// All others are %NULL
};

static const gchar *FilterDescription = "These parameters are used before choosing a song. The library first gets "
                                        "filtered and it excludes some items so they can not be chosen. Each filter "
                                        "can be enabled seperately and has it's own value. If a filter is enabled, it "
                                        "removes a song if it's value is lower then the one set here. If negative, it "
                                        "removes the song if it's value is higher.";
static const gchar *ProbabilityDescription = "After the filters have been applied, a numer is determined for every "
                                             "song. How that is determined, depends on these settings. The numer is "
                                             "essentially the amount of entries of a total number and thats how the "
                                             "probability is calculated.";

/* GLOBAL VARIABLES END */

/* CONSTRUCTORS BEGIN */

static void
preference_dialog_new_list_box(GtkWidget *list_box)
{
	g_return_if_fail(GTK_IS_LIST_BOX(list_box));

	gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(list_box), TRUE);
	g_signal_connect(list_box, "row-activated", G_CALLBACK(preference_dialog_row_activated_cb), NULL /* user_data */);
	g_signal_connect(list_box, "keynav-failed", G_CALLBACK(preference_dialog_keynav_failed_cb), NULL /* user_data */);
	PreferenceData.list_boxes = g_list_append(PreferenceData.list_boxes, list_box);
}

// Set up all the widgets for the preference dialog
static void
preference_dialog_construct(GtkWindow *parent_window)
{
	WfSongFilter *filters;
	WfSongEntries *entries;

	const gchar *tooltip;
	const gchar *string;
	GtkWidget *scroll_window;
	GtkWidget *header_bar;
	GtkWidget *status_bar;
	GtkWidget *main_box;
	GtkWidget *content_box;
	GtkWidget *box;
	GtkWidget *separator;
	GtkWidget *frame;
	GtkWidget *frame_box;
	GtkWidget *list_box;
	GtkWidget *action_row;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *select_box;
	GtkWidget *checkbox;
	GtkWidget *switcher;
	GtkWidget *spin_button;
	GtkWidget *spin_min;
	GtkWidget *spin_max;
	GtkAdjustment *adjust, *adjustment_min, *adjustment_max;
	gchar *str;

	filters = wf_settings_get_filter();
	entries = wf_settings_get_song_entry_modifiers();

	g_return_if_fail(filters != NULL);
	g_return_if_fail(entries != NULL);

	g_debug("Constructing preference window...");

	g_warn_if_fail(GTK_IS_WINDOW(parent_window));

	// Do not take action when things update
	PreferenceData.ignore_widget_updates = TRUE;

	// Create preference window with DIALOG as a hint
	PreferenceData.dialog_widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	PreferenceData.dialog_window = GTK_WINDOW(PreferenceData.dialog_widget);
	gtk_window_set_transient_for(PreferenceData.dialog_window, parent_window);
	gtk_window_set_destroy_with_parent(PreferenceData.dialog_window, TRUE);
	gtk_window_set_modal(PreferenceData.dialog_window, TRUE);
	gtk_window_set_skip_taskbar_hint(PreferenceData.dialog_window, TRUE);
	gtk_window_set_type_hint(PreferenceData.dialog_window, GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_title(PreferenceData.dialog_window, ("Configure " WF_NAME));
	gtk_window_set_default_size(PreferenceData.dialog_window, INTERFACE_DEFAULT_SMALL_WIDTH, INTERFACE_DEFAULT_SMALL_HEIGHT);
	g_signal_connect(PreferenceData.dialog_widget, "key-press-event", G_CALLBACK(preference_dialog_key_pressed), NULL /* user_data */);
	g_signal_connect(PreferenceData.dialog_widget, "destroy", G_CALLBACK(preference_dialog_destroy_cb), NULL /* user_data */);

	// Hide but keep the window if the user closes it. Free the container on destroy (e.g. application quits)
	g_signal_connect(PreferenceData.dialog_widget, "delete-event", G_CALLBACK(preference_dialog_delete_event_cb), NULL /* user_data */);

	// HeaderBar
	header_bar = gtk_header_bar_new();
	gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "Configure " WF_NAME);
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
	gtk_window_set_titlebar(GTK_WINDOW(PreferenceData.dialog_widget), header_bar);

	// Box for the content of the dialog
	main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(PreferenceData.dialog_widget), main_box);

	// Add the two buttons and a statusBar
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 8);
	gtk_box_pack_end(GTK_BOX(main_box), hbox, FALSE, TRUE, 0);
	status_bar = gtk_statusbar_new();
	PreferenceData.statusContextId = gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar), "prefs");
	gtk_box_pack_start(GTK_BOX(hbox), status_bar, FALSE, TRUE, 0);
	PreferenceData.status = status_bar;
	button = gtk_button_new_with_mnemonic("_Apply");
	g_signal_connect(GTK_WIDGET(button), "clicked", G_CALLBACK(preference_dialog_apply_cb), NULL /* user_data */);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);
	PreferenceData.apply_button = button;
	button = gtk_button_new_with_mnemonic("_Close");
	g_signal_connect(GTK_WIDGET(button), "clicked", G_CALLBACK(preference_dialog_close_cb), NULL /* user_data */);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);

	// Info message
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
	                     "<span style=\"italic\" weight=\"thin\">"
	                     "Nothing changed here will take effect until you click apply."
	                     "</span>");
	gtk_widget_set_margin_start(label, 4);
	gtk_widget_set_margin_end(label, 4);
	gtk_box_pack_start(GTK_BOX(main_box), label, FALSE, TRUE, 8);

	// Separator
	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(main_box), separator, FALSE, TRUE, 0);

	// Create a scroll window
	scroll_window = gtk_scrolled_window_new(NULL, NULL);
	content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_window), 140);
	gtk_container_set_border_width(GTK_CONTAINER(content_box), 12);
	gtk_container_add(GTK_CONTAINER(scroll_window), content_box);
	gtk_box_pack_start(GTK_BOX(main_box), scroll_window, TRUE, TRUE, 0);
	PreferenceData.scrolled_window = scroll_window;

	// Group General
	frame_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_box_pack_start(GTK_BOX(content_box), frame_box, FALSE, TRUE, 0);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), "<span size=\"large\" weight=\"bold\">General</span>");
	gtk_label_set_xalign (GTK_LABEL(label), 0.0);
	gtk_box_pack_start(GTK_BOX(frame_box), label, FALSE, TRUE, 0);

	frame = gtk_frame_new(NULL);
	list_box = gtk_list_box_new();
	preference_dialog_new_list_box(list_box);
	gtk_container_add(GTK_CONTAINER(frame), list_box);
	gtk_box_pack_start(GTK_BOX(frame_box), frame, FALSE, TRUE, 0);

	// New item
	tooltip = "When desktop notifications should be send";
	action_row = widget_action_list_row_new("Notification", tooltip);
	select_box = gtk_combo_box_text_new();
	for (guint x = 0; x < PREF_NOT_DEFINED; x++)
	{
		string = preference_dialog_get_notifications_str(x);

		if (string != NULL)
		{
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(select_box), string);
		}
	}
	g_signal_connect(select_box, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), select_box);
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);
	PreferenceData.notifications = GTK_COMBO_BOX(select_box);

	// New item
	tooltip = "Defines the interval (in milliseconds) used between interface updates while playing "
	          "(0 will disable interface updates).";
	action_row = widget_action_list_row_new("Interface update interval", tooltip);
	adjust = gtk_adjustment_new(0.0, // Value
	                            0.0, // Lowest value
	                            60000.0, // Highest value
	                            100.0, // Step Increment
	                            1000.0, // Page Increment
	                            0.0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 1.0, 0);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), spin_button);
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);
	PreferenceData.update_interval = GTK_SPIN_BUTTON(spin_button);

	// New item
	tooltip = "Prefer to first read a file to memory and then start the playback that runs completely from "
	          "memory. Be aware that this will lead to higher memory usage and potentially noticable latencies "
	          "before the playback actually starts.";
	action_row = widget_action_list_row_new("Prefer to play from memory", tooltip);
	switcher = gtk_switch_new();
	g_signal_connect(switcher, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), switcher);
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);
	PreferenceData.prefer_play_ram = GTK_SWITCH(switcher);

	// New item
	tooltip = "Show timestamp instead of how long ago a song has been played";
	action_row = widget_action_list_row_new("Last played as timestamp", tooltip);
	switcher = gtk_switch_new();
	g_signal_connect(switcher, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), switcher);
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);
	PreferenceData.timestamp = GTK_SWITCH(switcher);

	// New item
	tooltip = "Minimum percentage of a song that must be played in order to update "
	          "things like the play count and the last played timestamp";
	action_row = widget_action_list_row_new("Minimum play threshold", tooltip);
	adjust = gtk_adjustment_new(0.0, // Value
	                            0.0, // Lowest value
	                            100.0, // Highest value
	                            2.0, // Step Increment
	                            10.0, // Page Increment
	                            0.0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 1.0, 0);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), spin_button);
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);
	PreferenceData.min_play_percentage = GTK_SPIN_BUTTON(spin_button);

	// New item
	tooltip = "Minimum percentage of a song that must be played to consider a song as fully played. This has an effect "
	          "on for example the score that gets updated if the track is skipped after this threshold.";
	action_row = widget_action_list_row_new("Fully played threshold", tooltip);
	adjust = gtk_adjustment_new(0.0, // Value
	                            0.0, // Lowest value
	                            100.0, // Highest value
	                            2.0, // Step Increment
	                            10.0, // Page Increment
	                            0.0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 1.0, 0);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), spin_button);
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);
	PreferenceData.full_play_percentage = GTK_SPIN_BUTTON(spin_button);

	// Group Filter
	frame_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_box_pack_start(GTK_BOX(content_box), frame_box, FALSE, TRUE, 0);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), "<span size=\"large\" weight=\"bold\">Filters</span>");
	gtk_label_set_xalign (GTK_LABEL(label), 0.0);
	gtk_box_pack_start(GTK_BOX(frame_box), label, FALSE, TRUE, 0);

	str = g_markup_printf_escaped("<span size=\"small\">%s</span>", FilterDescription);
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), str);
	gtk_label_set_xalign (GTK_LABEL(label), 0.0);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(frame_box), label, FALSE, TRUE, 0);
	g_free(str);

	frame = gtk_frame_new(NULL);
	list_box = gtk_list_box_new();
	preference_dialog_new_list_box(list_box);
	gtk_container_add(GTK_CONTAINER(frame), list_box);
	gtk_box_pack_start(GTK_BOX(frame_box), frame, FALSE, TRUE, 0);

	// New item
	tooltip = NULL; // "Amount of artists from songs that were recently played to match up with songs to remove";
	action_row = widget_action_list_row_new("Filter out artists that have been played", tooltip);
	adjust = gtk_adjustment_new(0, // Value
	                            0, // Lowest value
	                            25, // Highest value
	                            1, // Step Increment
	                            5, // Page Increment
	                            0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 1, 0);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), spin_button);
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);
	PreferenceData.filter_recent_artists = GTK_SPIN_BUTTON(spin_button);

	// New item
	tooltip = "Amount of recently played songs to exclude from the qualification list when choosing the next track";
	action_row = widget_action_list_row_new("Filter out recently played songs", tooltip);
	adjust = gtk_adjustment_new(0.0, // Value
	                            0.0, // Lowest value
	                            100.0, // Highest value
	                            1.0, // Step Increment
	                            5.0, // Page Increment
	                            0.0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 1.0, 0);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), spin_button);
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);
	PreferenceData.filter_recents_amount = GTK_SPIN_BUTTON(spin_button);

	// New item
	tooltip = "Percentage of the library to exclude from the qualification list when choosing the next track, "
	          "sorted by last played";
	action_row = widget_action_list_row_new("Percentage of played items to filter out", tooltip);
	adjust = gtk_adjustment_new(0.0, // Value
	                            0.0, // Lowest value
	                            100.0, // Highest value
	                            2.0, // Step Increment
	                            20.0, // Page Increment
	                            0.0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 1.0, 0);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), spin_button);
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);
	PreferenceData.filter_recents_percentage = GTK_SPIN_BUTTON(spin_button);

	// New sub item
	tooltip = "While selecting a song, if a songs rating is within this range (including the numbers itself), "
	          "qualify the song to be chosen.";
	action_row = widget_action_list_row_new("Filter range for ratings", tooltip);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), GTK_WIDGET(box));
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);

	// Toggle check box
	checkbox = gtk_check_button_new_with_label("Enable");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	widget_action_list_row_set_activatable_widget(WIDGET_ACTION_LIST_ROW(action_row), checkbox);
	PreferenceData.filter_rating = GTK_CHECK_BUTTON(checkbox);

	// Extra "include zero" option
	tooltip = "If enabled, also qualify a song if its rating is not set (zero)";
	checkbox = gtk_check_button_new_with_label("Include unrated");
	gtk_widget_set_tooltip_text(checkbox, tooltip);
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	PreferenceData.rating_inc_zero = GTK_CHECK_BUTTON(checkbox);

	// Range widgets
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_end(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	label = gtk_label_new("From ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	adjustment_min = gtk_adjustment_new(0, // Value
	                                    0, // Lowest value
	                                    10, // Highest value
	                                    1, // Step Increment
	                                    5, // Page Increment
	                                    0); // Page Size
	adjustment_max = preference_dialog_adjustment_copy(adjustment_min);
	spin_min = gtk_spin_button_new(adjustment_min, 2, 0);
	gtk_widget_set_valign(spin_min, GTK_ALIGN_CENTER);
	g_signal_connect(spin_min, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), spin_min, FALSE, TRUE, 0);

	label = gtk_label_new(" to ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	spin_max = gtk_spin_button_new(adjustment_max, 2, 0);
	gtk_widget_set_valign(spin_max, GTK_ALIGN_CENTER);
	g_signal_connect(spin_max, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), spin_max, FALSE, TRUE, 0);

	g_signal_connect(adjustment_min, "value-changed", G_CALLBACK(preference_dialog_range_min_updated_cb), adjustment_max);
	g_signal_connect(adjustment_max, "value-changed", G_CALLBACK(preference_dialog_range_max_updated_cb), adjustment_min);
	PreferenceData.rating_min = GTK_SPIN_BUTTON(spin_min);
	PreferenceData.rating_max = GTK_SPIN_BUTTON(spin_max);

	// New sub item
	tooltip = "While selecting a song, if a songs score is within this range (including the numbers itself), "
	          "qualify the song to be chosen.";
	action_row = widget_action_list_row_new("Filter range for scores", tooltip);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), GTK_WIDGET(box));
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);

	// Toggle check box
	checkbox = gtk_check_button_new_with_label("Enable");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	widget_action_list_row_set_activatable_widget(WIDGET_ACTION_LIST_ROW(action_row), checkbox);
	PreferenceData.filter_score = GTK_CHECK_BUTTON(checkbox);

	// Range widgets
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_end(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	label = gtk_label_new("From ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	adjustment_min = gtk_adjustment_new(0, // Value
	                                    0, // Lowest value
	                                    100, // Highest value
	                                    5, // Step Increment
	                                    20, // Page Increment
	                                    0); // Page Size
	adjustment_max = preference_dialog_adjustment_copy(adjustment_min);
	spin_min = gtk_spin_button_new(adjustment_min, 2, 0);
	gtk_widget_set_valign(spin_min, GTK_ALIGN_CENTER);
	g_signal_connect(spin_min, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), spin_min, FALSE, TRUE, 0);

	label = gtk_label_new(" to ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	spin_max = gtk_spin_button_new(adjustment_max, 2, 0);
	gtk_widget_set_valign(spin_max, GTK_ALIGN_CENTER);
	g_signal_connect(spin_max, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), spin_max, FALSE, TRUE, 0);

	g_signal_connect(adjustment_min, "value-changed", G_CALLBACK(preference_dialog_range_min_updated_cb), adjustment_max);
	g_signal_connect(adjustment_max, "value-changed", G_CALLBACK(preference_dialog_range_max_updated_cb), adjustment_min);
	PreferenceData.score_min = GTK_SPIN_BUTTON(spin_min);
	PreferenceData.score_max = GTK_SPIN_BUTTON(spin_max);

	// New sub item
	tooltip = "Only songs with this minimum (or maximum) play count will be qualified.";
	action_row = widget_action_list_row_new("Play count threshold", tooltip);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), GTK_WIDGET(box));
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);

	// Toggle check box
	checkbox = gtk_check_button_new_with_label("Enable");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	widget_action_list_row_set_activatable_widget(WIDGET_ACTION_LIST_ROW(action_row), checkbox);
	PreferenceData.filter_playcount = GTK_CHECK_BUTTON(checkbox);

	// Threshold invert
	tooltip = "If enabled, the threshold is the maximum accepted value instead of the minimum";
	checkbox = gtk_check_button_new_with_label("Is maximum");
	gtk_widget_set_tooltip_text(checkbox, tooltip);
	g_signal_connect(checkbox, "toggled", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	PreferenceData.playcount_invert = GTK_CHECK_BUTTON(checkbox);

	// Range widget
	adjust = gtk_adjustment_new(0, // Value
	                            0, // Lowest value
	                            G_MAXINT, // Highest value
	                            1, // Step Increment
	                            10, // Page Increment
	                            0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 2.0, 0);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(box), spin_button, FALSE, TRUE, 0);
	PreferenceData.playcount_th = GTK_SPIN_BUTTON(spin_button);

	// New sub item
	tooltip = "Only songs with this minimum (or maximum) skip count will be qualified.";
	action_row = widget_action_list_row_new("Skip count threshold", tooltip);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), GTK_WIDGET(box));
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);

	// Toggle check box
	checkbox = gtk_check_button_new_with_label("Enable");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	widget_action_list_row_set_activatable_widget(WIDGET_ACTION_LIST_ROW(action_row), checkbox);
	PreferenceData.filter_skipcount = GTK_CHECK_BUTTON(checkbox);

	// Threshold invert
	tooltip = "If enabled, the threshold is the maximum accepted value instead of the minimum";
	checkbox = gtk_check_button_new_with_label("Is maximum");
	gtk_widget_set_tooltip_text(checkbox, tooltip);
	g_signal_connect(checkbox, "toggled", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	PreferenceData.skipcount_invert = GTK_CHECK_BUTTON(checkbox);

	// Range widget
	adjust = gtk_adjustment_new(0.0, // Value
	                            0.0, // Lowest value
	                            G_MAXINT, // Highest value
	                            1.0, // Step Increment
	                            10.0, // Page Increment
	                            0.0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 1.5, 0);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(box), spin_button, FALSE, TRUE, 0);
	PreferenceData.skipcount_th = GTK_SPIN_BUTTON(spin_button);

	// New sub item
	tooltip = "Only songs with this minimum (or maximum) of seconds since the last play time will be qualified.";
	action_row = widget_action_list_row_new("Last played threshold", tooltip);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), GTK_WIDGET(box));
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);

	// Toggle check box
	checkbox = gtk_check_button_new_with_label("Enable");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	widget_action_list_row_set_activatable_widget(WIDGET_ACTION_LIST_ROW(action_row), checkbox);
	PreferenceData.filter_lastplayed = GTK_CHECK_BUTTON(checkbox);

	// Threshold invert
	tooltip = "If enabled, the threshold is the maximum accepted value instead of the minimum";
	checkbox = gtk_check_button_new_with_label("Is maximum");
	gtk_widget_set_tooltip_text(checkbox, tooltip);
	g_signal_connect(checkbox, "toggled", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	PreferenceData.lastplayed_invert = GTK_CHECK_BUTTON(checkbox);

	// Range widget
	adjust = gtk_adjustment_new(0.0, // Value
	                            0.0, // Lowest value
	                            G_MAXINT64, // Highest value
	                            1.0, // Step Increment
	                            10.0, // Page Increment
	                            0.0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 1.5, 0);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(box), spin_button, FALSE, TRUE, 0);
	PreferenceData.lastplayed_th = GTK_SPIN_BUTTON(spin_button);

	// Group Algorithm
	frame_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	gtk_box_pack_start(GTK_BOX(content_box), frame_box, FALSE, TRUE, 0);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), "<span size=\"large\" weight=\"bold\">Song choosing</span>");
	gtk_label_set_xalign (GTK_LABEL(label), 0.0);
	gtk_box_pack_start(GTK_BOX(frame_box), label, FALSE, TRUE, 0);

	str = g_markup_printf_escaped("<span size=\"small\">%s</span>", ProbabilityDescription);
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), str);
	gtk_label_set_xalign (GTK_LABEL(label), 0.0);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(frame_box), label, FALSE, TRUE, 0);
	g_free(str);

	frame = gtk_frame_new(NULL);
	list_box = gtk_list_box_new();
	preference_dialog_new_list_box(list_box);
	gtk_container_add(GTK_CONTAINER(frame), list_box);
	gtk_box_pack_start(GTK_BOX(frame_box), frame, FALSE, TRUE, 0);

	// New item
	tooltip = "Take ratings into account when determining song probability. When enabled, higher ratings results in a "
	          "higher chance. When \"Invert\" is checked, lower ratings result in a higher chance. A non-zero default "
	          "rating can be set that is used for songs that have no rating set.";
	action_row = widget_action_list_row_new("Use ratings to modify song probability", tooltip);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), GTK_WIDGET(box));
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);

	// Toggle check box
	checkbox = gtk_check_button_new_with_label("Enable");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	widget_action_list_row_set_activatable_widget(WIDGET_ACTION_LIST_ROW(action_row), checkbox);
	PreferenceData.use_rating = GTK_CHECK_BUTTON(checkbox);

	// Invert check box
	checkbox = gtk_check_button_new_with_label("Invert");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	PreferenceData.invert_rating_prop = GTK_CHECK_BUTTON(checkbox);

	// Multiplier box
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	label = gtk_label_new("Multiplier: ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	// Multiplier widget
	adjust = gtk_adjustment_new(0.0, // Value
	                            0.0, // Lowest value
	                            25.0, // Highest value
	                            0.1, // Step Increment
	                            1.0, // Page Increment
	                            0.0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 1.0, 1);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), spin_button, FALSE, TRUE, 0);
	PreferenceData.rating_multiplier = GTK_SPIN_BUTTON(spin_button);

	// Default value box
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	label = gtk_label_new("Default rating: ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	// Default value widget
	adjust = gtk_adjustment_new(0.0, // Value
	                            0.0, // Lowest value
	                            10.0, // Highest value
	                            1.0, // Step Increment
	                            5.0, // Page Increment
	                            0.0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 1.0, 0);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), spin_button, FALSE, TRUE, 0);
	PreferenceData.rating_default = GTK_SPIN_BUTTON(spin_button);

	// New item
	tooltip = "Take scores into account when determining song probability. When enabled, higher scores results in a "
	          "higher chance. When \"Invert\" is checked, lower scores result in a higher chance.";
	action_row = widget_action_list_row_new("Use scores to modify song probability", tooltip);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), GTK_WIDGET(box));
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);

	// Toggle check box
	checkbox = gtk_check_button_new_with_label("Enable");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	widget_action_list_row_set_activatable_widget(WIDGET_ACTION_LIST_ROW(action_row), checkbox);
	PreferenceData.use_score = GTK_CHECK_BUTTON(checkbox);

	// Invert check box
	checkbox = gtk_check_button_new_with_label("Invert");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	PreferenceData.invert_score_prop = GTK_CHECK_BUTTON(checkbox);

	// Multiplier box
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	label = gtk_label_new("Multiplier: ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	// Multiplier widget
	adjust = gtk_adjustment_new(0.0, // Value
	                            0.0, // Lowest value
	                            25.0, // Highest value
	                            0.1, // Step Increment
	                            1.0, // Page Increment
	                            0.0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 1.0, 1);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), spin_button, FALSE, TRUE, 0);
	PreferenceData.score_multiplier = GTK_SPIN_BUTTON(spin_button);

	// New item
	tooltip = "Take play counts into account when determining song probability. When enabled, higher play counts "
	          "results in a higher chance. When \"Invert\" is checked, lower play counts result in a higher chance.";
	action_row = widget_action_list_row_new("Use play count to modify song probability", tooltip);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), GTK_WIDGET(box));
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);

	// Toggle check box
	checkbox = gtk_check_button_new_with_label("Enable");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	widget_action_list_row_set_activatable_widget(WIDGET_ACTION_LIST_ROW(action_row), checkbox);
	PreferenceData.use_playcount = GTK_CHECK_BUTTON(checkbox);

	// Invert check box
	checkbox = gtk_check_button_new_with_label("Invert");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	PreferenceData.invert_playcount_prop = GTK_CHECK_BUTTON(checkbox);

	// Multiplier box
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	label = gtk_label_new("Multiplier: ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	// Multiplier widget
	adjust = gtk_adjustment_new(0.0, // Value
	                            0.0, // Lowest value
	                            25.0, // Highest value
	                            0.1, // Step Increment
	                            1.0, // Page Increment
	                            0.0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 1.0, 1);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), spin_button, FALSE, TRUE, 0);
	PreferenceData.playcount_multiplier = GTK_SPIN_BUTTON(spin_button);

	// New item
	tooltip = "Take skip counts into account when determining song probability. When enabled, higher skip counts "
	          "results in a higher chance. When \"Invert\" is checked, lower skip counts result in a higher chance.";
	action_row = widget_action_list_row_new("Use skip count to modify song probability", tooltip);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), GTK_WIDGET(box));
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);

	// Toggle check box
	checkbox = gtk_check_button_new_with_label("Enable");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	widget_action_list_row_set_activatable_widget(WIDGET_ACTION_LIST_ROW(action_row), checkbox);
	PreferenceData.use_skipcount = GTK_CHECK_BUTTON(checkbox);

	// Invert check box
	checkbox = gtk_check_button_new_with_label("Invert");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	PreferenceData.invert_skipcount_prop = GTK_CHECK_BUTTON(checkbox);

	// Multiplier box
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	label = gtk_label_new("Multiplier: ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	// Multiplier widget
	adjust = gtk_adjustment_new(0.0, // Value
	                            0.0, // Lowest value
	                            25.0, // Highest value
	                            0.1, // Step Increment
	                            1.0, // Page Increment
	                            0.0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 1.0, 1);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), spin_button, FALSE, TRUE, 0);
	PreferenceData.skipcount_multiplier = GTK_SPIN_BUTTON(spin_button);

	// New item
	tooltip = "Take last play statistics into account when determining song probability. When enabled, songs that have "
	          "not been played recently will receive a higher chance. When \"Invert\" is checked, the result is invert "
	          "and thus will result in a lower chance.";
	action_row = widget_action_list_row_new("Use last play statistics to modify song probability", tooltip);
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	widget_action_list_row_set_child_widget(WIDGET_ACTION_LIST_ROW(action_row), GTK_WIDGET(box));
	gtk_list_box_insert(GTK_LIST_BOX(list_box), action_row, -1);

	// Toggle check box
	checkbox = gtk_check_button_new_with_label("Enable");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	widget_action_list_row_set_activatable_widget(WIDGET_ACTION_LIST_ROW(action_row), checkbox);
	PreferenceData.use_lastplayed = GTK_CHECK_BUTTON(checkbox);

	// Invert check box
	checkbox = gtk_check_button_new_with_label("Invert");
	g_signal_connect(checkbox, "notify::active", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, TRUE, 0);
	PreferenceData.invert_lastplayed_prop = GTK_CHECK_BUTTON(checkbox);

	// Multiplier box
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	label = gtk_label_new("Multiplier: ");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	// Multiplier widget
	adjust = gtk_adjustment_new(0.0, // Value
	                            0.0, // Lowest value
	                            25.0, // Highest value
	                            0.1, // Step Increment
	                            1.0, // Page Increment
	                            0.0); // Page Size
	spin_button = gtk_spin_button_new(adjust, 1.0, 1);
	gtk_widget_set_valign(spin_button, GTK_ALIGN_CENTER);
	g_signal_connect(spin_button, "notify::value", G_CALLBACK(preference_dialog_widget_updated_cb), NULL /* user_data */);
	gtk_box_pack_start(GTK_BOX(hbox), spin_button, FALSE, TRUE, 0);
	PreferenceData.lastplayed_multiplier = GTK_SPIN_BUTTON(spin_button);

	// Separator
	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(main_box), separator, FALSE, TRUE, 0);

	// Add information to container
	PreferenceData.constructed = TRUE;

	// Show all widgets
	gtk_widget_show_all(PreferenceData.dialog_widget);

	// Stop ignoring widget value updates
	PreferenceData.ignore_widget_updates = FALSE;
}

/* CONSTRUCTORS END */

/* GETTERS/SETTERS BEGIN */

static void
preference_dialog_set_message(const gchar *msg)
{
	// Free the old one (if present) first
	g_free(PreferenceData.current_message);

	// Copy new message and store the pointer statically
	PreferenceData.current_message = g_strdup(msg);
}

gboolean
preference_dialog_is_visible(void)
{
	// Return result of gtk_widget_is_visible if constructed, FALSE otherwise
	return PreferenceData.constructed ? gtk_widget_is_visible(PreferenceData.dialog_widget) : FALSE;
}

void
preference_dialog_connect_close(func_report_close cb_func)
{
	PreferenceData.events.close_func = cb_func;
}

/* GETTERS/SETTERS END */

/* CALLBACK FUNCTIONS BEGIN */

// Exactly like gtk_widget_hide_on_delete but emit the module's signal "close"
static gboolean
preference_dialog_delete_event_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	// Push a message (if set) to the main window
	preference_dialog_emit_close(&PreferenceData.events);
	preference_dialog_set_message(NULL);

	return gtk_widget_hide_on_delete(widget);
}

static void
preference_dialog_destroy_cb(GtkWidget *object, gpointer user_data)
{
	preference_dialog_destruct();
}

static void
preference_dialog_widget_updated_cb(GObject *gobject, GParamSpec *pspec, gpointer user_data)
{
	if ( !PreferenceData.ignore_widget_updates )
	{
		preference_dialog_set_apply_enabled(TRUE);
	}
}

/*
 * Sets the new value of the "spin button min" as the value of the
 * "spin button max" if it is higher. This prevents a spin button that is
 * used to specify the minimum of a range to have a higher value than
 * the spin button that is used to specify a maximum of the range.
 */
static void
preference_dialog_range_min_updated_cb(GtkAdjustment *main_adjustment, gpointer user_data)
{
	GtkAdjustment *other_adjustment = user_data;
	gdouble main_value = 0.0, other_value = 0.0;

	g_return_if_fail(GTK_IS_ADJUSTMENT(main_adjustment));
	g_return_if_fail(GTK_IS_ADJUSTMENT(other_adjustment));

	// move other adjustment if value gets higher than other
	main_value = gtk_adjustment_get_value(main_adjustment);
	other_value = gtk_adjustment_get_value(other_adjustment);

	if (main_value > other_value)
	{
		gtk_adjustment_set_value(other_adjustment, main_value);
	}
}

/*
 * Sets the new value of the "spin button max" as the value of the
 * "spin button min" if it is lower. This prevents a spin button that is
 * used to specify the maximum of a range to have a lower value than
 * the spin button that is used to specify a minimum of the range.
 */
static void
preference_dialog_range_max_updated_cb(GtkAdjustment *main_adjustment, gpointer user_data)
{
	GtkAdjustment *other_adjustment = user_data;
	gdouble main_value = 0.0, other_value = 0.0;

	g_return_if_fail(GTK_IS_ADJUSTMENT(main_adjustment));
	g_return_if_fail(GTK_IS_ADJUSTMENT(other_adjustment));

	main_value = gtk_adjustment_get_value(main_adjustment);
	other_value = gtk_adjustment_get_value(other_adjustment);

	if (main_value < other_value)
	{
		gtk_adjustment_set_value(other_adjustment, main_value);
	}
}

// ListBox got actived, now activate, swap, or focus it's respective widget
static void
preference_dialog_row_activated_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
	WidgetActionListRow *list_row;

	g_return_if_fail(WIDGET_IS_ACTION_LIST_ROW(row));

	list_row = WIDGET_ACTION_LIST_ROW(row);
	widget_action_list_row_activate_child(list_row);
}

static gboolean
preference_dialog_key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	gboolean handled = FALSE;

	if (event->type == GDK_KEY_PRESS && event->keyval == GDK_KEY_Escape)
	{
		handled = TRUE;
		g_info("Key press: <Esc>");

		// Now hide the window for the user
		preference_dialog_hide();
	}

	return handled;
}

// ListBox failed to continue keyboard navigation, activate the next widget if available
static gboolean
preference_dialog_keynav_failed_cb(GtkWidget *widget, GtkDirectionType direction, gpointer user_data)
{
	// Keynav failed and current focus is ok or the focus is moved
	const gboolean focus_ok = TRUE, focus_move = FALSE;

	GtkAdjustment *adjustment;
	GList *boxes, *item, *other;
	GtkWidget *next_box;
	gdouble position, lower, upper, page_size, max;

	g_return_val_if_fail(PreferenceData.list_boxes != NULL, focus_ok);
	g_return_val_if_fail(widget != NULL, focus_ok);

	boxes = PreferenceData.list_boxes;
	item = g_list_find(boxes, widget);

	if (item == NULL)
	{
		g_warning("Focused widget can not be found in the widget list "
		          "to provide focus to the next widget (at preference_dialog_keynav_failed_cb)");

		return focus_ok;
	}
	else if (direction == GTK_DIR_DOWN)
	{
		// Get the next widget in the list
		other = item->next;
	}
	else if (direction == GTK_DIR_UP)
	{
		// Get the previous widget in the list
		other = item->prev;
	}
	else
	{
		g_info("Keynavigation failed, but ignoring (direction: %d)", direction);

		return focus_ok;
	}

	// Now focus the found widget (if valid)
	if (other == NULL || other->data == NULL)
	{
		g_info("No other widget to focus (direction: %d)", direction);

		// Adjust scrollbar so the window is scrolled fully up or fully down
		adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(PreferenceData.scrolled_window));
		page_size = gtk_adjustment_get_page_size(adjustment);
		position = gtk_adjustment_get_value(adjustment);
		lower = gtk_adjustment_get_lower(adjustment);
		upper = gtk_adjustment_get_upper(adjustment);
		max = upper - page_size; // Actual maximum value

		if ( direction == GTK_DIR_DOWN && position < max )
		{
			gtk_adjustment_set_value(adjustment, max);
		}
		else if ( direction == GTK_DIR_UP && position > lower )
		{
			gtk_adjustment_set_value(adjustment, lower);
		}

		return focus_ok;
	}
	else
	{
		next_box = other->data;

		gtk_widget_child_focus(GTK_WIDGET(next_box), direction);
		return focus_move;
	}
}

// Collect widget values and update the setting structures
static void preference_dialog_apply_cb(GtkWidget *button, gpointer user_data)
{
	WfSongFilter *filters;
	WfSongEntries *entries;
	gint v_int;
	gint64 v_int64;
	gdouble v_double;
	gboolean v_bool;

	filters = wf_settings_get_filter();
	entries = wf_settings_get_song_entry_modifiers();

	g_return_if_fail(filters != NULL);
	g_return_if_fail(entries != NULL);

	g_info("Updating preferences...");

	v_int = gtk_combo_box_get_active(PreferenceData.notifications);
	interface_settings_set_notification(preference_dialog_get_notification_setting(v_int));
	v_int = gtk_spin_button_get_value_as_int(PreferenceData.update_interval);
	wf_settings_static_set_int(WF_SETTING_UPDATE_INTERVAL, v_int);
	v_bool = gtk_switch_get_active(PreferenceData.prefer_play_ram);
	wf_settings_static_set_bool(WF_SETTING_PREFER_PLAY_FROM_RAM, v_bool);
	v_bool = gtk_switch_get_active(PreferenceData.timestamp);
	interface_settings_set_last_played_timestamp(v_bool);
	v_double = gtk_spin_button_get_value(PreferenceData.min_play_percentage);
	wf_settings_static_set_double(WF_SETTING_MIN_PLAYED_FRACTION, v_double / 100.0);
	v_double = gtk_spin_button_get_value(PreferenceData.full_play_percentage);
	wf_settings_static_set_double(WF_SETTING_FULL_PLAYED_FRACTION, v_double / 100.0);

	v_int = gtk_spin_button_get_value_as_int(PreferenceData.filter_recent_artists);
	wf_settings_static_set_int(WF_SETTING_FILTER_RECENT_ARTISTS, v_int);
	v_int = gtk_spin_button_get_value_as_int(PreferenceData.filter_recents_amount);
	wf_settings_static_set_int(WF_SETTING_FILTER_RECENT_AMOUNT, v_int);
	v_double = gtk_spin_button_get_value(PreferenceData.filter_recents_percentage);
	wf_settings_static_set_double(WF_SETTING_FILTER_RECENT_PERCENTAGE, v_double);

	v_bool= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.filter_rating));
	wf_settings_static_set_bool(WF_SETTING_FILTER_RATING, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.filter_score));
	wf_settings_static_set_bool(WF_SETTING_FILTER_SCORE, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.filter_playcount));
	wf_settings_static_set_bool(WF_SETTING_FILTER_PLAYCOUNT, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.filter_skipcount));
	wf_settings_static_set_bool(WF_SETTING_FILTER_SKIPCOUNT, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.filter_lastplayed));
	wf_settings_static_set_bool(WF_SETTING_FILTER_LASTPLAYED, v_bool);

	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.rating_inc_zero));
	wf_settings_static_set_bool(WF_SETTING_FILTER_RATING_INC_ZERO, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.playcount_invert));
	wf_settings_static_set_bool(WF_SETTING_FILTER_PLAYCOUNT_INV, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.skipcount_invert));
	wf_settings_static_set_bool(WF_SETTING_FILTER_SKIPCOUNT_INV, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.lastplayed_invert));
	wf_settings_static_set_bool(WF_SETTING_FILTER_LASTPLAYED_INV, v_bool);

	v_int = gtk_spin_button_get_value_as_int(PreferenceData.rating_min);
	wf_settings_static_set_int(WF_SETTING_FILTER_RATING_MIN, v_int * 10);
	v_int = gtk_spin_button_get_value_as_int(PreferenceData.rating_max);
	wf_settings_static_set_int(WF_SETTING_FILTER_RATING_MAX, v_int * 10);
	v_double = gtk_spin_button_get_value(PreferenceData.score_min);
	wf_settings_static_set_double(WF_SETTING_FILTER_SCORE_MIN, v_double);
	v_double = gtk_spin_button_get_value(PreferenceData.score_max);
	wf_settings_static_set_double(WF_SETTING_FILTER_SCORE_MAX, v_double);
	v_int = gtk_spin_button_get_value_as_int(PreferenceData.playcount_th);
	wf_settings_static_set_int(WF_SETTING_FILTER_PLAYCOUNT_TH, v_int);
	v_int = gtk_spin_button_get_value_as_int(PreferenceData.skipcount_th);
	wf_settings_static_set_int(WF_SETTING_FILTER_SKIPCOUNT_TH, v_int);
	v_int64 = gtk_spin_button_get_value_as_int(PreferenceData.lastplayed_th);
	wf_settings_static_set_int64(WF_SETTING_FILTER_LASTPLAYED_TH, v_int64);

	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.use_rating));
	wf_settings_static_set_bool(WF_SETTING_MOD_RATING, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.invert_rating_prop));
	wf_settings_static_set_bool(WF_SETTING_MOD_RATING_INV, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.use_score));
	wf_settings_static_set_bool(WF_SETTING_MOD_SCORE, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.invert_score_prop));
	wf_settings_static_set_bool(WF_SETTING_MOD_SCORE_INV, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.use_playcount));
	wf_settings_static_set_bool(WF_SETTING_MOD_PLAYCOUNT, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.invert_playcount_prop));
	wf_settings_static_set_bool(WF_SETTING_MOD_PLAYCOUNT_INV, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.use_skipcount));
	wf_settings_static_set_bool(WF_SETTING_MOD_SKIPCOUNT, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.invert_skipcount_prop));
	wf_settings_static_set_bool(WF_SETTING_MOD_SKIPCOUNT_INV, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.use_lastplayed));
	wf_settings_static_set_bool(WF_SETTING_MOD_LASTPLAYED, v_bool);
	v_bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PreferenceData.invert_lastplayed_prop));
	wf_settings_static_set_bool(WF_SETTING_MOD_LASTPLAYED_INV, v_bool);
	v_int = gtk_spin_button_get_value_as_int(PreferenceData.rating_default);
	wf_settings_static_set_int(WF_SETTING_MOD_DEFAULT_RATING, v_int * 10);
	v_double = gtk_spin_button_get_value(PreferenceData.rating_multiplier);
	wf_settings_static_set_double(WF_SETTING_MOD_RATING_MULTI, v_double);
	v_double = gtk_spin_button_get_value(PreferenceData.score_multiplier);
	wf_settings_static_set_double(WF_SETTING_MOD_SCORE_MULTI, v_double);
	v_double = gtk_spin_button_get_value(PreferenceData.playcount_multiplier);
	wf_settings_static_set_double(WF_SETTING_MOD_PLAYCOUNT_MULTI, v_double);
	v_double = gtk_spin_button_get_value(PreferenceData.skipcount_multiplier);
	wf_settings_static_set_double(WF_SETTING_MOD_SKIPCOUNT_MULTI, v_double);
	v_double = gtk_spin_button_get_value(PreferenceData.lastplayed_multiplier);
	wf_settings_static_set_double(WF_SETTING_MOD_LASTPLAYED_MULTI, v_double);

	g_info("Preferences updated. Writing preferences to disk...");

	if (wf_settings_write())
	{
		preference_dialog_update_status("Preferences updated to disk");
		preference_dialog_set_apply_enabled(FALSE);

		preference_dialog_set_message("Preferences updated");

		wf_app_settings_updated();
	}
	else
	{
		preference_dialog_set_message("Could not write preferences");
		preference_dialog_update_status("Could not write preferences to disk");
	}
}

// Hide the dialog, but do not destroy it
static void
preference_dialog_close_cb(GtkWidget *button, gpointer user_data)
{
	preference_dialog_emit_close(&PreferenceData.events);

	preference_dialog_hide();
}

/* CALLBACK FUNCTIONS END */

/* MODULE FUNCTIONS BEGIN */

static void
preference_dialog_emit_close(PreferenceEvents *events)
{
	g_return_if_fail(events != NULL);

	if (events->close_func != NULL)
	{
		events->close_func(PreferenceData.current_message);
	}
}

static void
preference_dialog_update_status(gchar *message)
{
	gtk_statusbar_push(GTK_STATUSBAR(PreferenceData.status), PreferenceData.statusContextId, message);
}

static void
preference_dialog_set_apply_enabled(gboolean enable)
{
	gtk_widget_set_sensitive(PreferenceData.apply_button, enable);
}

void
preference_dialog_activate(GtkWindow *parent_window)
{
	if (!PreferenceData.constructed)
	{
		preference_dialog_construct(parent_window);
	}

	preference_dialog_update_widgets();

	gtk_widget_show(PreferenceData.dialog_widget);
}

void
preference_dialog_hide(void)
{
	if (!PreferenceData.constructed)
	{
		return;
	}

	gtk_widget_hide(PreferenceData.dialog_widget);

	gtk_statusbar_remove_all(GTK_STATUSBAR(PreferenceData.status), PreferenceData.statusContextId);
}

static void
preference_dialog_update_widgets(void)
{
	WfSongFilter *filters;
	WfSongEntries *entries;
	gint v_int;
	gint64 v_int64;
	gdouble v_double;
	gboolean v_bool;

	filters = wf_settings_get_filter();
	entries = wf_settings_get_song_entry_modifiers();

	g_return_if_fail(filters != NULL);
	g_return_if_fail(entries != NULL);

	PreferenceData.ignore_widget_updates = TRUE;

	// General settings; get setting in value and then set it to the widget
	v_int = interface_settings_get_notification(); // enum to int
	gtk_combo_box_set_active(PreferenceData.notifications, preference_dialog_get_notification_preference(v_int));
	v_int = wf_settings_static_get_int(WF_SETTING_UPDATE_INTERVAL);
	gtk_spin_button_set_value(PreferenceData.update_interval, v_int);
	v_bool = wf_settings_static_get_bool(WF_SETTING_PREFER_PLAY_FROM_RAM);
	gtk_switch_set_active(PreferenceData.prefer_play_ram, v_bool);
	v_int = interface_settings_get_last_played_timestamp();
	gtk_switch_set_active(PreferenceData.timestamp, v_int);
	v_double = wf_settings_static_get_double(WF_SETTING_MIN_PLAYED_FRACTION);
	gtk_spin_button_set_value(PreferenceData.min_play_percentage, v_double * 100.0);
	v_double = wf_settings_static_get_double(WF_SETTING_FULL_PLAYED_FRACTION);
	gtk_spin_button_set_value(PreferenceData.full_play_percentage, v_double * 100.0);

	// Filter settings; get setting and set it to the widget;
	v_int = wf_settings_static_get_int(WF_SETTING_FILTER_RECENT_ARTISTS);
	gtk_spin_button_set_value(PreferenceData.filter_recent_artists, v_int);
	v_int = wf_settings_static_get_int(WF_SETTING_FILTER_RECENT_AMOUNT);
	gtk_spin_button_set_value(PreferenceData.filter_recents_amount, v_int);
	v_double = wf_settings_static_get_double(WF_SETTING_FILTER_RECENT_PERCENTAGE);
	gtk_spin_button_set_value(PreferenceData.filter_recents_percentage, v_double);
	v_bool = wf_settings_static_get_bool(WF_SETTING_FILTER_RATING);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.filter_rating), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_FILTER_SCORE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.filter_score), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_FILTER_PLAYCOUNT);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.filter_playcount), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_FILTER_SKIPCOUNT);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.filter_skipcount), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_FILTER_LASTPLAYED);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.filter_lastplayed), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_FILTER_RATING_INC_ZERO);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.rating_inc_zero), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_FILTER_PLAYCOUNT_INV);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.playcount_invert), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_FILTER_SKIPCOUNT_INV);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.skipcount_invert), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_FILTER_LASTPLAYED_INV);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.lastplayed_invert), v_bool);
	v_int = wf_settings_static_get_int(WF_SETTING_FILTER_RATING_MIN);
	gtk_spin_button_set_value(PreferenceData.rating_min, wf_utils_round((gdouble) v_int / 10));
	v_int = wf_settings_static_get_int(WF_SETTING_FILTER_RATING_MAX);
	gtk_spin_button_set_value(PreferenceData.rating_max, wf_utils_round((gdouble) v_int / 10));
	v_double = wf_settings_static_get_double(WF_SETTING_FILTER_SCORE_MIN);
	gtk_spin_button_set_value(PreferenceData.score_min, v_double);
	v_double = wf_settings_static_get_double(WF_SETTING_FILTER_SCORE_MAX);
	gtk_spin_button_set_value(PreferenceData.score_max, v_double);
	v_int = wf_settings_static_get_int(WF_SETTING_FILTER_PLAYCOUNT_TH);
	gtk_spin_button_set_value(PreferenceData.playcount_th, v_int);
	v_int = wf_settings_static_get_int(WF_SETTING_FILTER_SKIPCOUNT_TH);
	gtk_spin_button_set_value(PreferenceData.skipcount_th, v_int);
	v_int64 = wf_settings_static_get_int64(WF_SETTING_FILTER_LASTPLAYED_TH);
	gtk_spin_button_set_value(PreferenceData.lastplayed_th, v_int64);

	// Probability settings
	v_bool = wf_settings_static_get_bool(WF_SETTING_MOD_RATING);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.use_rating), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_MOD_RATING_INV);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.invert_rating_prop), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_MOD_SCORE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.use_score), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_MOD_SCORE_INV);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.invert_score_prop), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_MOD_PLAYCOUNT);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.use_playcount), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_MOD_PLAYCOUNT_INV);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.invert_playcount_prop), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_MOD_SKIPCOUNT);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.use_skipcount), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_MOD_SKIPCOUNT_INV);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.invert_skipcount_prop), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_MOD_LASTPLAYED);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.use_lastplayed), v_bool);
	v_bool = wf_settings_static_get_bool(WF_SETTING_MOD_LASTPLAYED_INV);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PreferenceData.invert_lastplayed_prop), v_bool);

	// Default rating setting
	v_int = wf_settings_static_get_int(WF_SETTING_MOD_DEFAULT_RATING);
	gtk_spin_button_set_value(PreferenceData.rating_default, wf_utils_round((gdouble) v_int / 10));

	// Entry multiplier settings
	v_double = wf_settings_static_get_double(WF_SETTING_MOD_RATING_MULTI);
	gtk_spin_button_set_value(PreferenceData.rating_multiplier, v_double);
	v_double = wf_settings_static_get_double(WF_SETTING_MOD_SCORE_MULTI);
	gtk_spin_button_set_value(PreferenceData.score_multiplier, v_double);
	v_double = wf_settings_static_get_double(WF_SETTING_MOD_PLAYCOUNT_MULTI);
	gtk_spin_button_set_value(PreferenceData.playcount_multiplier, v_double);
	v_double = wf_settings_static_get_double(WF_SETTING_MOD_SKIPCOUNT_MULTI);
	gtk_spin_button_set_value(PreferenceData.skipcount_multiplier, v_double);
	v_double = wf_settings_static_get_double(WF_SETTING_MOD_LASTPLAYED_MULTI);
	gtk_spin_button_set_value(PreferenceData.lastplayed_multiplier, v_double);

	// Disable apply button
	preference_dialog_set_apply_enabled(FALSE);

	// Allow widget updates
	PreferenceData.ignore_widget_updates = FALSE;
}

/* MODULE FUNCTIONS END */

/* MODULE UTILITIES BEGIN */

static GtkAdjustment *
preference_dialog_adjustment_copy(GtkAdjustment *adjustment)
{
	GtkAdjustment *new_adjustment;
	gdouble value, lower, upper, step_increment, page_increment, page_size;

	g_return_val_if_fail(GTK_IS_ADJUSTMENT(adjustment), NULL);

	// Get values
	value = gtk_adjustment_get_value(adjustment);
	lower = gtk_adjustment_get_lower(adjustment);
	upper = gtk_adjustment_get_upper(adjustment);
	step_increment = gtk_adjustment_get_step_increment(adjustment);
	page_increment = gtk_adjustment_get_page_increment(adjustment);
	page_size = gtk_adjustment_get_page_size(adjustment);

	// Copy adjustment
	new_adjustment = gtk_adjustment_new(value,
	                                    lower,
	                                    upper,
	                                    step_increment,
	                                    page_increment,
	                                    page_size);

	return new_adjustment;
}

static NotificationSetting
preference_dialog_get_notification_setting(PreferenceNotifications x)
{
	switch (x)
	{
		case PREF_NOT_NEVER:
			return NOTIFICATIONS_NEVER;
		case PREF_NOT_HIDDEN_ONLY:
			return NOTIFICATIONS_HIDDEN_ONLY;
		case PREF_NOT_UNFOCUSED_ONLY:
			return NOTIFICATIONS_UNFOCUSED_ONLY;
		case PREF_NOT_ALWAYS:
			return NOTIFICATIONS_ALWAYS;
		default:
			g_warning("Unknown notification setting (%d)", x);
	}

	return NOTIFICATIONS_NEVER;
}

static PreferenceNotifications
preference_dialog_get_notification_preference(NotificationSetting x)
{
	switch (x)
	{
		case NOTIFICATIONS_NEVER:
			return PREF_NOT_NEVER;
		case NOTIFICATIONS_HIDDEN_ONLY:
			return PREF_NOT_HIDDEN_ONLY;
		case NOTIFICATIONS_UNFOCUSED_ONLY:
			return PREF_NOT_UNFOCUSED_ONLY;
		case NOTIFICATIONS_ALWAYS:
			return PREF_NOT_ALWAYS;
		default:
			g_warning("Unknown notification setting (%d)", x);
	}

	return PREF_NOT_NEVER;
}

static const gchar *
preference_dialog_get_notifications_str(PreferenceNotifications x)
{
	NotificationSetting sett;

	sett = preference_dialog_get_notification_setting(x);

	return interface_settings_get_notifications_pretty_str(sett);
}

/* MODULE UTILITIES END */

/* DESTRUCTORS BEGIN */

/*
 * Since gtk_window_set_destroy_with_parent() has been set during construction,
 * no widget destructors are needed when the application is about to quit.
 */

static void
preference_dialog_destruct(void)
{
	// Free this allocated list
	g_list_free(PreferenceData.list_boxes);

	// Reset all
	PreferenceData = (PreferenceDetails) { 0 };

	// Explicitly set @constructed to %FALSE
	PreferenceData.constructed = FALSE;
}

/* DESTRUCTORS END */

/* END OF FILE */
