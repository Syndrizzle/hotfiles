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


        xfwm4    - (c) 2002-2021 Olivier Fourdan

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include <common/xfwm-common.h>

#include "client.h"
#include "moveresize.h"
#include "compositor.h"
#include "display.h"
#include "frame.h"
#include "focus.h"
#include "hints.h"
#include "misc.h"
#include "netwm.h"
#include "screen.h"
#include "stacking.h"
#include "terminate.h"
#include "transients.h"
#include "workspaces.h"

void
clientSetNetState (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Atom data[16];
    int i;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    i = 0;
    if (FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
    {
        TRACE ("shaded");
        data[i++] = display_info->atoms[NET_WM_STATE_SHADED];
    }
    if (FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
    {
        TRACE ("sticky");
        data[i++] = display_info->atoms[NET_WM_STATE_STICKY];
    }
    if (FLAG_TEST (c->flags, CLIENT_FLAG_STATE_MODAL))
    {
        TRACE ("modal");
        data[i++] = display_info->atoms[NET_WM_STATE_MODAL];
    }
    if (FLAG_TEST (c->flags, CLIENT_FLAG_SKIP_PAGER))
    {
        TRACE ("skip_pager");
        data[i++] = display_info->atoms[NET_WM_STATE_SKIP_PAGER];
    }
    if (FLAG_TEST (c->flags, CLIENT_FLAG_SKIP_TASKBAR))
    {
        TRACE ("skip_taskbar");
        data[i++] = display_info->atoms[NET_WM_STATE_SKIP_TASKBAR];
    }
    if (FLAG_TEST_ALL (c->flags, CLIENT_FLAG_MAXIMIZED))
    {
        TRACE ("maximize vert + horiz");
        data[i++] = display_info->atoms[NET_WM_STATE_MAXIMIZED_HORZ];
        data[i++] = display_info->atoms[NET_WM_STATE_MAXIMIZED_VERT];
    }
    else if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_HORIZ))
    {
        TRACE ("maximize horiz");
        data[i++] = display_info->atoms[NET_WM_STATE_MAXIMIZED_HORZ];
    }
    else if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_VERT))
    {
        TRACE ("vert");
        data[i++] = display_info->atoms[NET_WM_STATE_MAXIMIZED_VERT];
    }
    if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        TRACE ("fullscreen");
        data[i++] = display_info->atoms[NET_WM_STATE_FULLSCREEN];
    }
    else if (FLAG_TEST (c->flags, CLIENT_FLAG_ABOVE))
    {
        TRACE ("above");
        data[i++] = display_info->atoms[NET_WM_STATE_ABOVE];
    }
    else if (FLAG_TEST (c->flags, CLIENT_FLAG_BELOW))
    {
        TRACE ("below");
        data[i++] = display_info->atoms[NET_WM_STATE_BELOW];
    }
    if (FLAG_TEST (c->flags, CLIENT_FLAG_ICONIFIED))
    {
        TRACE ("hidden");
        data[i++] = display_info->atoms[NET_WM_STATE_HIDDEN];
    }
    if (FLAG_TEST (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION))
    {
        TRACE ("demands_attention");
        data[i++] = display_info->atoms[NET_WM_STATE_DEMANDS_ATTENTION];
    }
    if (c == clientGetFocus () || c->type & WINDOW_TYPE_STATE_FOCUSED)
    {
        TRACE ("focused");
        data[i++] = display_info->atoms[NET_WM_STATE_FOCUSED];
    }

    myDisplayErrorTrapPush (display_info);
    XChangeProperty (display_info->dpy, c->window,
                     display_info->atoms[NET_WM_STATE], XA_ATOM, 32,
                     PropModeReplace, (unsigned char *) data, i);
    myDisplayErrorTrapPopIgnored (display_info);
}

void
clientGetNetState (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    int n_atoms;
    Atom *atoms;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    atoms = NULL;
    n_atoms = 0;

    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_SESSION_MANAGED))
    {
        if (FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
        {
            TRACE ("shaded from session management");
            FLAG_SET (c->flags, CLIENT_FLAG_SHADED);
        }
        if (FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
        {
            TRACE ("sticky from session management");
            FLAG_SET (c->flags, CLIENT_FLAG_STICKY);
        }
        if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_HORIZ))
        {
            TRACE ("maximized horiz from session management");
            FLAG_SET (c->flags, CLIENT_FLAG_MAXIMIZED_HORIZ | CLIENT_FLAG_RESTORE_SIZE_POS);
        }
        if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_VERT))
        {
            TRACE ("maximized vert from session management");
            FLAG_SET (c->flags, CLIENT_FLAG_MAXIMIZED_VERT | CLIENT_FLAG_RESTORE_SIZE_POS);
        }
    }

    if (getAtomList (display_info, c->window, NET_WM_STATE, &atoms, &n_atoms))
    {
        int i;
        TRACE ("%i atoms detected", n_atoms);

        i = 0;
        while (i < n_atoms)
        {
            if (atoms[i] == display_info->atoms[NET_WM_STATE_SHADED])
            {
                TRACE ("shaded");
                FLAG_SET (c->flags, CLIENT_FLAG_SHADED);
            }
            else if (atoms[i] == display_info->atoms[NET_WM_STATE_STICKY])
            {
                TRACE ("sticky");
                FLAG_SET (c->flags, CLIENT_FLAG_STICKY);
            }
            else if (atoms[i] == display_info->atoms[NET_WM_STATE_MAXIMIZED_HORZ])
            {
                TRACE ("maximized horiz");
                FLAG_SET (c->flags, CLIENT_FLAG_MAXIMIZED_HORIZ | CLIENT_FLAG_RESTORE_SIZE_POS);
            }
            else if (atoms[i] == display_info->atoms[NET_WM_STATE_MAXIMIZED_VERT])
            {
                TRACE ("maximized vert");
                FLAG_SET (c->flags, CLIENT_FLAG_MAXIMIZED_VERT | CLIENT_FLAG_RESTORE_SIZE_POS);
            }
            else if (atoms[i] == display_info->atoms[NET_WM_STATE_FULLSCREEN])
            {
                if (!FLAG_TEST_ALL (c->flags, CLIENT_FLAG_ABOVE | CLIENT_FLAG_BELOW))
                {
                    TRACE ("fullscreen");
                    FLAG_SET (c->flags, CLIENT_FLAG_FULLSCREEN);
                }
            }
            else if (atoms[i] == display_info->atoms[NET_WM_STATE_ABOVE])
            {
                if (!FLAG_TEST_ALL (c->flags, CLIENT_FLAG_FULLSCREEN | CLIENT_FLAG_BELOW))
                {
                    TRACE ("above");
                    FLAG_SET (c->flags, CLIENT_FLAG_ABOVE);
                }
            }
            else if (atoms[i] == display_info->atoms[NET_WM_STATE_BELOW])
            {
                if (!FLAG_TEST_ALL (c->flags, CLIENT_FLAG_ABOVE | CLIENT_FLAG_FULLSCREEN))
                {
                    TRACE ("below");
                    FLAG_SET (c->flags, CLIENT_FLAG_BELOW);
                }
            }
            else if (atoms[i] == display_info->atoms[NET_WM_STATE_MODAL])
            {
                TRACE ("modal");
                FLAG_SET (c->flags, CLIENT_FLAG_STATE_MODAL);
            }
            else if (atoms[i] == display_info->atoms[NET_WM_STATE_SKIP_PAGER])
            {
                TRACE ("skip_pager");
                FLAG_SET (c->flags, CLIENT_FLAG_SKIP_PAGER);
            }
            else if (atoms[i] == display_info->atoms[NET_WM_STATE_SKIP_TASKBAR])
            {
                TRACE ("skip_taskbar");
                FLAG_SET (c->flags, CLIENT_FLAG_SKIP_TASKBAR);
            }
            else if (atoms[i] == display_info->atoms[NET_WM_STATE_HIDDEN])
            {
                TRACE ("state_hidden");
                FLAG_SET (c->flags, CLIENT_FLAG_ICONIFIED);
            }
            else if (atoms[i] == display_info->atoms[NET_WM_STATE_DEMANDS_ATTENTION])
            {
                TRACE ("demands_attention");
                FLAG_SET (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION);
            }
            else if (atoms[i] == display_info->atoms[NET_WM_STATE_FOCUSED])
            {
                TRACE ("focused, ignored...");
            }
            else
            {
                gchar *atom_name;
                atom_name = get_atom_name (display_info, atoms[i]);
                g_warning ("unmanaged net_wm_state (window 0x%lx, atom \"%s\")", c->window, atom_name);
                g_free (atom_name);
            }

            ++i;
        }
        if (atoms)
        {
            XFree (atoms);
        }
    }
}

