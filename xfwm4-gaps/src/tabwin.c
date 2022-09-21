/*      $Id$

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2, or (at your option)
        any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., Inc., 51 Franklin Street, Fifth Floor, Boston,
        MA 02110-1301, USA.


        xfwm4    - (c) 2002-2015 Olivier Fourdan

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef WIN_ICON_SIZE
#define WIN_ICON_SIZE 48
#endif

#ifndef WIN_PREVIEW_SIZE
#define WIN_PREVIEW_SIZE (6 * WIN_ICON_SIZE)
#endif

#ifndef LISTVIEW_WIN_ICON_SIZE
#define LISTVIEW_WIN_ICON_SIZE (WIN_ICON_SIZE / 2)
#endif

#ifndef WIN_ICON_BORDER
#define WIN_ICON_BORDER 5
#endif

#ifndef WIN_MAX_RATIO
#define WIN_MAX_RATIO 0.80
#endif

#include <math.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include <common/xfwm-common.h>

#include "icons.h"
#include "focus.h"
#include "tabwin.h"
#include "settings.h"
#include "compositor.h"

#define XFWM_TABWIN_NAME "xfwm-tabwin"

static const gchar *xfwm_tabwin_default_css =
"#" XFWM_TABWIN_NAME " {"
"  padding: 4px;"
"  border-radius: 10px;"
"  border: 1px solid @theme_selected_bg_color;"
"  background-color: @theme_bg_color;"
"}";

static void tabwin_widget_class_init (TabwinWidgetClass *klass, gpointer data);

static GType
tabwin_widget_get_type (void)
{
    static GType type = G_TYPE_INVALID;

    if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
        static const GTypeInfo info =
        {
            sizeof (TabwinWidgetClass),
            NULL,
            NULL,
            (GClassInitFunc) tabwin_widget_class_init,
            NULL,
            NULL,
            sizeof (TabwinWidget),
            0,
            NULL,
            NULL,
        };

        type = g_type_register_static (GTK_TYPE_WINDOW, "XfwmTabwinWidget", &info, 0);
    }

    return type;
}

static gboolean
tabwin_draw (GtkWidget *tabwin_widget, cairo_t *cr, gpointer data)
{
    GtkAllocation    allocation;
    GtkStyleContext *ctx;
    GdkScreen       *screen;
    GdkRGBA         *bg_color = NULL;
    GdkRGBA         *border_color = NULL;
    GtkBorder        border = {0, };

    gtk_widget_get_allocation (tabwin_widget, &allocation);
    ctx = gtk_widget_get_style_context (tabwin_widget);
    screen = gtk_widget_get_screen (tabwin_widget);

    if (gdk_screen_is_composited (screen))
    {
        gtk_render_background (ctx, cr, 0, 0, allocation.width, allocation.height);
        gtk_render_frame (ctx, cr, 0, 0, allocation.width, allocation.height);
    }
    else
    {
        gtk_style_context_get (ctx, GTK_STATE_FLAG_NORMAL,
                               GTK_STYLE_PROPERTY_BACKGROUND_COLOR, &bg_color,
                               GTK_STYLE_PROPERTY_BORDER_COLOR, &border_color,
                               NULL);
        gtk_style_context_get_border (ctx, GTK_STATE_FLAG_NORMAL, &border);

        if (border_color != NULL)
        {
            border_color->alpha = 1;
            cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
            gdk_cairo_set_source_rgba (cr, border_color);
            cairo_fill (cr);
            gdk_rgba_free (border_color);
        }

        if (bg_color != NULL)
        {
            bg_color->alpha = 1;
            cairo_rectangle (cr, border.left, border.top,
                             allocation.width - border.left - border.right,
                             allocation.height - border.top - border.bottom);
            gdk_cairo_set_source_rgba (cr, bg_color);
            cairo_fill (cr);
            gdk_rgba_free (bg_color);
        }
    }

    return FALSE;
}

static void
apply_default_theme (TabwinWidget *tabwin_widget, ScreenInfo *screen_info)
{
    GtkSettings    *settings;
    gchar          *theme;
    GtkCssProvider *provider;
    gchar          *css;

    if (!screen_info->tabwin_provider_ready)
    {
        settings = gtk_settings_get_default ();

        g_object_get (settings, "gtk-theme-name", &theme, NULL);
        g_return_if_fail (theme != NULL);

        provider = gtk_css_provider_get_named (theme, NULL);
        g_return_if_fail (provider != NULL);

        css = gtk_css_provider_to_string (provider);
        if (g_strrstr (css, "#" XFWM_TABWIN_NAME) == NULL)
        {
            /* apply default css style */
            provider = gtk_css_provider_new ();
            gtk_css_provider_load_from_data (provider, xfwm_tabwin_default_css, -1, NULL);
            screen_info->tabwin_provider = provider;
        }
        g_free (css);

        screen_info->tabwin_provider_ready = TRUE;
    }

    if (screen_info->tabwin_provider != NULL)
    {
        gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (tabwin_widget)),
                                        GTK_STYLE_PROVIDER (screen_info->tabwin_provider),
                                        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
}

/* Efficiency is definitely *not* the goal here! */
static gchar *
pretty_string (const gchar *s)
{
    gchar *canonical;

    if (s)
    {
        canonical = g_strdup_printf ("%s",s);
        g_strcanon (canonical, "[]()0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", ' ');
        g_strstrip (canonical);
    }
    else
    {
        canonical = g_strdup ("...");
    }

    return canonical;
}

