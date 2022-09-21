/*
 *  Copyright (c) 2008 Brian Tarricone <bjt23@cornell.edu>
 *  Copyright (c) 2008 Stephan Arts <stephan@xfce.org>
 *  Copyright (c) 2008 Jannis Pohlmann <jannis@xfce.org>
 *  Copyright (c) 2008 Mike Massonnet <mmassonnet@xfce.org>
 *  Copyright (c) 2008 Olivier Fourdan <olivier@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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


#include <config.h>
#include <string.h>

#include <glib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>

#ifdef XFCONF_LEGACY
#include <dbus/dbus-glib.h>
#endif
#include <libwnck/libwnck.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <xfconf/xfconf.h>

#include <common/xfwm-common.h>

#include "xfwm4-workspace-dialog_ui.h"

#define WORKSPACES_CHANNEL         "xfwm4"

#define WORKSPACE_NAMES_PROP       "/general/workspace_names"
#define WORKSPACE_COUNT_PROP       "/general/workspace_count"

static Window opt_socket_id = 0;
static gboolean opt_version = FALSE;


enum
{
    COL_NUMBER = 0,
    COL_NAME,
    N_COLS,
};

static void
workspace_names_update_xfconf(gint workspace,
                              const gchar *new_name)
{
    WnckScreen *screen = wnck_screen_get_default();
    XfconfChannel *channel;
    gchar **names;
    gboolean do_update_xfconf = TRUE;

    channel = xfconf_channel_get(WORKSPACES_CHANNEL);
    names = xfconf_channel_get_string_list(channel, WORKSPACE_NAMES_PROP);

    if(!names) {
        /* the property doesn't exist; let's build one from scratch */
        gint i, n_workspaces = wnck_screen_get_workspace_count(screen);

        names = g_new0 (gchar *, n_workspaces + 1);
        for(i = 0; i < n_workspaces; ++i) {
            if(G_LIKELY(i != workspace))
                names[i] = g_strdup_printf(_("Workspace %d"), i + 1);
            else
                names[i] = g_strdup(new_name);
        }
        names[n_workspaces] = NULL;
    } else {
        gint i, prop_len = g_strv_length(names);
        gint n_workspaces = wnck_screen_get_workspace_count(screen);

        if(prop_len < n_workspaces) {
            /* the property exists, but it's smaller than the current
             * actual number of workspaces */
            names = g_realloc(names, sizeof(gchar *) * (n_workspaces + 1));
            for(i = prop_len; i < n_workspaces; ++i) {
                if(i != workspace)
                    names[i] = g_strdup_printf(_("Workspace %d"), i + 1);
                else
                    names[i] = g_strdup(new_name);
            }
            names[n_workspaces] = NULL;
        } else {
            /* here we may have a |names| array longer than the actual
             * number of workspaces, but that's fine.  the user might
             * want to re-add a workspace or whatever, and may appreciate
             * that we remember the old name. */
            if(strcmp(names[workspace], new_name)) {
                g_free(names[workspace]);
                names[workspace] = g_strdup(new_name);
            } else {
                /* nothing's actually changed, so don't update the xfconf
                 * property.  this saves us some trouble later. */
                do_update_xfconf = FALSE;
            }
        }
    }

    if(do_update_xfconf)
        xfconf_channel_set_string_list(channel, WORKSPACE_NAMES_PROP, (const gchar **)names);

    g_strfreev(names);
}

static void
treeview_ws_names_cell_edited (GtkCellRendererText *cell,
                               const gchar         *path_string,
                               const gchar         *new_name,
                               gpointer             user_data)
{
    GtkTreeView *treeview;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gchar *old_name = NULL;
    gint ws_num = 1;

    treeview = (GtkTreeView *) user_data;
    model = gtk_tree_view_get_model(treeview);
    path = gtk_tree_path_new_from_string (path_string);
    gtk_tree_model_get_iter (model, &iter, path);

    gtk_tree_model_get(model, &iter, COL_NUMBER, &ws_num, COL_NAME, &old_name, -1);
    if(strcmp(old_name, new_name)) {
        gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_NAME, new_name, -1);
        workspace_names_update_xfconf(ws_num - 1, new_name);
    }

    g_free(old_name);

    gtk_tree_path_free (path);
}


