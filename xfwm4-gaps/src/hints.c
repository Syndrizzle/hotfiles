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
        Metacity - (c) 2001 Havoc Pennington
        xfwm4    - (c) 2002-2020 Olivier Fourdan

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#ifdef HAVE_XRES
#include <X11/extensions/XRes.h>
#endif

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libxfce4util/libxfce4util.h>

#include "display.h"
#include "screen.h"
#include "hints.h"

static gboolean
check_type_and_format (int expected_format, Atom expected_type, int n_items, int format, Atom type)
{
    if ((expected_format == format) && (expected_type == type) && (n_items < 0 || n_items > 0))
    {
        return TRUE;
    }
    return FALSE;
}

static gchar *
internal_utf8_strndup (const gchar *src, gssize max_len)
{
    const gchar *s;

    if (max_len <= 0)
    {
        return g_strdup (src);
    }

    s = src;
    while (max_len && *s)
    {
        s = g_utf8_next_char (s);
        max_len--;
    }

    return g_strndup (src, s - src);
}

unsigned long
getWMState (DisplayInfo *display_info, Window w)
{
    Atom real_type;
    int real_format;
    unsigned long items_read, items_left;
    unsigned char *data;
    unsigned long state;
    int result, status;

    TRACE ("window 0x%lx", w);

    data = NULL;
    state = WithdrawnState;

    myDisplayErrorTrapPush (display_info);
    status = XGetWindowProperty (display_info->dpy, w, display_info->atoms[WM_STATE],
                                 0, 3L, FALSE, display_info->atoms[WM_STATE],
                                 &real_type, &real_format, &items_read, &items_left,
                                 (unsigned char **) &data);
    result = myDisplayErrorTrapPop (display_info);

    if ((result == Success) &&
        (status == Success) &&
        (items_read) &&
        (data != NULL))
    {
        state = *data;
    }
    XFree (data);

    return state;
}

void
setWMState (DisplayInfo *display_info, Window w, unsigned long state)
{
    CARD32 data[2];

    TRACE ("window 0x%lx", w);

    data[0] = state;
    data[1] = None;
    myDisplayErrorTrapPush (display_info);
    XChangeProperty (display_info->dpy, w, display_info->atoms[WM_STATE],
                     display_info->atoms[WM_STATE], 32, PropModeReplace,
                     (unsigned char *) data, 2);
    myDisplayErrorTrapPopIgnored (display_info);
}

PropMwmHints *
getMotifHints (DisplayInfo *display_info, Window w)
{
    Atom real_type;
    int real_format;
    unsigned long items_read, items_left;
    unsigned char *data;
    PropMwmHints *hints;
    int result, status;

    TRACE ("window 0x%lx", w);

    data = NULL;
    hints = NULL;

    myDisplayErrorTrapPush (display_info);
    status = XGetWindowProperty (display_info->dpy, w, display_info->atoms[MOTIF_WM_HINTS], 0L,
                                 MWM_HINTS_ELEMENTS, FALSE, display_info->atoms[MOTIF_WM_HINTS],
                                 &real_type, &real_format, &items_read, &items_left,
                                 (unsigned char **) &data);
    result = myDisplayErrorTrapPop (display_info);

    if ((status == Success) &&
        (result == Success) &&
        (data != NULL) &&
        (items_read >= MWM_HINTS_ELEMENTS))
    {
        hints = g_new0 (PropMwmHints, 1);
        memcpy (hints, data, sizeof (PropMwmHints));
    }

    XFree (data);

    return hints;
}

unsigned int
getWMProtocols (DisplayInfo *display_info, Window w)
{
    Atom *protocols, *ap;
    gint i, n;
    Atom atype;
    int aformat;
    unsigned int flags;
    unsigned long bytes_remain, nitems;
    unsigned char *data;
    int result, status;

    TRACE ("window 0x%lx", w);

    flags = 0;
    protocols = NULL;
    data = NULL;

    myDisplayErrorTrapPush (display_info);
    status = XGetWMProtocols (display_info->dpy, w, &protocols, &n);
    result = myDisplayErrorTrapPop (display_info);

    if (status && (result == Success) && (protocols != NULL))
    {
        for (i = 0, ap = protocols; i < n; i++, ap++)
        {
            if (*ap == display_info->atoms[WM_TAKE_FOCUS])
            {
                flags |= WM_PROTOCOLS_TAKE_FOCUS;
            }
            if (*ap == display_info->atoms[WM_DELETE_WINDOW])
            {
                flags |= WM_PROTOCOLS_DELETE_WINDOW;
            }
            /* KDE extension */
            if (*ap == display_info->atoms[NET_WM_CONTEXT_HELP])
            {
                flags |= WM_PROTOCOLS_CONTEXT_HELP;
            }
            /* Ping */
            if (*ap == display_info->atoms[NET_WM_PING])
            {
                flags |= WM_PROTOCOLS_PING;
            }
        }
    }
    else
    {
        myDisplayErrorTrapPush (display_info);
        status = XGetWindowProperty (display_info->dpy, w,
                                     display_info->atoms[WM_PROTOCOLS],
                                     0L, 10L, FALSE,
                                     display_info->atoms[WM_PROTOCOLS],
                                     &atype, &aformat, &nitems, &bytes_remain,
                                     (unsigned char **) &data);
        result = myDisplayErrorTrapPop (display_info);

        if ((status == Success) &&
            (result == Success) &&
            (data != NULL))
        {
            for (i = 0, ap = (Atom *) data; (unsigned long) i < nitems; i++, ap++)
            {
                if (*ap == display_info->atoms[WM_TAKE_FOCUS])
                {
                    flags |= WM_PROTOCOLS_TAKE_FOCUS;
                }
                if (*ap == display_info->atoms[WM_DELETE_WINDOW])
                {
                    flags |= WM_PROTOCOLS_DELETE_WINDOW;
                }
                /* KDE extension */
                if (*ap == display_info->atoms[NET_WM_CONTEXT_HELP])
                {
                    flags |= WM_PROTOCOLS_CONTEXT_HELP;
                }
            }
        }
        XFree (data);
    }
    XFree (protocols);

    return flags;
}

