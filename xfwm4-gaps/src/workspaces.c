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


        oroborus - (c) 2001 Ken Lynch
        xfwm4    - (c) 2002-2011 Olivier Fourdan

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include <common/xfwm-common.h>

#include "display.h"
#include "screen.h"
#include "misc.h"
#include "transients.h"
#include "workspaces.h"
#include "settings.h"
#include "client.h"
#include "focus.h"
#include "frame.h"
#include "stacking.h"
#include "hints.h"

static void
workspaceGetPosition (ScreenInfo *screen_info, int n, int * row, int * col)
{
    NetWmDesktopLayout l;
    int major_length, minor_length, tmp;

    l = screen_info->desktop_layout;
    if (l.orientation == NET_WM_ORIENTATION_HORZ)
    {
        major_length = l.cols;
        minor_length = l.rows;
    }
    else
    {
        major_length = l.rows;
        minor_length = l.cols;
    }

    *row = n / major_length;
    *col = n % major_length;

    switch (l.start)
    {
        case NET_WM_TOPRIGHT:
            *col = major_length - *col - 1;
            break;
        case NET_WM_BOTTOMLEFT:
            *row = minor_length - *row - 1;
            break;
        case NET_WM_BOTTOMRIGHT:
            *col = major_length - *col - 1;
            *row = minor_length - *row - 1;
            break;
        default:
            break;
    }

    if (l.orientation == NET_WM_ORIENTATION_VERT)
    {
        tmp = *row;
        *row = *col;
        *col = tmp;
        if ((l.start == NET_WM_TOPRIGHT) || (l.start == NET_WM_BOTTOMLEFT))
        {
            *row = l.rows - *row - 1;
            *col = l.cols - *col - 1;
        }
    }
}

static gint
workspaceGetNumber (ScreenInfo *screen_info, gint row, gint col)
{
    NetWmDesktopLayout l;
    gulong major_length, minor_length;
    guint n, tmp;

    l = screen_info->desktop_layout;
    if (l.orientation == NET_WM_ORIENTATION_HORZ)
    {
        major_length = l.cols;
        minor_length = l.rows;
    }
    else
    {
        major_length = l.rows;
        minor_length = l.cols;
    }

    if (l.orientation == NET_WM_ORIENTATION_VERT)
    {
        tmp = row;
        row = col;
        col = tmp;
        if ((l.start == NET_WM_TOPRIGHT) || (l.start == NET_WM_BOTTOMLEFT))
        {
            row = minor_length - row - 1;
            col = major_length - col - 1;
        }
    }

    switch (l.start)
    {
        case NET_WM_TOPRIGHT:
            col = major_length - col - 1;
            break;
        case NET_WM_BOTTOMLEFT:
            row = minor_length - row - 1;
            break;
        case NET_WM_BOTTOMRIGHT:
            col = major_length - col - 1;
            row = minor_length - row - 1;
            break;
        default:
            break;
    }

    n = (row * major_length) + col;
    return n;
}

static int
modify_with_wrap (int value, int by, int limit, gboolean wrap)
{
    if (by >= limit) by = limit - 1;
    value += by;
    if (value >= limit)
    {
        if (!wrap)
        {
            value = limit - 1;
        }
        else
        {
            value = value % limit;
        }
    }
    else if (value < 0)
    {
        if (!wrap)
        {
            value = 0;
        }
        else
        {
            value = (value + limit) % limit;
        }
    }
    return value;
}