static void
xfconf_workspace_names_update(GPtrArray *names,
                              GtkTreeView *treeview)
{
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    WnckScreen *screen = wnck_screen_get_default();
    guint i, n_workspaces;
    GtkTreePath *path;
    GtkTreeIter iter;

    g_return_if_fail(GTK_IS_TREE_VIEW(treeview));

    n_workspaces = wnck_screen_get_workspace_count(screen);
    for(i = 0; i < n_workspaces && i < names->len; ++i) {
        GValue *val = g_ptr_array_index(names, i);
        const gchar *new_name;

        if(!G_VALUE_HOLDS_STRING(val)) {
            g_warning("(workspace names) Expected string but got %s for item %d",
                      G_VALUE_TYPE_NAME(val), i);
            continue;
        }

        new_name = g_value_get_string(val);

        path = gtk_tree_path_new_from_indices(i, -1);
        if(gtk_tree_model_get_iter(model, &iter, path)) {
            gchar *old_name = NULL;

            gtk_tree_model_get(model, &iter,
                               COL_NAME, &old_name,
                               -1);
            /* only update the names that have actually changed */
            if(strcmp(old_name, new_name)) {
                gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                                   COL_NAME, new_name,
                                   -1);
            }
            g_free(old_name);
        } else {
            /* must be a new workspace */
            gtk_list_store_append(GTK_LIST_STORE(model), &iter);
            gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                               COL_NUMBER, i + 1,
                               COL_NAME, new_name,
                               -1);
        }

        gtk_tree_path_free(path);
    }

    /* if workspaces got destroyed, we need to remove them from the treeview */
    path = gtk_tree_path_new_from_indices(n_workspaces, -1);
    while(gtk_tree_model_get_iter(model, &iter, path))
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
    gtk_tree_path_free(path);
}



static void
xfconf_workspace_names_changed(XfconfChannel *channel,
                               const gchar *property,
                               const GValue *value,
                               gpointer user_data)
{
    GPtrArray *names;

#ifdef XFCONF_LEGACY
    if(G_VALUE_TYPE(value) !=  dbus_g_type_get_collection("GPtrArray",
                                                          G_TYPE_VALUE))
#else
    if(G_VALUE_TYPE(value) != G_TYPE_PTR_ARRAY)
#endif
    {
        g_warning("(workspace names) Expected boxed GPtrArray property, got %s",
                  G_VALUE_TYPE_NAME(value));
        return;
    }

    names = g_value_get_boxed(value);
    if(!names)
        return;

    xfconf_workspace_names_update(names, user_data);
}



static void
workspace_dialog_count_changed(GtkTreeView *treeview)
{
    GPtrArray *names;
    XfconfChannel *channel;

    channel = xfconf_channel_get(WORKSPACES_CHANNEL);

    names = xfconf_channel_get_arrayv (channel, WORKSPACE_NAMES_PROP);
    if(names != NULL)
    {
        xfconf_workspace_names_update(names, treeview);
        xfconf_array_free(names);
    }
}


static void
workspace_dialog_setup_names_treeview(GtkBuilder *builder,
                                      XfconfChannel *channel)
{
    GtkWidget *treeview;
    GtkListStore *ls;
    GtkCellRenderer *render;
    GtkTreeViewColumn *col;
    WnckScreen *screen;

    treeview = GTK_WIDGET (gtk_builder_get_object(builder, "treeview_ws_names"));

    ls = gtk_list_store_new(N_COLS, G_TYPE_INT, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(ls));

    render = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("#", render,
                                                   "text", COL_NUMBER,
                                                   NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);

    render = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(render),
                 "editable", TRUE,
                 "ellipsize", PANGO_ELLIPSIZE_END,
                 "ellipsize-set", TRUE,
                 NULL);
    col = gtk_tree_view_column_new_with_attributes(_("Workspace Name"),
                                                   render,
                                                   "text", COL_NAME,
                                                   NULL);
    g_signal_connect (render, "edited", G_CALLBACK (treeview_ws_names_cell_edited), treeview);

    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);

    screen = wnck_screen_get_default();
    wnck_screen_force_update (screen);

    workspace_dialog_count_changed (GTK_TREE_VIEW (treeview));

    /* watch ws count changes */
    g_signal_connect_swapped(G_OBJECT(screen), "workspace-created",
                             G_CALLBACK (workspace_dialog_count_changed), treeview);
    g_signal_connect_swapped(G_OBJECT(screen), "workspace-destroyed",
                             G_CALLBACK (workspace_dialog_count_changed), treeview);

    g_signal_connect(G_OBJECT(channel),
                     "property-changed::" WORKSPACE_NAMES_PROP,
                     G_CALLBACK(xfconf_workspace_names_changed), treeview);
}