gboolean
getHint (DisplayInfo *display_info, Window w, int atom_id, long *value)
{
    Atom real_type;
    unsigned long items_read, items_left;
    unsigned char *data;
    int real_format;
    gboolean success;
    int result, status;

    g_return_val_if_fail (((atom_id >= 0) && (atom_id < ATOM_COUNT)), FALSE);
    TRACE ("window 0x%lx atom %i", w, atom_id);

    success = FALSE;
    *value = 0;
    data = NULL;

    myDisplayErrorTrapPush (display_info);
    status = XGetWindowProperty (display_info->dpy, w, display_info->atoms[atom_id],
                                 0L, 1L, FALSE, XA_CARDINAL, &real_type, &real_format,
                                 &items_read, &items_left, (unsigned char **) &data);
    result = myDisplayErrorTrapPop (display_info);

    if ((result == Success) &&
        (status == Success) &&
        (data != NULL) &&
        (items_read > 0))
    {
        *value = *((long *) data) & ((1LL << real_format) - 1);
        XFree (data);
        success = TRUE;
    }

    return success;
}

void
setHint (DisplayInfo *display_info, Window w, int atom_id, long value)
{
    g_return_if_fail ((atom_id >= 0) && (atom_id < ATOM_COUNT));
    TRACE ("window 0x%lx atom %i", w, atom_id);

    myDisplayErrorTrapPush (display_info);
    XChangeProperty (display_info->dpy, w, display_info->atoms[atom_id], XA_CARDINAL,
                     32, PropModeReplace, (unsigned char *) &value, 1);
    myDisplayErrorTrapPopIgnored (display_info);
}

void
getDesktopLayout (DisplayInfo *display_info, Window root, int ws_count, NetWmDesktopLayout * layout)
{
    Atom real_type;
    unsigned long items_read, items_left;
    unsigned long orientation, cols, rows, start;
    unsigned long *ptr;
    unsigned char *data;
    int real_format;
    gboolean success;
    int result, status;

    ptr = NULL;
    data = NULL;
    success = FALSE;

    myDisplayErrorTrapPush (display_info);
    status = XGetWindowProperty (display_info->dpy, root,
                                 display_info->atoms[NET_DESKTOP_LAYOUT],
                                 0L, 4L, FALSE, XA_CARDINAL,
                                 &real_type, &real_format, &items_read, &items_left,
                                 (unsigned char **) &data);
    result = myDisplayErrorTrapPop (display_info);

    if ((result == Success) &&
        (status == Success) &&
        (data != NULL) &&
        (items_read >= 3))
    {
        do
        {
            ptr = (unsigned long *) data;
            orientation = (unsigned long) *ptr++;
            cols = (unsigned long) *ptr++;
            rows = (unsigned long) *ptr++;
            start = (items_read >= 4) ? (unsigned long) *ptr++ : NET_WM_TOPLEFT;

            if (orientation > NET_WM_ORIENTATION_VERT)
            {
                break;
            }

            if (start > NET_WM_BOTTOMLEFT)
            {
                break;
            }

            if ((rows == 0) && (cols == 0))
            {
                break;
            }

            if (rows == 0)
            {
                rows = (ws_count - 1) / cols + 1;
            }

            if (cols == 0)
            {
                cols = (ws_count - 1) / rows + 1;
            }

            layout->orientation = (unsigned long) orientation;
            layout->cols = cols;
            layout->rows = rows;
            layout->start = start;
            success = TRUE;
        } while (0);
    }
    XFree (data);

    if (!success)
    {
        /* Assume HORZ, TOPLEFT, one row by default */
        layout->orientation = NET_WM_ORIENTATION_HORZ;
        layout->cols = ws_count;
        layout->rows = 1;
        layout->start = NET_WM_TOPLEFT;
    }
}

