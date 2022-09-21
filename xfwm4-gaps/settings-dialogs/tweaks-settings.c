/*
 *  Copyright (c) 2008 Stephan Arts <stephan@xfce.org>
 *  Copyright (c) 2008 Jannis Pohlmann <jannis@xfce.org>
 *  Copyright (c) 2008 Olivier Fourdan <olivier@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either opt_version 2 of the License, or
 *  (at your option) any later opt_version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <glib.h>

#if defined (GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>

#include <libxfce4ui/libxfce4ui.h>
#include <xfconf/xfconf.h>

#include <common/xfwm-common.h>

#include "xfwm4-tweaks-dialog_ui.h"
#include "range-debouncer.h"

static Window opt_socket_id = 0;
static gboolean opt_version = FALSE;

static const gchar *const modifier_list[] = {
    N_("None"),
    "Alt",
    "Control",
    "Hyper",
    "Meta",
    "Shift",
    "Super",
    "Mod1",
    "Mod2",
    "Mod3",
    "Mod4",
    "Mod5",
    NULL
};

static void
cb_easy_click_combo_box_changed (GtkComboBox *combo, XfconfChannel *channel)
{
    gint n;

    n = 0;
    while (modifier_list[n])
    {
        if (gtk_combo_box_get_active (combo) == n)
        {
            xfconf_channel_set_string (channel, "/general/easy_click", _(modifier_list[n]));
        }
        n++;
    }
}

static void
cb_use_compositing_check_button_toggled (GtkToggleButton *toggle, GtkWidget *box)
{
    gtk_widget_set_sensitive (box, gtk_toggle_button_get_active (toggle));
}
#if 0
static void
cb_prevent_focus_stealing_check_button_toggled (GtkToggleButton *toggle, GtkWidget *box)
{
    gtk_widget_set_sensitive (box, gtk_toggle_button_get_active (toggle));
}
#endif
static void
cb_activate_action_bring_radio_toggled (GtkToggleButton *toggle, XfconfChannel *channel)
{
    if (gtk_toggle_button_get_active (toggle))
    {
        xfconf_channel_set_string (channel, "/general/activate_action", "bring");
    }
}

static void
cb_activate_action_switch_radio_toggled (GtkToggleButton *toggle, XfconfChannel *channel)
{
    if (gtk_toggle_button_get_active (toggle))
    {
        xfconf_channel_set_string (channel, "/general/activate_action", "switch");
    }
}

static void
cb_activate_action_none_radio_toggled (GtkToggleButton *toggle, XfconfChannel *channel)
{
    if (gtk_toggle_button_get_active (toggle))
    {
        xfconf_channel_set_string (channel, "/general/activate_action", "none");
    }
}

static void
cb_activate_placement_center_radio_toggled (GtkToggleButton *toggle, XfconfChannel *channel)
{
    if (gtk_toggle_button_get_active (toggle))
    {
        xfconf_channel_set_string (channel, "/general/placement_mode", "center");
    }
}

static void
cb_activate_placement_mouse_radio_toggled (GtkToggleButton *toggle, XfconfChannel *channel)
{
    if (gtk_toggle_button_get_active (toggle))
    {
        xfconf_channel_set_string (channel, "/general/placement_mode", "mouse");
    }
}

static void
cb_urgent_blink_button_toggled (GtkToggleButton *toggle, GtkWidget *repeat_urgent_blink)
{
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (repeat_urgent_blink), FALSE);
    gtk_widget_set_sensitive (repeat_urgent_blink, gtk_toggle_button_get_active (toggle));
}

static void
cb_tile_on_move_check_toggled (GtkToggleButton *toggle, XfconfChannel *channel)
{
    if (gtk_toggle_button_get_active (toggle))
    {
        xfconf_channel_set_bool (channel, "/general/wrap_windows", FALSE);
    }
}

static void
cb_borderless_maximize_button_toggled (GtkToggleButton *toggle, GtkWidget *titleless_maximize_check)
{
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (titleless_maximize_check), FALSE);
    gtk_widget_set_sensitive (titleless_maximize_check, gtk_toggle_button_get_active (toggle));
}

static void
wm_tweaks_dialog_configure_widgets (GtkBuilder *builder)
{
    GtkWidget *vbox;
    GtkTreeIter iter;
    GtkListStore *list_store;
    GtkCellRenderer *renderer;
    XfconfChannel *xfwm4_channel = xfconf_channel_new ("xfwm4");
    gchar *easy_click = NULL;
    gchar *activate_action = NULL;
    gchar *default_placement = NULL;
    gboolean modifier_set = FALSE;
    guint n;

    /* Cycling tab */
    GtkWidget *cycle_workspaces_check = GTK_WIDGET (gtk_builder_get_object (builder, "cycle_workspaces_check"));
    GtkWidget *cycle_hidden_check = GTK_WIDGET (gtk_builder_get_object (builder, "cycle_hidden_check"));
    GtkWidget *cycle_minimum_check = GTK_WIDGET (gtk_builder_get_object (builder, "cycle_minimum_check"));
    GtkWidget *cycle_minimized_check = GTK_WIDGET (gtk_builder_get_object (builder, "cycle_minimized_check"));
    GtkWidget *cycle_draw_frame = GTK_WIDGET (gtk_builder_get_object (builder, "cycle_draw_frame"));
    GtkWidget *cycle_raise = GTK_WIDGET (gtk_builder_get_object (builder, "cycle_raise"));
    GtkWidget *cycle_tabwin_mode = GTK_WIDGET (gtk_builder_get_object (builder, "cycle_tabwin_mode"));

    /* Focus tab */
    GtkWidget *prevent_focus_stealing_check = GTK_WIDGET (gtk_builder_get_object (builder, "prevent_focus_stealing_check"));
    GtkWidget *focus_hint_check = GTK_WIDGET (gtk_builder_get_object (builder, "focus_hint_check"));

    GtkWidget *activate_action_bring_option = GTK_WIDGET (gtk_builder_get_object (builder, "activate_action_bring_option"));
    GtkWidget *activate_action_switch_option = GTK_WIDGET (gtk_builder_get_object (builder, "activate_action_switch_option"));
    GtkWidget *activate_action_none_option = GTK_WIDGET (gtk_builder_get_object (builder, "activate_action_none_option"));

    /* Accessibility tab */
    GtkWidget *easy_click_combo_box = GTK_WIDGET (gtk_builder_get_object (builder, "easy_click_combo_box"));
    GtkWidget *raise_with_any_button_check = GTK_WIDGET (gtk_builder_get_object (builder, "raise_with_any_button_check"));
    GtkWidget *borderless_maximize_check = GTK_WIDGET (gtk_builder_get_object (builder, "borderless_maximize_check"));
    GtkWidget *titleless_maximize_check = GTK_WIDGET (gtk_builder_get_object (builder, "titleless_maximize_check"));
    GtkWidget *tile_on_move_check = GTK_WIDGET (gtk_builder_get_object (builder, "tile_on_move_check"));
    GtkWidget *snap_resist_check = GTK_WIDGET (gtk_builder_get_object (builder, "snap_resist_check"));
    GtkWidget *urgent_blink = GTK_WIDGET (gtk_builder_get_object (builder, "urgent_blink"));
    GtkWidget *repeat_urgent_blink = GTK_WIDGET (gtk_builder_get_object (builder, "repeat_urgent_blink"));
    GtkWidget *mousewheel_rollup = GTK_WIDGET (gtk_builder_get_object (builder, "mousewheel_rollup"));

    /* Workspaces tab */
    GtkWidget *scroll_workspaces_check = GTK_WIDGET (gtk_builder_get_object (builder, "scroll_workspaces_check"));
    GtkWidget *toggle_workspaces_check = GTK_WIDGET (gtk_builder_get_object (builder, "toggle_workspaces_check"));
    GtkWidget *wrap_layout_check = GTK_WIDGET (gtk_builder_get_object (builder, "wrap_layout_check"));
    GtkWidget *wrap_cycle_check = GTK_WIDGET (gtk_builder_get_object (builder, "wrap_cycle_check"));

    /* Placement tab */
    GtkWidget *placement_ratio_scale = GTK_WIDGET (gtk_builder_get_object (builder, "placement_ratio_scale"));
    GtkWidget *placement_center_option = GTK_WIDGET (gtk_builder_get_object (builder, "placement_center_option"));
    GtkWidget *placement_mouse_option = GTK_WIDGET (gtk_builder_get_object (builder, "placement_mouse_option"));

    /* Compositing tab */
    GtkWidget *use_compositing_check = GTK_WIDGET (gtk_builder_get_object (builder, "use_compositing_check"));
    GtkWidget *use_compositing_box = GTK_WIDGET (gtk_builder_get_object (builder, "use_compositing_box"));

    GtkWidget *unredirect_overlays_check = GTK_WIDGET (gtk_builder_get_object (builder, "unredirect_overlays_check"));
    GtkWidget *cycle_preview_check = GTK_WIDGET (gtk_builder_get_object (builder, "cycle_preview_check"));
    GtkWidget *show_frame_shadow_check = GTK_WIDGET (gtk_builder_get_object (builder, "show_frame_shadow_check"));
    GtkWidget *show_popup_shadow_check = GTK_WIDGET (gtk_builder_get_object (builder, "show_popup_shadow_check"));
    GtkWidget *show_dock_shadow_check = GTK_WIDGET (gtk_builder_get_object (builder, "show_dock_shadow_check"));
    GtkWidget *zoom_desktop_check = GTK_WIDGET (gtk_builder_get_object (builder, "zoom_desktop_check"));
    GtkWidget *zoom_pointer_check = GTK_WIDGET (gtk_builder_get_object (builder, "zoom_pointer_check"));

    GtkWidget *frame_opacity_scale = GTK_WIDGET (gtk_builder_get_object (builder, "frame_opacity_scale"));
    GtkWidget *inactive_opacity_scale = GTK_WIDGET (gtk_builder_get_object (builder, "inactive_opacity_scale"));
    GtkWidget *move_opacity_scale = GTK_WIDGET (gtk_builder_get_object (builder, "move_opacity_scale"));
    GtkWidget *popup_opacity_scale = GTK_WIDGET (gtk_builder_get_object (builder, "popup_opacity_scale"));
    GtkWidget *resize_opacity_scale = GTK_WIDGET (gtk_builder_get_object (builder, "resize_opacity_scale"));


    /* Fill combo-box */
    list_store = gtk_list_store_new (1, G_TYPE_STRING);

    easy_click = xfconf_channel_get_string (xfwm4_channel, "/general/easy_click", "Alt");
    gtk_cell_layout_clear (GTK_CELL_LAYOUT (easy_click_combo_box));
    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (easy_click_combo_box), renderer, TRUE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (easy_click_combo_box), renderer, "text", 0);
    gtk_combo_box_set_model (GTK_COMBO_BOX (easy_click_combo_box), GTK_TREE_MODEL (list_store));
    for (n = 0; modifier_list[n]; n++)
    {
        gtk_list_store_append (list_store, &iter);
        gtk_list_store_set (list_store, &iter, 0, N_(modifier_list[n]), -1);
        if (!modifier_set && !strcmp (easy_click, modifier_list[n]))
        {
            gtk_combo_box_set_active (GTK_COMBO_BOX (easy_click_combo_box), n);
            modifier_set = TRUE;
        }
    }
    /* If not specified, set to "None" */
    if (!modifier_set)
        gtk_combo_box_set_active (GTK_COMBO_BOX (easy_click_combo_box), 0);

    activate_action = xfconf_channel_get_string (xfwm4_channel, "/general/activate_action", "bring");
    if (!strcmp (activate_action, "switch"))
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (activate_action_bring_option), FALSE);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (activate_action_switch_option), TRUE);
    }
    if (!strcmp (activate_action, "none"))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (activate_action_none_option), TRUE);

    default_placement = xfconf_channel_get_string (xfwm4_channel, "/general/placement_mode", "center");
    if (!strcmp (default_placement, "mouse"))
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (placement_center_option), FALSE);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (placement_mouse_option), TRUE);
    }

    /* not so easy properties */
    g_signal_connect (G_OBJECT (activate_action_bring_option),
                      "toggled",
                      G_CALLBACK (cb_activate_action_bring_radio_toggled),
                      xfwm4_channel);
    g_signal_connect (G_OBJECT (activate_action_switch_option),
                      "toggled",
                      G_CALLBACK (cb_activate_action_switch_radio_toggled),
                      xfwm4_channel);
    g_signal_connect (G_OBJECT (activate_action_none_option),
                      "toggled",
                      G_CALLBACK (cb_activate_action_none_radio_toggled),
                      xfwm4_channel);
    g_signal_connect (G_OBJECT (borderless_maximize_check),
                      "toggled",
                      G_CALLBACK (cb_borderless_maximize_button_toggled),
                      titleless_maximize_check);
    g_signal_connect (G_OBJECT (placement_center_option),
                      "toggled",
                      G_CALLBACK (cb_activate_placement_center_radio_toggled),
                      xfwm4_channel);
    g_signal_connect (G_OBJECT (placement_mouse_option),
                      "toggled",
                      G_CALLBACK (cb_activate_placement_mouse_radio_toggled),
                      xfwm4_channel);
    g_signal_connect (G_OBJECT (use_compositing_check),
                      "toggled",
                      G_CALLBACK (cb_use_compositing_check_button_toggled),
                      use_compositing_box);
    g_signal_connect (G_OBJECT (easy_click_combo_box),
                      "changed",
                      G_CALLBACK (cb_easy_click_combo_box_changed),
                      xfwm4_channel);
    g_signal_connect (G_OBJECT (urgent_blink),
                      "toggled",
                      G_CALLBACK (cb_urgent_blink_button_toggled),
                      repeat_urgent_blink);
    g_signal_connect (G_OBJECT (tile_on_move_check),
                      "toggled",
                      G_CALLBACK (cb_tile_on_move_check_toggled),
                      xfwm4_channel);

    /* Bind easy properties */
    /* Cycling tab */
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/cycle_minimum",
                            G_TYPE_BOOLEAN,
                            (GObject *)cycle_minimum_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/cycle_minimized",
                            G_TYPE_BOOLEAN,
                            (GObject *)cycle_minimized_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/cycle_hidden",
                            G_TYPE_BOOLEAN,
                            (GObject *)cycle_hidden_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/cycle_workspaces",
                            G_TYPE_BOOLEAN,
                            (GObject *)cycle_workspaces_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/cycle_draw_frame",
                            G_TYPE_BOOLEAN,
                            (GObject *)cycle_draw_frame, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/cycle_raise",
                            G_TYPE_BOOLEAN,
                            (GObject *)cycle_raise, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/cycle_tabwin_mode",
                            G_TYPE_INT,
                            (GObject *)cycle_tabwin_mode, "active");

    /* Focus tab */
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/prevent_focus_stealing",
                            G_TYPE_BOOLEAN,
                            (GObject *)prevent_focus_stealing_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/focus_hint",
                            G_TYPE_BOOLEAN,
                            (GObject *)focus_hint_check, "active");

    /* Accessibility tab */
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/raise_with_any_button",
                            G_TYPE_BOOLEAN,
                            (GObject *)raise_with_any_button_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/borderless_maximize",
                            G_TYPE_BOOLEAN,
                            (GObject *)borderless_maximize_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/titleless_maximize",
                            G_TYPE_BOOLEAN,
                            (GObject *)titleless_maximize_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/tile_on_move",
                            G_TYPE_BOOLEAN,
                            (GObject *)tile_on_move_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/snap_resist",
                            G_TYPE_BOOLEAN,
                            (GObject *)snap_resist_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/urgent_blink",
                            G_TYPE_BOOLEAN,
                            (GObject *)urgent_blink, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/repeat_urgent_blink",
                            G_TYPE_BOOLEAN,
                            (GObject *)repeat_urgent_blink, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/mousewheel_rollup",
                            G_TYPE_BOOLEAN,
                            (GObject *)mousewheel_rollup, "active");
    gtk_widget_set_sensitive (repeat_urgent_blink,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (urgent_blink)));
    gtk_widget_set_sensitive (titleless_maximize_check,
                              gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (borderless_maximize_check)));

    /* Workspaces tab */
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/toggle_workspaces",
                            G_TYPE_BOOLEAN,
                            (GObject *)toggle_workspaces_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/scroll_workspaces",
                            G_TYPE_BOOLEAN,
                            (GObject *)scroll_workspaces_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/wrap_layout",
                            G_TYPE_BOOLEAN,
                            (GObject *)wrap_layout_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/wrap_cycle",
                            G_TYPE_BOOLEAN,
                            (GObject *)wrap_cycle_check, "active");

    /* Placement tab */
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/placement_ratio",
                            G_TYPE_INT,
                            (GObject *) gtk_range_get_adjustment (GTK_RANGE (placement_ratio_scale)), "value");

    /* Compositing tab */
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/use_compositing",
                            G_TYPE_BOOLEAN,
                            (GObject *)use_compositing_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/unredirect_overlays",
                            G_TYPE_BOOLEAN,
                            (GObject *)unredirect_overlays_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/cycle_preview",
                            G_TYPE_BOOLEAN,
                            (GObject *)cycle_preview_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/show_frame_shadow",
                            G_TYPE_BOOLEAN,
                            (GObject *)show_frame_shadow_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/show_popup_shadow",
                            G_TYPE_BOOLEAN,
                            (GObject *)show_popup_shadow_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/show_dock_shadow",
                            G_TYPE_BOOLEAN,
                            (GObject *)show_dock_shadow_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/zoom_desktop",
                            G_TYPE_BOOLEAN,
                            (GObject *)zoom_desktop_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/zoom_pointer",
                            G_TYPE_BOOLEAN,
                            (GObject *)zoom_pointer_check, "active");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/frame_opacity",
                            G_TYPE_INT,
                            (GObject *) range_debouncer_bind (GTK_RANGE (frame_opacity_scale)), "value");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/resize_opacity",
                            G_TYPE_INT,
                            (GObject *) range_debouncer_bind (GTK_RANGE (resize_opacity_scale)), "value");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/move_opacity",
                            G_TYPE_INT,
                            (GObject *) range_debouncer_bind (GTK_RANGE (move_opacity_scale)), "value");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/inactive_opacity",
                            G_TYPE_INT,
                            (GObject *) range_debouncer_bind (GTK_RANGE (inactive_opacity_scale)), "value");
    xfconf_g_property_bind (xfwm4_channel,
                            "/general/popup_opacity",
                            G_TYPE_INT,
                            (GObject *) range_debouncer_bind (GTK_RANGE (popup_opacity_scale)), "value");

    vbox = GTK_WIDGET (gtk_builder_get_object (builder, "main-vbox"));
    gtk_widget_show_all (vbox);

    g_free (easy_click);
    g_free (activate_action);
}