void
clientUpdateNetWmDesktop (Client * c, XClientMessageEvent * ev)
{
    ScreenInfo *screen_info;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx), value 0x%lx", c->name, c->window, ev->data.l[0]);

    screen_info = c->screen_info;

    if ((guint) ev->data.l[0] == ALL_WORKSPACES)
    {
        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_STICK) && !FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
        {
            clientStick (c, TRUE);
            frameQueueDraw (c, FALSE);
        }
    }
    else if ((guint) ev->data.l[0] < (guint) screen_info->workspace_count)
    {
        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_STICK) && FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
        {
            clientUnstick (c, TRUE);
            frameQueueDraw (c, FALSE);
        }
        if ((guint) ev->data.l[0] != (guint) c->win_workspace)
        {
            clientSetWorkspace (c, (guint) ev->data.l[0], TRUE);
        }
    }
    else
    {
        TRACE ("invalid NET_WM_DESKTOP value 0x%lx specified for client \"%s\" (0x%lx)", ev->data.l[0], c->name, c->window);
    }
}

void
clientUpdateNetState (Client * c, XClientMessageEvent * ev)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    unsigned long action, mode;
    Atom first;
    Atom second;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    action = ev->data.l[0];
    first  = ev->data.l[1];
    second = ev->data.l[2];

    if ((first  == display_info->atoms[NET_WM_STATE_HIDDEN]) ||
        (second == display_info->atoms[NET_WM_STATE_HIDDEN]))
    {
        if ((action == NET_WM_STATE_ADD) && !FLAG_TEST (c->flags, CLIENT_FLAG_ICONIFIED))
        {
            if (CLIENT_CAN_HIDE_WINDOW (c))
            {
                clientWithdraw (c, c->win_workspace, TRUE);
            }
        }
        else if ((action == NET_WM_STATE_REMOVE) && FLAG_TEST (c->flags, CLIENT_FLAG_ICONIFIED))
        {
            clientShow (c, TRUE);
        }
        else if (action == NET_WM_STATE_TOGGLE)
        {
            if (FLAG_TEST (c->flags, CLIENT_FLAG_ICONIFIED))
            {
                clientShow (c, TRUE);
            }
            else if (CLIENT_CAN_HIDE_WINDOW (c))
            {
                clientWithdraw (c, c->win_workspace, TRUE);
            }
        }
    }

    if ((first  == display_info->atoms[NET_WM_STATE_SHADED]) ||
        (second == display_info->atoms[NET_WM_STATE_SHADED]))
    {
        if ((action == NET_WM_STATE_ADD) && !FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
        {
            clientShade (c);
        }
        else if ((action == NET_WM_STATE_REMOVE) && FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
        {
            clientUnshade (c);
        }
        else if (action == NET_WM_STATE_TOGGLE)
        {
            clientToggleShaded (c);
        }
    }

    if ((first  == display_info->atoms[NET_WM_STATE_STICKY]) ||
        (second == display_info->atoms[NET_WM_STATE_STICKY]))
    {
        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_STICK))
        {
            if ((action == NET_WM_STATE_ADD) && !FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
            {
                clientStick (c, TRUE);
            }
            else if ((action == NET_WM_STATE_REMOVE) && FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
            {
                clientUnstick (c, TRUE);
            }
            else if (action == NET_WM_STATE_TOGGLE)
            {
                clientToggleSticky (c, TRUE);
            }
            frameQueueDraw (c, FALSE);
        }
    }

    if ((first  == display_info->atoms[NET_WM_STATE_MAXIMIZED_HORZ]) ||
        (second == display_info->atoms[NET_WM_STATE_MAXIMIZED_HORZ]) ||
        (first  == display_info->atoms[NET_WM_STATE_MAXIMIZED_VERT]) ||
        (second == display_info->atoms[NET_WM_STATE_MAXIMIZED_VERT]))
    {
        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_MAXIMIZE))
        {
            if ((action == NET_WM_STATE_ADD) && !FLAG_TEST_ALL (c->flags, CLIENT_FLAG_MAXIMIZED))
            {
                mode = FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED);
                if ((first  == display_info->atoms[NET_WM_STATE_MAXIMIZED_HORZ]) ||
                    (second == display_info->atoms[NET_WM_STATE_MAXIMIZED_HORZ]))
                {
                    mode |= !FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_HORIZ) ? CLIENT_FLAG_MAXIMIZED_HORIZ : 0;
                }
                if ((first  == display_info->atoms[NET_WM_STATE_MAXIMIZED_VERT]) ||
                    (second == display_info->atoms[NET_WM_STATE_MAXIMIZED_VERT]))
                {
                    mode |= !FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_VERT) ? CLIENT_FLAG_MAXIMIZED_VERT : 0;
                }
                clientToggleMaximized (c, mode, TRUE);
            }
            else if ((action == NET_WM_STATE_REMOVE) && FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED))
            {
                mode = 0;
                if ((first  == display_info->atoms[NET_WM_STATE_MAXIMIZED_HORZ]) ||
                    (second == display_info->atoms[NET_WM_STATE_MAXIMIZED_HORZ]))
                {
                    mode |= FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_HORIZ);
                }
                if ((first  == display_info->atoms[NET_WM_STATE_MAXIMIZED_VERT]) ||
                    (second == display_info->atoms[NET_WM_STATE_MAXIMIZED_VERT]))
                {
                    mode |= FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_VERT);
                }
                clientToggleMaximized (c, mode, TRUE);
            }
            else if (action == NET_WM_STATE_TOGGLE)
            {
                mode = 0;
                if ((first  == display_info->atoms[NET_WM_STATE_MAXIMIZED_HORZ]) ||
                    (second == display_info->atoms[NET_WM_STATE_MAXIMIZED_HORZ]))
                {
                    mode |= CLIENT_FLAG_MAXIMIZED_HORIZ;
                }
                if ((first  == display_info->atoms[NET_WM_STATE_MAXIMIZED_VERT]) ||
                    (second == display_info->atoms[NET_WM_STATE_MAXIMIZED_VERT]))
                {
                    mode |= CLIENT_FLAG_MAXIMIZED_VERT;
                }
                clientToggleMaximized (c, mode, TRUE);
            }
        }
    }

    if ((first  == display_info->atoms[NET_WM_STATE_MODAL]) ||
        (second == display_info->atoms[NET_WM_STATE_MODAL]))
    {
        if ((action == NET_WM_STATE_ADD) && !FLAG_TEST (c->flags, CLIENT_FLAG_STATE_MODAL))
        {
            FLAG_SET (c->flags, CLIENT_FLAG_STATE_MODAL);
            clientWindowType (c);
            clientSetNetState (c);
        }
        else if ((action == NET_WM_STATE_REMOVE) && FLAG_TEST (c->flags, CLIENT_FLAG_STATE_MODAL))
        {
            FLAG_UNSET (c->flags, CLIENT_FLAG_STATE_MODAL);
            clientWindowType (c);
            clientSetNetState (c);
        }
        else if (action == NET_WM_STATE_TOGGLE)
        {
            FLAG_TOGGLE (c->flags, CLIENT_FLAG_STATE_MODAL);
            clientWindowType (c);
            clientSetNetState (c);
        }
        frameQueueDraw (c, TRUE);
    }

    if ((first  == display_info->atoms[NET_WM_STATE_FULLSCREEN]) ||
        (second == display_info->atoms[NET_WM_STATE_FULLSCREEN]))
    {
        if (!clientIsValidTransientOrModal (c))
        {
            if ((action == NET_WM_STATE_ADD) && !FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
            {
                FLAG_SET (c->flags, CLIENT_FLAG_FULLSCREEN);
                clientUpdateFullscreenState (c);
            }
            else if ((action == NET_WM_STATE_REMOVE) && FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
            {
                FLAG_UNSET (c->flags, CLIENT_FLAG_FULLSCREEN);
                clientUpdateFullscreenState (c);
            }
            else if (action == NET_WM_STATE_TOGGLE)
            {
                FLAG_TOGGLE (c->flags, CLIENT_FLAG_FULLSCREEN);
                clientUpdateFullscreenState (c);
            }
        }
    }

    if ((first  == display_info->atoms[NET_WM_STATE_ABOVE]) ||
        (second == display_info->atoms[NET_WM_STATE_ABOVE]))
    {
        if (!FLAG_TEST (c->flags, CLIENT_FLAG_BELOW))
        {
            if ((action == NET_WM_STATE_ADD) && !FLAG_TEST (c->flags, CLIENT_FLAG_ABOVE))
            {
                FLAG_SET (c->flags, CLIENT_FLAG_ABOVE);
                clientUpdateLayerState (c);
            }
            else if ((action == NET_WM_STATE_REMOVE) && FLAG_TEST (c->flags, CLIENT_FLAG_ABOVE))
            {
                FLAG_UNSET (c->flags, CLIENT_FLAG_ABOVE);
                clientUpdateLayerState (c);
            }
            else if (action == NET_WM_STATE_TOGGLE)
            {
                FLAG_TOGGLE (c->flags, CLIENT_FLAG_ABOVE);
                clientUpdateLayerState (c);
            }
        }
    }

    if ((first  == display_info->atoms[NET_WM_STATE_BELOW]) ||
        (second == display_info->atoms[NET_WM_STATE_BELOW]))
    {
        if (!FLAG_TEST (c->flags, CLIENT_FLAG_ABOVE))
        {
            if ((action == NET_WM_STATE_ADD) && !FLAG_TEST (c->flags, CLIENT_FLAG_BELOW))
            {
                FLAG_SET (c->flags, CLIENT_FLAG_BELOW);
                clientUpdateLayerState (c);
            }
            else if ((action == NET_WM_STATE_REMOVE) && FLAG_TEST (c->flags, CLIENT_FLAG_BELOW))
            {
                FLAG_UNSET (c->flags, CLIENT_FLAG_BELOW);
                clientUpdateLayerState (c);
            }
            else if (action == NET_WM_STATE_TOGGLE)
            {
                FLAG_TOGGLE (c->flags, CLIENT_FLAG_BELOW);
                clientUpdateLayerState (c);
            }
        }
    }

    if ((first  == display_info->atoms[NET_WM_STATE_SKIP_PAGER]) ||
        (second == display_info->atoms[NET_WM_STATE_SKIP_PAGER]))
    {
        if ((action == NET_WM_STATE_ADD) && !FLAG_TEST (c->flags, CLIENT_FLAG_SKIP_PAGER))
        {
            FLAG_SET (c->flags, CLIENT_FLAG_SKIP_PAGER);
            clientSetNetState (c);
        }
        else if ((action == NET_WM_STATE_REMOVE) && FLAG_TEST (c->flags, CLIENT_FLAG_SKIP_PAGER))
        {
            FLAG_UNSET (c->flags, CLIENT_FLAG_SKIP_PAGER);
            clientSetNetState (c);
        }
        else if (action == NET_WM_STATE_TOGGLE)
        {
            FLAG_TOGGLE (c->flags, CLIENT_FLAG_SKIP_PAGER);
            clientSetNetState (c);
        }
    }

    if ((first  == display_info->atoms[NET_WM_STATE_SKIP_TASKBAR]) ||
        (second == display_info->atoms[NET_WM_STATE_SKIP_TASKBAR]))
    {
        if ((action == NET_WM_STATE_ADD) && !FLAG_TEST (c->flags, CLIENT_FLAG_SKIP_TASKBAR))
        {
            FLAG_SET (c->flags, CLIENT_FLAG_SKIP_TASKBAR);
            clientSetNetState (c);
        }
        else if ((action == NET_WM_STATE_REMOVE) && FLAG_TEST (c->flags, CLIENT_FLAG_SKIP_TASKBAR))
        {
            FLAG_UNSET (c->flags, CLIENT_FLAG_SKIP_TASKBAR);
            clientSetNetState (c);
        }
        else if (action == NET_WM_STATE_TOGGLE)
        {
            FLAG_TOGGLE (c->flags, CLIENT_FLAG_SKIP_TASKBAR);
            clientSetNetState (c);
        }
        frameQueueDraw (c, TRUE);
    }

    if ((first  == display_info->atoms[NET_WM_STATE_DEMANDS_ATTENTION]) ||
        (second == display_info->atoms[NET_WM_STATE_DEMANDS_ATTENTION]))
    {
        if ((action == NET_WM_STATE_ADD) && !FLAG_TEST (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION))
        {
            /* Do not apply NET_WM_STATE_DEMANDS_ATTENTION if client is already focused */
            if (c != clientGetFocusOrPending ())
            {
                FLAG_SET (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION);
                clientSetNetState (c);
            }
        }
        else if ((action == NET_WM_STATE_REMOVE) && FLAG_TEST (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION))
        {
            FLAG_UNSET (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION);
            clientSetNetState (c);
        }
        else if (action == NET_WM_STATE_TOGGLE)
        {
            /* Do not apply NET_WM_STATE_DEMANDS_ATTENTION if client is already focused */
            if (c != clientGetFocusOrPending () || !FLAG_TEST (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION))
            {
                FLAG_TOGGLE (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION);
                clientSetNetState (c);
            }
        }
    }
}

void
clientNetMoveResize (Client * c, XClientMessageEvent * ev)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    int x_root, y_root, action, button;
    int corner;
    gboolean resize; /* true == resize, false == move */
    XEvent *xevent;
    XfwmEvent *event;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    x_root = (int) ev->data.l[0];
    y_root = (int) ev->data.l[1];
    action = (int) ev->data.l[2];
    button = (int) ev->data.l[3];
    xevent  = (XEvent *) ev;

    /* We don't deal with button > 7, in such a case we pretent it's just any button */
    if (button > Button7)
    {
        button = AnyButton;
    }

    corner = CORNER_BOTTOM_RIGHT;

    xevent->xbutton.button = button;
    xevent->xbutton.x_root = xevent->xkey.x_root = x_root;
    xevent->xbutton.y_root = xevent->xkey.y_root = y_root;
    xevent->xbutton.time = xevent->xkey.time = (Time) myDisplayGetCurrentTime (display_info);

    switch (action)
    {
        /* Keyboard */
        case NET_WM_MOVERESIZE_SIZE_KEYBOARD:
            xevent->type = KeyPress;
            corner = CORNER_BOTTOM_RIGHT;
            resize = TRUE; /* Resize */
            break;
        case NET_WM_MOVERESIZE_MOVE_KEYBOARD:
            xevent->type = KeyPress;
            resize = FALSE; /* Move */
            break;

        /* Sides */
        case NET_WM_MOVERESIZE_SIZE_TOP:
            xevent->type = ButtonPress;
            corner = CORNER_COUNT + SIDE_TOP;
            resize = TRUE; /* Resize */
            break;
        case NET_WM_MOVERESIZE_SIZE_BOTTOM:
            xevent->type = ButtonPress;
            corner = CORNER_COUNT + SIDE_BOTTOM;
            resize = TRUE; /* Resize */
            break;
        case NET_WM_MOVERESIZE_SIZE_RIGHT:
            xevent->type = ButtonPress;
            corner = CORNER_COUNT + SIDE_RIGHT;
            resize = TRUE; /* Resize */
            break;
        case NET_WM_MOVERESIZE_SIZE_LEFT:
            xevent->type = ButtonPress;
            corner = CORNER_COUNT + SIDE_LEFT;
            resize = TRUE; /* Resize */
            break;

        /* Corners */
        case NET_WM_MOVERESIZE_SIZE_TOPLEFT:
            xevent->type = ButtonPress;
            corner = CORNER_TOP_LEFT;
            resize = TRUE; /* Resize */
            break;
        case NET_WM_MOVERESIZE_SIZE_TOPRIGHT:
            xevent->type = ButtonPress;
            corner = CORNER_TOP_RIGHT;
            resize = TRUE; /* Resize */
            break;
        case NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT:
            xevent->type = ButtonPress;
            corner = CORNER_BOTTOM_LEFT;
            resize = TRUE; /* Resize */
            break;
        case NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT:
            xevent->type = ButtonPress;
            corner = CORNER_BOTTOM_RIGHT;
            resize = TRUE; /* Resize */
            break;
        case NET_WM_MOVERESIZE_MOVE:
            xevent->type = ButtonPress;
            resize = FALSE; /* Move */
            break;
        case NET_WM_MOVERESIZE_CANCEL:
            FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING);
            FALLTHROUGH;
        default: /* Do nothing */
            return;
    }

    if (!FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        if (resize && FLAG_TEST_ALL (c->xfwm_flags, XFWM_FLAG_HAS_RESIZE | XFWM_FLAG_IS_RESIZABLE))
        {
            event = xfwm_device_translate_event (display_info->devices, xevent, NULL);
            clientResize (c, corner, event->meta.type == XFWM_EVENT_BUTTON ? &event->button : NULL);
            xfwm_device_free_event (event);
        }
        else if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_MOVE))
        {
            event = xfwm_device_translate_event (display_info->devices, xevent, NULL);
            clientMove (c, event->meta.type == XFWM_EVENT_BUTTON ? &event->button : NULL);
            xfwm_device_free_event (event);
        }
    }
}