static void
tabwinSetLabel (TabwinWidget *tabwin_widget, GtkWidget *buttonlabel, gchar *class, gchar *label, int workspace)
{
    gchar *message;
    PangoLayout *layout;

    g_return_if_fail (tabwin_widget);
    TRACE ("class \"%s\", label \"%s\", workspace %i", class, label, workspace);

    message = pretty_string (class);
    gtk_label_set_text (GTK_LABEL (buttonlabel), message);
    g_free (message);

    if (tabwin_widget->tabwin->display_workspace)
    {
        message = g_strdup_printf ("[%i] - %s", workspace + 1, label);
    }
    else
    {
        message = g_strdup_printf ("%s", label);
    }

    gtk_label_set_text (GTK_LABEL (tabwin_widget->label), message);
    /* Need to update the layout after setting the text */
    layout = gtk_label_get_layout (GTK_LABEL(tabwin_widget->label));
    pango_layout_set_auto_dir (layout, FALSE);
    /* the layout belong to the gtk_label and must not be freed */

    g_free (message);
}

static void
tabwinSetSelected (TabwinWidget *tabwin_widget, GtkWidget *w, GtkWidget *l)
{
    Client *c;
    gchar *classname;

    g_return_if_fail (tabwin_widget);
    g_return_if_fail (GTK_IS_WIDGET(w));

    if (tabwin_widget->selected)
    {
        gtk_widget_unset_state_flags (tabwin_widget->selected, GTK_STATE_FLAG_CHECKED);
    }
    tabwin_widget->selected = w;
    gtk_widget_set_state_flags (w, GTK_STATE_FLAG_CHECKED, FALSE);
    c = g_object_get_data (G_OBJECT (tabwin_widget->selected), "client-ptr-val");

    if (c != NULL)
    {
        TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

        /* We don't update labels here */
        if (c->screen_info->params->cycle_tabwin_mode == OVERFLOW_COLUMN_GRID)
        {
            return;
        }

        classname = g_strdup(c->class.res_class);
        tabwinSetLabel (tabwin_widget, l, classname, c->name, c->win_workspace);
        g_free (classname);
    }
}

static Client *
tabwinSelectWidget (Tabwin *tabwin)
{
    GList *tabwin_list, *widgets, *selected, *children;
    GtkWidget *window_button, *buttonbox, *buttonlabel;
    TabwinWidget *tabwin_widget;
    Client *c;

    g_return_val_if_fail (tabwin != NULL, NULL);
    TRACE ("entering");

    for (tabwin_list = tabwin->tabwin_list; tabwin_list; tabwin_list = g_list_next (tabwin_list))
    {
        tabwin_widget = (TabwinWidget *) tabwin_list->data;
        for (widgets = tabwin_widget->widgets; widgets; widgets = g_list_next (widgets))
        {
            window_button = GTK_WIDGET (widgets->data);
            children = gtk_container_get_children (GTK_CONTAINER (window_button));
            buttonbox = GTK_WIDGET (g_list_nth_data (children, 0));
            g_list_free (children);

            children = gtk_container_get_children (GTK_CONTAINER (buttonbox));
            buttonlabel = GTK_WIDGET (g_list_nth_data (children, 1) );
            g_list_free (children);
            gtk_label_set_text (GTK_LABEL (buttonlabel), "");

            if (gtk_widget_is_focus (window_button))
            {
                c = g_object_get_data (G_OBJECT (window_button), "client-ptr-val");
                selected = g_list_find (*tabwin->client_list, c);

                if (selected)
                {
                    tabwin->selected = selected;
                }

                tabwinSetSelected (tabwin_widget, window_button, buttonlabel);
                gtk_widget_queue_draw (GTK_WIDGET (tabwin_widget));
            }
        }
    }

    return tabwinGetSelected (tabwin);
}

static GtkWidget *
createWindowIcon (GdkScreen *screen, GdkPixbuf *icon_pixbuf, gint size, gint scale)
{
    GtkIconTheme *icon_theme;
    GtkWidget * icon;
    cairo_surface_t *surface;

    TRACE ("entering");

    if (icon_pixbuf == NULL)
    {
        icon_theme = gtk_icon_theme_get_for_screen (screen);
        icon_pixbuf = gtk_icon_theme_load_icon (icon_theme, "xfwm4-default",
                                                size * scale, 0, NULL);
    }

    icon = gtk_image_new ();
    surface = gdk_cairo_surface_create_from_pixbuf (icon_pixbuf, scale, NULL);
    if (surface != NULL) {
        gtk_image_set_from_surface (GTK_IMAGE (icon), surface);
        cairo_surface_destroy (surface);
    }
    return icon;
}

static int
getMinMonitorWidth (ScreenInfo *screen_info)
{
    int i, min_width, num_monitors = myScreenGetNumMonitors (screen_info);
    for (min_width = i = 0; i < num_monitors; i++)
    {
        GdkRectangle monitor;
        xfwm_get_monitor_geometry (screen_info->gscr, i, &monitor, FALSE);
        if (min_width == 0 || monitor.width < min_width)
            min_width = monitor.width;
    }
    return min_width;
}

static int
getMinMonitorHeight (ScreenInfo *screen_info)
{
    int i, min_height, num_monitors = myScreenGetNumMonitors (screen_info);
    for (min_height = i = 0; i < num_monitors; i++)
    {
        GdkRectangle monitor;
        xfwm_get_monitor_geometry (screen_info->gscr, i, &monitor, FALSE);
        if (min_height == 0 || monitor.height < min_height)
        {
            min_height = monitor.height;
        }
    }
    return min_height;
}