/* returns TRUE if the workspace was changed, FALSE otherwise */
gboolean
workspaceMove (ScreenInfo *screen_info, gint rowmod, gint colmod, Client * c, guint32 timestamp)
{
    gint row, col, newrow, newcol, n;
    guint previous_ws;

    g_return_val_if_fail (screen_info != NULL, FALSE);

    TRACE ("row %i, mod %i, timestamp %u", rowmod, colmod, timestamp);

    workspaceGetPosition (screen_info, screen_info->current_ws, &row, &col);
    newrow = modify_with_wrap (row, rowmod, screen_info->desktop_layout.rows, screen_info->params->wrap_layout);
    newcol = modify_with_wrap (col, colmod, screen_info->desktop_layout.cols, screen_info->params->wrap_layout);
    n = workspaceGetNumber (screen_info, newrow, newcol);

    if (n == (gint) screen_info->current_ws)
    {
        return FALSE;
    }

    previous_ws = screen_info->current_ws;
    if ((n >= 0) && (n < (gint) screen_info->workspace_count))
    {
        workspaceSwitch (screen_info, n, c, TRUE, timestamp);
    }
    else if (screen_info->params->wrap_layout)
    {
        if (colmod < 0)
        {
           n = screen_info->workspace_count - 1;
        }
        else
        {
            if (colmod > 0)
            {
                newcol = 0;
            }
            else if (rowmod > 0)
            {
                newrow = 0;
            }
            else if (rowmod < 0)
            {
                newrow--;
            }
            else
            {
                return FALSE;
            }

            n = workspaceGetNumber (screen_info, newrow, newcol);
        }
        workspaceSwitch (screen_info, n, c, TRUE, timestamp);
    }

    return (screen_info->current_ws != previous_ws);
}

void
workspaceSwitch (ScreenInfo *screen_info, gint new_ws, Client * c2, gboolean update_focus, guint32 timestamp)
{
    DisplayInfo *display_info;
    Client *c, *new_focus;
    Client *previous;
    GList *list;
    Window dr, window;
    gint rx, ry, wx, wy;
    unsigned int mask;

    g_return_if_fail (screen_info != NULL);

    TRACE ("workspace %i, timestamp %u", new_ws, timestamp);

    display_info = screen_info->display_info;
    if ((new_ws == (gint) screen_info->current_ws) && (screen_info->params->toggle_workspaces))
    {
        new_ws = (gint) screen_info->previous_ws;
    }

    if (new_ws == (gint) screen_info->current_ws)
    {
        return;
    }

    if (screen_info->params->wrap_cycle)
    {
        if (new_ws > (gint) screen_info->workspace_count - 1)
        {
            new_ws = 0;
        }
        if (new_ws < 0)
        {
            new_ws = (gint) screen_info->workspace_count - 1;
        }
    }
    else if ((new_ws > (gint) screen_info->workspace_count - 1) || (new_ws < 0))
    {
        return;
    }

    screen_info->previous_ws = screen_info->current_ws;
    screen_info->current_ws = new_ws;

    new_focus = NULL;
    previous  = NULL;
    c = clientGetFocus ();

    if (c2)
    {
        clientSetWorkspace (c2, new_ws, FALSE);
    }

    if (c)
    {
        if (c->type & WINDOW_REGULAR_FOCUSABLE)
        {
            previous = c;
        }
        if (c2 == c)
        {
            new_focus = c2;
        }
    }

    /* First pass: Show, from top to bottom */
    for (list = g_list_last(screen_info->windows_stack); list; list = g_list_previous (list))
    {
        c = (Client *) list->data;
        if (FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
        {
            clientSetWorkspace (c, new_ws, TRUE);
        }
        else if (new_ws == (gint) c->win_workspace)
        {
            if (!FLAG_TEST (c->flags, CLIENT_FLAG_ICONIFIED) && !FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE))
            {
                if (!clientIsTransientOrModal (c) || !clientTransientOrModalHasAncestor (c, new_ws))
                {
                    clientShow (c, FALSE);
                }
            }
        }
    }

    /* Second pass: Hide from bottom to top */
    for (list = screen_info->windows_stack; list; list = g_list_next (list))
    {
        c = (Client *) list->data;

        if (new_ws != (gint) c->win_workspace)
        {
            if (c == previous)
            {
                FLAG_SET (previous->xfwm_flags, XFWM_FLAG_FOCUS);
                clientSetFocus (screen_info, NULL, timestamp, FOCUS_IGNORE_MODAL);
            }
            if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE) && !FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
            {
                if (!clientIsTransientOrModal (c) || !clientTransientOrModalHasAncestor (c, new_ws))
                {
                    clientWithdraw (c, new_ws, FALSE);
                }
            }
        }
    }

    /* Third pass: Check for focus, from top to bottom */
    for (list = g_list_last(screen_info->windows_stack); list; list = g_list_previous (list))
    {
        c = (Client *) list->data;

        if (FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
        {
            if ((!new_focus) && (c == previous) && clientSelectMask (c, NULL, 0, WINDOW_REGULAR_FOCUSABLE))
            {
                new_focus = c;
            }
            FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_FOCUS);
        }
        else if (new_ws == (gint) c->win_workspace)
        {
            if ((!new_focus) && FLAG_TEST (c->xfwm_flags, XFWM_FLAG_FOCUS))
            {
                new_focus = c;
            }
            FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_FOCUS);
        }
    }

    setNetCurrentDesktop (display_info, screen_info->xroot, new_ws);
    if (!(screen_info->params->click_to_focus))
    {
        if (!(c2) && (XQueryPointer (myScreenGetXDisplay (screen_info), screen_info->xroot, &dr, &window, &rx, &ry, &wx, &wy, &mask)))
        {
            c = clientAtPosition (screen_info, rx, ry, NULL);
            if (c)
            {
                new_focus = c;
            }
        }
    }

    if (update_focus)
    {
        if (new_focus)
        {
            if ((screen_info->params->click_to_focus) && (screen_info->params->raise_on_click))
            {
                if (!(screen_info->params->raise_on_focus) && !clientIsTopMost (new_focus))
                {
                    clientRaise (new_focus, None);
                }
            }
            clientSetFocus (screen_info, new_focus, timestamp, FOCUS_SORT);
        }
        else
        {
            clientFocusTop (screen_info, WIN_LAYER_FULLSCREEN, timestamp);
        }
    }
}

