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


        Metacity - (c) 2001 Havoc Pennington
        xfwm4    - (c) 2002-2011 Olivier Fourdan

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <string.h>
#include <libxfce4util/libxfce4util.h>

#include <common/xfwm-common.h>

#include "event_filter.h"
#include "menu.h"
#include "misc.h"

enum
{
    GRAB_DEVICE_POINTER,
    GRAB_DEVICE_KEYBOARD
};

static GtkWidget *menu_open = NULL;
static MenuItem menuitems[] = {
    {MENU_OP_MAXIMIZE,     MENU_TYPE_REGULAR,  N_("Ma_ximize")},
    {MENU_OP_UNMAXIMIZE,   MENU_TYPE_REGULAR,  N_("Unma_ximize")},
    {MENU_OP_MINIMIZE,     MENU_TYPE_REGULAR,  N_("Mi_nimize")},
    {MENU_OP_MINIMIZE_ALL, MENU_TYPE_REGULAR,  N_("Minimize _Other Windows")},
    {MENU_OP_UNMINIMIZE,   MENU_TYPE_REGULAR,  N_("S_how")},
    {MENU_OP_MOVE,         MENU_TYPE_REGULAR,  N_("_Move")},
    {MENU_OP_RESIZE,       MENU_TYPE_REGULAR,  N_("_Resize")},
    {0, 0, NULL}, /* ----------------------------------------------*/
    {MENU_OP_ABOVE,        MENU_TYPE_RADIO,    N_("Always on _Top")},
    {MENU_OP_NORMAL,       MENU_TYPE_RADIO,    N_("_Same as Other Windows")},
    {MENU_OP_BELOW,        MENU_TYPE_RADIO,    N_("Always _Below Other Windows")},
    {0, 0, NULL}, /* ----------------------------------------------*/
    {MENU_OP_SHADE,        MENU_TYPE_REGULAR,  N_("Roll Window Up")},
    {MENU_OP_UNSHADE,      MENU_TYPE_REGULAR,  N_("Roll Window Down")},
    {MENU_OP_FULLSCREEN,   MENU_TYPE_REGULAR,  N_("_Fullscreen")},
    {MENU_OP_UNFULLSCREEN, MENU_TYPE_REGULAR,  N_("Leave _Fullscreen")},
    {MENU_OP_CONTEXT_HELP, MENU_TYPE_REGULAR,  N_("Context _Help")},
    {0, 0, NULL}, /* ----------------------------------------------*/
    {MENU_OP_STICK,        MENU_TYPE_CHECKBOX, N_("Always on _Visible Workspace")},
    {MENU_OP_WORKSPACES,   MENU_TYPE_REGULAR,  N_("Move to Another _Workspace")},
    {0, 0, NULL}, /* ----------------------------------------------*/
    {MENU_OP_DELETE,       MENU_TYPE_REGULAR,  N_("_Close")},
#if 0
    {0, NULL, NULL}, /* -------------------------------------------------------- */
    {MENU_OP_DESTROY,      N_("Destroy")},
    {0, NULL, NULL}, /* -------------------------------------------------------- */
#endif
    {MENU_OP_QUIT,         MENU_TYPE_REGULAR, N_("_Quit")},
    {MENU_OP_RESTART,      MENU_TYPE_REGULAR, N_("Restart")},
};

static eventFilterStatus
menu_filter (XfwmEvent *event, gpointer data)
{
    switch (event->meta.type)
    {
        case XFWM_EVENT_KEY:
        case XFWM_EVENT_BUTTON:
        case XFWM_EVENT_MOTION:
        case XFWM_EVENT_CROSSING:
            return EVENT_FILTER_STOP;
        case XFWM_EVENT_XEVENT:
            break;
    }
    return EVENT_FILTER_CONTINUE;
}

static gboolean
activate_cb (GtkWidget * menuitem, gpointer data)
{
    MenuData *menudata;

    g_return_val_if_fail (GTK_IS_WIDGET (menuitem), FALSE);
    TRACE ("entering");

    menu_open = NULL;
    menudata = data;

    eventFilterPop (menudata->menu->filter_setup);
    (*menudata->menu->func) (menudata->menu,
                             menudata->op,
                             menudata->menu->xid,
                             menudata->menu->data,
                             menudata->data);
    return FALSE;
}

static gboolean
menu_closed (GtkMenu * widget, gpointer data)
{
    Menu *menu;

    TRACE ("entering");

    menu = data;
    menu_open = NULL;

    eventFilterPop (menu->filter_setup);
    (*menu->func) (menu, 0, menu->xid, menu->data, NULL);
    return FALSE;
}