static gboolean
cb_window_button_enter (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    TabwinWidget *tabwin_widget = user_data;
    Client *c;
    GtkWidget *buttonbox, *buttonlabel;
    GList *children;
    gchar *classname;

    TRACE ("entering");

    g_return_val_if_fail (tabwin_widget != NULL, FALSE);

    /* keep track of which widget we're hovering over */
    tabwin_widget->hovered = widget;

    /* make sure the hovered style is applied */
    if (tabwin_widget->hovered == tabwin_widget->selected)
        gtk_widget_unset_state_flags (tabwin_widget->selected, GTK_STATE_FLAG_CHECKED);

    c = g_object_get_data (G_OBJECT (widget), "client-ptr-val");

    /* when hovering over a window icon, display it's label but don't
     * select it */
    if (c != NULL)
    {
        /* we don't update the labels on mouse over for this mode */
        if (c->screen_info->params->cycle_tabwin_mode == OVERFLOW_COLUMN_GRID)
        {
            return FALSE;
        }

        children = gtk_container_get_children (GTK_CONTAINER (widget));
        buttonbox = GTK_WIDGET (g_list_nth_data (children, 0));
        g_list_free (children);

        children = gtk_container_get_children (GTK_CONTAINER (buttonbox));
        buttonlabel = GTK_WIDGET (g_list_nth_data (children, 1));
        g_list_free (children);

        classname = g_strdup (c->class.res_class);
        tabwinSetLabel (tabwin_widget, buttonlabel, classname, c->name, c->win_workspace);
        g_free (classname);
    }

    return FALSE;
}

static gboolean
cb_window_button_leave (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    TabwinWidget *tabwin_widget = user_data;
    Client *c;

    TRACE ("entering");

    g_return_val_if_fail (tabwin_widget != NULL, FALSE);

    tabwin_widget->hovered = NULL;

    /* don't do anything if we have the focus */
    if (gtk_widget_is_focus (widget))
    {
        gtk_widget_set_state_flags (tabwin_widget->selected, GTK_STATE_FLAG_CHECKED, FALSE);
        return FALSE;
    }

    c = g_object_get_data (G_OBJECT (widget), "client-ptr-val");

    /* when hovering over a window icon, display it's label but don't
     * select it */
    if (c != NULL)
    {
        /* we don't update the labels for this mode */
        if (c->screen_info->params->cycle_tabwin_mode == OVERFLOW_COLUMN_GRID)
        {
            return FALSE;
        }

        /* reselect the selected widget, it will clear everything else out */
        tabwinSelectWidget (tabwin_widget->tabwin);
    }

    return FALSE;
}

static GtkWidget *
createWindowlist (ScreenInfo *screen_info, TabwinWidget *tabwin_widget)
{
    Client *c;
    GList *client_list;
    GList *icon_list;
    GtkWidget *windowlist;
    GtkWidget *icon;
    GtkWidget *selected;
    GtkWidget *window_button;
    GtkWidget *buttonbox;
    GtkWidget *buttonlabel;
    GtkWidget *selected_label;
    GdkPixbuf *icon_pixbuf;
    gint packpos;
    gint label_width;
    gint size_request;
    Tabwin *tabwin;

    TRACE ("entering");
    g_return_val_if_fail (tabwin_widget != NULL, NULL);
    tabwin = tabwin_widget->tabwin;
    g_return_val_if_fail (tabwin->client_count > 0, NULL);

    packpos = 0;
    c = NULL;
    selected = NULL;
    tabwin_widget->widgets = NULL;
    size_request = tabwin->icon_size + tabwin->label_height + 2 * WIN_ICON_BORDER;

    windowlist = gtk_grid_new ();
    gtk_grid_set_row_homogeneous (GTK_GRID (windowlist), TRUE);
    gtk_grid_set_row_spacing (GTK_GRID (windowlist), 4);
    gtk_grid_set_column_homogeneous (GTK_GRID (windowlist), TRUE);
    gtk_grid_set_column_spacing (GTK_GRID (windowlist), 4);

    /* pack the client icons */
    icon_list = tabwin->icon_list;
    for (client_list = *tabwin->client_list; client_list; client_list = g_list_next (client_list))
    {
        c = (Client *) client_list->data;
        TRACE ("adding \"%s\" (0x%lx)", c->name, c->window);
        icon_pixbuf = (GdkPixbuf *) icon_list->data;
        icon_list = g_list_next (icon_list);

        window_button = gtk_button_new ();
        gtk_button_set_relief (GTK_BUTTON (window_button), GTK_RELIEF_NONE);
        g_object_set_data (G_OBJECT (window_button), "client-ptr-val", c);
        g_signal_connect (window_button, "enter-notify-event",
                          G_CALLBACK (cb_window_button_enter), tabwin_widget);
        g_signal_connect (window_button, "leave-notify-event",
                          G_CALLBACK (cb_window_button_leave), tabwin_widget);
        gtk_widget_add_events (window_button, GDK_ENTER_NOTIFY_MASK);

        icon = createWindowIcon (screen_info->gscr, icon_pixbuf, tabwin->icon_size, tabwin->icon_scale);
        if (screen_info->params->cycle_tabwin_mode == STANDARD_ICON_GRID)
        {
            gtk_widget_set_size_request (GTK_WIDGET (window_button), size_request, size_request);
            buttonbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
            buttonlabel = gtk_label_new ("");
            gtk_label_set_xalign (GTK_LABEL (buttonlabel), 0.5);
            gtk_label_set_yalign (GTK_LABEL (buttonlabel), 1.0);

            gtk_widget_set_halign (icon, GTK_ALIGN_CENTER);
            gtk_widget_set_valign (icon, GTK_ALIGN_END);
            gtk_box_pack_start (GTK_BOX (buttonbox), icon, TRUE, TRUE, 0);
        }
        else
        {
            label_width = tabwin->monitor_width / (tabwin->grid_cols + 1);

            if (tabwin->icon_size < tabwin->label_height)
            {
                gtk_widget_set_size_request (GTK_WIDGET (window_button),
                                             label_width, tabwin->label_height + 8);
            }
            else
            {
                gtk_widget_set_size_request (GTK_WIDGET (window_button),
                                             label_width, tabwin->icon_size + 8);
            }
            buttonbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
            buttonlabel = gtk_label_new (c->name);
            gtk_label_set_xalign (GTK_LABEL (buttonlabel), 0);
            gtk_label_set_yalign (GTK_LABEL (buttonlabel), 0.5);

            gtk_widget_set_halign (icon, GTK_ALIGN_CENTER);
            gtk_widget_set_valign (icon, GTK_ALIGN_CENTER);
            gtk_box_pack_start (GTK_BOX (buttonbox), icon, FALSE, FALSE, 0);
        }
        gtk_container_add (GTK_CONTAINER (window_button), buttonbox);

        gtk_label_set_justify (GTK_LABEL (buttonlabel), GTK_JUSTIFY_CENTER);
        gtk_label_set_ellipsize (GTK_LABEL (buttonlabel), PANGO_ELLIPSIZE_END);
        gtk_box_pack_start (GTK_BOX (buttonbox), buttonlabel, TRUE, TRUE, 0);

        if (screen_info->params->cycle_tabwin_mode == STANDARD_ICON_GRID)
        {
            gtk_grid_attach (GTK_GRID (windowlist), GTK_WIDGET (window_button),
                             packpos % tabwin->grid_cols, packpos / tabwin->grid_cols,
                             1, 1);
        }
        else
        {
            gtk_grid_attach (GTK_GRID (windowlist), GTK_WIDGET (window_button),
                             packpos / tabwin->grid_rows, packpos % tabwin->grid_rows,
                             1, 1);
        }

        tabwin_widget->widgets = g_list_append (tabwin_widget->widgets, window_button);
        packpos++;
        if (c == tabwin->selected->data)
        {
            selected = window_button;
            selected_label = buttonlabel;
            gtk_widget_grab_focus (selected);
        }
    }
    if (selected)
    {
        tabwinSetSelected (tabwin_widget, selected, selected_label);
    }
    return windowlist;
}

