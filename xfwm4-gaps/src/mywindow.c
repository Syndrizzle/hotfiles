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


        xfwm4    - (c) 2002-2011 Olivier Fourdan

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <libxfce4util/libxfce4util.h>
#include "mypixmap.h"
#include "mywindow.h"
#include "screen.h"

static void
xfwmWindowSetVisual (xfwmWindow * win, Visual *visual, gint depth)
{
    g_return_if_fail (win->screen_info != NULL);

    if (visual)
    {
        win->visual = visual;
    }
    else
    {
        win->visual = win->screen_info->visual;
    }

    if (depth)
    {
        win->depth = depth;
    }
    else
    {
        win->depth = win->screen_info->depth;
    }
}

void
xfwmWindowInit (xfwmWindow * win)
{
    g_return_if_fail (win != NULL);

    win->window = None;
    win->map = FALSE;
    win->screen_info = NULL;
    win->depth = 0;
    win->x = 0;
    win->y = 0;
    win->width = 1;
    win->height = 1;
#ifdef HAVE_RENDER
    win->pict_format = NULL;
#endif
}

void
xfwmWindowSetCursor (xfwmWindow * win, Cursor cursor)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (win != NULL);

    screen_info = win->screen_info;
    display_info = screen_info->display_info;

    if ((win->window != None) && (cursor != None))
    {
        myDisplayErrorTrapPush (display_info);
        XDefineCursor (myScreenGetXDisplay (screen_info), win->window, cursor);
        myDisplayErrorTrapPopIgnored (display_info);
    }
}

void
xfwmWindowCreate (ScreenInfo * screen_info, Visual *visual, gint depth, Window parent,
                  xfwmWindow * win, long eventmask, Cursor cursor)
{
    XSetWindowAttributes attributes;
    unsigned long valuemask;

    TRACE ("parent (0x%lx)", parent);

    g_return_if_fail (screen_info != NULL);

    attributes.event_mask = eventmask;
    valuemask = 0L;
    if (eventmask != NoEventMask)
    {
        valuemask |= CWEventMask;
    }

    win->window = XCreateWindow (myScreenGetXDisplay (screen_info),
                                 parent, 0, 0, 1, 1, 0, 0,
                                 InputOutput, CopyFromParent,
                                 valuemask, &attributes);

    TRACE ("new XID (0x%lx)", win->window);

    win->map = FALSE;
    win->screen_info = screen_info;
    win->x = 0;
    win->y = 0;
    win->width = 1;
    win->height = 1;
    xfwmWindowSetVisual (win, visual, depth);
    xfwmWindowSetCursor (win, cursor);
#ifdef HAVE_RENDER
    win->pict_format = XRenderFindVisualFormat (myScreenGetXDisplay (screen_info), win->visual);
#endif
#ifdef HAVE_XI2
    xfwm_device_configure_xi2_event_mask (screen_info->display_info->devices,
                                          screen_info->display_info->dpy,
                                          win->window, eventmask);
#endif
}

void
xfwmWindowDelete (xfwmWindow * win)
{
    TRACE ("win %p (0x%lx)", win, win->window);

    if (win->window != None)
    {
        XDestroyWindow (myScreenGetXDisplay (win->screen_info),
                        win->window);
        win->window = None;
    }
    win->map = FALSE;
}

void
xfwmWindowShow (xfwmWindow * win, int x, int y, int width, int height,
    gboolean refresh)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    TRACE ("win %p (0x%lx) at (%i,%i) [%i×%i]", win, win->window, x, y, width, height);

    if (!(win->window))
    {
        return;
    }
    if ((width < 1) || (height < 1))
    {
        xfwmWindowHide (win);
        return;
    }

    screen_info = win->screen_info;
    display_info = screen_info->display_info;
    myDisplayErrorTrapPush (display_info);

    if (!(win->map))
    {
        XMapWindow (myScreenGetXDisplay (screen_info),
                    win->window);
        win->map = TRUE;
    }

    if (((x != win->x) || (y != win->y)) && ((width != win->width)
            || (height != win->height)))
    {
        XMoveResizeWindow (myScreenGetXDisplay (screen_info),
                           win->window, x, y,
                           (unsigned int) width,
                           (unsigned int) height);
        win->x = x;
        win->y = y;
        win->width = width;
        win->height = height;
    }
    else if ((x != win->x) || (y != win->y))
    {
        XMoveWindow (myScreenGetXDisplay (screen_info),
                     win->window,
                     x, y);
        if (refresh)
        {
            XClearWindow (myScreenGetXDisplay (screen_info),
                          win->window);
        }
        win->x = x;
        win->y = y;
    }
    else if ((width != win->width) || (height != win->height))
    {
        XResizeWindow (myScreenGetXDisplay (screen_info),
                       win->window,
                       (unsigned int) width,
                       (unsigned int) height);
        win->width = width;
        win->height = height;
    }
    else if (refresh)
    {
        XClearWindow (myScreenGetXDisplay (screen_info),
                      win->window);
    }
    myDisplayErrorTrapPopIgnored (display_info);
}