void
setNetSupportedHint (DisplayInfo *display_info, Window root, Window check_win)
{
    Atom atoms[ATOM_COUNT];
    unsigned long data[1];
    int i;

    i = 0;
    atoms[i++] = display_info->atoms[NET_ACTIVE_WINDOW];
    atoms[i++] = display_info->atoms[NET_CLIENT_LIST];
    atoms[i++] = display_info->atoms[NET_CLIENT_LIST_STACKING];
    atoms[i++] = display_info->atoms[NET_CLOSE_WINDOW];
    atoms[i++] = display_info->atoms[NET_CURRENT_DESKTOP];
    atoms[i++] = display_info->atoms[NET_DESKTOP_GEOMETRY];
    atoms[i++] = display_info->atoms[NET_DESKTOP_LAYOUT];
    atoms[i++] = display_info->atoms[NET_DESKTOP_NAMES];
    atoms[i++] = display_info->atoms[NET_DESKTOP_VIEWPORT];
    atoms[i++] = display_info->atoms[NET_FRAME_EXTENTS];
    atoms[i++] = display_info->atoms[NET_MOVERESIZE_WINDOW];
    atoms[i++] = display_info->atoms[NET_NUMBER_OF_DESKTOPS];
    atoms[i++] = display_info->atoms[NET_REQUEST_FRAME_EXTENTS];
    atoms[i++] = display_info->atoms[NET_SHOWING_DESKTOP];
    atoms[i++] = display_info->atoms[NET_SUPPORTED];
    atoms[i++] = display_info->atoms[NET_SUPPORTING_WM_CHECK];
    atoms[i++] = display_info->atoms[NET_SYSTEM_TRAY_OPCODE];
    atoms[i++] = display_info->atoms[NET_WM_ACTION_ABOVE];
    atoms[i++] = display_info->atoms[NET_WM_ACTION_BELOW];
    atoms[i++] = display_info->atoms[NET_WM_ACTION_CHANGE_DESKTOP];
    atoms[i++] = display_info->atoms[NET_WM_ACTION_CLOSE];
    atoms[i++] = display_info->atoms[NET_WM_ACTION_FULLSCREEN];
    atoms[i++] = display_info->atoms[NET_WM_ACTION_MAXIMIZE_HORZ];
    atoms[i++] = display_info->atoms[NET_WM_ACTION_MAXIMIZE_VERT];
    atoms[i++] = display_info->atoms[NET_WM_ACTION_MINIMIZE];
    atoms[i++] = display_info->atoms[NET_WM_ACTION_MOVE];
    atoms[i++] = display_info->atoms[NET_WM_ACTION_RESIZE];
    atoms[i++] = display_info->atoms[NET_WM_ACTION_SHADE];
    atoms[i++] = display_info->atoms[NET_WM_ACTION_STICK];
    atoms[i++] = display_info->atoms[NET_WM_ALLOWED_ACTIONS];
    atoms[i++] = display_info->atoms[NET_WM_BYPASS_COMPOSITOR];
    atoms[i++] = display_info->atoms[NET_WM_CONTEXT_HELP];
    atoms[i++] = display_info->atoms[NET_WM_DESKTOP];
    atoms[i++] = display_info->atoms[NET_WM_FULLSCREEN_MONITORS];
    atoms[i++] = display_info->atoms[NET_WM_ICON];
    atoms[i++] = display_info->atoms[NET_WM_ICON_GEOMETRY];
    atoms[i++] = display_info->atoms[NET_WM_ICON_NAME];
    atoms[i++] = display_info->atoms[NET_WM_MOVERESIZE];
    atoms[i++] = display_info->atoms[NET_WM_NAME];
    atoms[i++] = display_info->atoms[NET_WM_OPAQUE_REGION];
    atoms[i++] = display_info->atoms[NET_WM_PID];
    atoms[i++] = display_info->atoms[NET_WM_PING];
    atoms[i++] = display_info->atoms[NET_WM_STATE];
    atoms[i++] = display_info->atoms[NET_WM_STATE_ABOVE];
    atoms[i++] = display_info->atoms[NET_WM_STATE_BELOW];
    atoms[i++] = display_info->atoms[NET_WM_STATE_DEMANDS_ATTENTION];
    atoms[i++] = display_info->atoms[NET_WM_STATE_FOCUSED];
    atoms[i++] = display_info->atoms[NET_WM_STATE_FULLSCREEN];
    atoms[i++] = display_info->atoms[NET_WM_STATE_HIDDEN];
    atoms[i++] = display_info->atoms[NET_WM_STATE_MAXIMIZED_HORZ];
    atoms[i++] = display_info->atoms[NET_WM_STATE_MAXIMIZED_VERT];
    atoms[i++] = display_info->atoms[NET_WM_STATE_MODAL];
    atoms[i++] = display_info->atoms[NET_WM_STATE_SHADED];
    atoms[i++] = display_info->atoms[NET_WM_STATE_SKIP_PAGER];
    atoms[i++] = display_info->atoms[NET_WM_STATE_SKIP_TASKBAR];
    atoms[i++] = display_info->atoms[NET_WM_STATE_STICKY];
    atoms[i++] = display_info->atoms[NET_WM_STRUT];
    atoms[i++] = display_info->atoms[NET_WM_STRUT_PARTIAL];
    atoms[i++] = display_info->atoms[NET_WM_SYNC_REQUEST];
    atoms[i++] = display_info->atoms[NET_WM_SYNC_REQUEST_COUNTER];
    atoms[i++] = display_info->atoms[NET_WM_USER_TIME];
    atoms[i++] = display_info->atoms[NET_WM_USER_TIME_WINDOW];
    atoms[i++] = display_info->atoms[NET_WM_WINDOW_OPACITY];
    atoms[i++] = display_info->atoms[NET_WM_WINDOW_OPACITY_LOCKED];
    atoms[i++] = display_info->atoms[NET_WM_WINDOW_TYPE];
    atoms[i++] = display_info->atoms[NET_WM_WINDOW_TYPE_DESKTOP];
    atoms[i++] = display_info->atoms[NET_WM_WINDOW_TYPE_DIALOG];
    atoms[i++] = display_info->atoms[NET_WM_WINDOW_TYPE_DOCK];
    atoms[i++] = display_info->atoms[NET_WM_WINDOW_TYPE_MENU];
    atoms[i++] = display_info->atoms[NET_WM_WINDOW_TYPE_NORMAL];
    atoms[i++] = display_info->atoms[NET_WM_WINDOW_TYPE_SPLASH];
    atoms[i++] = display_info->atoms[NET_WM_WINDOW_TYPE_TOOLBAR];
    atoms[i++] = display_info->atoms[NET_WM_WINDOW_TYPE_UTILITY];
    atoms[i++] = display_info->atoms[NET_WORKAREA];
    /* GTK specific hints */
    atoms[i++] = display_info->atoms[GTK_FRAME_EXTENTS];
    atoms[i++] = display_info->atoms[GTK_HIDE_TITLEBAR_WHEN_MAXIMIZED];
    atoms[i++] = display_info->atoms[GTK_SHOW_WINDOW_MENU];
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    atoms[i++] = display_info->atoms[NET_STARTUP_ID];
#endif
#ifdef ENABLE_KDE_SYSTRAY_PROXY
    atoms[i++] = display_info->atoms[KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR];
#endif
    g_assert (i < ATOM_COUNT);
    data[0] = check_win;
    XChangeProperty (display_info->dpy, root, display_info->atoms[NET_SUPPORTED],
                     XA_ATOM, 32, PropModeReplace, (unsigned char *) atoms, i);
    XChangeProperty (display_info->dpy, check_win, display_info->atoms[NET_SUPPORTING_WM_CHECK],
                     XA_WINDOW, 32, PropModeReplace, (unsigned char *) data, 1);
    XChangeProperty (display_info->dpy, root, display_info->atoms[NET_SUPPORTING_WM_CHECK],
                     XA_WINDOW, 32, PropModeReplace, (unsigned char *) data, 1);
}