static gboolean
tabwinConfigure (TabwinWidget *tabwin_widget, GdkEventConfigure *event)
{
    GdkRectangle monitor;
    gint x, y;

    g_return_val_if_fail (tabwin_widget != NULL, FALSE);
    TRACE ("entering");

    if ((tabwin_widget->width == event->width) && (tabwin_widget->height == event->height))
    {
        return FALSE;
    }

    xfwm_get_monitor_geometry (gtk_widget_get_screen (GTK_WIDGET (tabwin_widget)),
                               tabwin_widget->monitor_num, &monitor, FALSE);
    x = monitor.x + (monitor.width - event->width) / 2;
    y = monitor.y + (monitor.height - event->height) / 2;
    gtk_window_move (GTK_WINDOW (tabwin_widget), x, y);

    tabwin_widget->width = event->width;
    tabwin_widget->height = event->height;

    return FALSE;
}

static void
tabwin_widget_class_init (TabwinWidgetClass *klass, gpointer data)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    gtk_widget_class_install_style_property (widget_class,
                                             g_param_spec_int ("preview-size",
                                                               "preview size",
                                                                "the size of the application preview",
                                                                48, 512,
                                                                WIN_PREVIEW_SIZE,
                                                                G_PARAM_READABLE));
    gtk_widget_class_install_style_property (widget_class,
                                             g_param_spec_int ("icon-size",
                                                               "icon size",
                                                                "the size of the application icon",
                                                                24, 128,
                                                                WIN_ICON_SIZE,
                                                                G_PARAM_READABLE));
    gtk_widget_class_install_style_property (widget_class,
                                             g_param_spec_int ("listview-icon-size",
                                                               "listview icon size",
                                                               "the size of the application icon in listview",
                                                               16, 48,
                                                               LISTVIEW_WIN_ICON_SIZE,
                                                               G_PARAM_READABLE));
}