void
xfwmWindowHide (xfwmWindow * win)
{
    TRACE ("win %p (0x%lx)", win, win->window);

    if (win->map)
    {
        g_assert (win->window);
        XUnmapWindow (myScreenGetXDisplay (win->screen_info), win->window);
        win->map = FALSE;
    }
}

gboolean
xfwmWindowVisible (xfwmWindow *win)
{
    g_return_val_if_fail (win, FALSE);

    return win->map;
}

gboolean
xfwmWindowDeleted (xfwmWindow *win)
{
    g_return_val_if_fail (win, TRUE);

    return (win->window == None);
}

void
xfwmWindowTemp (ScreenInfo *screen_info, Visual *visual,
                gint depth, Window parent,
                xfwmWindow * win,
                int x, int y, int width, int height,
                long eventmask,
                gboolean bottom)
{
    XSetWindowAttributes attributes;
    DisplayInfo *display_info;

    display_info = screen_info->display_info;
    myDisplayErrorTrapPush (display_info);

    attributes.event_mask = eventmask;
    attributes.override_redirect = TRUE;
    win->window = XCreateWindow (myScreenGetXDisplay (screen_info),
                                 parent, x, y, width, height, 0, 0,
                                 InputOnly, CopyFromParent,
                                 CWEventMask | CWOverrideRedirect,
                                 &attributes);
    if (bottom)
    {
        XLowerWindow (myScreenGetXDisplay (screen_info), win->window);
    }
    else
    {
        XRaiseWindow (myScreenGetXDisplay (screen_info), win->window);
    }

    XMapWindow (myScreenGetXDisplay (screen_info), win->window);
    win->map = TRUE;
    win->screen_info = screen_info;
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    xfwmWindowSetVisual (win, visual, depth);
#ifdef HAVE_XI2
    xfwm_device_configure_xi2_event_mask (screen_info->display_info->devices,
                                          screen_info->display_info->dpy,
                                          win->window, eventmask);
#endif
    myDisplayErrorTrapPopIgnored (display_info);
}

#ifdef HAVE_RENDER
static gboolean
xfwmWindowCopyComposite (xfwmWindow * win, xfwmPixmap * pix)
{
    if (myDisplayHaveRender (win->screen_info->display_info))
    {
        Picture pict;
        Pixmap temp;

        if (!pix->pict)
        {
            TRACE ("pixmap picture does not exist");
            return FALSE;
        }

        if (!win->pict_format)
        {
            TRACE ("window picture format is unknown");
            return FALSE;
        }

        temp = XCreatePixmap (myScreenGetXDisplay (win->screen_info),
                              win->window,
                              pix->width, pix->height, win->depth);

        if (!temp)
        {
            return FALSE;
        }

        pict = XRenderCreatePicture (myScreenGetXDisplay (win->screen_info),
                                     temp, win->pict_format, 0, NULL);

        if (!pict)
        {
            XFreePixmap (myScreenGetXDisplay (win->screen_info), temp);
            return FALSE;
        }

        XRenderComposite (myScreenGetXDisplay (win->screen_info), PictOpSrc, pix->pict, None, pict, 0, 0, 0, 0, 0, 0, pix->width, pix->height);

        XRenderFreePicture (myScreenGetXDisplay (win->screen_info), pict);

        XSetWindowBackgroundPixmap (myScreenGetXDisplay (win->screen_info), win->window, temp);

        XFreePixmap (myScreenGetXDisplay (win->screen_info), temp);
        return TRUE;
    }
    return FALSE;
}
#endif

void
xfwmWindowSetBG (xfwmWindow * win, xfwmPixmap * pix)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    gboolean done;

    if ((win->width < 1) || (win->height < 1) || (pix->width < 1) || (pix->height < 1))
    {
        return;
    }

    screen_info = win->screen_info;
    display_info = screen_info->display_info;
    myDisplayErrorTrapPush (display_info);

    done = FALSE;
#ifdef HAVE_RENDER
    if ((win->visual != screen_info->visual) ||
        (win->depth  != screen_info->depth))
    {
        /* Try to use Render */
        done = xfwmWindowCopyComposite (win, pix);
    }
#endif

    if (!done)
    {
        /* Use the good old way */
        XSetWindowBackgroundPixmap (myScreenGetXDisplay (screen_info), win->window, pix->pixmap);
    }

    myDisplayErrorTrapPopIgnored (display_info);
}