static void
workspace_dialog_configure_widgets (GtkBuilder *builder,
                                    XfconfChannel *channel)
{
    GtkWidget *vbox;

    GdkPixbuf *monitor;
    GtkWidget *image;

    gint wmax, hmax;

    GtkWidget *workspace_count_spinbutton = GTK_WIDGET (gtk_builder_get_object (builder, "workspace_count_spinbutton"));

    GtkWidget *margin_top_spinbutton = GTK_WIDGET (gtk_builder_get_object (builder, "margin_top_spinbutton"));
    GtkWidget *margin_right_spinbutton = GTK_WIDGET (gtk_builder_get_object (builder, "margin_right_spinbutton"));
    GtkWidget *margin_bottom_spinbutton = GTK_WIDGET (gtk_builder_get_object (builder, "margin_bottom_spinbutton"));
    GtkWidget *margin_left_spinbutton = GTK_WIDGET (gtk_builder_get_object (builder, "margin_left_spinbutton"));

    /* Set monitor icon */
    monitor = gdk_pixbuf_new_from_resource ("/org/xfce/xfwm4/monitor-icon.pixdata", NULL);
    if(G_LIKELY (monitor != NULL))
    {
        image = GTK_WIDGET (gtk_builder_get_object (builder, "monitor_icon"));
        gtk_image_set_from_pixbuf (GTK_IMAGE (image), monitor);
        g_object_unref (monitor);
    }

    /* Set max margins range */
    xfwm_get_screen_dimensions (&wmax, &hmax);
    wmax /= 4;
    hmax /= 4;

    gtk_spin_button_set_range (GTK_SPIN_BUTTON (margin_top_spinbutton), 0, hmax);
    gtk_spin_button_set_range (GTK_SPIN_BUTTON (margin_right_spinbutton), 0, wmax);
    gtk_spin_button_set_range (GTK_SPIN_BUTTON (margin_bottom_spinbutton), 0, hmax);
    gtk_spin_button_set_range (GTK_SPIN_BUTTON (margin_left_spinbutton), 0, wmax);

    /* Bind easy properties */
    xfconf_g_property_bind (channel,
                            "/general/workspace_count",
                            G_TYPE_INT,
                            (GObject *)workspace_count_spinbutton, "value");

    xfconf_g_property_bind (channel,
                            "/general/margin_top",
                            G_TYPE_INT,
                            (GObject *)margin_top_spinbutton, "value");
    xfconf_g_property_bind (channel,
                            "/general/margin_right",
                            G_TYPE_INT,
                            (GObject *)margin_right_spinbutton, "value");
    xfconf_g_property_bind (channel,
                            "/general/margin_bottom",
                            G_TYPE_INT,
                            (GObject *)margin_bottom_spinbutton, "value");
    xfconf_g_property_bind (channel,
                            "/general/margin_left",
                            G_TYPE_INT,
                            (GObject *)margin_left_spinbutton, "value");

    workspace_dialog_setup_names_treeview(builder, channel);

    vbox = GTK_WIDGET (gtk_builder_get_object (builder, "main-vbox"));

    gtk_widget_show_all(vbox);
}


static void
workspace_dialog_response (GtkWidget *dialog,
                           gint response_id)
{
    if (response_id == GTK_RESPONSE_HELP)
    {
        xfce_dialog_show_help (GTK_WINDOW (dialog), "xfwm4",
                               "workspaces", NULL);
    }
    else
    {
        gtk_main_quit ();
    }
}


static GOptionEntry entries[] =
{
    { "socket-id", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_INT, &opt_socket_id, N_("Settings manager socket"), N_("SOCKET ID") },
    { "version", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &opt_version, N_("Version information"), NULL },
    { NULL }
};


int
main(int argc, gchar **argv)
{
    GtkBuilder *builder;
    GtkWidget *dialog;
    GtkWidget *plug;
    GtkWidget *plug_child;
    XfconfChannel *channel;
    GError *cli_error = NULL;

    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    if(!gtk_init_with_args(&argc, &argv, _("."), entries, PACKAGE, &cli_error))
    {
        if (cli_error != NULL)
        {
            g_print (_("%s: %s\nTry %s --help to see a full list of available command line options.\n"), PACKAGE, cli_error->message, PACKAGE_NAME);
            g_error_free (cli_error);
            return 1;
        }
    }

    if(opt_version)
    {
        g_print("%s\n", PACKAGE_STRING);
        return 0;
    }

    if(!xfconf_init (&cli_error)) {
        g_critical ("Failed to contact xfconfd: %s", cli_error->message);
        g_error_free (cli_error);
        return 1;
    }

    channel = xfconf_channel_get(WORKSPACES_CHANNEL);

    if (xfce_titled_dialog_get_type () == 0)
      return 1;

    builder = gtk_builder_new();
    gtk_builder_add_from_string(builder, workspace_dialog_ui, workspace_dialog_ui_length, NULL);

    if(builder) {
        workspace_dialog_configure_widgets (builder, channel);

        if(opt_socket_id == 0) {
            dialog = GTK_WIDGET (gtk_builder_get_object (builder, "main-dialog"));
            gtk_widget_show (dialog);
            g_signal_connect (dialog, "response", G_CALLBACK (workspace_dialog_response), NULL);

            /* To prevent the settings dialog to be saved in the session */
            gdk_x11_set_sm_client_id ("FAKE ID");

            gtk_main ();

            gtk_widget_destroy(dialog);
        } else {
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
    }

    xfconf_shutdown();

    return 0;
}