static void
computeTabwinData (ScreenInfo *screen_info, TabwinWidget *tabwin_widget)
{
    Tabwin *tabwin;
    GdkPixbuf *icon_pixbuf;
    PangoLayout *layout;
    GList *client_list;
    gint size_request;
    gint standard_icon_size;
    gboolean preview;

    TRACE ("entering");
    g_return_if_fail (GTK_IS_WIDGET(tabwin_widget));
    tabwin = tabwin_widget->tabwin;
    g_return_if_fail (tabwin->client_count > 0);

    tabwin->monitor_width = getMinMonitorWidth (screen_info);
    tabwin->monitor_height = getMinMonitorHeight (screen_info);
    tabwin->label_height = 30;
    preview = screen_info->params->cycle_preview && compositorIsActive (screen_info);
    tabwin->icon_scale = gtk_widget_get_scale_factor (GTK_WIDGET (tabwin_widget));

    /* We need to account for changes to the font size in the user's
     * appearance theme and gtkrc settings */
    layout = gtk_widget_create_pango_layout (GTK_WIDGET (tabwin_widget), "");
    pango_layout_get_pixel_size (layout, NULL, &tabwin->label_height);
    g_object_unref (layout);

    if (screen_info->params->cycle_tabwin_mode == STANDARD_ICON_GRID)
    {
        tabwin->icon_size = WIN_ICON_SIZE;
        gtk_widget_style_get (GTK_WIDGET (tabwin_widget),
                              "icon-size", &tabwin->icon_size, NULL);
        standard_icon_size = tabwin->icon_size;

        if (preview)
        {
            tabwin->icon_size = WIN_PREVIEW_SIZE;
            gtk_widget_style_get (GTK_WIDGET (tabwin_widget),
                                  "preview-size", &tabwin->icon_size, NULL);
        }
        else
        {
            tabwin->icon_size = standard_icon_size;
        }
        size_request = tabwin->icon_size + tabwin->label_height + 2 * WIN_ICON_BORDER;
        tabwin->grid_cols = (int) (floor ((double) tabwin->monitor_width * WIN_MAX_RATIO /
                                          (double) size_request));
        tabwin->grid_rows = (int) (ceil ((double) tabwin->client_count /
                                         (double) tabwin->grid_cols));

        /* If we run out of space, halve the icon size to make more room. */
        while ((size_request) * tabwin->grid_rows + tabwin->label_height >
               ((double) tabwin->monitor_height) * WIN_MAX_RATIO)
        {
            tabwin->icon_size = tabwin->icon_size / 2;
            if (preview && tabwin->icon_size <= standard_icon_size)
            {
                /* Disable preview otherwise it'd be too slow */
                preview = FALSE;
                /* switch back to regular icon size */
                tabwin->icon_size = standard_icon_size;
            }
            size_request = tabwin->icon_size + tabwin->label_height + 2 * WIN_ICON_BORDER;

            /* Recalculate with new icon size */
            tabwin->grid_cols = (int) (floor ((double) tabwin->monitor_width * WIN_MAX_RATIO /
                                              (double) size_request));
            tabwin->grid_rows = (int) (ceil ((double) tabwin->client_count /
                                             (double) tabwin->grid_cols));

            /* Shrinking the icon too much makes it hard to see */
            if (tabwin->icon_size < 8)
            {
                tabwin->icon_size = 8;
                break;
            }
        }
    }
    else
    {
        tabwin->icon_size = LISTVIEW_WIN_ICON_SIZE;
        gtk_widget_style_get (GTK_WIDGET (tabwin_widget),
                              "listview-icon-size", &tabwin->icon_size, NULL);
        tabwin->grid_rows = (int) (floor ((double) tabwin->monitor_height * WIN_MAX_RATIO /
                                          (double) (tabwin->icon_size + 2 * WIN_ICON_BORDER)));
        tabwin->grid_cols = (int) (ceil ((double) tabwin->client_count /
                                         (double) tabwin->grid_rows));
    }

    /* pack the client icons */
    for (client_list = *tabwin->client_list; client_list; client_list = g_list_next (client_list))
    {
        Client *c = (Client *) client_list->data;

        if (screen_info->params->cycle_tabwin_mode == STANDARD_ICON_GRID)
        {
            if (preview)
            {
                icon_pixbuf = getClientIcon (c, tabwin->icon_size * tabwin->icon_scale,
                                             tabwin->icon_size * tabwin->icon_scale);
            }
            else
            {
                icon_pixbuf = getAppIcon (c, tabwin->icon_size * tabwin->icon_scale,
                                          tabwin->icon_size * tabwin->icon_scale);
            }
        }
        else
        {
            /* No preview in list mode */
            icon_pixbuf = getAppIcon (c, tabwin->icon_size * tabwin->icon_scale,
                                      tabwin->icon_size * tabwin->icon_scale);
        }
        tabwin->icon_list = g_list_append(tabwin->icon_list, icon_pixbuf);
    }
}