gboolean
getAtomList (DisplayInfo *display_info, Window w, int atom_id, Atom ** atoms_p, int *n_atoms_p)
{
    Atom type;
    int format;
    unsigned long n_atoms;
    unsigned long bytes_after;
    unsigned char *data;
    Atom *atoms;
    int result, status;

    *atoms_p = NULL;
    *n_atoms_p = 0;

    g_return_val_if_fail (((atom_id >= 0) && (atom_id < ATOM_COUNT)), FALSE);
    TRACE ("window 0x%lx atom %i", w, atom_id);

    myDisplayErrorTrapPush (display_info);
    status = XGetWindowProperty (display_info->dpy, w, display_info->atoms[atom_id],
                                 0, G_MAXLONG, FALSE, XA_ATOM, &type, &format, &n_atoms,
                                 &bytes_after, (unsigned char **) &data);
    result = myDisplayErrorTrapPop (display_info);

    if ((result != Success) ||
        (status != Success) ||
        (data == NULL) ||
        (type == None))
    {
        XFree (data);
        return FALSE;
    }

    atoms = (Atom *) data;
    if (!check_type_and_format (32, XA_ATOM, -1, format, type))
    {
        XFree (atoms);
        *atoms_p = NULL;
        *n_atoms_p = 0;
        return FALSE;
    }

    *atoms_p = atoms;
    *n_atoms_p = n_atoms;

    return TRUE;
}

gboolean
getCardinalList (DisplayInfo *display_info, Window w, int atom_id, unsigned long **cardinals_p, int *n_cardinals_p)
{
    Atom type;
    int format;
    unsigned int i;
    unsigned long n_cardinals;
    unsigned long bytes_after;
    unsigned char *data;
    unsigned long *cardinals;
    int result, status;

    *cardinals_p = NULL;
    *n_cardinals_p = 0;

    g_return_val_if_fail (((atom_id >= 0) && (atom_id < ATOM_COUNT)), FALSE);
    TRACE ("window 0x%lx atom %i", w, atom_id);

    myDisplayErrorTrapPush (display_info);
    status = XGetWindowProperty (display_info->dpy, w, display_info->atoms[atom_id],
                                 0, G_MAXLONG, FALSE, XA_CARDINAL,
                                 &type, &format, &n_cardinals, &bytes_after,
                                 (unsigned char **) &data);
    result = myDisplayErrorTrapPop (display_info);

    if ((result != Success) ||
        (status != Success) ||
        (data == NULL) ||
        (type == None))
    {
        XFree (data);
        return FALSE;
    }

    cardinals = (unsigned long *) data;
    if (!check_type_and_format (32, XA_CARDINAL, -1, format, type))
    {
        XFree (cardinals);
        return FALSE;
    }

    *cardinals_p = cardinals;
    *n_cardinals_p = n_cardinals;
    for (i = 0; i < n_cardinals; i++)
    {
        (*cardinals_p)[i] = (*cardinals_p)[i] & ((1LL << format) - 1);
    }

    return TRUE;
}

void
setNetWorkarea (DisplayInfo *display_info, Window root, int nb_workspaces, int width, int height, int * m)
{
    unsigned long *data, *ptr;
    int i, j;

    TRACE ("workspaces %i [%i×%i]", nb_workspaces, width, height);

    j = (nb_workspaces ? nb_workspaces : 1);
    data = (unsigned long *) g_new0 (unsigned long, j * 4);
    ptr = data;
    for (i = 0; i < j; i++)
    {
        *ptr++ = (unsigned long) m[STRUTS_LEFT];
        *ptr++ = (unsigned long) m[STRUTS_TOP];
        *ptr++ = (unsigned long) (width  - (m[STRUTS_LEFT] + m[STRUTS_RIGHT]));
        *ptr++ = (unsigned long) (height - (m[STRUTS_TOP]  + m[STRUTS_BOTTOM]));
    }

    XChangeProperty (display_info->dpy, root, display_info->atoms[NET_WORKAREA],
                     XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data, j * 4);
    g_free (data);
}

void
setNetFrameExtents (DisplayInfo *display_info, Window w, int top, int left, int right, int bottom)
{
    unsigned long data[4] = { 0, 0, 0, 0 };

    TRACE ("window 0x%lx [left=%i,right=%i,top=%i,bottom=%i]", w, left, right, top, bottom);

    data[0] = (unsigned long) left;
    data[1] = (unsigned long) right;
    data[2] = (unsigned long) top;
    data[3] = (unsigned long) bottom;
    myDisplayErrorTrapPush (display_info);
    XChangeProperty (display_info->dpy, w, display_info->atoms[NET_FRAME_EXTENTS],
                     XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data, 4);
    myDisplayErrorTrapPopIgnored (display_info);
}

void
setNetFullscreenMonitors (DisplayInfo *display_info, Window w, gint top, gint bottom, gint left, gint right)
{
    unsigned long data[4] = { 0, 0, 0, 0 };

    TRACE ("window 0x%lx [top=%i,bottom=%i,left=%i,right=%i]", w, top, bottom, left, right);

    data[0] = (unsigned long) top;;
    data[1] = (unsigned long) bottom;
    data[2] = (unsigned long) left;
    data[3] = (unsigned long) right;
    myDisplayErrorTrapPush (display_info);
    XChangeProperty (display_info->dpy, w, display_info->atoms[NET_WM_FULLSCREEN_MONITORS],
                     XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data, 4);
    myDisplayErrorTrapPopIgnored (display_info);
}

int
getNetCurrentDesktop (DisplayInfo *display_info, Window root)
{
    long ws;

    getHint (display_info, root, NET_CURRENT_DESKTOP, &ws);
    return (int) ws;
}

void
setNetCurrentDesktop (DisplayInfo *display_info, Window root, int workspace)
{
    unsigned long data[2];

    TRACE ("workspace %i", workspace);

    data[0] = 0;
    data[1] = 0;
    XChangeProperty (display_info->dpy, root, display_info->atoms[NET_DESKTOP_VIEWPORT],
                     XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data, 2);
    data[0] = workspace;
    XChangeProperty (display_info->dpy, root, display_info->atoms[NET_CURRENT_DESKTOP],
                     XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data, 1);
}