static GtkWidget *
menu_workspace (Menu * menu, MenuOp insensitive, gint ws, gint nws, gchar **wsn, gint wsn_items)
{
    GtkWidget *menu_widget;
    GtkWidget *menuitem;
    MenuData *menudata;
    gchar *name;
    gint i;

    menu_widget = gtk_menu_new ();
    gtk_menu_set_screen (GTK_MENU (menu->menu), menu->screen);

    for (i = 0; i < nws; i++)
    {
        if ((i < wsn_items) && wsn[i] && *(wsn[i]))
        {
            if (((i+1) < 10) && (strchr(wsn[i],'_')==NULL))
            {
                /* In the 1st 10, there is a name, but it doesn't have _ */
                name = g_strdup_printf ("_%i (%s)", i + 1, wsn[i]);
                menuitem = gtk_menu_item_new_with_mnemonic (name);
            }
            else
            {
                name = g_strdup_printf ("%i (%s)", i + 1, wsn[i]);
                menuitem = gtk_menu_item_new_with_label (name);
            }
        }
        else
        {
	    /* No workspace name */
            if ((i+1) < 10)
            {
		name = g_strdup_printf ("_%i", i + 1);
		menuitem = gtk_menu_item_new_with_mnemonic (name);
            }
            else
            {
		name = g_strdup_printf ("%i", i + 1);
		menuitem = gtk_menu_item_new_with_label (name);
            }
        }
        g_free (name);
        gtk_widget_set_sensitive (menuitem, !(insensitive & MENU_OP_WORKSPACES) && (i != ws));
        gtk_widget_show (menuitem);

        menudata = g_new0 (MenuData, 1);
        menudata->menu = menu;
        menudata->op = MENU_OP_WORKSPACES;
        menudata->data = GINT_TO_POINTER (i);
        menu_item_connect (menuitem, menudata);

        gtk_menu_shell_append (GTK_MENU_SHELL (menu_widget), menuitem);
    }

    return (menu_widget);
}

Menu *
menu_default (GdkScreen *gscr, Window xid, MenuOp ops, MenuOp insensitive, MenuFunc func,
    gint ws, gint nws, gchar **wsn, gint wsn_items, eventFilterSetup *filter_setup, gpointer data)
{
    GtkWidget *menuitem;
    GtkWidget *ws_menu;
    MenuData *menudata;
    Menu *menu;
    const gchar *label;
    int i;

    TRACE ("entering");

    menu = g_new0 (Menu, 1);
    menu->func = func;
    menu->filter_setup = filter_setup;
    menu->data = data;
    menu->ops = ops;
    menu->insensitive = insensitive;
    menu->screen = gscr;
    menu->xid = xid;
    menu->menu = gtk_menu_new ();
    gtk_menu_set_screen (GTK_MENU (menu->menu), menu->screen);

    i = 0;
    while (i < (int) (sizeof (menuitems) / sizeof (MenuItem)))
    {
        if ((ops & menuitems[i].op) || (menuitems[i].type != MENU_TYPE_REGULAR))
        {
            label = _(menuitems[i].label);
            ws_menu = NULL;
            switch (menuitems[i].op)
            {
                case MENU_OP_SEPARATOR:
                    menuitem = gtk_separator_menu_item_new ();
                    break;
                case MENU_OP_WORKSPACES:
                    menuitem = gtk_menu_item_new_with_mnemonic (label);
                    if (insensitive & menuitems[i].op)
                    {
                        gtk_widget_set_sensitive (menuitem, FALSE);
                    }
                    ws_menu = menu_workspace (menu, insensitive, ws, nws, wsn, wsn_items);
                    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), ws_menu);
                    g_signal_connect (G_OBJECT (ws_menu), "selection-done", G_CALLBACK (menu_closed), menu);
                    break;
                default:
                    switch (menuitems[i].type)
                    {
                        case MENU_TYPE_RADIO:
                            menuitem = gtk_check_menu_item_new_with_mnemonic (label);
                            gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (menuitem), TRUE);
                            /* Set item as checked if it is *not* a valid op. This is because it is already checked and cannot be reselected */
                            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), !(ops & menuitems[i].op));
                            break;
                        case MENU_TYPE_CHECKBOX:
                            menuitem = gtk_check_menu_item_new_with_mnemonic (label);
                            /* Set item as checked if it is *not* a valid op. This is because it is already checked and cannot be reselected */
                            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), !(ops & menuitems[i].op));
                            break;
                        default:
                            menuitem = gtk_menu_item_new_with_mnemonic (label);
                    }
                    if (insensitive & menuitems[i].op)
                    {
                        gtk_widget_set_sensitive (menuitem, FALSE);
                    }
                    menudata = g_new0 (MenuData, 1);
                    menudata->menu = menu;
                    menudata->op = menuitems[i].op;
                    menudata->data = data;
                    menu_item_connect (menuitem, menudata);
                    break;
            }
            gtk_menu_shell_append (GTK_MENU_SHELL (menu->menu), menuitem);
            gtk_widget_show (menuitem);
        }
        ++i;
    }
    g_signal_connect (G_OBJECT (menu->menu), "selection-done", G_CALLBACK (menu_closed), menu);

    return (menu);
}