static TabwinWidget *
tabwinCreateWidget (Tabwin *tabwin, ScreenInfo *screen_info, gint monitor_num)
{
    TabwinWidget *tabwin_widget;
    GdkScreen *screen;
    GdkVisual *visual;
    GtkStyleContext *ctx;
    GtkWidget *vbox;
    GtkWidget *windowlist;
    GdkRectangle monitor;
    gint border_radius = 0;
    GtkBorder border, padding;

    TRACE ("monitor %i", monitor_num);

    tabwin_widget = g_object_new (tabwin_widget_get_type(), "type", GTK_WINDOW_POPUP, NULL);

    tabwin_widget->monitor_num = monitor_num;
    tabwin_widget->tabwin = tabwin;
    tabwin_widget->selected = NULL;
    tabwin_widget->selected_callback = 0;
    tabwin_widget->width = -1;
    tabwin_widget->height = -1;

    gtk_window_set_screen (GTK_WINDOW (tabwin_widget), screen_info->gscr);
    gtk_window_set_default_size (GTK_WINDOW (tabwin_widget), 0, 0);
    gtk_widget_set_name (GTK_WIDGET (tabwin_widget), XFWM_TABWIN_NAME);
    apply_default_theme (tabwin_widget, screen_info);

    /* Check for compositing and set visual for it */
    screen = gtk_widget_get_screen (GTK_WIDGET (tabwin_widget));
    if (gdk_screen_is_composited (screen)) {
        visual = gdk_screen_get_rgba_visual (screen);
        if (visual)
        {
            gtk_widget_set_visual (GTK_WIDGET (tabwin_widget), visual);
        }
    }
    gtk_widget_set_app_paintable (GTK_WIDGET (tabwin_widget), TRUE);
    gtk_widget_realize (GTK_WIDGET (tabwin_widget));

    if (tabwin->icon_list == NULL)
    {
        computeTabwinData (screen_info, tabwin_widget);
    }
    ctx = gtk_widget_get_style_context (GTK_WIDGET (tabwin_widget));
    gtk_style_context_get (ctx, GTK_STATE_FLAG_NORMAL,
                           GTK_STYLE_PROPERTY_BORDER_RADIUS, &border_radius,
                           NULL);
    gtk_style_context_get_border (ctx, GTK_STATE_FLAG_NORMAL, &border);
    gtk_style_context_get_padding (ctx, GTK_STATE_FLAG_NORMAL, &padding);
    gtk_container_set_border_width (GTK_CONTAINER (tabwin_widget),
                                    border_radius +
                                    MAX (border.left, MAX (border.top, (MAX (border.right, border.bottom)))) +
                                    MAX (padding.left, MAX (padding.top, (MAX (padding.right, padding.bottom)))));
    gtk_window_set_position (GTK_WINDOW (tabwin_widget), GTK_WIN_POS_NONE);
    xfwm_get_monitor_geometry (screen_info->gscr, tabwin_widget->monitor_num, &monitor, FALSE);
    gtk_window_move (GTK_WINDOW (tabwin_widget), monitor.x + monitor.width / 2,
                                      monitor.y + monitor.height / 2);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
    gtk_container_add (GTK_CONTAINER (tabwin_widget), vbox);

    if (screen_info->params->cycle_tabwin_mode == STANDARD_ICON_GRID)
    {
        tabwin_widget->label = gtk_label_new ("");
        gtk_label_set_use_markup (GTK_LABEL (tabwin_widget->label), TRUE);
        gtk_label_set_justify (GTK_LABEL (tabwin_widget->label), GTK_JUSTIFY_CENTER);
        gtk_label_set_ellipsize (GTK_LABEL (tabwin_widget->label), PANGO_ELLIPSIZE_END);
        gtk_box_pack_end (GTK_BOX (vbox), tabwin_widget->label, TRUE, TRUE, 0);

        gtk_style_context_add_class (ctx, "tabwin-app-grid");
    }
    else
    {
        gtk_style_context_add_class (ctx, "tabwin-app-list");
    }

    windowlist = createWindowlist (screen_info, tabwin_widget);
    tabwin_widget->container = windowlist;
    gtk_box_pack_start (GTK_BOX (vbox), windowlist, TRUE, TRUE, 0);

    g_signal_connect_swapped (tabwin_widget, "configure-event",
                              G_CALLBACK (tabwinConfigure),
                              (gpointer) tabwin_widget);

    g_signal_connect (tabwin_widget, "draw",
                      G_CALLBACK (tabwin_draw),
                      (gpointer) tabwin_widget);

    gtk_widget_show_all (GTK_WIDGET (tabwin_widget));

    return tabwin_widget;
}

static Client *
tabwinChange2Selected (Tabwin *tabwin, GList *selected)
{
    GList *tabwin_list, *widgets, *children;
    GtkWidget *window_button, *buttonbox, *buttonlabel;
    TabwinWidget *tabwin_widget;
    Client *c;

    tabwin->selected = selected;
    for (tabwin_list = tabwin->tabwin_list; tabwin_list; tabwin_list = g_list_next (tabwin_list))
    {
        tabwin_widget = (TabwinWidget *) tabwin_list->data;
        for (widgets = tabwin_widget->widgets; widgets; widgets = g_list_next (widgets))
        {
            window_button = GTK_WIDGET (widgets->data);

            children = gtk_container_get_children (GTK_CONTAINER (window_button));
            buttonbox = GTK_WIDGET (g_list_nth_data (children, 0));
            g_list_free (children);

            children = gtk_container_get_children (GTK_CONTAINER (buttonbox));
            buttonlabel = GTK_WIDGET (g_list_nth_data (children, 1));
            g_list_free (children);

            c = g_object_get_data (G_OBJECT (window_button), "client-ptr-val");

            if (c != NULL)
            {
                /* don't clear label if mouse is inside the previously
                 * selected button */
                if (c->screen_info->params->cycle_tabwin_mode == STANDARD_ICON_GRID
                    && window_button != tabwin_widget->hovered)
                {
                    gtk_label_set_text (GTK_LABEL (buttonlabel), "");
                }

                if (c == tabwin->selected->data)
                {
                    gtk_widget_grab_focus (window_button);
                    tabwinSetSelected (tabwin_widget, window_button, buttonlabel);
                    gtk_widget_queue_draw (GTK_WIDGET (tabwin_widget));
                }
            }
        }
    }
    return tabwinGetSelected (tabwin);
}