void
clientNetMoveResizeWindow (Client * c, XClientMessageEvent * ev)
{
    XWindowChanges wc;
    unsigned long mask;
    int gravity;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING))
    {
        /* not allowed */
        return;
    }

    gravity = (ev->data.l[0] & 0xff);
    if (!gravity)
    {
        gravity = c->gravity;
    }

    wc.x = ev->data.l[1];
    wc.y = ev->data.l[2];
    wc.width = ev->data.l[3];
    wc.height = ev->data.l[4];
    mask = (ev->data.l[0] & 0xf00) >> 8;

    clientAdjustCoordGravity (c, gravity, &wc, &mask);
    clientMoveResizeWindow (c, &wc, mask);
}

void
clientUpdateFullscreenState (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    int layer;

    g_return_if_fail (c != NULL);
    TRACE ("Update fullscreen state for client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        c->pre_fullscreen_geometry.x = c->x;
        c->pre_fullscreen_geometry.y = c->y;
        c->pre_fullscreen_geometry.width = c->width;
        c->pre_fullscreen_geometry.height = c->height;
        c->pre_fullscreen_layer = c->win_layer;
        layer = WIN_LAYER_FULLSCREEN;
        clientUntile (c);
    }
    else
    {
        layer = c->pre_fullscreen_layer;
    }
    clientSetLayer (c, layer);
    clientUpdateFullscreenSize (c);

    /* Fullscreen has no decoration at all, update NET_FRAME_EXTENTS accordingly */
    setNetFrameExtents (display_info,
                        c->window,
                        frameTop (c),
                        frameLeft (c),
                        frameRight (c),
                        frameBottom (c));
    clientSetNetState (c);
}