void
workspaceSetNames (ScreenInfo * screen_info, gchar **names, int items)
{
    g_return_if_fail (screen_info != NULL);
    g_return_if_fail (names != NULL);

    TRACE ("entering");

    if (screen_info->workspace_names)
    {
        g_strfreev (screen_info->workspace_names);
    }

    screen_info->workspace_names = names;
    screen_info->workspace_names_items = items;
}

void
workspaceSetCount (ScreenInfo * screen_info, guint count)
{
    DisplayInfo *display_info;
    Client *c;
    GList *list;

    g_return_if_fail (screen_info != NULL);

    TRACE ("count %u", count);

    if (count < 1)
    {
        count = 1;
    }
    if (count == screen_info->workspace_count)
    {
        return;
    }

    display_info = screen_info->display_info;
    setHint (display_info, screen_info->xroot, NET_NUMBER_OF_DESKTOPS, count);
    screen_info->workspace_count = count;

    for (list = screen_info->windows_stack; list; list = g_list_next (list))
    {
        c = (Client *) list->data;
        if (c->win_workspace > count - 1)
        {
            clientSetWorkspace (c, count - 1, TRUE);
        }
    }
    if (screen_info->current_ws > count - 1)
    {
        workspaceSwitch (screen_info, count - 1, NULL, TRUE, myDisplayGetCurrentTime (display_info));
    }
    setNetWorkarea (display_info, screen_info->xroot, screen_info->workspace_count,
                    screen_info->width, screen_info->height, screen_info->margins);
    /* Recompute the layout based on the (changed) number of desktops */
    getDesktopLayout (display_info, screen_info->xroot, screen_info->workspace_count,
                     &screen_info->desktop_layout);
}

void
workspaceInsert (ScreenInfo * screen_info, guint position)
{
    Client *c;
    GList *list;
    guint count;

    g_return_if_fail (screen_info != NULL);

    TRACE ("position %u", position);

    count = screen_info->workspace_count;
    workspaceSetCount(screen_info, count + 1);

    if (position > count)
    {
        return;
    }

    for (list = screen_info->windows_stack; list; list = g_list_next (list))
    {
        c = (Client *) list->data;
        if (c->win_workspace >= position)
        {
            clientSetWorkspace (c, c->win_workspace + 1, TRUE);
        }
    }
}