void
setNetDesktopInfo (DisplayInfo *display_info, Window root, int workspace, int width, int height)
{
    unsigned long data[2];

    TRACE ("workspace %i [%i×%i]", workspace, width, height);
    
    data[0] = width;
    data[1] = height;
    XChangeProperty (display_info->dpy, root, display_info->atoms[NET_DESKTOP_GEOMETRY],
                     XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data, 2);
    setNetCurrentDesktop (display_info, root, workspace);
}

void
setUTF8StringHint (DisplayInfo *display_info, Window w, int atom_id, const gchar *val)
{
    g_return_if_fail ((atom_id >= 0) && (atom_id < ATOM_COUNT));

    TRACE ("window 0x%lx atom %i", w, atom_id);
    myDisplayErrorTrapPush (display_info);
    XChangeProperty (display_info->dpy, w, display_info->atoms[atom_id],
                     display_info->atoms[UTF8_STRING], 8, PropModeReplace,
                     (unsigned char *) val, strlen (val));
    myDisplayErrorTrapPopIgnored (display_info);
}

void
getTransientFor (DisplayInfo *display_info, Window root, Window w, Window * transient_for)
{
    int result, status;

    TRACE ("window 0x%lx", w);

    myDisplayErrorTrapPush (display_info);
    status = XGetTransientForHint (display_info->dpy, w, transient_for);
    result = myDisplayErrorTrapPop (display_info);

    if ((result == Success) && status)
    {
        if (*transient_for == None)
        {
            /* Treat transient for "none" same as transient for root */
            *transient_for = root;
        }
        else if (*transient_for == w)
        {
            /* Very unlikely to happen, but who knows, maybe a braindead app */
            *transient_for = None;
        }
    }
    else
    {
        *transient_for = None;
    }

    TRACE ("window (0x%lx) is transient for (0x%lx)", w, *transient_for);
}

static char *
textPropertyToUTF8 (DisplayInfo *display_info, const XTextProperty * prop)
{
    char **list;
    int count;
    char *retval;

    list = NULL;
    count = gdk_text_property_to_utf8_list_for_display (display_info->gdisplay,
                                                        gdk_x11_xatom_to_atom (prop->encoding),
                                                        prop->format, prop->value, prop->nitems, &list);
    if (count == 0)
    {
        TRACE ("gdk_text_property_to_utf8_list returned 0");
        return NULL;
    }
    retval = list[0];
    list[0] = g_strdup ("");
    g_strfreev (list);

    return retval;
}

static char *
getTextProperty (DisplayInfo *display_info, Window w, Atom a)
{
    XTextProperty text;
    char *retval;
    int result, status;

    TRACE ("window 0x%lx", w);

    text.nitems = 0;
    text.value = NULL;

    myDisplayErrorTrapPush (display_info);
    status = XGetTextProperty (display_info->dpy, w, &text, a);
    result = myDisplayErrorTrapPop (display_info);

    if ((result == Success) && status)
    {
        retval = textPropertyToUTF8 (display_info, &text);
        if (retval)
        {
            xfce_utf8_remove_controls((gchar *) retval, MAX_STR_LENGTH, NULL);
        }
    }
    else
    {
        retval = NULL;
        TRACE ("XGetTextProperty() failed");
    }
    XFree (text.value);

    return retval;
}

static gboolean
getUTF8StringData (DisplayInfo *display_info, Window w, int atom_id, gchar **str_p, guint *length)
{
    Atom type;
    int format;
    unsigned long bytes_after;
    unsigned char *str;
    unsigned long n_items;
    int result, status;

    g_return_val_if_fail (((atom_id >= 0) && (atom_id < ATOM_COUNT)), FALSE);
    TRACE ("window 0x%lx atom %i", w, atom_id);

    *str_p = NULL;
    str = NULL;

    myDisplayErrorTrapPush (display_info);
    status = XGetWindowProperty (display_info->dpy, w, display_info->atoms[atom_id],
                                 0, G_MAXLONG, FALSE, display_info->atoms[UTF8_STRING],
                                 &type, &format, &n_items, &bytes_after,
                                 (unsigned char **) &str);
    result = myDisplayErrorTrapPop (display_info);

    if ((result != Success) ||
        (status != Success) ||
        (str == NULL) ||
        (type == None))
    {
        TRACE ("no UTF8_STRING property found");
        XFree (str);
        return FALSE;
    }

    if (!check_type_and_format (8, display_info->atoms[UTF8_STRING], -1, format, type))
    {
        TRACE ("UTF8_STRING value invalid");
        XFree (str);
        return FALSE;
    }

    *str_p = (char *) str;
    *length = n_items;

    return TRUE;
}

gboolean
getUTF8String (DisplayInfo *display_info, Window w, int atom_id, gchar **str_p, guint *length)
{
    char *xstr;

    g_return_val_if_fail (((atom_id >= 0) && (atom_id < ATOM_COUNT)), FALSE);
    TRACE ("window 0x%lx atom id %i", w, atom_id);

    if (!getUTF8StringData (display_info, w, atom_id, &xstr, length))
    {
        *str_p = NULL;
        *length = 0;

        return FALSE;
    }

    /* gmalloc the returned string */
    *str_p = internal_utf8_strndup (xstr, MAX_STR_LENGTH);
    XFree (xstr);

    if (!g_utf8_validate (*str_p, -1, NULL))
    {
        TRACE ("getUTF8String() returned invalid UTF-8 characters");
        g_free (*str_p);
        str_p = NULL;
        length = 0;

        return FALSE;
    }

    if (*str_p)
    {
        xfce_utf8_remove_controls((gchar *) *str_p, -1, NULL);
    }

    return TRUE;
}