void
clientGetNetWmType (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    int n_atoms;
    Atom *atoms;
    int i;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    n_atoms = 0;
    atoms = NULL;
    c->type_atom = None;

    if (!getAtomList (display_info, c->window, NET_WM_WINDOW_TYPE, &atoms, &n_atoms))
    {
        switch (c->win_layer)
        {
            case WIN_LAYER_DESKTOP:
                c->type_atom = display_info->atoms[NET_WM_WINDOW_TYPE_DESKTOP];
                break;
            case WIN_LAYER_DOCK:
                c->type_atom = display_info->atoms[NET_WM_WINDOW_TYPE_DOCK];
                break;
            default:
                if (c->transient_for != None)
                {
                    c->type_atom = display_info->atoms[NET_WM_WINDOW_TYPE_DIALOG];
                }
                else
                {
                    c->type_atom = display_info->atoms[NET_WM_WINDOW_TYPE_NORMAL];
                }
                break;
        }
    }
    else
    {
        i = 0;
        while (i < n_atoms)
        {
            if ((atoms[i] == display_info->atoms[NET_WM_WINDOW_TYPE_DESKTOP]) ||
                (atoms[i] == display_info->atoms[NET_WM_WINDOW_TYPE_DOCK])    ||
                (atoms[i] == display_info->atoms[NET_WM_WINDOW_TYPE_TOOLBAR]) ||
                (atoms[i] == display_info->atoms[NET_WM_WINDOW_TYPE_MENU])    ||
                (atoms[i] == display_info->atoms[NET_WM_WINDOW_TYPE_DIALOG])  ||
                (atoms[i] == display_info->atoms[NET_WM_WINDOW_TYPE_NORMAL])  ||
                (atoms[i] == display_info->atoms[NET_WM_WINDOW_TYPE_UTILITY]) ||
                (atoms[i] == display_info->atoms[NET_WM_WINDOW_TYPE_SPLASH])  ||
                (atoms[i] == display_info->atoms[NET_WM_WINDOW_TYPE_NOTIFICATION]))
            {
                c->type_atom = atoms[i];
                break;
            }
            ++i;
        }
        if (atoms)
        {
            XFree (atoms);
        }
    }
    clientWindowType (c);
}