void
workspaceDelete (ScreenInfo * screen_info, guint position)
{
    Client *c;
    guint i, count;

    g_return_if_fail (screen_info != NULL);

    TRACE ("position %u", position);

    count = screen_info->workspace_count;
    if ((count < 1) || (position > count))
    {
        return;
    }

    for (c = screen_info->clients, i = 0; i < screen_info->client_count; c = c->next, i++)
    {
        if (c->win_workspace > position)
        {
            clientSetWorkspace (c, c->win_workspace - 1, TRUE);
        }
    }

    workspaceSetCount(screen_info, count - 1);
}

void
workspaceUpdateArea (ScreenInfo *screen_info)
{
    DisplayInfo *display_info;
    Client *c;
    GdkRectangle top, left, right, bottom, workarea, win_area;
    int prev_top;
    int prev_left;
    int prev_right;
    int prev_bottom;
    guint i;


    g_return_if_fail (screen_info != NULL);
    g_return_if_fail (screen_info->margins != NULL);
    g_return_if_fail (screen_info->gnome_margins != NULL);

    TRACE ("entering");

    display_info = screen_info->display_info;
    prev_top = screen_info->margins[STRUTS_TOP];
    prev_left = screen_info->margins[STRUTS_LEFT];
    prev_right = screen_info->margins[STRUTS_RIGHT];
    prev_bottom = screen_info->margins[STRUTS_BOTTOM];

    if (!xfwm_get_primary_monitor_geometry (screen_info->gscr, &workarea, TRUE))
    {
        TRACE ("No monitor attached");
        return;
    }

    for (i = 0; i < 4; i++)
    {
        screen_info->margins[i] = screen_info->gnome_margins[i];
    }

    for (c = screen_info->clients, i = 0; i < screen_info->client_count; c = c->next, i++)
    {
        win_area.x = frameExtentX (c);
        win_area.y = frameExtentY (c);
        win_area.width = frameExtentWidth (c);
        win_area.height = frameExtentHeight (c);

        if (!gdk_rectangle_intersect (&win_area, &workarea, NULL))
        {
             continue;
        }

        if (strutsToRectangles (c, &left, &right, &top, &bottom))
        {
            /*
             * NET_WORKAREA doesn't support L shaped displays at all.
             * gdk works around this by ignoring it unless dealing with
             * the primary monitor.
             * Mimic this behaviour by ignoring struts not on the primary
             * display when calculating NET_WORKAREA
             */
            if (gdk_rectangle_intersect (&left, &workarea, NULL))
            {
                screen_info->margins[STRUTS_LEFT] = MAX(screen_info->margins[STRUTS_LEFT],
                                                         c->struts[STRUTS_LEFT]);
            }

            if (gdk_rectangle_intersect (&right, &workarea, NULL))
            {
                screen_info->margins[STRUTS_RIGHT] = MAX(screen_info->margins[STRUTS_RIGHT],
                                                         c->struts[STRUTS_RIGHT]);
            }

            if (gdk_rectangle_intersect (&top, &workarea, NULL))
            {
                screen_info->margins[STRUTS_TOP] = MAX(screen_info->margins[STRUTS_TOP],
                                                       c->struts[STRUTS_TOP]);
            }

            if (gdk_rectangle_intersect (&bottom, &workarea, NULL))
            {
                screen_info->margins[STRUTS_BOTTOM] = MAX(screen_info->margins[STRUTS_BOTTOM],
                                                          c->struts[STRUTS_BOTTOM]);
            }
        }
    }

    if ((prev_top != screen_info->margins[STRUTS_TOP]) ||
        (prev_left != screen_info->margins[STRUTS_LEFT]) ||
        (prev_right != screen_info->margins[STRUTS_RIGHT]) ||
        (prev_bottom != screen_info->margins[STRUTS_BOTTOM]))
    {
        TRACE ("margins have changed, updating net_workarea");
        setNetWorkarea (display_info, screen_info->xroot, screen_info->workspace_count,
                        screen_info->width, screen_info->height, screen_info->margins);
    }

    /* Also prevent windows from being off screen, just like when screen is resized */
    clientScreenResize(screen_info, FALSE);
}