gboolean
getUTF8StringList (DisplayInfo *display_info, Window w, int atom_id, gchar ***str_p, guint *n_items)
{
    char *xstr, *ptr;
    gchar **retval;
    guint i, length;

    g_return_val_if_fail (((atom_id >= 0) && (atom_id < ATOM_COUNT)), FALSE);
    TRACE ("window 0x%lx atom id %i", w, atom_id);

    *str_p = NULL;
    *n_items = 0;

    if (!getUTF8StringData (display_info, w, atom_id, &xstr, &length) || !length)
    {
        return FALSE;
    }

    i = 0;
    while (i < length)
    {
        if (!xstr[i])
        {
            *n_items = *n_items + 1;
        }
        i++;
    }
    if (xstr[length - 1])
    {
        *n_items = *n_items + 1;
    }

    retval = g_new0 (gchar *, *n_items + 1);
    ptr = xstr;

    for (i = 0; i < *n_items; i++)
    {
        if (g_utf8_validate (ptr, -1, NULL))
        {
            retval[i] = internal_utf8_strndup (ptr, MAX_STR_LENGTH);
            xfce_utf8_remove_controls((gchar *) retval[i], -1, NULL);
        }
        else
        {
            TRACE ("getUTF8StringList() returned invalid UTF-8 characters in string #%i.", i);
            retval[i] = g_strdup ("");
        }
        ptr += strlen (ptr) + 1;
    }
    XFree (xstr);
    *str_p = retval;

    return TRUE;
}

gboolean
getWindowProp (DisplayInfo *display_info, Window window, int atom_id, Window *w)
{
    Atom type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *prop;
    int result, status;

    g_return_val_if_fail (window != None, FALSE);
    g_return_val_if_fail (((atom_id >= 0) && (atom_id < ATOM_COUNT)), FALSE);
    TRACE ("window 0x%lx atom id %i", window, atom_id);

    *w = None;
    prop = NULL;

    myDisplayErrorTrapPush (display_info);
    status = XGetWindowProperty (display_info->dpy, window, display_info->atoms[atom_id],
                                 0L, 1L, FALSE, XA_WINDOW, &type, &format, &nitems,
                                 &bytes_after, (unsigned char **) &prop);
    result = myDisplayErrorTrapPop (display_info);

    if ((status == Success) && (result == Success))
    {
        if (prop)
        {
            *w = *((Window *) prop);
            XFree (prop);
        }
        if (!prop || !check_type_and_format (32, XA_WINDOW, -1, format, type))
        {
            *w = None;
            return FALSE;
        }
    }

    return (result == Success);
}

gboolean
getWindowHostname (DisplayInfo *display_info, Window w, gchar **machine)
{
    TRACE ("window 0x%lx", w);

    g_return_val_if_fail (machine != NULL, FALSE);
    g_return_val_if_fail (w != None, FALSE);
    g_return_val_if_fail (display_info != NULL, FALSE);

    *machine = getTextProperty (display_info, w, display_info->atoms[WM_CLIENT_MACHINE]);
    if (*machine == NULL)
    {
        *machine = g_strdup ("");
        return FALSE;
    }

    return TRUE;
}

gboolean
getWindowName (DisplayInfo *display_info, Window w, gchar **name)
{
    char *str;
    guint len;

    TRACE ("window 0x%lx", w);

    g_return_val_if_fail (name != NULL, FALSE);
    *name = NULL;
    g_return_val_if_fail (w != None, FALSE);

    if (getUTF8StringData (display_info, w, NET_WM_NAME, &str, &len))
    {
        *name = internal_utf8_strndup (str, MAX_STR_LENGTH);
        xfce_utf8_remove_controls(*name, -1, NULL);
        XFree (str);
        return TRUE;
    }

    *name = getTextProperty (display_info, w, XA_WM_NAME);
    if (*name == NULL)
    {
        *name = g_strdup ("");
        return FALSE;
    }

    xfce_utf8_remove_controls(*name, -1, NULL);

    return TRUE;
}

gboolean
getWindowRole (DisplayInfo *display_info, Window window, gchar **role)
{
    g_return_val_if_fail (role != NULL, FALSE);
    g_return_val_if_fail (window != None, FALSE);
    TRACE ("window 0x%lx", window);

    *role = getTextProperty (display_info, window, display_info->atoms[WM_WINDOW_ROLE]);

    return (*role != NULL);
}

Window
getClientLeader (DisplayInfo *display_info, Window window)
{
    Window client_leader;

    g_return_val_if_fail (window != None, None);
    TRACE ("window 0x%lx", window);

    client_leader = None;
    getWindowProp (display_info, window, WM_CLIENT_LEADER, &client_leader);

    return client_leader;
}

gboolean
getNetWMUserTime (DisplayInfo *display_info, Window window, guint32 *timestamp)
{
    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *data = NULL;
    int result, status;

    g_return_val_if_fail (window != None, FALSE);
    TRACE ("window 0x%lx", window);

    myDisplayErrorTrapPush (display_info);
    status = XGetWindowProperty (display_info->dpy, window,
                                 display_info->atoms[NET_WM_USER_TIME],
                                 0L, 1L, FALSE, XA_CARDINAL, &actual_type,
                                 &actual_format, &nitems, &bytes_after,
                                 (unsigned char **) &data);
    result = myDisplayErrorTrapPop (display_info);

    if ((status == Success) &&
        (result == Success) &&
        (data != NULL) &&
        (actual_type == XA_CARDINAL) &&
        (nitems == 1) &&
        (bytes_after == 0))
    {
        *timestamp = *((guint32 *) data);
        XFree (data);
        return TRUE;
    }

    XFree (data);
    *timestamp = (guint32) CurrentTime;

    return FALSE;
}

Window
getNetWMUserTimeWindow (DisplayInfo *display_info, Window window)
{
    Window user_time_win;

    g_return_val_if_fail (window != None, None);
    TRACE ("window 0x%lx", window);

    user_time_win = None;
    if (getWindowProp (display_info, window, NET_WM_USER_TIME_WINDOW, &user_time_win))
    {
        return user_time_win;
    }
    return window;
}

gboolean
getClientID (DisplayInfo *display_info, Window window, gchar **client_id)
{
    Window id;

    g_return_val_if_fail (client_id != NULL, FALSE);
    *client_id = NULL;
    g_return_val_if_fail (window != None, FALSE);
    TRACE ("window 0x%lx", window);

    if (getWindowProp (display_info, window, WM_CLIENT_LEADER, &id) && (id != None))
    {
        *client_id = getTextProperty (display_info, id, display_info->atoms[SM_CLIENT_ID]);
    }

    return (*client_id != NULL);
}