void
clientGetInitialNetWmDesktop (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Client *c2;
    long val;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    val = 0;

    if (!FLAG_TEST (c->xfwm_flags, XFWM_FLAG_SESSION_MANAGED)
        &&  !FLAG_TEST (c->xfwm_flags, XFWM_FLAG_WORKSPACE_SET))
    {
        FLAG_SET (c->xfwm_flags, XFWM_FLAG_WORKSPACE_SET);
        c->win_workspace = c->screen_info->current_ws;
    }
    if (getHint (display_info, c->window, NET_WM_DESKTOP, &val))
    {
        TRACE ("atom net_wm_desktop detected");
        if (val == (int) ALL_WORKSPACES)
        {
            if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_STICK) && !FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
            {
                TRACE ("atom net_wm_desktop specifies window \"%s\" is sticky", c->name);
                FLAG_SET (c->flags, CLIENT_FLAG_STICKY);
            }
            c->win_workspace = c->screen_info->current_ws;
        }
        else
        {
            TRACE ("atom net_wm_desktop specifies window \"%s\" is on desk %i", c->name, (int) val);
            c->win_workspace = (int) val;
        }
        FLAG_SET (c->xfwm_flags, XFWM_FLAG_WORKSPACE_SET);
    }

    /* This is to make sure that transient are shown with their "ancestor" window */
    if (!FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
    {
        c2 = clientGetTransient (c);
        if (c2)
        {
            FLAG_SET (c->xfwm_flags, XFWM_FLAG_WORKSPACE_SET);
            c->win_workspace = c2->win_workspace;
            if (FLAG_TEST (c2->flags, CLIENT_FLAG_STICKY))
            {
                FLAG_SET (c->flags, CLIENT_FLAG_STICKY);
                c->win_workspace = c->screen_info->current_ws;
            }
        }
    }

    TRACE ("initial desktop for window \"%s\" is %i", c->name, c->win_workspace);
    if (c->win_workspace > c->screen_info->workspace_count - 1)
    {
        TRACE ("value off limits, using %i instead", c->screen_info->workspace_count - 1);
        c->win_workspace = c->screen_info->workspace_count - 1;
        FLAG_SET (c->xfwm_flags, XFWM_FLAG_WORKSPACE_SET);
    }
    TRACE ("initial desktop for window \"%s\" is %i", c->name, c->win_workspace);
    if (FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
    {
        setHint (display_info, c->window, NET_WM_DESKTOP, (unsigned long) ALL_WORKSPACES);
    }
    else
    {
        setHint (display_info, c->window, NET_WM_DESKTOP, (unsigned long) c->win_workspace);
    }
}

void
clientSetNetClientList (ScreenInfo * screen_info, Atom a, GList * list)
{
    Window *listw;
    Window *index_dest;
    GList *index_src;
    gint size, i;

    TRACE ("entering");

    size = g_list_length (list);
    if (size < 1)
    {
        XDeleteProperty (myScreenGetXDisplay (screen_info), screen_info->xroot, a);
        return;
    }

    listw = g_new0 (Window, size + 1);
    if (listw)
    {
        TRACE ("%i windows in list for %i clients", size, screen_info->client_count);
        for (i = 0, index_dest = listw, index_src = list; i < size;
            i++, index_dest++, index_src = g_list_next (index_src))
        {
            Client *c = (Client *) index_src->data;
            *index_dest = c->window;
        }
        XChangeProperty (myScreenGetXDisplay (screen_info),
                         screen_info->xroot, a, XA_WINDOW, 32,
                         PropModeReplace, (unsigned char *) listw, size);
        g_free (listw);
    }
}

gboolean
clientValidateNetStrut (Client * c)
{
    ScreenInfo *screen_info;
    gboolean valid;

    g_return_val_if_fail (c != NULL, TRUE);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);
    screen_info = c->screen_info;
    valid = TRUE;

    if (c->struts[STRUTS_TOP] > screen_info->height - screen_info->margins[STRUTS_BOTTOM])
    {
       c->struts[STRUTS_TOP] = screen_info->height - screen_info->margins[STRUTS_BOTTOM];
       g_warning ("Top strut value for application window 0x%lx confined to %d", c->window, c->struts[STRUTS_TOP]);
       valid = FALSE;
    }

    if (c->struts[STRUTS_BOTTOM] > screen_info->height - screen_info->margins[STRUTS_TOP])
    {
       c->struts[STRUTS_BOTTOM] = screen_info->height - screen_info->margins[STRUTS_TOP];
       g_warning ("Bottom strut value for application window 0x%lx confined to %d", c->window, c->struts[STRUTS_BOTTOM]);
       valid = FALSE;
    }

    if (c->struts[STRUTS_LEFT] > screen_info->width - screen_info->margins[STRUTS_RIGHT])
    {
       c->struts[STRUTS_LEFT] = screen_info->height - screen_info->margins[STRUTS_RIGHT];
       g_warning ("Left strut value for application window 0x%lx confined to %d", c->window, c->struts[STRUTS_LEFT]);
       valid = FALSE;
    }
    if (c->struts[STRUTS_RIGHT] > screen_info->width - screen_info->margins[STRUTS_LEFT])
    {
       c->struts[STRUTS_RIGHT] = screen_info->height - screen_info->margins[STRUTS_LEFT];
       g_warning ("Right strut value for application window 0x%lx confined to %d", c->window, c->struts[STRUTS_RIGHT]);
       valid = FALSE;
    }

    return valid;
}