Tabwin *
tabwinCreate (GList **client_list, GList *selected, gboolean display_workspace)
{
    ScreenInfo *screen_info;
    Client *c;
    Tabwin *tabwin;
    TabwinWidget *win;
    int num_monitors, i;
    gboolean has_primary;

    g_return_val_if_fail (selected, NULL);
    g_return_val_if_fail (client_list, NULL);
    g_return_val_if_fail (*client_list, NULL);

    c = (Client *) selected->data;
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    tabwin = g_new0 (Tabwin, 1);
    screen_info = c->screen_info;
    tabwin->display_workspace = display_workspace;
    tabwin->client_list = client_list;
    tabwin->client_count = g_list_length (*client_list);
    tabwin->selected = selected;
    tabwin->tabwin_list = NULL;
    tabwin->icon_list = NULL;

    num_monitors = myScreenGetNumMonitors (screen_info);
    has_primary = myScreenHasPrimaryMonitor (screen_info, c->window);

    for (i = 0; i < num_monitors; i++)
    {
        gint monitor_index;

        monitor_index = myScreenGetMonitorIndex (screen_info, i);

        if (has_primary &&
            !xfwm_monitor_is_primary (screen_info->gscr, monitor_index))
        {
            continue;
        }

        win = tabwinCreateWidget (tabwin, screen_info, monitor_index);
        tabwin->tabwin_list  = g_list_append (tabwin->tabwin_list, win);
    }

    return tabwin;
}

Client *
tabwinGetSelected (Tabwin *tabwin)
{
    g_return_val_if_fail (tabwin != NULL, NULL);
    TRACE ("entering");

    if (tabwin->selected)
    {
        return (Client *) tabwin->selected->data;
    }

    return NULL;
}