gboolean
getWindowCommand (DisplayInfo *display_info, Window window, char ***argv, int *argc)
{
    Window id;

    *argc = 0;
    g_return_val_if_fail (window != None, FALSE);
    TRACE ("window 0x%lx", window);

    if (XGetCommand (display_info->dpy, window, argv, argc) && (*argc > 0))
    {
        return TRUE;
    }
    if (getWindowProp (display_info, window, WM_CLIENT_LEADER, &id) && (id != None))
    {
        if (XGetCommand (display_info->dpy, id, argv, argc) && (*argc > 0))
        {
            return TRUE;
        }
    }
    return FALSE;
}

gboolean
getKDEIcon (DisplayInfo *display_info, Window window, Pixmap * pixmap, Pixmap * mask)
{
    Atom type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *data;
    Pixmap *icons;
    int result, status;

    TRACE ("window 0x%lx", window);

    *pixmap = None;
    *mask = None;
    icons = NULL;
    data = NULL;

    myDisplayErrorTrapPush (display_info);
    status = XGetWindowProperty (display_info->dpy, window,
                                 display_info->atoms[KWM_WIN_ICON],
                                 0L, G_MAXLONG, FALSE,
                                 display_info->atoms[KWM_WIN_ICON],
                                 &type, &format, &nitems, &bytes_after,
                                 (unsigned char **) &data);
    result = myDisplayErrorTrapPop (display_info);

    if ((status != Success) ||
        (result != Success) ||
        (data == NULL) ||
        (type != display_info->atoms[KWM_WIN_ICON]))
    {
        XFree (data);
        return FALSE;
    }

    icons = (Pixmap *) data;
    *pixmap = icons[0];
    *mask = icons[1];

    XFree (icons);

    return TRUE;
}

gboolean
getRGBIconData (DisplayInfo *display_info, Window window, unsigned long **data, unsigned long *nitems)
{
    Atom type;
    int format;
    unsigned long bytes_after;
    int result, status;

    TRACE ("window 0x%lx", window);

    *data = NULL;
    type = None;

    myDisplayErrorTrapPush (display_info);
    status = XGetWindowProperty (display_info->dpy, window,
                                 display_info->atoms[NET_WM_ICON],
                                 0L, G_MAXLONG, FALSE, XA_CARDINAL,
                                 &type, &format, nitems, &bytes_after,
                                 (unsigned char **) data);
    result = myDisplayErrorTrapPop (display_info);

    if ((status != Success) ||
        (result != Success) ||
        (type != XA_CARDINAL))
    {
        XFree (*data);
        *data = NULL;
        return FALSE;
    }

    return (data != NULL);
}

gboolean
getOpacity (DisplayInfo *display_info, Window window, guint32 *opacity)
{
    long val;

    g_return_val_if_fail (window != None, FALSE);
    g_return_val_if_fail (opacity != NULL, FALSE);
    TRACE ("window 0x%lx", window);

    val = 0;
    if (getHint (display_info, window, NET_WM_WINDOW_OPACITY, &val))
    {
        *opacity = (guint32) val;
        return TRUE;
    }

    return FALSE;
}

gboolean
getBypassCompositor (DisplayInfo *display_info, Window window, guint32 *bypass)
{
    long val;

    g_return_val_if_fail (window != None, FALSE);
    g_return_val_if_fail (bypass != NULL, FALSE);
    TRACE ("window 0x%lx", window);

    val = 0;
    if (getHint (display_info, window, NET_WM_BYPASS_COMPOSITOR, &val))
    {
        *bypass = (guint32) val;
        return TRUE;
    }

    return FALSE;
}

gboolean
getOpacityLock (DisplayInfo *display_info, Window window)
{
    long val;

    g_return_val_if_fail (window != None, FALSE);
    TRACE ("window 0x%lx", window);

    /* only presence/absence matters */
    return !!getHint (display_info, window, NET_WM_WINDOW_OPACITY_LOCKED, &val);
}

gboolean
setXAtomManagerOwner (DisplayInfo *display_info, Atom atom, Window root, Window w)
{
    XClientMessageEvent ev;
    guint32 server_time;
    int status;

    g_return_val_if_fail (root != None, FALSE);
    TRACE ("window 0x%lx", w);

    server_time = myDisplayGetCurrentTime (display_info);
    status = XSetSelectionOwner (display_info->dpy, atom, w, server_time);

    if ((status == BadAtom) || (status == BadWindow))
    {
        return FALSE;
    }

    if (XGetSelectionOwner (display_info->dpy, atom) == w)
    {
        ev.type = ClientMessage;
        ev.message_type = atom;
        ev.format = 32;
        ev.data.l[0] = (long) server_time;
        ev.data.l[1] = (long) atom;
        ev.data.l[2] = (long) w;
        ev.data.l[3] = (long) 0L;
        ev.data.l[4] = (long) 0L;
        ev.window = root;

        XSendEvent (display_info->dpy, root, FALSE, StructureNotifyMask, (XEvent *) &ev);

        return TRUE;
    }

    return FALSE;
}

gboolean
setAtomIdManagerOwner (DisplayInfo *display_info, int atom_id, Window root, Window w)
{
    g_return_val_if_fail (((atom_id >= 0) && (atom_id < ATOM_COUNT)), FALSE);
    TRACE ("atom %i", atom_id);

    return setXAtomManagerOwner(display_info, display_info->atoms[atom_id], root, w);
}

void
updateXserverTime (DisplayInfo *display_info)
{
    char c = '\0';

    g_return_if_fail (display_info);

    XChangeProperty (display_info->dpy, display_info->timestamp_win,
                     display_info->atoms[XFWM4_TIMESTAMP_PROP],
                     display_info->atoms[XFWM4_TIMESTAMP_PROP],
                     8, PropModeReplace, (unsigned char *) &c, 1);
}