gboolean
clientGetNetStruts (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    unsigned long old_flags, new_flags;
    int old_struts[STRUTS_SIZE];
    gulong *struts;
    int nitems;
    int i;

    g_return_val_if_fail (c != NULL, FALSE);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    struts = NULL;

    /* Save old values */
    old_flags = c->flags & (CLIENT_FLAG_HAS_STRUT | CLIENT_FLAG_HAS_STRUT_PARTIAL);
    for (i = 0; i < STRUTS_SIZE; i++)
    {
        old_struts[i] = c->struts[i];
        c->struts[i] = 0;
    }
    FLAG_UNSET (c->flags, CLIENT_FLAG_HAS_STRUT);
    FLAG_UNSET (c->flags, CLIENT_FLAG_HAS_STRUT_PARTIAL);

    if (getCardinalList (display_info, c->window, NET_WM_STRUT_PARTIAL, &struts, &nitems))
    {
        if (nitems != STRUTS_SIZE)
        {
            if (struts)
            {
                XFree (struts);
            }
            /* Restore old values */
            if (old_flags)
            {
                FLAG_SET (c->flags, old_flags);
                for (i = 0; i < STRUTS_SIZE; i++)
                {
                    c->struts[i] = old_struts[i];
                }
            }
            return FALSE;
        }

        FLAG_SET (c->flags, CLIENT_FLAG_HAS_STRUT);
        FLAG_SET (c->flags, CLIENT_FLAG_HAS_STRUT_PARTIAL);
        for (i = 0; i < STRUTS_SIZE; i++)
        {
            c->struts[i] = (int) struts[i];
        }

        XFree (struts);
    }
    else if (getCardinalList (display_info, c->window, NET_WM_STRUT, &struts, &nitems))
    {
        if (nitems != 4)
        {
            if (struts)
            {
                XFree (struts);
            }
            /* Restore old values */
            if (old_flags)
            {
                FLAG_SET (c->flags, old_flags);
                for (i = 0; i < STRUTS_SIZE; i++)
                {
                    c->struts[i] = old_struts[i];
                }
            }
            return FALSE;
        }

        FLAG_SET (c->flags, CLIENT_FLAG_HAS_STRUT);
        for (i = 0; i < 4; i++)
        {
            c->struts[i] = (int) struts[i];
        }
        for (i = 4; i < STRUTS_SIZE; i++)
        {
            c->struts[i] = 0;
        }
        /* Fill(in values as for partial struts */
        c->struts[STRUTS_TOP_START_X] = c->struts[STRUTS_BOTTOM_START_X] = 0;
        c->struts[STRUTS_TOP_END_X] = c->struts[STRUTS_BOTTOM_END_X] =
            c->screen_info->width;
        c->struts[STRUTS_LEFT_START_Y] = c->struts[STRUTS_RIGHT_START_Y] = 0;
        c->struts[STRUTS_LEFT_END_Y] = c->struts[STRUTS_RIGHT_END_Y] =
            c->screen_info->height;

        XFree (struts);
    }

    if (FLAG_TEST (c->flags, CLIENT_FLAG_HAS_STRUT))
    {
        clientValidateNetStrut (c);
    }

    /* check for a change in struts flags */
    new_flags = c->flags & (CLIENT_FLAG_HAS_STRUT | CLIENT_FLAG_HAS_STRUT_PARTIAL);
    if (old_flags != new_flags)
    {
        return TRUE;
    }

    /* Flags haven't changed, check values */
    if (new_flags)
    {
        for (i = 0; i < STRUTS_SIZE; i++)
        {
            if (old_struts[i] != c->struts[i])
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

void
clientSetNetActions (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Atom atoms[16];
    int i;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    i = 0;

    /* Actions available for all */
    atoms[i++] = display_info->atoms[NET_WM_ACTION_CLOSE];

    /* Actions depending on the window type and current status */
    if (c->type & WINDOW_REGULAR_FOCUSABLE)
    {
        atoms[i++] = display_info->atoms[NET_WM_ACTION_ABOVE];
        atoms[i++] = display_info->atoms[NET_WM_ACTION_BELOW];
    }
    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE))
    {
        atoms[i++] = display_info->atoms[NET_WM_ACTION_FULLSCREEN];
        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_MOVE))
        {
            atoms[i++] = display_info->atoms[NET_WM_ACTION_MOVE];
        }
            if (FLAG_TEST_ALL (c->xfwm_flags, XFWM_FLAG_HAS_RESIZE | XFWM_FLAG_IS_RESIZABLE) &&
                !FLAG_TEST_ALL (c->flags, CLIENT_FLAG_MAXIMIZED))
        {
            atoms[i++] = display_info->atoms[NET_WM_ACTION_RESIZE];
        }
        if (CLIENT_CAN_MAXIMIZE_WINDOW (c))
        {
            atoms[i++] = display_info->atoms[NET_WM_ACTION_MAXIMIZE_HORZ];
            atoms[i++] = display_info->atoms[NET_WM_ACTION_MAXIMIZE_VERT];
        }
        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_BORDER))
        {
            atoms[i++] = display_info->atoms[NET_WM_ACTION_SHADE];
        }
    }
    if (CLIENT_CAN_HIDE_WINDOW (c))
    {
        atoms[i++] = display_info->atoms[NET_WM_ACTION_MINIMIZE];
    }
    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_STICK))
    {
        atoms[i++] = display_info->atoms[NET_WM_ACTION_CHANGE_DESKTOP];
        atoms[i++] = display_info->atoms[NET_WM_ACTION_STICK];
    }

    myDisplayErrorTrapPush (display_info);
    XChangeProperty (clientGetXDisplay (c), c->window, display_info->atoms[NET_WM_ALLOWED_ACTIONS],
                     XA_ATOM, 32, PropModeReplace, (unsigned char *) atoms, i);
    myDisplayErrorTrapPopIgnored (display_info);
}