Client *
tabwinRemoveClient (Tabwin *tabwin, Client *c)
{
    GList *client_list, *tabwin_list, *widgets;
    GtkWidget *icon;
    TabwinWidget *tabwin_widget;

    g_return_val_if_fail (tabwin != NULL, NULL);
    g_return_val_if_fail (c != NULL, NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (!*tabwin->client_list)
    {
        return NULL;
    }

    /* First, remove the client from our own client list */
    for (client_list = *tabwin->client_list; client_list; client_list = g_list_next (client_list))
    {
        if (client_list->data == c)
        {
            if (client_list == tabwin->selected)
            {
                tabwinSelectNext (tabwin);
            }
            *tabwin->client_list = g_list_delete_link (*tabwin->client_list, client_list);
            break;
        }
    }

    /* Second, remove the icon from all boxes */
    for (tabwin_list = tabwin->tabwin_list; tabwin_list; tabwin_list = g_list_next (tabwin_list))
    {
        tabwin_widget = (TabwinWidget *) tabwin_list->data;
        for (widgets = tabwin_widget->widgets; widgets; widgets = g_list_next (widgets))
        {
            icon = GTK_WIDGET (widgets->data);
            if (((Client *) g_object_get_data (G_OBJECT(icon), "client-ptr-val")) == c)
            {
                gtk_container_remove (GTK_CONTAINER (tabwin_widget->container), icon);
                tabwin_widget->widgets = g_list_delete_link (tabwin_widget->widgets, widgets);
            }
        }
    }

    return tabwinGetSelected (tabwin);
}

Client *
tabwinSelectHead (Tabwin *tabwin)
{
    GList *head, *tabwin_list, *widgets, *children;
    GtkWidget *window_button, *buttonbox, *buttonlabel;
    TabwinWidget *tabwin_widget;

    g_return_val_if_fail (tabwin != NULL, NULL);
    TRACE ("entering");

    head = *tabwin->client_list;
    if (!head)
    {
        return NULL;
    }
    tabwin->selected = head;
    for (tabwin_list = tabwin->tabwin_list; tabwin_list; tabwin_list = g_list_next (tabwin_list))
    {
        tabwin_widget = (TabwinWidget *) tabwin_list->data;
        for (widgets = tabwin_widget->widgets; widgets; widgets = g_list_next (widgets))
        {
            window_button = GTK_WIDGET (widgets->data);

            children = gtk_container_get_children (GTK_CONTAINER (window_button));
            buttonbox = GTK_WIDGET (g_list_nth_data (children, 0));
            g_list_free (children);

            children = gtk_container_get_children (GTK_CONTAINER (buttonbox));
            buttonlabel = GTK_WIDGET (g_list_nth_data (children, 1));
            g_list_free (children);

            if (((Client *) g_object_get_data (G_OBJECT (window_button), "client-ptr-val")) == head->data)
            {
                tabwinSetSelected (tabwin_widget, window_button, buttonlabel);
                gtk_widget_queue_draw (GTK_WIDGET (tabwin_widget));
            }
        }
    }

    return tabwinGetSelected (tabwin);
}

Client *
tabwinSelectNext (Tabwin *tabwin)
{
    GList *next;

    g_return_val_if_fail (tabwin != NULL, NULL);
    TRACE ("entering");

    next = g_list_next (tabwin->selected);
    if (!next)
    {
        next = *tabwin->client_list;
        g_return_val_if_fail (next != NULL, NULL);
    }
    return tabwinChange2Selected (tabwin, next);
}

Client *
tabwinSelectPrev (Tabwin *tabwin)
{
    GList *prev;

    g_return_val_if_fail (tabwin != NULL, NULL);
    TRACE ("entering");

    prev = g_list_previous (tabwin->selected);
    if (!prev)
    {
        prev = g_list_last (*tabwin->client_list);
        g_return_val_if_fail (prev != NULL, NULL);
    }
    return tabwinChange2Selected (tabwin, prev);
}

Client *
tabwinSelectDelta (Tabwin *tabwin, int row_delta, int col_delta)
{
    GList *selected;
    int pos_current, col_current, row_current, nitems, cols, rows;
    Client *c;
    ScreenInfo *screen_info;

    TRACE ("entering");
    g_return_val_if_fail (tabwin != NULL, NULL);

    pos_current = g_list_position (*tabwin->client_list, tabwin->selected);
    nitems = g_list_length (*tabwin->client_list);

    if (tabwin->selected)
    {
        c = (Client *) tabwin->selected->data;
        screen_info = c->screen_info;
        if (!screen_info)
        {
            return NULL;
        }
    }
    else if (tabwin->client_list)
    {
        c = (Client *) tabwin->client_list;
        screen_info = c->screen_info;
        if (!screen_info)
        {
            return NULL;
        }
    }
    else
    {
        /* There's no items? */
        return NULL;
    }

    if (screen_info->params->cycle_tabwin_mode == STANDARD_ICON_GRID)
    {
        cols = MIN (nitems, tabwin->grid_cols);
        rows = (nitems - 1) / cols + 1;
        col_current = pos_current % cols;
        row_current = pos_current / cols;
    }
    else
    {
        rows = MIN (nitems, tabwin->grid_rows);
        cols = (nitems - 1) / rows + 1;
        row_current = pos_current % rows;
        col_current = pos_current / rows;
    }

    /* Wrap column */
    col_current += col_delta;
    if (col_current < 0)
    {
        col_current = cols - 1;
        row_current--;
        if (row_current < 0)
        {
            row_current = rows - 1;
        }
    }
    else if (col_current >= cols)
    {
        col_current = 0;
        if (rows > 1)
        {
            row_current++;
        }
        else
        {
            /* If there's only 1 row then col needs to wrap back to
             * the head of the grid */
            col_current = 0;
        }
    }

    /* Wrap row */
    row_current += row_delta;
    if (row_current < 0)
    {
        row_current = rows - 1;
        col_current--;
        if (col_current < 0)
        {
            col_current = cols - 1;
        }
    }
    else if (row_current >= rows)
    {
        row_current = 0;
        col_current++;
        if (col_current >= cols)
        {
            if (rows != 1)
            {
                col_current = cols - 1;
            }
            else
            {
                /* If there's only 1 row then col needs to wrap back to
                 * the head of the grid */
                col_current = 0;
            }
        }
    }

    /* So here we are at the new (wrapped) position in the rectangle */
    if (screen_info->params->cycle_tabwin_mode == STANDARD_ICON_GRID)
    {
        pos_current = col_current + row_current * cols;
    }
    else
    {
        pos_current = row_current + col_current * rows;
    }

    if (pos_current >= nitems)   /* If that position does not exist */
    {
        if (col_delta)            /* Let horizontal prevail */
        {
            if (col_delta > 0)
            {
                if (screen_info->params->cycle_tabwin_mode == STANDARD_ICON_GRID)
                {
                    /* In this case we're going past the tail, reset to the head
                     * of the grid */
                    col_current = 0;
                    row_current = 0;
                }
                else
                {
                    col_current = 0;
                    row_current++;
                    if (row_current >= rows)
                    {
                        row_current = 0;
                    }
                }
            }
            else
            {
                if (screen_info->params->cycle_tabwin_mode == STANDARD_ICON_GRID)
                {
                    col_current = (nitems - 1) % cols;
                }
                else
                {
                    col_current = cols - 1;
                    row_current = (nitems - 1) % rows;
                }
            }
        }
        else
        {
            if (row_delta > 0)
            {
                row_current = 0;
                col_current++;
                if (col_current >= cols)
                {
                    col_current = 0;
                }
            }
            else
            {
                if (screen_info->params->cycle_tabwin_mode == STANDARD_ICON_GRID)
                {
                    row_current = row_current - 1;
                }
                else
                {
                    row_current = (nitems - 1) % rows;
                    col_current = cols - 1;
                }
            }

        }

        if (screen_info->params->cycle_tabwin_mode == STANDARD_ICON_GRID)
        {
            pos_current = col_current + row_current * cols;
        }
        else
        {
            pos_current = row_current + col_current * rows;
        }
    }

    selected = g_list_nth (*tabwin->client_list, pos_current);

    return tabwinChange2Selected (tabwin, selected);
}

Client*
tabwinSelectHovered (Tabwin *tabwin)
{
    GList *tabwin_list, *selected;
    TabwinWidget *tabwin_widget;
    Client *c;

    g_return_val_if_fail (tabwin != NULL, NULL);
    TRACE ("entering");

    for (tabwin_list = tabwin->tabwin_list; tabwin_list; tabwin_list = g_list_next (tabwin_list))
    {
        tabwin_widget = (TabwinWidget *) tabwin_list->data;
        if (tabwin_widget->hovered)
        {
            gtk_widget_grab_focus (tabwin_widget->hovered);
            c = g_object_get_data (G_OBJECT (tabwin_widget->hovered), "client-ptr-val");
            selected = g_list_find (*tabwin->client_list, c);
            if (selected)
            {
                tabwin->selected = selected;
            }
            return c;
        }
    }

    return tabwinSelectWidget (tabwin);
}

void
tabwinDestroy (Tabwin *tabwin)
{
    GList *tabwin_list;
    TabwinWidget *tabwin_widget;

    g_return_if_fail (tabwin != NULL);
    TRACE ("entering");

    for (tabwin_list = tabwin->tabwin_list; tabwin_list; tabwin_list = g_list_next (tabwin_list))
    {
        tabwin_widget = (TabwinWidget *) tabwin_list->data;
        g_list_free (tabwin_widget->widgets);
        gtk_widget_destroy (GTK_WIDGET (tabwin_widget));
    }
    g_list_free_full (tabwin->icon_list, g_object_unref);
    g_list_free (tabwin->tabwin_list);
}