static void
closure_notify (gpointer data, GClosure * closure)
{
    TRACE ("entering");
    
    if (data)
    {
        TRACE ("freeing data");
        g_free (data);
    }
}

GtkWidget *
menu_item_connect (GtkWidget * item, MenuData * item_data)
{
    g_return_val_if_fail (item != NULL, NULL);
    g_return_val_if_fail (GTK_IS_MENU_ITEM (item), NULL);
    TRACE ("entering");

    g_signal_connect_closure (G_OBJECT (item), "activate",
        g_cclosure_new (G_CALLBACK (activate_cb), item_data,
            (GClosureNotify) closure_notify), FALSE);
    return (item);
}

gboolean
menu_is_opened (void)
{
    TRACE ("entering");
    return (menu_open != NULL);
}

gboolean
menu_check_and_close (void)
{
    TRACE ("entering");
    if (menu_open)
    {
        TRACE ("emitting deactivate signal");
        g_signal_emit_by_name (G_OBJECT (menu_open), "deactivate");
        menu_open = NULL;
        return TRUE;
    }
    return FALSE;
}

static GdkEvent *
menu_popup_event (Menu *menu, gint root_x, gint root_y, guint button, guint32 timestamp,
                  GdkWindow *window)
{
    GdkEvent *event;
    GdkSeat *seat;
    GdkDevice *device;

    TRACE ("entering");

    event = gtk_get_current_event ();

    if (event != NULL)
    {
        event = gdk_event_copy (event);
    }
    else
    {
        /* Create fake event since menu can be show without any events */

        seat = gdk_display_get_default_seat (gdk_window_get_display (window));
        device = gdk_seat_get_pointer (seat);

        event = gdk_event_new (GDK_BUTTON_PRESS);
        event->button.window = g_object_ref (window);
        event->button.time = timestamp;
        event->button.x = event->button.x_root = root_x;
        event->button.y = event->button.y_root = root_y;
        event->button.button = button;
        event->button.device = device;
        gdk_event_set_device (event, device);
    }

    return event;
}

gboolean
menu_popup (Menu *menu, gint root_x, gint root_y, guint button, guint32 timestamp)
{
    GdkWindow *window;
    GdkEvent *event;
    GdkRectangle rectangle;

    g_return_val_if_fail (menu != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_MENU (menu->menu), FALSE);
    TRACE ("entering");

    window = gdk_screen_get_root_window (menu->screen);

    if (!menu_check_and_close ())
    {
        TRACE ("opening new menu");
        menu_open = menu->menu;
        eventFilterPush (menu->filter_setup, menu_filter, NULL);

        rectangle.x = root_x;
        rectangle.y = root_y;
        rectangle.width = 1;
        rectangle.height = 1;

        event = menu_popup_event (menu, root_x, root_y, button, timestamp, window);
        gtk_menu_popup_at_rect (GTK_MENU (menu->menu), window, &rectangle,
                                GDK_GRAVITY_NORTH_WEST, GDK_GRAVITY_NORTH_WEST, event);
        gdk_event_free (event);

        if (g_object_get_data (G_OBJECT (menu->menu), "gtk-menu-transfer-window") == NULL)
        {
            myDisplayBeep (myDisplayGetDefault ());
            g_message (_("%s: GtkMenu failed to grab the pointer\n"), g_get_prgname ());
            gtk_menu_popdown (GTK_MENU (menu->menu));
            menu_open = NULL;
            eventFilterPop (menu->filter_setup);
            return FALSE;
        }
    }
    return TRUE;
}

void
menu_free (Menu * menu)
{
    g_return_if_fail (menu != NULL);
    g_return_if_fail (menu->menu != NULL);
    g_return_if_fail (GTK_IS_MENU (menu->menu));

    TRACE ("entering");

    gtk_widget_destroy (menu->menu);
    g_free (menu);
}