static void
wm_tweaks_dialog_response (GtkWidget *dialog,
                           gint response_id)
{
    if (response_id == GTK_RESPONSE_HELP)
    {
        xfce_dialog_show_help (GTK_WINDOW (dialog), "xfwm4",
                               "wmtweaks", NULL);
    }
    else
    {
        gtk_main_quit ();
    }
}

static GOptionEntry entries[] =
{
    { "socket-id", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_INT, &opt_socket_id, N_ ("Session manager socket"), N_ ("SOCKET ID") },
    { "version", 'v', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &opt_version, N_ ("Version information"), NULL },
    { NULL }
};

int
main (int argc, gchar **argv)
{
    GtkWidget *dialog;
    GtkWidget *plug;
    GtkWidget *plug_child;
    GtkBuilder *builder;
    GError *cli_error = NULL;
    const gchar  *wm_name;

#ifdef ENABLE_NLS
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");
#endif

    if (!gtk_init_with_args (&argc, &argv, _ ("."), entries, PACKAGE, &cli_error))
    {
        if (cli_error != NULL)
        {
            g_print (_ ("%s: %s\nTry %s --help to see a full list of available command line options.\n"), PACKAGE, cli_error->message, PACKAGE_NAME);
            g_error_free (cli_error);
            return 1;
        }
    }

    wm_name = gdk_x11_screen_get_window_manager_name (gdk_screen_get_default ());
    if (G_UNLIKELY (g_ascii_strcasecmp (wm_name, "Xfwm4")))
    {
        g_print ("These settings cannot work with your current window manager (%s)\n", wm_name);
        return 1;
    }

    if (opt_version)
    {
        g_print ("%s\n", PACKAGE_STRING);
        return 0;
    }

    xfconf_init (NULL);

    builder = gtk_builder_new ();

    if (xfce_titled_dialog_get_type () == 0)
      return 1;

    gtk_builder_add_from_string (builder, tweaks_dialog_ui, tweaks_dialog_ui_length, NULL);

    if (builder)
    {
        wm_tweaks_dialog_configure_widgets (builder);

        if (opt_socket_id == 0)
        {
            dialog = GTK_WIDGET (gtk_builder_get_object (builder, "main-dialog"));
            gtk_widget_show (dialog);
            g_signal_connect (dialog, "response", G_CALLBACK (wm_tweaks_dialog_response), NULL);

            /* To prevent the settings dialog to be saved in the session */
            gdk_x11_set_sm_client_id ("FAKE ID");

            gtk_main ();

            gtk_widget_destroy (dialog);
        }
        else
        {
            /* Create plug widget */
            plug = gtk_plug_new (opt_socket_id);
            g_signal_connect (plug, "delete-event", G_CALLBACK (gtk_main_quit), NULL);
            gtk_widget_show (plug);

            /* Get plug child widget */
            plug_child = GTK_WIDGET (gtk_builder_get_object (builder, "plug-child"));
            xfwm_widget_reparent (plug_child, plug);
            gtk_widget_show (plug_child);

            /* To prevent the settings dialog to be saved in the session */
            gdk_x11_set_sm_client_id ("FAKE ID");

            /* Stop startup notification */
            gdk_notify_startup_complete ();

            /* Enter main loop */
            gtk_main ();
        }

        g_object_unref (builder);
    }

    xfconf_shutdown ();

    return 0;
}