void
clientWindowType (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    netWindowType old_type;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    old_type = c->type;
    c->initial_layer = c->win_layer;
    clientApplyMWMHints (c, FALSE);

    if (c->type_atom != None)
    {
        if (c->type_atom == display_info->atoms[NET_WM_WINDOW_TYPE_DESKTOP])
        {
            TRACE ("atom net_wm_window_type_desktop detected");
            c->type = WINDOW_DESKTOP;
            c->initial_layer = WIN_LAYER_DESKTOP;
            FLAG_SET (c->flags,
                CLIENT_FLAG_SKIP_PAGER | CLIENT_FLAG_STICKY |
                CLIENT_FLAG_SKIP_TASKBAR);
            FLAG_UNSET (c->xfwm_flags,
                XFWM_FLAG_HAS_RESIZE | XFWM_FLAG_HAS_MOVE |
                XFWM_FLAG_HAS_HIDE | XFWM_FLAG_HAS_MAXIMIZE |
                XFWM_FLAG_HAS_MENU | XFWM_FLAG_HAS_STICK |
                XFWM_FLAG_HAS_BORDER);
        }
        else if (c->type_atom == display_info->atoms[NET_WM_WINDOW_TYPE_DOCK])
        {
            TRACE ("atom net_wm_window_type_dock detected");
            c->type = WINDOW_DOCK;
            c->initial_layer = WIN_LAYER_DOCK;
             FLAG_SET (c->flags,
                CLIENT_FLAG_SKIP_PAGER | CLIENT_FLAG_STICKY |
                CLIENT_FLAG_SKIP_TASKBAR);
            FLAG_UNSET (c->xfwm_flags,
                XFWM_FLAG_HAS_BORDER |  XFWM_FLAG_HAS_MOVE |
                XFWM_FLAG_HAS_HIDE | XFWM_FLAG_HAS_MAXIMIZE |
                XFWM_FLAG_HAS_MENU);
        }
        else if (c->type_atom == display_info->atoms[NET_WM_WINDOW_TYPE_TOOLBAR])
        {
            TRACE ("atom net_wm_window_type_toolbar detected");
            c->type = WINDOW_TOOLBAR;
            c->initial_layer = WIN_LAYER_NORMAL;
            FLAG_SET (c->flags,
                CLIENT_FLAG_SKIP_PAGER | CLIENT_FLAG_SKIP_TASKBAR);
            FLAG_UNSET (c->xfwm_flags,
                XFWM_FLAG_HAS_HIDE | XFWM_FLAG_HAS_MAXIMIZE);
        }
        else if (c->type_atom == display_info->atoms[NET_WM_WINDOW_TYPE_MENU])
        {
            TRACE ("atom net_wm_window_type_menu detected");
            c->type = WINDOW_MENU;
            c->initial_layer = WIN_LAYER_NORMAL;
            /* The policy here is unclear :
               http://mail.gnome.org/archives/wm-spec-list/2002-May/msg00001.html
               As it seems, GNOME and KDE don't treat menu the same way...
             */
            FLAG_SET (c->flags,
                CLIENT_FLAG_SKIP_PAGER | CLIENT_FLAG_SKIP_TASKBAR);
            FLAG_UNSET (c->xfwm_flags,
                XFWM_FLAG_HAS_HIDE | XFWM_FLAG_HAS_MAXIMIZE);
        }
        else if (c->type_atom == display_info->atoms[NET_WM_WINDOW_TYPE_DIALOG])
        {
            TRACE ("atom net_wm_window_type_dialog detected");
            c->type = WINDOW_DIALOG;
            c->initial_layer = WIN_LAYER_NORMAL;
            /* Treat DIALOG without transient_for set as transient for group */
            if ((c->transient_for == None) || (!clientGetTransient (c)))
            {
                TRACE ("invalid transient 0x%lx specified for dialog window 0x%lx (%s)",
                       c->transient_for, c->window, c->name);
                c->transient_for = c->screen_info->xroot;
            }
            else
            {
                FLAG_SET (c->flags, CLIENT_FLAG_SKIP_TASKBAR);
            }
        }
        else if (c->type_atom == display_info->atoms[NET_WM_WINDOW_TYPE_NORMAL])
        {
            TRACE ("atom net_wm_window_type_normal detected");
            c->type = WINDOW_NORMAL;
            c->initial_layer = WIN_LAYER_NORMAL;
        }
        else if (c->type_atom == display_info->atoms[NET_WM_WINDOW_TYPE_UTILITY])
        {
            TRACE ("atom net_wm_window_type_utility detected");
            FLAG_SET (c->flags,
                CLIENT_FLAG_SKIP_PAGER | CLIENT_FLAG_SKIP_TASKBAR);
            c->type = WINDOW_UTILITY;
            c->initial_layer = WIN_LAYER_NORMAL;
            /* Treat UTILITY without transient_for set as transient for group */
            if ((c->transient_for == None) || (!clientGetTransient (c)))
            {
                TRACE ("invalid transient 0x%lx specified for utility window 0x%lx (%s)",
                       c->transient_for, c->window, c->name);
                c->transient_for = c->screen_info->xroot;
            }
        }
        else if (c->type_atom == display_info->atoms[NET_WM_WINDOW_TYPE_SPLASH])
        {
            TRACE ("atom net_wm_window_type_splash detected");
            c->type = WINDOW_SPLASHSCREEN;
            c->initial_layer = WIN_LAYER_NORMAL;
            FLAG_SET (c->flags,
                CLIENT_FLAG_SKIP_PAGER | CLIENT_FLAG_SKIP_TASKBAR);
            FLAG_UNSET (c->xfwm_flags,
                XFWM_FLAG_HAS_BORDER | XFWM_FLAG_HAS_HIDE |
                XFWM_FLAG_HAS_MENU | XFWM_FLAG_HAS_MOVE |
                XFWM_FLAG_HAS_RESIZE);
            /* Treat SPLASHSCREEN as transient for group to work around
             * broken apps placing splashscreens above and then complaining
             * it hides their dialogs, sigh.
             */
            if ((c->transient_for == None) || (!clientGetTransient (c)))
            {
                c->transient_for = c->screen_info->xroot;
            }
        }
        else if (c->type_atom == display_info->atoms[NET_WM_WINDOW_TYPE_NOTIFICATION])
        {
            TRACE ("atom net_wm_window_type_notification detected");
            c->type = WINDOW_NOTIFICATION;
            c->initial_layer = WIN_LAYER_NOTIFICATION;
            /* We unset these because CLIENT_FLAG_ABOVE will interfere with
               our layer placement and put the window in the ABOVE_DOCK
               layer, which is below the FULLSCREEN layer when the flags
               are processed later. */
            FLAG_UNSET (c->flags,
                CLIENT_FLAG_ABOVE | CLIENT_FLAG_BELOW);
        }
    }
    else
    {
        TRACE ("no \"net\" atom detected");
        c->type = WINDOW_NORMAL;
        c->initial_layer = c->win_layer;
    }

    if (clientIsValidTransientOrModal (c))
    {
        Client *c2;

        TRACE ("client \"%s\" is a transient or a modal", c->name);

        c2 = clientGetHighestTransientOrModalFor (c);
        if ((c2) && (c->initial_layer < c2->win_layer))
        {
            c->initial_layer = c2->win_layer;
            TRACE ("applied layer is %lu", c->initial_layer);
        }
    }
    if (!FLAG_TEST (c->flags, CLIENT_FLAG_ABOVE|CLIENT_FLAG_BELOW|CLIENT_FLAG_FULLSCREEN))
    {
        if ((old_type != c->type) || (c->initial_layer != c->win_layer))
        {
            TRACE ("setting layer %lu", c->initial_layer);
            clientSetLayer (c, c->initial_layer);
            clientSetNetState (c);
        }
    }
}

