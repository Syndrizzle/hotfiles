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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <libxfce4util/libxfce4util.h>

#include "display.h"
#include "screen.h"
#include "mywindow.h"
#include "client.h"
#include "misc.h"

guint
getMouseXY (ScreenInfo *screen_info, gint *x2, gint *y2)
{
    Window w1, w2;
    guint mask;
    gint x1, y1;

    TRACE ("entering");

    myDisplayErrorTrapPush (screen_info->display_info);
    XQueryPointer (myScreenGetXDisplay (screen_info), screen_info->xroot, &w1, &w2, &x1, &y1, x2, y2, &mask);
    myDisplayErrorTrapPopIgnored (screen_info->display_info);

    return mask;
}

GC
createGC (ScreenInfo *screen_info, char *col, int func, XFontStruct * font,
          int line_width, gboolean inc_sw)
{
    XGCValues gv;
    XColor xc1, xc2;
    GC gc;
    int mask;

    TRACE ("color=%s", col);

    mask = GCForeground | GCFunction;
    XAllocNamedColor (myScreenGetXDisplay (screen_info), screen_info->cmap, col, &xc1, &xc2);
    gv.foreground = xc2.pixel;
    gv.function = func;
    if (font)
    {
        gv.font = font->fid;
        mask = mask | GCFont;
    }
    if (inc_sw)
    {
        gv.subwindow_mode = IncludeInferiors;
        mask = mask | GCSubwindowMode;
    }
    if (line_width > -1)
    {
        gv.line_width = line_width;
        mask = mask | GCLineWidth;
    }
    gc = XCreateGC (myScreenGetXDisplay (screen_info), screen_info->xroot, mask, &gv);
    return gc;
}

void
sendClientMessage (ScreenInfo *screen_info, Window w, int atom_id, guint32 timestamp)
{
    DisplayInfo *display_info;
    XClientMessageEvent ev;

    g_return_if_fail ((atom_id > 0) && (atom_id < ATOM_COUNT));
    TRACE ("atom %i, timestamp %u", atom_id, (unsigned int) timestamp);

    display_info = screen_info->display_info;
    ev.type = ClientMessage;
    ev.window = w;
    ev.message_type = display_info->atoms[WM_PROTOCOLS];
    ev.format = 32;
    ev.send_event = TRUE;
    ev.data.l[0] = display_info->atoms[atom_id];
    ev.data.l[1] = timestamp;
    myDisplayErrorTrapPush (screen_info->display_info);
    XSendEvent (myScreenGetXDisplay (screen_info), w, FALSE, 0L, (XEvent *)&ev);
    myDisplayErrorTrapPopIgnored (screen_info->display_info);
}

void
sendRootMessage (ScreenInfo *screen_info, int atom_id, long value, guint32 timestamp)
{
    DisplayInfo *display_info;
    XClientMessageEvent ev;

    g_return_if_fail ((atom_id > 0) && (atom_id < ATOM_COUNT));
    TRACE ("atom %i, timestamp %u", atom_id, (unsigned int) timestamp);

    display_info = screen_info->display_info;
    ev.type = ClientMessage;
    ev.window = screen_info->xroot;
    ev.message_type = display_info->atoms[atom_id];
    ev.format = 32;
    ev.data.l[0] = value;
    ev.data.l[1] = timestamp;
    XSendEvent (myScreenGetXDisplay (screen_info), screen_info->xroot, FALSE,
                SubstructureRedirectMask | SubstructureNotifyMask, (XEvent *)&ev);
}

/*
 * it's safer to grab the display before calling this routine
 * Returns true if the given window is present and mapped on root
 */
gboolean
checkWindowOnRoot(ScreenInfo *screen_info, Window w)
{
    DisplayInfo *display_info;
    Window dummy_root, parent;
    Window *wins;
    Status test;
    unsigned int count;
    gint ret;

    g_return_val_if_fail (screen_info != NULL, FALSE);
    g_return_val_if_fail (w != None, FALSE);
    TRACE ("window 0x%lx", w);

    display_info = screen_info->display_info;
    wins = NULL;

    myDisplayErrorTrapPush (display_info);
    test = XQueryTree(display_info->dpy, w, &dummy_root, &parent, &wins, &count);
    if (wins)
    {
        XFree (wins);
    }
    ret = myDisplayErrorTrapPop (display_info);

    return ((ret == 0) && (test != 0) && (dummy_root == parent));
}

void
placeSidewalks(ScreenInfo *screen_info, gboolean activate)
{
    NetWmDesktopLayout l;

    g_return_if_fail (MYWINDOW_XWINDOW (screen_info->sidewalk[0]) != None);
    g_return_if_fail (MYWINDOW_XWINDOW (screen_info->sidewalk[1]) != None);
    g_return_if_fail (MYWINDOW_XWINDOW (screen_info->sidewalk[2]) != None);
    g_return_if_fail (MYWINDOW_XWINDOW (screen_info->sidewalk[3]) != None);
    TRACE ("entering");

    l = screen_info->desktop_layout;
    if ((activate) && (l.cols > 1))
    {
        /*left*/
        xfwmWindowShow (&screen_info->sidewalk[0],
                        0, 0,
                        1, screen_info->height, FALSE);

        /*right*/
         xfwmWindowShow (&screen_info->sidewalk[1],
                        screen_info->width - 1, 0,
                        1, screen_info->height, FALSE);
    }
    else /* Place the windows off screen */
    {
        /*left*/
        xfwmWindowShow (&screen_info->sidewalk[0],
                        -1, 0,
                        1, screen_info->height, FALSE);

        /*right*/
        xfwmWindowShow (&screen_info->sidewalk[1],
                        screen_info->width, 0,
                        1, screen_info->height, FALSE);
    }

    if ((activate) && (l.rows > 1))
    {
        /*top*/
        xfwmWindowShow (&screen_info->sidewalk[2],
                        0, 0,
                        screen_info->width, 1, FALSE);

        /*bottom*/
        xfwmWindowShow (&screen_info->sidewalk[3],
                        0, screen_info->height - 1,
                        screen_info->width, 1, FALSE);
    }
    else /* Place the windows off screen */
    {
        /*top*/
        xfwmWindowShow (&screen_info->sidewalk[2],
                        0, -1,
                        screen_info->width, 1, FALSE);

        /*bottom*/
        xfwmWindowShow (&screen_info->sidewalk[3],
                        0, screen_info->height,
                        screen_info->width, 1, FALSE);
    }
}

gchar*
get_atom_name (DisplayInfo *display_info, Atom atom)
{
    gchar *value;
    gchar *xname;

    if (atom)
    {
        xname = (gchar *) XGetAtomName (display_info->dpy, atom);
        if (xname)
        {
            value = g_strdup (xname);
            XFree ((char *) xname);
        }
        else
        {
            value = g_strdup ("Unknown");
        }
    }
    else
    {
        value = g_strdup ("None");
    }

    return value;
}