guint32
getXServerTime (DisplayInfo *display_info)
{
    ScreenInfo *screen_info;
    XEvent xevent;
    XfwmEvent *event;
    guint32 timestamp;

    g_return_val_if_fail (display_info, CurrentTime);
    timestamp = myDisplayGetCurrentTime (display_info);
    if (timestamp == CurrentTime)
    {
        screen_info = myDisplayGetDefaultScreen (display_info);
        g_return_val_if_fail (screen_info,  CurrentTime);

        TRACE ("using X server roundtrip");
        updateXserverTime (display_info);
        XWindowEvent (display_info->dpy, display_info->timestamp_win, PropertyChangeMask, &xevent);
        event = xfwm_device_translate_event (display_info->devices, &xevent, NULL);
        timestamp = myDisplayUpdateCurrentTime (display_info, event);
        xfwm_device_free_event (event);
    }

    TRACE ("timestamp=%u", (guint32) timestamp);
    return timestamp;
}

#ifdef ENABLE_KDE_SYSTRAY_PROXY
gboolean
checkKdeSystrayWindow (DisplayInfo *display_info, Window window)
{
    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *data;
    Window trayIconForWindow;
    int result, status;

    g_return_val_if_fail (window != None, FALSE);
    TRACE ("window 0x%lx", window);

    trayIconForWindow = None;
    data = NULL;

    myDisplayErrorTrapPush (display_info);
    status = XGetWindowProperty (display_info->dpy, window,
                                 display_info->atoms[KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR],
                                 0L, sizeof(Window), FALSE, XA_WINDOW, &actual_type,
                                 &actual_format, &nitems, &bytes_after,
                                 (unsigned char **) &data);
    result = myDisplayErrorTrapPop (display_info);

    if ((status != Success) || (result != Success))
    {
        XFree (data);
        return FALSE;
    }

    if (data)
    {
        trayIconForWindow = *((Window *) data);
        XFree (data);
    }

    if ((actual_format == None) || (actual_type != XA_WINDOW) || (trayIconForWindow == None))
    {
        return FALSE;
    }

    return TRUE;
}

void
sendSystrayReqDock(DisplayInfo *display_info, Window window, Window systray)
{
    XClientMessageEvent xev;

    g_return_if_fail (window != None);
    g_return_if_fail (systray != None);
    TRACE ("window 0x%lx", window);

    xev.type = ClientMessage;
    xev.window = systray;
    xev.message_type = display_info->atoms[NET_SYSTEM_TRAY_OPCODE];
    xev.format = 32;
    xev.data.l[0] = (long) myDisplayGetCurrentTime (display_info);
    xev.data.l[1] = (long) 0L; /* SYSTEM_TRAY_REQUEST_DOCK */
    xev.data.l[2] = (long) window;
    xev.data.l[3] = (long) 0L; /* Nada */
    xev.data.l[4] = (long) 0L; /* Niet */

    XSendEvent (display_info->dpy, systray, FALSE, NoEventMask, (XEvent *) & xev);
}

Window
getSystrayWindow (DisplayInfo *display_info, Atom net_system_tray_selection)
{
    Window systray_win;

    TRACE ("entering");

    myDisplayErrorTrapPush (display_info);
    systray_win = XGetSelectionOwner (display_info->dpy, net_system_tray_selection);
    if (systray_win)
    {
        XSelectInput (display_info->dpy, systray_win, StructureNotifyMask);
    }
    myDisplayErrorTrapPopIgnored (display_info);
    TRACE ("new systray window:  0x%lx", systray_win);

    return systray_win;
}
#endif

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
gboolean
getWindowStartupId (DisplayInfo *display_info, Window w, gchar **startup_id)
{
    char *str;
    guint len;

    g_return_val_if_fail (startup_id != NULL, FALSE);
    *startup_id = NULL;
    g_return_val_if_fail (w != None, FALSE);
    TRACE ("window 0x%lx", w);

    if (getUTF8StringData (display_info, w, NET_STARTUP_ID, &str, &len))
    {
        *startup_id = g_strdup (str);
        XFree (str);
        return TRUE;
    }

    *startup_id = getTextProperty (display_info, w, display_info->atoms[NET_STARTUP_ID]);

    return (*startup_id != NULL);
}
#endif

GPid
getWindowPID (DisplayInfo *display_info, Window w)
{
    long pid = 0;
#ifdef HAVE_XRES
    XResClientIdSpec client_specs;
    XResClientIdValue *client_ids = NULL;
    int i;
    int result;
    long num_ids;

    if (display_info->have_xres)
    {
        client_specs.client = w;
        client_specs.mask = XRES_CLIENT_ID_PID_MASK;

        myDisplayErrorTrapPush (display_info);

        XResQueryClientIds (display_info->dpy, 1, &client_specs, &num_ids, &client_ids);

        result = myDisplayErrorTrapPop (display_info);

        if (result == Success)
        {
            for (i = 0; i < num_ids; i++)
            {
                if (client_ids[i].spec.mask == XRES_CLIENT_ID_PID_MASK)
                {
                    CARD32 *value = client_ids[i].value;
                    pid = (long) *value;
                    break;
                }
            }

            XFree(client_ids);

            if (pid > 0)
            {
                return (GPid) pid;
            }
        }
    }
#endif /* HAVE_XRES */

    getHint (display_info, w, NET_WM_PID, (long *) &pid);

    return (GPid) pid;
}

unsigned int
getOpaqueRegionRects (DisplayInfo *display_info, Window w, XRectangle **p_rects)
{
    gulong *data;
    XRectangle *rects;
    int i, nitems, nrects;

    TRACE ("window 0x%lx", w);
    if (getCardinalList (display_info, w, NET_WM_OPAQUE_REGION, &data, &nitems))
    {
        if (nitems % 4)
        {
            if (data)
            {
                XFree (data);
            }

            return 0;
        }

        rects = g_new0 (XRectangle, nitems / 4);
        nrects = 0;
        i = 0;

        while (i < nitems)
        {
            XRectangle *rect = &rects[nrects++];

            rect->x = data[i++];
            rect->y = data[i++];
            rect->width = data[i++];
            rect->height = data[i++];
        }

        XFree (data);
        *p_rects = rects;
        return (unsigned int) nrects;
    }

    return 0;
}