void
clientUpdateLayerState (Client * c)
{
    int layer;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (FLAG_TEST (c->flags, CLIENT_FLAG_ABOVE))
    {
        layer = WIN_LAYER_ABOVE_DOCK;
    }
    else if (FLAG_TEST (c->flags, CLIENT_FLAG_BELOW))
    {
        layer = WIN_LAYER_BELOW;
    }
    else
    {
        layer = c->initial_layer;
    }
    clientSetLayer (c, layer);
    clientSetNetState (c);
}

void
clientSetNetActiveWindow (ScreenInfo *screen_info, Client *c, guint32 timestamp)
{
    DisplayInfo *display_info;
    unsigned long data[2];

    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    display_info = screen_info->display_info;
    data[0] = (unsigned long) None;
    data[1] = (unsigned long) timestamp;
    if (c)
    {
        data[0] = (unsigned long) c->window;
    }
    XChangeProperty (myScreenGetXDisplay (screen_info), screen_info->xroot,
                     display_info->atoms[NET_ACTIVE_WINDOW], XA_WINDOW, 32,
                     PropModeReplace, (unsigned char *) data, 2);
}

void
clientHandleNetActiveWindow (Client *c, guint32 timestamp, gboolean source_is_application)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;
    guint32 ev_time, current_time;
    Client *focused;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    ev_time = myDisplayGetTime (display_info, timestamp);
    if (!source_is_application && (timestamp == 0))
    {
         TRACE ("client \"%s\" (0x%lx) sent a NET_ACTIVE_WINDOW message with a timestamp of 0", c->name, c->window);
         source_is_application = TRUE;
    }
    if (source_is_application)
    {
        current_time = myDisplayGetLastUserTime (display_info);

        TRACE ("time of event received is %u, current XServer time is %u", (guint32) ev_time, (guint32) current_time);
        if ((screen_info->params->prevent_focus_stealing) &&
            (TIMESTAMP_IS_BEFORE((guint32) ev_time, (guint32) current_time) ||
             (screen_info->params->activate_action == ACTIVATE_ACTION_NONE)))
        {
            focused = clientGetFocus ();
            /* We do not want to set the demand attention flag if the window is focused though */
            if (c != focused)
            {
                TRACE ("setting WM_STATE_DEMANDS_ATTENTION flag on \"%s\" (0x%lx)", c->name, c->window);
                FLAG_SET (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION);
                clientSetNetState (c);
            }
        }
        else
        {
            clientActivate (c, ev_time, source_is_application);
        }
    }
    else
    {
        /* The request is either from a pager or an older client, use the most accurate timestamp */
        clientActivate (c, getXServerTime (display_info), source_is_application);
    }
}

static gboolean
ping_timeout_cb (gpointer data)
{
    Client *c;

    TRACE ("entering");

    c = (Client *) data;
    if (c)
    {
        c->ping_timeout_id = 0;
        TRACE ("ping timeout on client \"%s\"", c->name);
        terminateShowDialog (c);
    }
    return FALSE;
}

void
clientRemoveNetWMPing (Client *c)
{
    g_return_if_fail (c != NULL);

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (c->ping_timeout_id)
    {
        g_source_remove (c->ping_timeout_id);
    }
    c->ping_timeout_id = 0;
    c->ping_time = 0;
}

void
clientReceiveNetWMPong (ScreenInfo *screen_info, guint32 timestamp)
{
    Client *c;
    guint i;

    g_return_if_fail (screen_info != NULL);
    g_return_if_fail (timestamp != CurrentTime);

    TRACE ("timestamp %u", (unsigned int) timestamp);

    for (c = screen_info->clients, i = 0; i < screen_info->client_count; c = c->next, i++)
    {
        if (c->ping_time == timestamp)
        {
            clientRemoveNetWMPing (c);
        }
    }
}

gboolean
clientSendNetWMPing (Client *c, guint32 timestamp)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_val_if_fail (c != NULL, FALSE);

    TRACE ("client \"%s\" (0x%lx) timestamp %u", c->name, c->window, (unsigned int) timestamp);

    if (!FLAG_TEST (c->wm_flags, WM_FLAG_PING))
    {
        return FALSE;
    }

    clientRemoveNetWMPing (c);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    /* Working around GDK bugs (multi-screen has been removed from GDK a while ago) */
    if (!xfwm_is_default_screen (screen_info->gscr))
    {
        return FALSE;
    }

    /* Makes sure the timestamp is meaningfull */
    c->ping_time = myDisplayGetTime (display_info, timestamp);
    g_return_val_if_fail (timestamp != CurrentTime, FALSE);

    sendClientMessage (screen_info, c->window, NET_WM_PING, timestamp);
    c->ping_timeout_id =
        g_timeout_add_full (G_PRIORITY_DEFAULT,
                            CLIENT_PING_TIMEOUT,
                            ping_timeout_cb, c, NULL);
    return TRUE;
}

gboolean
clientGetUserTime (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_val_if_fail (c != NULL, FALSE);
    g_return_val_if_fail (c->window != None, FALSE);

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    /*
     * We can use "c->user_time_win" safely here because this will be
     * the same as "c->window" if the app does not support the protocol
     * NET_WM_USER_TIME_WINDOW
     */

    if (getNetWMUserTime (display_info, c->user_time_win, &c->user_time))
    {
        guint32 last_user_time;

        last_user_time = myDisplayGetLastUserTime (display_info);
        if (c->user_time && TIMESTAMP_IS_BEFORE(last_user_time, c->user_time))
        {
            myDisplaySetLastUserTime (display_info, c->user_time);
        }
        FLAG_SET (c->flags, CLIENT_FLAG_HAS_USER_TIME);

        return TRUE;
    }

    return FALSE;
}

void
clientAddUserTimeWin (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (c != NULL);
    g_return_if_fail (c->window != None);

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if ((c->user_time_win != None) && (c->user_time_win != c->window))
    {
        myDisplayErrorTrapPush (display_info);
        XSelectInput (display_info->dpy, c->user_time_win, PropertyChangeMask);
        myDisplayErrorTrapPopIgnored (display_info);
    }
}

void
clientRemoveUserTimeWin (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (c != NULL);
    g_return_if_fail (c->window != None);

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if ((c->user_time_win != None) && (c->user_time_win != c->window))
    {
        myDisplayErrorTrapPush (display_info);
        XSelectInput (display_info->dpy, c->user_time_win, NoEventMask);
        myDisplayErrorTrapPopIgnored (display_info);
    }
}
