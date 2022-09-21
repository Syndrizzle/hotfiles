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
        xfwm4    - (c) 2002-2020 Olivier Fourdan

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include <common/xfwm-common.h>

#include "client.h"
#include "compositor.h"
#include "focus.h"
#include "frame.h"
#include "hints.h"
#include "icons.h"
#include "misc.h"
#include "moveresize.h"
#include "mypixmap.h"
#include "mywindow.h"
#include "netwm.h"
#include "placement.h"
#include "screen.h"
#include "session.h"
#include "settings.h"
#include "stacking.h"
#include "startup_notification.h"
#include "transients.h"
#include "workspaces.h"
#include "xsync.h"
#include "event_filter.h"

/* Event mask definition */

#define POINTER_EVENT_MASK \
    ButtonPressMask|\
    ButtonReleaseMask

#define FRAME_EVENT_MASK \
    SubstructureNotifyMask|\
    SubstructureRedirectMask|\
    PointerMotionMask|\
    ButtonMotionMask|\
    FocusChangeMask|\
    EnterWindowMask|\
    PropertyChangeMask

#define CLIENT_EVENT_MASK \
    StructureNotifyMask|\
    FocusChangeMask|\
    PropertyChangeMask

#define BUTTON_EVENT_MASK \
    EnterWindowMask|\
    LeaveWindowMask

/* Useful macros */
#define START_ICONIC(c) \
    ((c->wmhints) && \
     (c->wmhints->initial_state == IconicState) && \
     !clientIsTransientOrModal (c))

#define OPACITY_SET_STEP        (guint) 0x16000000
#define OPACITY_SET_MIN         (guint) 0x40000000

typedef struct _ButtonPressData ButtonPressData;
struct _ButtonPressData
{
    int b;
    Client *c;
};

/* Forward decl */
static void
clientUpdateIconPix (Client *c);
static gboolean
clientNewMaxSize (Client *c, XWindowChanges *wc, GdkRectangle *);

Display *
clientGetXDisplay (Client *c)
{
    g_return_val_if_fail (c, NULL);

    return myScreenGetXDisplay (c->screen_info);
}

void
clientInstallColormaps (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    XWindowAttributes attr;
    gboolean installed;
    int i;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    myDisplayErrorTrapPush (display_info);

    installed = FALSE;
    if (c->ncmap)
    {
        for (i = c->ncmap - 1; i >= 0; i--)
        {
            XGetWindowAttributes (display_info->dpy, c->cmap_windows[i], &attr);
            XInstallColormap (display_info->dpy, attr.colormap);
            if (c->cmap_windows[i] == c->window)
            {
                installed = TRUE;
            }
        }
    }
    if ((!installed) && (c->cmap))
    {
        XInstallColormap (display_info->dpy, c->cmap);
    }

    myDisplayErrorTrapPopIgnored (display_info);
}

void
clientUpdateColormaps (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    int result, status;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (c->ncmap)
    {
        XFree (c->cmap_windows);
        c->ncmap = 0;
    }

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    myDisplayErrorTrapPush (display_info);
    status = XGetWMColormapWindows (display_info->dpy, c->window,
                                    &c->cmap_windows, &c->ncmap);
    result = myDisplayErrorTrapPop (display_info);
    if ((result != Success) || !status)
    {
        c->cmap_windows = NULL;
        c->ncmap = 0;
    }
}

static gchar*
clientCreateTitleName (Client *c, gchar *name, gchar *hostname)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    gchar *title;

    g_return_val_if_fail (c != NULL, NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if (strlen (hostname) && (display_info->hostname) && (g_ascii_strcasecmp (display_info->hostname, hostname)))
    {
        /* TRANSLATORS: "(on %s)" is like "running on" the name of the other host */
        title = g_strdup_printf (_("%s (on %s)"), name, hostname);
    }
    else
    {
        title = g_strdup (name);
    }

    return title;
}

void
clientUpdateName (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    gchar *hostname;
    gchar *wm_name;
    gchar *name;
    gboolean refresh;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    getWindowName (display_info, c->window, &wm_name);
    getWindowHostname (display_info, c->window, &hostname);
    refresh = FALSE;

    /* Update hostname too, as it's used when terminating a client */
    if (hostname)
    {
        if (c->hostname)
        {
            g_free (c->hostname);
        }
        c->hostname = hostname;
    }

    if (wm_name)
    {
        name = clientCreateTitleName (c, wm_name, hostname);
        g_free (wm_name);
        if (c->name)
        {
            if (strcmp (name, c->name))
            {
                refresh = TRUE;
                FLAG_SET (c->flags, CLIENT_FLAG_NAME_CHANGED);
            }
            g_free (c->name);
        }
        c->name = name;
    }

    if (refresh)
    {
        frameQueueDraw (c, TRUE);
    }
}

static void
clientRecomputeMaximizeSize (Client *c)
{
    unsigned long maximization_flags = 0L;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    /* Recompute size and position of maximized windows */
    maximization_flags = c->flags & CLIENT_FLAG_MAXIMIZED;

    /* Force an update by clearing the internal flags */
    FLAG_UNSET (c->flags, CLIENT_FLAG_MAXIMIZED);
    clientToggleMaximized (c, maximization_flags, FALSE);
}

void
clientUpdateAllFrames (ScreenInfo *screen_info, int mask)
{
    Client *c;
    guint i;

    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    for (c = screen_info->clients, i = 0; i < screen_info->client_count; c = c->next, i++)
    {
        unsigned short configure_flags = 0;

        if (mask & UPDATE_BUTTON_GRABS)
        {
            clientUngrabButtons (c);
            clientGrabButtons (c);
            clientGrabMouseButton (c);
        }
        if (mask & UPDATE_CACHE)
        {
            clientUpdateIconPix (c);
        }
        if (mask & UPDATE_GRAVITY)
        {
            clientCoordGravitate (c, c->gravity, REMOVE, &c->x, &c->y);
            clientCoordGravitate (c, c->gravity, APPLY, &c->x, &c->y);
            setNetFrameExtents (screen_info->display_info,
                                c->window,
                                frameTop (c),
                                frameLeft (c),
                                frameRight (c),
                                frameBottom (c));
            configure_flags |= CFG_FORCE_REDRAW;
            mask &= ~UPDATE_FRAME;
        }
        if (mask & UPDATE_MAXIMIZE)
        {
            /* Recompute size and position of maximized windows */
            if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED))
            {
                clientRecomputeMaximizeSize (c);

                configure_flags |= CFG_FORCE_REDRAW;
                mask &= ~UPDATE_FRAME;
            }
        }
        if (configure_flags != 0L)
        {
            clientReconfigure (c, configure_flags);
        }
        if (mask & UPDATE_FRAME)
        {
            frameQueueDraw (c, TRUE);
        }

    }
}

void
clientGrabButtons (Client *c)
{
    ScreenInfo *screen_info;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    if (screen_info->params->easy_click)
    {
        grabButton (c->screen_info->display_info->devices, clientGetXDisplay (c),
                    AnyButton, screen_info->params->easy_click, c->window);
    }
}

void
clientUngrabButtons (Client *c)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);
    xfwm_device_ungrab_button (c->screen_info->display_info->devices, clientGetXDisplay (c),
                               AnyButton, AnyModifier, c->window);
}

static gboolean
urgent_cb (gpointer data)
{
    Client *c;
    ScreenInfo *screen_info;

    c = (Client *) data;
    g_return_val_if_fail (c != NULL, FALSE);
    TRACE ("iteration %i", c->blink_iterations);

    screen_info = c->screen_info;
    if (c != clientGetFocus ())
    {
        /*
         * If we do not blink on urgency, check if the window was last
         * drawn focused and redraw it unfocused.
         * This is for th case when the tuser changes the settings
         * in between two redraws.
         */
        if (!screen_info->params->urgent_blink)
        {
            if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_SEEN_ACTIVE))
            {
                FLAG_TOGGLE (c->xfwm_flags, XFWM_FLAG_SEEN_ACTIVE);
                frameQueueDraw (c, FALSE);
            }

            if (c->blink_iterations)
            {
                c->blink_iterations = 0;
            }
            return TRUE;
        }
        /*
         * If we blink on urgency, check if we've not reach the number
         * of iterations and if not, simply change the status and redraw
         */
        if (c->blink_iterations < (2 * MAX_BLINK_ITERATIONS))
        {
            c->blink_iterations++;
            FLAG_TOGGLE (c->xfwm_flags, XFWM_FLAG_SEEN_ACTIVE);
            frameQueueDraw (c, FALSE);
            return TRUE;
        }
        /*
         * If we reached the max number of iterations, check if we
         * repeat. If repeat_urgent_blink is set, redraw the frame and
         * restart counting from 1
         */
        if (screen_info->params->repeat_urgent_blink)
        {
            FLAG_TOGGLE (c->xfwm_flags, XFWM_FLAG_SEEN_ACTIVE);
            frameQueueDraw (c, FALSE);
            c->blink_iterations = 1;
            return TRUE;
        }
    }
    else if (c->blink_iterations)
    {
        c->blink_iterations = 0;
    }
    return TRUE;
}

void
clientUpdateUrgency (Client *c)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_SEEN_ACTIVE);
    if (c->blink_timeout_id)
    {
        g_source_remove (c->blink_timeout_id);
        frameQueueDraw (c, FALSE);
    }
    FLAG_UNSET (c->wm_flags, WM_FLAG_URGENT);

    c->blink_timeout_id = 0;
    c->blink_iterations = 0;
    if ((c->wmhints) && (c->wmhints->flags & XUrgencyHint))
    {
        FLAG_SET (c->wm_flags, WM_FLAG_URGENT);
        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE))
        {
            c->blink_timeout_id =
                g_timeout_add_full (G_PRIORITY_DEFAULT,
                                    CLIENT_BLINK_TIMEOUT,
                                    urgent_cb, c, NULL);
        }
    }
    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_SEEN_ACTIVE)
        && !FLAG_TEST (c->wm_flags, WM_FLAG_URGENT)
        && (c != clientGetFocus ()))
    {
        FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_SEEN_ACTIVE);
        frameQueueDraw (c, FALSE);
    }
}

/* ICCCM Section 4 - Client to Window Manager Communication
 *
 * The win_gravity may be any of the values specified for WINGRAVITY in the core
 * protocol  except  for  Unmap  :  NorthWest (1),  North  (2),  NorthEast  (3),
 * West (4), Center (5), East (6), SouthWest (7), South (8),  and SouthEast (9).
 * It specifies how  and whether the client  window wants to be shifted to  make
 * room for the window manager frame.
 *
 * If the win_gravity is Static , the window manager frame is positioned so that
 * the  inside  border of  the  client  window  inside the frame  is in the same
 * position  on the screen  as it was when the client  requested  the transition
 * from Withdrawn state.  Other values of win_gravity specify a window reference
 * point.  For NorthWest ,  NorthEast ,  SouthWest , and SouthEast the reference
 * point  is  the  specified outer  corner of the window  (on the outside border
 * edge).  For North , South , East , and West the reference point is the center
 * of the specified  outer edge of the window border.   For Center the reference
 * point is the center of the window.  The reference point of the window manager
 * frame is placed  at the location on the screen  where the reference point  of
 * the client window was when the client requested the transition from Withdrawn
 * state.
 */

void
clientCoordGravitate (Client *c, int gravity, int mode, int *x, int *y)
{
    int dx, dy;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    switch (gravity)
    {
        case CenterGravity:
            dx = (frameLeft (c) - frameRight (c) + 1) / 2;
            dy = (frameTop (c) - frameBottom (c) + 1) / 2;
            break;
        case NorthGravity:
            dx = (frameLeft (c) - frameRight (c) + 1) / 2;
            dy = frameTop (c);
            break;
        case SouthGravity:
            dx = (frameLeft (c) - frameRight (c) + 1) / 2;
            dy = - frameBottom (c);
            break;
        case EastGravity:
            dx = - frameRight (c);
            dy = (frameTop (c) - frameBottom (c) + 1) / 2;
            break;
        case WestGravity:
            dx = frameLeft (c);
            dy = (frameTop (c) - frameBottom (c) + 1) / 2;
            break;
        case NorthWestGravity:
            dx = frameLeft (c);
            dy = frameTop (c);
            break;
        case NorthEastGravity:
            dx = - frameRight (c);
            dy = frameTop (c);
            break;
        case SouthWestGravity:
            dx = frameLeft (c);
            dy = - frameBottom (c);
            break;
        case SouthEastGravity:
            dx = - frameRight (c);
            dy = - frameBottom (c);
            break;
        case StaticGravity:
        default:
            dx = 0;
            dy = 0;
            break;
    }
    *x = *x + (dx * mode);
    *y = *y + (dy * mode);
}

void
clientAdjustCoordGravity (Client *c, int gravity, XWindowChanges *wc, unsigned long *mask)
{
    int tx, ty, dw, dh;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    tx = wc->x;
    ty = wc->y;

    if (*mask & (CWX | CWY))
    {
        clientCoordGravitate (c, gravity, APPLY, &tx, &ty);
    }

    if (*mask & CWWidth)
    {
        wc->width = clientCheckWidth (c, wc->width, TRUE);
    }

    if (*mask & CWHeight)
    {
        wc->height = clientCheckHeight (c, wc->height, TRUE);
    }

    switch (gravity)
    {
        case CenterGravity:
            dw = (c->width - wc->width) / 2;
            dh = (c->height - wc->height) / 2;
            break;
        case NorthGravity:
            dw = (c->width - wc->width) / 2;
            dh = 0;
            break;
        case SouthGravity:
            dw = (c->width  - wc->width) / 2;
            dh = (c->height - wc->height);
            break;
        case EastGravity:
            dw = (c->width  - wc->width);
            dh = (c->height - wc->height) / 2;
            break;
        case WestGravity:
            dw = 0;
            dh = (c->height - wc->height) / 2;
            break;
        case NorthWestGravity:
            dw = 0;
            dh = 0;
            break;
        case NorthEastGravity:
            dw = (c->width - wc->width);
            dh = 0;
            break;
        case SouthWestGravity:
            dw = 0;
            dh = (c->height - wc->height);
            break;
        case SouthEastGravity:
            dw = (c->width  - wc->width);
            dh = (c->height - wc->height);
            break;
        case StaticGravity:
        default:
            dw = 0;
            dh = 0;
            break;
    }

    if (*mask & CWX)
    {
        wc->x = tx;
    }
    else if (*mask & CWWidth)
    {
        wc->x = c->x + dw;
        *mask |= CWX;
    }

    if (*mask & CWY)
    {
        wc->y = ty;
    }
    else if (*mask & CWHeight)
    {
        wc->y = c->y + dh;
        *mask |= CWY;
    }
}

static void
clientConfigureWindows (Client *c, XWindowChanges * wc, unsigned long mask, unsigned short flags)
{
    unsigned long change_mask_frame, change_mask_client;
    XWindowChanges change_values;
    DisplayInfo *display_info;
    ScreenInfo *screen_info;

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    change_mask_frame = mask & (CWX | CWY | CWWidth | CWHeight);
    change_mask_client = mask & (CWWidth | CWHeight);

    if ((mask & (CWWidth | CWHeight)) || (flags & CFG_FORCE_REDRAW))
    {
        frameDraw (c, (flags & CFG_FORCE_REDRAW));
    }

    if (flags & CFG_FORCE_REDRAW)
    {
        change_mask_client |= (CWX | CWY);
    }

    myDisplayErrorTrapPush (display_info);
    if (change_mask_frame & (CWX | CWY | CWWidth | CWHeight))
    {
        change_values.x = frameX (c);
        change_values.y = frameY (c);
        change_values.width = frameWidth (c);
        change_values.height = frameHeight (c);
        XConfigureWindow (display_info->dpy, c->frame, change_mask_frame, &change_values);
    }

    if (change_mask_client & (CWX | CWY | CWWidth | CWHeight))
    {
        change_values.x = frameLeft (c);
        change_values.y = frameTop (c);
        change_values.width = c->width;
        change_values.height = c->height;
        XConfigureWindow (display_info->dpy, c->window, change_mask_client, &change_values);
    }
    myDisplayErrorTrapPopIgnored (display_info);
}

void
clientSendConfigureNotify (Client *c)
{
    XConfigureEvent ce;
    DisplayInfo *display_info;
    ScreenInfo *screen_info;

    g_return_if_fail (c != NULL);
    g_return_if_fail (c->window != None);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    DBG ("Sending ConfigureNotify");
    ce.type = ConfigureNotify;
    ce.display = display_info->dpy;
    ce.send_event = TRUE;
    ce.event = c->window;
    ce.window = c->window;
    ce.x = c->x;
    ce.y = c->y;
    ce.width = c->width;
    ce.height = c->height;
    ce.border_width = 0;
    ce.above = None;
    ce.override_redirect = FALSE;

    myDisplayErrorTrapPush (display_info);
    XSendEvent (display_info->dpy, c->window, TRUE,
                StructureNotifyMask, (XEvent *) & ce);
    myDisplayErrorTrapPopIgnored (display_info);
}

void
clientConfigure (Client *c, XWindowChanges * wc, unsigned long mask, unsigned short flags)
{
    int px, py, pwidth, pheight;
    gboolean win_moved, win_resized;

    g_return_if_fail (c != NULL);
    g_return_if_fail (c->window != None);

    TRACE ("client \"%s\" (0x%lx) %s, type %u", c->name, c->window,
           flags & CFG_CONSTRAINED ? "constrained" : "not contrained", c->type);

    px = c->x;
    py = c->y;
    pwidth = c->width;
    pheight = c->height;

    if (mask & CWX)
    {
        if (!FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING))
        {
            c->x = wc->x;
        }
    }
    if (mask & CWY)
    {
        if (!FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING))
        {
            c->y = wc->y;
        }
    }
    if (mask & CWWidth)
    {
        c->width = clientCheckWidth (c, wc->width, flags & CFG_REQUEST);
    }
    if (mask & CWHeight)
    {
        c->height = clientCheckHeight (c, wc->height, flags & CFG_REQUEST);
    }
    if (mask & CWBorderWidth)
    {
        c->border_width = wc->border_width;
    }
    if (mask & CWStackMode)
    {
        switch (wc->stack_mode)
        {
            /*
             * Limitation: we don't support neither
             * TopIf, BottomIf nor Opposite ...
             */
            case Above:
                TRACE ("above");
                if (mask & CWSibling)
                {
                    clientRaise (c, wc->sibling);
                }
                else
                {
                    clientRaise (c, None);
                }
                break;
            case Below:
                TRACE ("below");
                if (mask & CWSibling)
                {
                    clientLower (c, wc->sibling);
                }
                else
                {
                    clientLower (c, None);
                }

                break;
            case Opposite:
            case TopIf:
            case BottomIf:
            default:
                break;
        }
    }
    mask &= ~(CWStackMode | CWSibling);

    /* Keep control over what the application does. */
    if (((flags & (CFG_CONSTRAINED | CFG_REQUEST)) == (CFG_CONSTRAINED | CFG_REQUEST))
         && CONSTRAINED_WINDOW (c))
    {
        clientConstrainPos (c, flags & CFG_KEEP_VISIBLE);

        if (c->x != px)
        {
            mask |= CWX;
        }
        else
        {
            mask &= ~CWX;
        }

        if (c->y != py)
        {
            mask |= CWY;
        }
        else
        {
            mask &= ~CWY;
        }

        if (c->width != pwidth)
        {
            mask |= CWWidth;
        }
        else
        {
            mask &= ~CWWidth;
        }

        if (c->height != pheight)
        {
            mask |= CWHeight;
        }
        else
        {
            mask &= ~CWHeight;
        }
    }

    clientConfigureWindows (c, wc, mask, flags);
    /*

      We reparent the client window. According to the ICCCM spec, the
      WM must send a senthetic event when the window is moved and not resized.

      But, since we reparent the window, we must also send a synthetic
      configure event when the window is moved and resized.

      See this thread for the rational:
      http://www.mail-archive.com/wm-spec-list@gnome.org/msg00379.html

      And specifically this post from Carsten Haitzler:
      http://www.mail-archive.com/wm-spec-list@gnome.org/msg00382.html

     */
    win_moved = (c->x != c->applied_geometry.x ||
                 c->y != c->applied_geometry.y);
    win_resized = (c->width != c->applied_geometry.width ||
                   c->height != c->applied_geometry.height);

    if ((win_moved) || (flags & (CFG_NOTIFY | CFG_FORCE_REDRAW)) ||
        ((flags & CFG_REQUEST) && !(win_moved || win_resized)))
    {
        clientSendConfigureNotify (c);
    }

    c->applied_geometry.x = c->x;
    c->applied_geometry.y = c->y;
    c->applied_geometry.width = c->width;
    c->applied_geometry.height = c->height;
}

void
clientReconfigure (Client *c, unsigned short flags)
{
    XWindowChanges wc;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    wc.x = c->x;
    wc.y = c->y;
    wc.width = c->width;
    wc.height = c->height;
    clientConfigure (c, &wc, CWX | CWY | CWWidth | CWHeight, flags);
}

void
clientMoveResizeWindow (Client *c, XWindowChanges * wc, unsigned long mask)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    unsigned short flags;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    if (c->type == WINDOW_DESKTOP)
    {
        /* Ignore stacking request for DESKTOP windows */
        mask &= ~(CWSibling | CWStackMode);
    }
    if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN)
        || (FLAG_TEST_ALL (c->flags, CLIENT_FLAG_MAXIMIZED)
            && (c->screen_info->params->borderless_maximize)))
    {
        /* Not allowed in fullscreen mode */
        mask &= ~(CWX | CWY | CWWidth | CWHeight);
    }
    /* clean up buggy requests that set all flags */
    if ((mask & CWX) && (wc->x == c->x))
    {
        mask &= ~CWX;
    }
    if ((mask & CWY) && (wc->y == c->y))
    {
        mask &= ~CWY;
    }
    if ((mask & CWWidth) && (wc->width == c->width))
    {
        mask &= ~CWWidth;
    }
    if ((mask & CWHeight) && (wc->height == c->height))
    {
        mask &= ~CWHeight;
    }

    /* Still a move/resize after cleanup? */
    flags = CFG_REQUEST;
    if (mask & (CWX | CWY | CWWidth | CWHeight))
    {
        /* Clear any previously saved pos flag from screen resize */
        FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_SAVED_POS);

        if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED))
        {
            clientRemoveMaximizeFlag (c);
            flags |= CFG_FORCE_REDRAW;
        }

        flags |= CFG_CONSTRAINED;
    }
    if ((mask & (CWWidth | CWHeight)) && !(mask & (CWX | CWY)))
    {
        /*
         * The client is resizing its window, but did not specify a
         * position, make sure the window remains fully visible in that
         *case so that the user does not have to relocate the window
         */
        flags |= CFG_KEEP_VISIBLE;
    }
    /*
     * Let's say that if the client performs a XRaiseWindow, we show the window if focus
     * stealing prevention is not activated, otherwise we just set the "demands attention"
     * flag...
     */
    if ((mask & CWStackMode) && (wc->stack_mode == Above) && (wc->sibling == None) && !(c->type & WINDOW_TYPE_DONT_FOCUS))
    {
        Client *last_raised;

        last_raised = clientGetLastRaise (screen_info);
        if (last_raised && (c != last_raised))
        {
            if ((screen_info->params->prevent_focus_stealing) && (screen_info->params->activate_action == ACTIVATE_ACTION_NONE))
            {
                mask &= ~(CWSibling | CWStackMode);
                TRACE ("setting WM_STATE_DEMANDS_ATTENTION flag on \"%s\" (0x%lx)", c->name, c->window);
                FLAG_SET (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION);
                clientSetNetState (c);
            }
            else
            {
                clientActivate (c, getXServerTime (display_info), TRUE);
            }
        }
    }
    /* And finally, configure the window */
    clientConfigure (c, wc, mask, flags);
}

void
clientGetMWMHints (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (c != NULL);
    g_return_if_fail (c->window != None);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if (c->mwm_hints)
    {
        g_free (c->mwm_hints);
    }
    c->mwm_hints = getMotifHints (display_info, c->window);
}

void
clientApplyMWMHints (Client *c, gboolean update)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    XWindowChanges wc;

    g_return_if_fail (c != NULL);
    g_return_if_fail (c->window != None);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if (c->mwm_hints)
    {
        if ((c->mwm_hints->flags & MWM_HINTS_DECORATIONS))
        {
            if (!FLAG_TEST (c->flags, CLIENT_FLAG_HAS_SHAPE))
            {
                if (c->mwm_hints->decorations & MWM_DECOR_ALL)
                {
                    FLAG_SET (c->xfwm_flags, XFWM_FLAG_HAS_BORDER | XFWM_FLAG_HAS_MENU);
                }
                else
                {
                    FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_HAS_BORDER | XFWM_FLAG_HAS_MENU);

                    if (c->mwm_hints-> decorations & (MWM_DECOR_TITLE | MWM_DECOR_BORDER))
                    {
                        FLAG_SET (c->xfwm_flags,  XFWM_FLAG_HAS_BORDER);
                    }
                    if (c->mwm_hints->decorations & MWM_DECOR_MENU)
                    {
                        FLAG_SET (c->xfwm_flags, XFWM_FLAG_HAS_MENU);
                    }
                    if (c->mwm_hints->decorations & MWM_DECOR_MINIMIZE)
                    {
                        FLAG_SET (c->xfwm_flags, XFWM_FLAG_HAS_HIDE);
                    }
                    if (c->mwm_hints->decorations & MWM_DECOR_MAXIMIZE)
                    {
                        FLAG_SET (c->xfwm_flags, XFWM_FLAG_HAS_MAXIMIZE);
                    }
                }
            }
        }
        /* The following is from Metacity : */
        if (c->mwm_hints->flags & MWM_HINTS_FUNCTIONS)
        {
            if (!(c->mwm_hints->functions & MWM_FUNC_ALL))
            {
                FLAG_UNSET (c->xfwm_flags,
                    XFWM_FLAG_HAS_CLOSE | XFWM_FLAG_HAS_HIDE |
                    XFWM_FLAG_HAS_MAXIMIZE | XFWM_FLAG_HAS_MOVE |
                    XFWM_FLAG_HAS_RESIZE);
            }
            else
            {
                FLAG_SET (c->xfwm_flags,
                    XFWM_FLAG_HAS_CLOSE | XFWM_FLAG_HAS_HIDE |
                    XFWM_FLAG_HAS_MAXIMIZE | XFWM_FLAG_HAS_MOVE |
                    XFWM_FLAG_HAS_RESIZE);
            }

            if (c->mwm_hints->functions & MWM_FUNC_CLOSE)
            {
                FLAG_TOGGLE (c->xfwm_flags, XFWM_FLAG_HAS_CLOSE);
            }
            if (c->mwm_hints->functions & MWM_FUNC_MINIMIZE)
            {
                FLAG_TOGGLE (c->xfwm_flags, XFWM_FLAG_HAS_HIDE);
            }
            if (c->mwm_hints->functions & MWM_FUNC_MAXIMIZE)
            {
                FLAG_TOGGLE (c->xfwm_flags, XFWM_FLAG_HAS_MAXIMIZE);
            }
            if (c->mwm_hints->functions & MWM_FUNC_RESIZE)
            {
                FLAG_TOGGLE (c->xfwm_flags, XFWM_FLAG_HAS_RESIZE);
            }
            if (c->mwm_hints->functions & MWM_FUNC_MOVE)
            {
                FLAG_TOGGLE (c->xfwm_flags, XFWM_FLAG_HAS_MOVE);
            }
        }
    }

    if (update)
    {
        wc.x = c->x;
        wc.y = c->y;
        wc.width = c->width;
        wc.height = c->height;

        if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
        {
            clientUpdateFullscreenSize (c);
        }
        /* If client is maximized, we need to update its coordonates and size as well */
        else if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED))
        {
            GdkRectangle rect;
            myScreenFindMonitorAtPoint (screen_info,
                                        frameX (c) + (frameWidth (c) / 2),
                                        frameY (c) + (frameHeight (c) / 2), &rect);
            clientNewMaxSize (c, &wc, &rect);
        }

        clientConfigure (c, &wc, CWX | CWY | CWWidth | CWHeight, CFG_FORCE_REDRAW);

        /* MWM hints can add or remove decorations, update NET_FRAME_EXTENTS accordingly */
        setNetFrameExtents (display_info,
                            c->window,
                            frameTop (c),
                            frameLeft (c),
                            frameRight (c),
                            frameBottom (c));
    }
}

void
clientGetWMNormalHints (Client *c, gboolean update)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    XWindowChanges wc;
    unsigned long previous_value;
    long dummy;
    int result, status;

    g_return_if_fail (c != NULL);
    g_return_if_fail (c->window != None);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (!c->size)
    {
        c->size = XAllocSizeHints ();
    }
    g_assert (c->size);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    dummy = 0;
    myDisplayErrorTrapPush (display_info);
    status = XGetWMNormalHints (display_info->dpy, c->window, c->size, &dummy);
    result = myDisplayErrorTrapPop (display_info);

    if ((result != Success) || !status)
    {
        c->size->flags = 0;
    }

    /* Set/update gravity */
    c->gravity = c->size->flags & PWinGravity ? c->size->win_gravity : NorthWestGravity;

    previous_value = FLAG_TEST (c->xfwm_flags, XFWM_FLAG_IS_RESIZABLE);
    FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_IS_RESIZABLE);

    wc.x = c->x;
    wc.y = c->y;
    wc.width = c->width;
    wc.height = c->height;

    if (!(c->size->flags & PMaxSize))
    {
        c->size->max_width = G_MAXINT;
        c->size->max_height = G_MAXINT;
        c->size->flags |= PMaxSize;
    }

    if (!(c->size->flags & PBaseSize))
    {
        c->size->base_width = 0;
        c->size->base_height = 0;
    }

    if (!(c->size->flags & PMinSize))
    {
        if ((c->size->flags & PBaseSize))
        {
            c->size->min_width = c->size->base_width;
            c->size->min_height = c->size->base_height;
        }
        else
        {
            c->size->min_width = 1;
            c->size->min_height = 1;
        }
        c->size->flags |= PMinSize;
    }

    if (c->size->flags & PResizeInc)
    {
        if (c->size->width_inc < 1)
        {
            c->size->width_inc = 1;
        }
        if (c->size->height_inc < 1)
        {
            c->size->height_inc = 1;
        }
    }
    else
    {
        c->size->width_inc = 1;
        c->size->height_inc = 1;
    }

    if (c->size->flags & PAspect)
    {
        if (c->size->min_aspect.x < 1)
        {
            c->size->min_aspect.x = 1;
        }
        if (c->size->min_aspect.y < 1)
        {
            c->size->min_aspect.y = 1;
        }
        if (c->size->max_aspect.x < 1)
        {
            c->size->max_aspect.x = 1;
        }
        if (c->size->max_aspect.y < 1)
        {
            c->size->max_aspect.y = 1;
        }
    }
    else
    {
        c->size->min_aspect.x = 1;
        c->size->min_aspect.y = 1;
        c->size->max_aspect.x = G_MAXINT;
        c->size->max_aspect.y = G_MAXINT;
    }

    if (c->size->min_width < 1)
    {
        c->size->min_width = 1;
    }
    if (c->size->min_height < 1)
    {
        c->size->min_height = 1;
    }
    if (c->size->max_width < 1)
    {
        c->size->max_width = 1;
    }
    if (c->size->max_height < 1)
    {
        c->size->max_height = 1;
    }
    if (wc.width > c->size->max_width)
    {
        wc.width = c->size->max_width;
    }
    if (wc.height > c->size->max_height)
    {
        wc.height = c->size->max_height;
    }
    if (wc.width < c->size->min_width)
    {
        wc.width = c->size->min_width;
    }
    if (wc.height < c->size->min_height)
    {
        wc.height = c->size->min_height;
    }

    if ((c->size->min_width < c->size->max_width) ||
        (c->size->min_height < c->size->max_height))
    {
        FLAG_SET (c->xfwm_flags, XFWM_FLAG_IS_RESIZABLE);
    }

    if (update)
    {
        if ((c->width != wc.width) || (c->height != wc.height))
        {
            if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED))
            {
                clientRemoveMaximizeFlag (c);
            }
            clientConfigure (c, &wc, CWX | CWY | CWWidth | CWHeight, CFG_CONSTRAINED | CFG_FORCE_REDRAW);
        }
        else if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_IS_RESIZABLE) != previous_value)
        {
            frameQueueDraw (c, FALSE);
        }
    }
    else
    {
        c->width = wc.width;
        c->height = wc.height;
    }
}

void
clientGetWMProtocols (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    unsigned int wm_protocols_flags;

    g_return_if_fail (c != NULL);
    g_return_if_fail (c->window != None);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    wm_protocols_flags = getWMProtocols (display_info, c->window);
    FLAG_SET (c->wm_flags,
        (wm_protocols_flags & WM_PROTOCOLS_DELETE_WINDOW) ?
        WM_FLAG_DELETE : 0);
    FLAG_SET (c->wm_flags,
        (wm_protocols_flags & WM_PROTOCOLS_TAKE_FOCUS) ?
        WM_FLAG_TAKEFOCUS : 0);
    /* KDE extension */
    FLAG_SET (c->wm_flags,
        (wm_protocols_flags & WM_PROTOCOLS_CONTEXT_HELP) ?
        WM_FLAG_CONTEXT_HELP : 0);
    /* Ping */
    FLAG_SET (c->wm_flags,
        (wm_protocols_flags & WM_PROTOCOLS_PING) ?
        WM_FLAG_PING : 0);
}

static void
clientFree (Client *c)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    clientClearFocus (c);
    if (clientGetLastRaise (c->screen_info) == c)
    {
        clientClearLastRaise (c->screen_info);
    }
    if (clientGetDelayedFocus () == c)
    {
        clientClearDelayedFocus ();
    }
    if (c->blink_timeout_id)
    {
        g_source_remove (c->blink_timeout_id);
    }
    if (c->icon_timeout_id)
    {
        g_source_remove (c->icon_timeout_id);
    }
    if (c->frame_timeout_id)
    {
        g_source_remove (c->frame_timeout_id);
    }
    if (c->ping_timeout_id)
    {
        clientRemoveNetWMPing (c);
    }
#ifdef HAVE_XSYNC
    if (c->xsync_alarm != None)
    {
        clientDestroyXSyncAlarm (c);
    }
    if (c->xsync_timeout_id)
    {
        g_source_remove (c->xsync_timeout_id);
    }
#endif /* HAVE_XSYNC */
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    if (c->startup_id)
    {
        g_free (c->startup_id);
    }
#endif /* HAVE_LIBSTARTUP_NOTIFICATION */
    if (c->name)
    {
        g_free (c->name);
    }
    if (c->hostname)
    {
        g_free (c->hostname);
    }
    if (c->size)
    {
        XFree (c->size);
    }
    if (c->wmhints)
    {
        XFree (c->wmhints);
    }
    if (c->mwm_hints)
    {
        g_free (c->mwm_hints);
    }
    if ((c->ncmap > 0) && (c->cmap_windows))
    {
        XFree (c->cmap_windows);
    }
    if (c->class.res_name)
    {
        XFree (c->class.res_name);
    }
    if (c->class.res_class)
    {
        XFree (c->class.res_class);
    }
    if (c->dialog_pid)
    {
        kill (c->dialog_pid, SIGKILL);
    }
    if (c->dialog_fd >= 0)
    {
        close (c->dialog_fd);
    }

    g_free (c);
}

static void
clientApplyInitialState (Client *c)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    /* We check that afterwards to make sure all states are now known */
    if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED))
    {
        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_MAXIMIZE))
        {
            unsigned long mode = 0L;

            TRACE ("applying client's initial state: maximized");
            mode = c->flags & CLIENT_FLAG_MAXIMIZED;

            /* Unset fullscreen mode so that clientToggleMaximized() really change the state */
            FLAG_UNSET (c->flags, CLIENT_FLAG_MAXIMIZED);
            clientToggleMaximized (c, mode, FALSE);
        }
    }
    if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        TRACE ("applying client's initial state: fullscreen");
        clientUpdateFullscreenState (c);
    }
    if (FLAG_TEST_AND_NOT (c->flags, CLIENT_FLAG_ABOVE, CLIENT_FLAG_BELOW))
    {
        TRACE ("applying client's initial state: above");
        clientUpdateLayerState (c);
    }
    if (FLAG_TEST_AND_NOT (c->flags, CLIENT_FLAG_BELOW, CLIENT_FLAG_ABOVE))
    {
        TRACE ("applying client's initial state: below");
        clientUpdateLayerState (c);
    }
    if (FLAG_TEST (c->flags, CLIENT_FLAG_STICKY) &&
        FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_STICK))
    {
        TRACE ("applying client's initial state: sticky");
        clientStick (c, TRUE);
    }
    if (FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
    {
        TRACE ("applying client's initial state: shaded");
        clientShade (c);
    }
}

static gboolean
clientCheckShape (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    int xws, yws, xbs, ybs;
    unsigned wws, hws, wbs, hbs;
    int boundingShaped, clipShaped;

    g_return_val_if_fail (c != NULL, FALSE);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    boundingShaped = 0;

    if (display_info->have_shape)
    {
        myDisplayErrorTrapPush (display_info);
        XShapeQueryExtents (display_info->dpy, c->window, &boundingShaped, &xws, &yws, &wws,
                            &hws, &clipShaped, &xbs, &ybs, &wbs, &hbs);
        myDisplayErrorTrapPopIgnored (display_info);
        return (boundingShaped != 0);
    }
    return FALSE;
}

static void
clientUpdateIconPix (Client *c)
{
    ScreenInfo *screen_info;
    gint size;
    GdkPixbuf *icon;
    int i;

    g_return_if_fail (c != NULL);
    g_return_if_fail (c->window != None);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    for (i = 0; i < STATE_TOGGLED; i++)
    {
        xfwmPixmapFree (&c->appmenu[i]);
    }

    if (xfwmPixmapNone(&screen_info->buttons[MENU_BUTTON][ACTIVE]))
    {
        /* The current theme has no menu button */
        return;
    }

    for (i = 0; i < STATE_TOGGLED; i++)
    {
        if (!xfwmPixmapNone(&screen_info->buttons[MENU_BUTTON][i]))
        {
            xfwmPixmapDuplicate (&screen_info->buttons[MENU_BUTTON][i], &c->appmenu[i]);
        }
    }
    size = MIN (screen_info->buttons[MENU_BUTTON][ACTIVE].width,
                screen_info->buttons[MENU_BUTTON][ACTIVE].height);

    if (size > 1)
    {
        icon = getAppIcon (c, size, size);
        if (icon)
        {
            for (i = 0; i < STATE_TOGGLED; i++)
            {
                if (!xfwmPixmapNone(&c->appmenu[i]))
                {
                    xfwmPixmapRenderGdkPixbuf (&c->appmenu[i], icon);
                }
            }
            g_object_unref (icon);
        }
    }
}

static gboolean
update_icon_idle_cb (gpointer data)
{
    Client *c;
    
    c = (Client *) data;
    g_return_val_if_fail (c, FALSE);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    clientUpdateIconPix (c);
    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE))
    {
        frameQueueDraw (c, FALSE);
    }
    c->icon_timeout_id = 0;

    return FALSE;
}

void
clientUpdateIcon (Client *c)
{
    g_return_if_fail (c);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (c->icon_timeout_id == 0)
    {
        c->icon_timeout_id = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                                              update_icon_idle_cb, c, NULL);
    }
}

void
clientSaveSizePos (Client *c)
{
    g_return_if_fail (c != NULL);

    if (!FLAG_TEST (c->flags, CLIENT_FLAG_RESTORE_SIZE_POS))
    {
        c->saved_geometry.x = c->x;
        c->saved_geometry.width = c->width;
        c->saved_geometry.y = c->y;
        c->saved_geometry.height = c->height;
    }
}

gboolean
clientRestoreSizePos (Client *c)
{
    g_return_val_if_fail (c != NULL, FALSE);

    if (FLAG_TEST (c->flags, CLIENT_FLAG_RESTORE_SIZE_POS))
    {
        c->x = c->saved_geometry.x;
        c->width = c->saved_geometry.width;
        c->y = c->saved_geometry.y;
        c->height = c->saved_geometry.height;

        FLAG_UNSET (c->flags, CLIENT_FLAG_RESTORE_SIZE_POS);
        return TRUE;
    }

    return FALSE;
}

Client *
clientFrame (DisplayInfo *display_info, Window w, gboolean recapture)
{
    ScreenInfo *screen_info;
    XWindowAttributes attr;
    XSetWindowAttributes attributes;
    Client *c = NULL;
    gboolean shaped;
    gchar *wm_name;
    unsigned long valuemask;
    int i;

    g_return_val_if_fail (w != None, NULL);
    g_return_val_if_fail (display_info != NULL, NULL);
    TRACE ("window 0x%lx", w);

    myDisplayGrabServer (display_info);
    myDisplayErrorTrapPush (display_info);

    if (!XGetWindowAttributes (display_info->dpy, w, &attr))
    {
        DBG ("Cannot get window attributes for window (0x%lx)", w);
        goto out;
    }

    screen_info = myDisplayGetScreenFromRoot (display_info, attr.root);
    if (!screen_info)
    {
        DBG ("Cannot determine screen info from window (0x%lx)", w);
        goto out;
    }

    if (w == screen_info->xfwm4_win)
    {
        TRACE ("not managing our own event window");
        compositorAddWindow (display_info, w, NULL);
        goto out;
    }

#ifdef ENABLE_KDE_SYSTRAY_PROXY
    if (checkKdeSystrayWindow (display_info, w))
    {
        TRACE ("detected KDE systray windows");
        if (screen_info->systray != None)
        {
            sendSystrayReqDock (display_info, w, screen_info->systray);
            goto out;
        }
        TRACE ("no systray found for this screen");
    }
#endif /* ENABLE_KDE_SYSTRAY_PROXY */

    if (attr.override_redirect)
    {
        TRACE ("override redirect window 0x%lx", w);
        goto out;
    }

    c = g_new0 (Client, 1);
    if (!c)
    {
        TRACE ("cannot allocate memory for the window structure");
        goto out;
    }

    c->window = w;
    c->screen_info = screen_info;
    c->serial = screen_info->client_serial++;

    /* Termination dialog */
    c->dialog_pid = 0;
    c->dialog_fd = -1;

    getWindowName (display_info, c->window, &wm_name);
    getWindowHostname (display_info, c->window, &c->hostname);
    c->name = clientCreateTitleName (c, wm_name, c->hostname);
    g_free (wm_name);

    getTransientFor (display_info, screen_info->xroot, c->window, &c->transient_for);
    XChangeSaveSet(display_info->dpy, c->window, SetModeInsert);

    /* Initialize structure */
    c->size = NULL;
    c->flags = 0L;
    c->wm_flags = 0L;
    c->xfwm_flags = XFWM_FLAG_INITIAL_VALUES;
    c->x = attr.x;
    c->y = attr.y;
    c->width = attr.width;
    c->height = attr.height;

    c->applied_geometry.x = c->x;
    c->applied_geometry.y = c->y;
    c->applied_geometry.width = c->width;
    c->applied_geometry.height = c->height;

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    c->startup_id = NULL;
#endif /* HAVE_LIBSTARTUP_NOTIFICATION */

    clientGetWMNormalHints (c, FALSE);
    clientGetMWMHints (c);
    c->size->x = c->x;
    c->size->y = c->y;
    c->size->width = c->width;
    c->size->height = c->height;
    c->frame_cache_width = -1;
    c->frame_cache_height = -1;
    c->border_width = attr.border_width;
    c->cmap = attr.colormap;

    shaped = clientCheckShape(c);
    if (shaped)
    {
        FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_HAS_BORDER);
        FLAG_SET (c->flags, CLIENT_FLAG_HAS_SHAPE);
    }

    if (((c->size->flags & (PMinSize | PMaxSize)) != (PMinSize | PMaxSize))
        || (((c->size->flags & (PMinSize | PMaxSize)) ==
                (PMinSize | PMaxSize))
            && ((c->size->min_width < c->size->max_width)
                || (c->size->min_height < c->size->max_height))))
    {
        FLAG_SET (c->xfwm_flags, XFWM_FLAG_IS_RESIZABLE);
    }

    for (i = 0; i < BUTTON_COUNT; i++)
    {
        c->button_status[i] = BUTTON_STATE_NORMAL;
    }

    if (!XGetWMColormapWindows (display_info->dpy, c->window, &c->cmap_windows, &c->ncmap))
    {
        c->ncmap = 0;
    }

    c->fullscreen_monitors[0] = 0;
    c->fullscreen_monitors[1] = 0;
    c->fullscreen_monitors[2] = 0;
    c->fullscreen_monitors[3] = 0;

    /* Opacity for compositing manager */
    c->opacity = NET_WM_OPAQUE;
    getOpacity (display_info, c->window, &c->opacity);
    c->opacity_applied = c->opacity;
    c->opacity_flags = 0;

    /* Keep count of blinking iterations */
    c->blink_iterations = 0;

    if (getOpacityLock (display_info, c->window))
    {
        FLAG_SET (c->xfwm_flags, XFWM_FLAG_OPACITY_LOCKED);
    }

    /* Timout for asynchronous icon update */
    c->icon_timeout_id = 0;
    /* Timout for asynchronous frame update */
    c->frame_timeout_id = 0;
    /* Timeout for blinking on urgency */
    c->blink_timeout_id = 0;
    /* Ping timeout  */
    c->ping_timeout_id = 0;
    /* Ping timeout  */
    c->ping_time = 0;

    c->class.res_name = NULL;
    c->class.res_class = NULL;
    XGetClassHint (display_info->dpy, w, &c->class);
    c->wmhints = XGetWMHints (display_info->dpy, c->window);
    c->group_leader = None;
    if (c->wmhints)
    {
        if (c->wmhints->flags & WindowGroupHint)
        {
            c->group_leader = c->wmhints->window_group;
        }
    }
    c->client_leader = getClientLeader (display_info, c->window);

    TRACE ("\"%s\" (0x%lx) initial map_state = %s",
                c->name, c->window,
                (attr.map_state == IsUnmapped) ?
                "IsUnmapped" :
                (attr.map_state == IsViewable) ?
                "IsViewable" :
                (attr.map_state == IsUnviewable) ?
                "IsUnviewable" :
                "(unknown)");
    if (attr.map_state != IsUnmapped)
    {
        /* Reparent will send us unmap/map events */
        FLAG_SET (c->xfwm_flags, XFWM_FLAG_MAP_PENDING);
    }
    c->ignore_unmap = 0;
    c->type = UNSET;
    c->type_atom = None;

    FLAG_SET (c->flags, START_ICONIC (c) ? CLIENT_FLAG_ICONIFIED : 0);
    FLAG_SET (c->wm_flags, HINTS_ACCEPT_INPUT (c->wmhints) ? WM_FLAG_INPUT : 0);

    clientGetWMProtocols (c);
    c->win_layer = WIN_LAYER_NORMAL;
    c->pre_fullscreen_layer = c->win_layer;

    /* net_wm_user_time standard */
    c->user_time = 0;
    c->user_time_win = getNetWMUserTimeWindow(display_info, c->window);
    clientAddUserTimeWin (c);
    clientGetUserTime (c);

    /* client PID */
    c->pid = getWindowPID (display_info, c->window);
    TRACE ("client \"%s\" (0x%lx) PID = %i", c->name, c->window, c->pid);

    /* Apply startup notification properties if available */
    sn_client_startup_properties (c);

    /* Reload from session */
    if (sessionMatchWinToSM (c))
    {
        FLAG_SET (c->xfwm_flags, XFWM_FLAG_SESSION_MANAGED);
    }

    /* Beware, order of calls is important here ! */
    clientGetNetState (c);
    clientGetNetWmType (c);
    clientGetInitialNetWmDesktop (c);
    /* workarea will be updated when shown, no need to worry here */
    clientGetNetStruts (c);

    /* GTK 3.x stuff */
    clientGetGtkFrameExtents(c);
    clientGetGtkHideTitlebar(c);

    /* Once we know the type of window, we can initialize window position */
    if (!FLAG_TEST (c->xfwm_flags, XFWM_FLAG_SESSION_MANAGED))
    {
        clientCoordGravitate (c, c->gravity, APPLY, &c->x, &c->y);
        if (attr.map_state == IsUnmapped)
        {
            clientInitPosition (c);
        }
    }

    /*
       Initialize "old" fields once the position is ensured, to avoid
       initially maximized or fullscreen windows being placed offscreen
       once de-maximized
     */
    c->saved_geometry.x = c->x;
    c->saved_geometry.y = c->y;
    c->saved_geometry.width = c->width;
    c->saved_geometry.height = c->height;

    c->pre_fullscreen_geometry.x = c->x;
    c->pre_fullscreen_geometry.y = c->y;
    c->pre_fullscreen_geometry.width = c->width;
    c->pre_fullscreen_geometry.height = c->height;

    /*
       We must call clientApplyInitialState() after having placed the
       window so that the inital position values are correctly set if the
       inital state is maximize or fullscreen
     */
    clientApplyInitialState (c);

    valuemask = CWEventMask|CWBitGravity|CWWinGravity;
    attributes.event_mask = (FRAME_EVENT_MASK | POINTER_EVENT_MASK);
    attributes.win_gravity = StaticGravity;
    attributes.bit_gravity = StaticGravity;

#ifdef HAVE_RENDER
    if ((attr.depth == 32) && (display_info->have_render))
    {
        c->visual = attr.visual;
        c->depth  = attr.depth;

        attributes.colormap = attr.colormap;
        attributes.background_pixmap = None;
        attributes.border_pixel = 0;
        attributes.background_pixel = 0;

        valuemask |= CWColormap|CWBackPixmap|CWBackPixel|CWBorderPixel;
    }
    else
    {
        /* Default depth/visual */
        c->visual = screen_info->visual;
        c->depth  = screen_info->depth;
    }
#else  /* HAVE_RENDER */
    /* We don't support multiple depth/visual w/out render */
    c->visual = screen_info->visual;
    c->depth  = screen_info->depth;
#endif /* HAVE_RENDER */

    c->frame =
        XCreateWindow (display_info->dpy, screen_info->xroot, 0, 0, 1, 1, 0,
        c->depth, InputOutput, c->visual, valuemask, &attributes);
#ifdef HAVE_XI2
    xfwm_device_configure_xi2_event_mask (display_info->devices, display_info->dpy,
                                          c->frame, attributes.event_mask);
#endif

    XSelectInput (display_info->dpy, c->window, NoEventMask);
    XSetWindowBorderWidth (display_info->dpy, c->window, 0);
    if (FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
    {
        XUnmapWindow (display_info->dpy, c->window);
    }
    XReparentWindow (display_info->dpy, c->window, c->frame, frameLeft (c), frameTop (c));
    valuemask = CWEventMask|CWWinGravity;

    /* Force win_gravity to NorthWest. Any other gravity has the
     * window move relative to its parent when the parent resizes.
     *
     * There are many bug reports related to libreoffice using
     * StaticGravity, including:
     *
     * http://www.linuxquestions.org/questions/linux-desktop-74/strange-libreoffice-problem-on-debian-with-xfce-4175469847/
     * https://bugs.launchpad.net/ubuntu/+source/nvidia-graphics-drivers/+bug/889212
     * https://bbs.archlinux.org/viewtopic.php?id=133137
     * http://forums.debian.net/viewtopic.php?f=6&t=105757
     */
    attributes.event_mask = (CLIENT_EVENT_MASK);
    attributes.win_gravity = NorthWestGravity;
    XChangeWindowAttributes (display_info->dpy, c->window, valuemask, &attributes);
    if (display_info->have_shape)
    {
        XShapeSelectInput (display_info->dpy, c->window, ShapeNotifyMask);
    }

    clientAddToList (c);
    clientGrabButtons(c);

    /* Initialize per client menu button pixmap */

    for (i = 0; i < STATE_TOGGLED; i++)
    {
        xfwmPixmapInit (screen_info, &c->appmenu[i]);
    }

    for (i = 0; i < SIDE_COUNT; i++)
    {
        if (i == SIDE_TOP)
            continue;  /* Keep SIDE_TOP for later */

        xfwmWindowCreate (screen_info, c->visual, c->depth, c->frame,
            &c->sides[i], NoEventMask,
            myDisplayGetCursorResize(screen_info->display_info, CORNER_COUNT + i));
    }

    for (i = 0; i < CORNER_COUNT; i++)
    {
        xfwmWindowCreate (screen_info, c->visual, c->depth, c->frame,
            &c->corners[i], NoEventMask,
            myDisplayGetCursorResize(screen_info->display_info, i));
    }

    xfwmWindowCreate (screen_info, c->visual, c->depth, c->frame,
        &c->title, NoEventMask, None);

    /*create the top side window AFTER the title window since they overlap
       and the top side window should be on top */

    xfwmWindowCreate (screen_info, c->visual, c->depth, c->frame,
        &c->sides[SIDE_TOP], NoEventMask,
        myDisplayGetCursorResize(screen_info->display_info, CORNER_COUNT + SIDE_TOP));

    for (i = 0; i < BUTTON_COUNT; i++)
    {
        xfwmWindowCreate (screen_info, c->visual, c->depth, c->frame,
            &c->buttons[i], BUTTON_EVENT_MASK, None);
    }
    clientUpdateIconPix (c);

    /* Put the window on top to avoid XShape, that speeds up hw accelerated
       GL apps dramatically */
    XRaiseWindow (display_info->dpy, c->window);

    TRACE ("now calling configure for the new window \"%s\" (0x%lx)", c->name, c->window);
    clientReconfigure (c, CFG_NOTIFY | CFG_FORCE_REDRAW);

    /* Notify the compositor about this new window */
    compositorAddWindow (display_info, c->frame, c);

    if (!FLAG_TEST (c->flags, CLIENT_FLAG_ICONIFIED))
    {
        if ((c->win_workspace == screen_info->current_ws) ||
            FLAG_TEST(c->flags, CLIENT_FLAG_STICKY))
        {
            if (recapture)
            {
                clientRaise (c, None);
                clientShow (c, TRUE);
                clientSortRing(c);
            }
            else
            {
                clientFocusNew(c);
            }
        }
        else
        {
            clientRaise (c, None);
            clientShow (c, TRUE);
            clientInitFocusFlag (c);
        }
    }
    else
    {
        clientRaise (c, None);
        setWMState (display_info, c->window, IconicState);
        clientSetNetActions (c);
    }
    clientUpdateOpacity (c);
    clientGrabMouseButton (c);
    setNetFrameExtents (display_info, c->window, frameTop (c), frameLeft (c),
                                                 frameRight (c), frameBottom (c));
    clientSetNetState (c);

#ifdef HAVE_XSYNC
    c->xsync_counter = None;
    c->xsync_alarm = None;
    c->xsync_timeout_id = 0;
    if (display_info->have_xsync)
    {
        clientGetXSyncCounter (c);
    }
    if (c->xsync_counter)
    {
        clientCreateXSyncAlarm (c);
    }
#endif /* HAVE_XSYNC */


    DBG ("client \"%s\" (0x%lx) is now managed", c->name, c->window);
    DBG ("client_count=%d", screen_info->client_count);

out:
    /* Window is reparented now, so we can safely release the grab
     * on the server
     */
    myDisplayErrorTrapPopIgnored (display_info);
    myDisplayUngrabServer (display_info);

    return c;
}

void
clientUnframe (Client *c, gboolean remap)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    XEvent ev;
    int i;
    gboolean reparented;

    TRACE ("client \"%s\" (0x%lx) [%s]",
            c->name, c->window, remap ? "remap" : "no remap");

    g_return_if_fail (c != NULL);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    clientRemoveFromList (c);
    compositorSetClient (display_info, c->frame, NULL);

    myDisplayGrabServer (display_info);
    myDisplayErrorTrapPush (display_info);

    clientRemoveUserTimeWin (c);
    clientUngrabButtons (c);
    XUnmapWindow (display_info->dpy, c->frame);
    clientCoordGravitate (c, c->gravity, REMOVE, &c->x, &c->y);
    XSelectInput (display_info->dpy, c->window, NoEventMask);
    XChangeSaveSet(display_info->dpy, c->window, SetModeDelete);

    reparented = XCheckTypedWindowEvent (display_info->dpy, c->window, ReparentNotify, &ev);

    if (remap || !reparented)
    {
        XReparentWindow (display_info->dpy, c->window, c->screen_info->xroot, c->x, c->y);
        XSetWindowBorderWidth (display_info->dpy, c->window, c->border_width);
        if (remap)
        {
            compositorAddWindow (display_info, c->window, NULL);
            XMapWindow (display_info->dpy, c->window);
        }
        else
        {
            XUnmapWindow (display_info->dpy, c->window);
            setWMState (display_info, c->window, WithdrawnState);
        }
    }

    if (!remap)
    {
        XDeleteProperty (display_info->dpy, c->window,
                         display_info->atoms[NET_WM_STATE]);
        XDeleteProperty (display_info->dpy, c->window,
                         display_info->atoms[NET_WM_DESKTOP]);
        XDeleteProperty (display_info->dpy, c->window,
                         display_info->atoms[NET_WM_ALLOWED_ACTIONS]);
    }

    xfwmWindowDelete (&c->title);

    for (i = 0; i < SIDE_COUNT; i++)
    {
        xfwmWindowDelete (&c->sides[i]);
    }
    for (i = 0; i < CORNER_COUNT; i++)
    {
        xfwmWindowDelete (&c->corners[i]);
    }
    for (i = 0; i < STATE_TOGGLED; i++)
    {
        xfwmPixmapFree (&c->appmenu[i]);
    }
    for (i = 0; i < BUTTON_COUNT; i++)
    {
        xfwmWindowDelete (&c->buttons[i]);
    }
    if (FLAG_TEST (c->flags, CLIENT_FLAG_HAS_STRUT))
    {
        workspaceUpdateArea (c->screen_info);
    }
    XDestroyWindow (display_info->dpy, c->frame);

    myDisplayErrorTrapPopIgnored (display_info);
    myDisplayUngrabServer (display_info);
    clientFree (c);
}

void
clientFrameAll (ScreenInfo *screen_info)
{
    DisplayInfo *display_info;
    XWindowAttributes attr;
    xfwmWindow shield;
    Window w1, w2, *wins;
    unsigned int count, i;

    TRACE ("entering");

    display_info = screen_info->display_info;
    clientSetFocus (screen_info, NULL, myDisplayGetCurrentTime (display_info), NO_FOCUS_FLAG);
    xfwmWindowTemp (screen_info,
                    NULL, 0,
                    screen_info->xroot,
                    &shield,
                    0, 0,
                    screen_info->width,
                    screen_info->height,
                    EnterWindowMask,
                    FALSE);

    XSync (display_info->dpy, FALSE);
    myDisplayGrabServer (display_info);
    XQueryTree (display_info->dpy, screen_info->xroot, &w1, &w2, &wins, &count);
    for (i = 0; i < count; i++)
    {
        XGetWindowAttributes (display_info->dpy, wins[i], &attr);
        if ((attr.map_state == IsViewable) && (attr.root == screen_info->xroot))
        {
            Client *c = clientFrame (display_info, wins[i], TRUE);
            if ((c) && ((screen_info->params->raise_on_click) || (screen_info->params->click_to_focus)))
            {
                clientGrabMouseButton (c);
            }
        }
        else
        {
             compositorAddWindow (display_info, wins[i], NULL);
        }
    }
    if (wins)
    {
        XFree (wins);
    }
    clientFocusTop (screen_info, WIN_LAYER_FULLSCREEN, myDisplayGetCurrentTime (display_info));
    xfwmWindowDelete (&shield);
    myDisplayUngrabServer (display_info);
    XSync (display_info->dpy, FALSE);
}

void
clientUnframeAll (ScreenInfo *screen_info)
{
    DisplayInfo *display_info;
    Client *c;
    Window w1, w2, *wins;
    unsigned int count, i;

    TRACE ("entering");

    display_info = screen_info->display_info;
    clientSetFocus (screen_info, NULL, myDisplayGetCurrentTime (display_info), FOCUS_IGNORE_MODAL);
    XSync (display_info->dpy, FALSE);
    myDisplayGrabServer (display_info);
    XQueryTree (display_info->dpy, screen_info->xroot, &w1, &w2, &wins, &count);
    for (i = 0; i < count; i++)
    {
        c = myScreenGetClientFromWindow (screen_info, wins[i], SEARCH_FRAME);
        if (c)
        {
            clientUnframe (c, TRUE);
        }
    }
    myDisplayUngrabServer (display_info);
    XSync(display_info->dpy, FALSE);
    if (wins)
    {
        XFree (wins);
    }
}

Client *
clientGetFromWindow (Client *c, Window w, unsigned short mode)
{
    int b;

    g_return_val_if_fail (w != None, NULL);
    g_return_val_if_fail (c != NULL, NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (mode & SEARCH_WINDOW)
    {
        if (c->window == w)
        {
            TRACE ("found \"%s\" (mode WINDOW)", c->name);
            return (c);
        }
    }

    if (mode & SEARCH_FRAME)
    {
        if (c->frame == w)
        {
            TRACE ("found \"%s\" (mode FRAME)", c->name);
            return (c);
        }
    }

    if (mode & SEARCH_WIN_USER_TIME)
    {
        if (c->user_time_win == w)
        {
            TRACE ("found \"%s\" (mode WIN_USER_TIME)", c->name);
            return (c);
        }
    }

    if (mode & SEARCH_BUTTON)
    {
        for (b = 0; b < BUTTON_COUNT; b++)
        {
            if (MYWINDOW_XWINDOW(c->buttons[b]) == w)
            {
                TRACE ("found \"%s\" (mode BUTTON)", c->name);
                return (c);
            }
        }
    }

    TRACE ("no client found");

    return NULL;
}

static void
clientSetWorkspaceSingle (Client *c, guint ws)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if (ws > screen_info->workspace_count - 1)
    {
        ws = screen_info->workspace_count - 1;
        TRACE ("value off limits, using %i instead", ws);
    }

    if (c->win_workspace != ws)
    {
        TRACE ("setting client \"%s\" (0x%lx) to current_ws %d", c->name, c->window, ws);
        c->win_workspace = ws;
        if (FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
        {
            setHint (display_info, c->window, NET_WM_DESKTOP, (unsigned long) ALL_WORKSPACES);
        }
        else
        {
            setHint (display_info, c->window, NET_WM_DESKTOP, (unsigned long) ws);
        }
    }
    FLAG_SET (c->xfwm_flags, XFWM_FLAG_WORKSPACE_SET);
}

void
clientSetWorkspace (Client *c, guint ws, gboolean manage_mapping)
{
    Client *c2;
    GList *list_of_windows;
    GList *list;
    guint previous_ws;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (ws > c->screen_info->workspace_count - 1)
    {
        g_warning ("Requested workspace %d does not exist", ws);
        return;
    }

    list_of_windows = clientListTransientOrModal (c);
    for (list = list_of_windows; list; list = g_list_next (list))
    {
        c2 = (Client *) list->data;

        if (c2->win_workspace != ws)
        {
            TRACE ("setting client \"%s\" (0x%lx) to current_ws %d", c->name, c->window, ws);

            previous_ws = c2->win_workspace;
            clientSetWorkspaceSingle (c2, ws);

            if (manage_mapping && !FLAG_TEST (c2->flags, CLIENT_FLAG_ICONIFIED))
            {
                if (previous_ws == c2->screen_info->current_ws)
                {
                    clientWithdraw (c2, c2->screen_info->current_ws, FALSE);
                }
                if (FLAG_TEST (c2->flags, CLIENT_FLAG_STICKY) || (ws == c2->screen_info->current_ws))
                {
                    clientShow (c2, FALSE);
                }
            }
        }
    }
    g_list_free (list_of_windows);
}

static void
clientShowSingle (Client *c, gboolean deiconify)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (c != NULL);

    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE))
    {
        /* Should we map the window if it is visible? */
        return;
    }

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if ((c->win_workspace == screen_info->current_ws) || FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
    {
        TRACE ("showing client \"%s\" (0x%lx)", c->name, c->window);
        FLAG_SET (c->xfwm_flags, XFWM_FLAG_VISIBLE);
        myDisplayErrorTrapPush (display_info);
        XMapWindow (display_info->dpy, c->frame);
        if (!FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
        {
            XMapWindow (display_info->dpy, c->window);
        }
        myDisplayErrorTrapPopIgnored (display_info);
        /* Adjust to urgency state as the window is visible */
        clientUpdateUrgency (c);
    }
    if (deiconify)
    {
        FLAG_UNSET (c->flags, CLIENT_FLAG_ICONIFIED);
        setWMState (display_info, c->window, NormalState);
    }
    clientSetNetActions (c);
    clientSetNetState (c);
}

void
clientShow (Client *c, gboolean deiconify)
{
    Client *c2;
    GList *list_of_windows;
    GList *list;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    list_of_windows = clientListTransientOrModal (c);
    for (list = g_list_last (list_of_windows); list; list = g_list_previous (list))
    {
        c2 = (Client *) list->data;
        clientSetWorkspaceSingle (c2, c->win_workspace);
        /* Ignore request before if the window is not yet managed */
        if (!FLAG_TEST (c2->xfwm_flags, XFWM_FLAG_MANAGED))
        {
            continue;
        }
        clientShowSingle (c2, deiconify);
    }
    g_list_free (list_of_windows);

    /* Update working area as windows have been shown */
    workspaceUpdateArea (c->screen_info);
}

static void
clientWithdrawSingle (Client *c, GList *exclude_list, gboolean iconify)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    clientPassFocus(c->screen_info, c, exclude_list);
    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE))
    {
        FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_VISIBLE);
        c->ignore_unmap++;
        /* Adjust to urgency state as the window is not visible */
        clientUpdateUrgency (c);
    }

    myDisplayErrorTrapPush (display_info);
    XUnmapWindow (display_info->dpy, c->frame);
    XUnmapWindow (display_info->dpy, c->window);
    myDisplayErrorTrapPopIgnored (display_info);

    if (iconify)
    {
        FLAG_SET (c->flags, CLIENT_FLAG_ICONIFIED);
        setWMState (display_info, c->window, IconicState);
        if (!screen_info->show_desktop && !screen_info->params->cycle_minimized)
        {
            clientSetLast (c);
        }
    }
    clientSetNetActions (c);
    clientSetNetState (c);
}

void
clientWithdraw (Client *c, guint ws, gboolean iconify)
{
    Client *c2;
    GList *list_of_windows;
    GList *list;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    list_of_windows = clientListTransientOrModal (c);
    for (list = list_of_windows; list; list = g_list_next (list))
    {
        c2 = (Client *) list->data;

        /* Ignore request before if the window is not yet managed */
        if (!FLAG_TEST (c2->xfwm_flags, XFWM_FLAG_MANAGED))
        {
            continue;
        }

        if (FLAG_TEST (c2->flags, CLIENT_FLAG_STICKY) && !iconify)
        {
            continue;
        }

        if (c->win_layer < c2->win_layer && iconify)
        {
            continue;
        }

        if (clientIsTransientOrModalForGroup (c2))
        {
            if ((c2 != c) &&
                clientTransientOrModalHasAncestor (c2, c2->win_workspace))
            {
                /* Other ancestors for that transient for group are still
                 * visible on current workspace, so don't hide it...
                 */
                continue;
            }
            if ((ws != c2->win_workspace) &&
                clientTransientOrModalHasAncestor (c2, ws))
            {
                /* ws is used when transitioning between desktops, to avoid
                   hiding a transient for group that will be shown again on the new
                   workspace (transient for groups can be transients for multiple
                   ancesors splitted across workspaces...)
                 */
                continue;
            }
        }
        clientWithdrawSingle (c2, list_of_windows, iconify);
    }
    g_list_free (list_of_windows);

    /* Update working area as windows have been hidden */
    workspaceUpdateArea (c->screen_info);
}

void
clientWithdrawAll (Client *c, guint ws)
{
    GList *list;
    Client *c2;
    ScreenInfo *screen_info;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    for (list = screen_info->windows_stack; list; list = g_list_next (list))
    {
        c2 = (Client *) list->data;

        if ((c2 != c)
            && CLIENT_CAN_HIDE_WINDOW (c2)
            && !clientIsTransientOrModal (c2))
        {
            if (((!c) && (c2->win_workspace == ws))
                 || ((c) && !clientIsTransientOrModalFor (c, c2)
                         && (c2->win_workspace == c->win_workspace)))
            {
                clientWithdraw (c2, ws, TRUE);
            }
        }
    }
}

void
clientClearAllShowDesktop (ScreenInfo *screen_info)
{
    GList *list;

    TRACE ("entering");

    if (screen_info->show_desktop)
    {
        for (list = screen_info->windows_stack; list; list = g_list_next (list))
        {
            Client *c = (Client *) list->data;
            FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_WAS_SHOWN);
        }
        screen_info->show_desktop = FALSE;
        sendRootMessage (screen_info, NET_SHOWING_DESKTOP, screen_info->show_desktop,
                         myDisplayGetCurrentTime (screen_info->display_info));
    }
}

void
clientToggleShowDesktop (ScreenInfo *screen_info)
{
    GList *list;

    TRACE ("entering");

    clientSetFocus (screen_info, NULL,
                    myDisplayGetCurrentTime (screen_info->display_info),
                    FOCUS_IGNORE_MODAL);
    if (screen_info->show_desktop)
    {
        for (list = screen_info->windows_stack; list; list = g_list_next (list))
        {
            Client *c = (Client *) list->data;
            if ((c->type & WINDOW_REGULAR_FOCUSABLE)
                && !FLAG_TEST (c->flags, CLIENT_FLAG_ICONIFIED | CLIENT_FLAG_SKIP_TASKBAR))
            {
                FLAG_SET (c->xfwm_flags, XFWM_FLAG_WAS_SHOWN);
                clientWithdraw (c, c->win_workspace, TRUE);
            }
        }
        clientFocusTop (screen_info, WIN_LAYER_DESKTOP, myDisplayGetCurrentTime (screen_info->display_info));
    }
    else
    {
        for (list = g_list_last(screen_info->windows_stack); list; list = g_list_previous (list))
        {
            Client *c = (Client *) list->data;
            if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_WAS_SHOWN))
            {
                clientShow (c, TRUE);
            }
            FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_WAS_SHOWN);
        }
        clientFocusTop (screen_info, WIN_LAYER_FULLSCREEN, myDisplayGetCurrentTime (screen_info->display_info));
    }
}

void
clientActivate (Client *c, guint32 timestamp, gboolean source_is_application)
{
    ScreenInfo *screen_info;
    Client *focused;
    Client *ancestor;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    ancestor = clientGetTransientFor(c);
    focused = clientGetFocus ();

    if ((screen_info->current_ws == c->win_workspace) || (screen_info->params->activate_action != ACTIVATE_ACTION_NONE))
    {
        if ((focused) && (c != focused))
        {
            /* We might be able to avoid this if we are about to switch workspace */
            clientAdjustFullscreenLayer (focused, FALSE);
        }
        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_WAS_SHOWN))
        {
            /* We are explicitely activating a window that was shown before show-desktop */
            clientClearAllShowDesktop (screen_info);
        }
        if (screen_info->current_ws != c->win_workspace)
        {
            if (screen_info->params->activate_action == ACTIVATE_ACTION_BRING)
            {
                clientSetWorkspace (c, screen_info->current_ws, TRUE);
            }
            else
            {
                workspaceSwitch (screen_info, c->win_workspace, NULL, FALSE, timestamp);
            }
        }

        clientShow (ancestor, TRUE);
        clientRaise (c, None);

        if (!source_is_application || screen_info->params->click_to_focus || (c->type & WINDOW_TYPE_DONT_FOCUS))
        {
            /*
               It's a bit tricky here, we want to honor the activate request only if:

               - The window use the _NET_ACTIVE_WINDOW protocol and identify itself as a pager,
               - Or we use the click to focus model, in that case we focus the window anyway,
               - Or the request comes from an application that we would not focus by default,
                 such as panels for example
             */

            clientSetFocus (screen_info, c, timestamp, NO_FOCUS_FLAG);
        }
    }
    else
    {
        TRACE ("setting WM_STATE_DEMANDS_ATTENTION flag on \"%s\" (0x%lx)", c->name, c->window);
        FLAG_SET (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION);
        clientSetNetState (c);
    }
}

void
clientClose (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    guint32 timestamp;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    timestamp = myDisplayGetCurrentTime (display_info);
    timestamp = myDisplayGetTime (display_info, timestamp);

    if (FLAG_TEST (c->wm_flags, WM_FLAG_DELETE))
    {
        sendClientMessage (screen_info, c->window, WM_DELETE_WINDOW, timestamp);
        if (FLAG_TEST (c->wm_flags, WM_FLAG_PING))
        {
            clientSendNetWMPing (c, timestamp);
        }
    }
    else
    {
        clientKill (c);
    }
}

void
clientKill (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    myDisplayErrorTrapPush (display_info);
    XKillClient (display_info->dpy, c->window);
    myDisplayErrorTrapPopIgnored (display_info);
}

void
clientTerminate (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if ((c->hostname) && (display_info->hostname) && (c->pid > 0))
    {
        if (!strcmp (display_info->hostname, c->hostname))
        {
            TRACE ("sending client %s (pid %i) signal SIGKILL\n", c->name, c->pid);

            if (kill (c->pid, SIGKILL) < 0)
            {
                g_warning ("Failed to kill client id %d: %s", c->pid, strerror (errno));
            }
        }
    }

    clientKill (c);
}

void
clientEnterContextMenuState (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if (FLAG_TEST (c->wm_flags, WM_FLAG_CONTEXT_HELP))
    {
        sendClientMessage (c->screen_info, c->window, NET_WM_CONTEXT_HELP,
                           myDisplayGetCurrentTime (display_info));
    }
}

void
clientSetLayer (Client *c, guint l)
{
    GList *list_of_windows = NULL;
    GList *list = NULL;
    Client *c2 = NULL;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx) on layer %d", c->name, c->window, l);

    list_of_windows = clientListTransientOrModal (c);
    for (list = list_of_windows; list; list = g_list_next (list))
    {
        c2 = (Client *) list->data;
        if (c2->win_layer != l)
        {
            TRACE ("setting client \"%s\" (0x%lx) layer to %d", c2->name,
                c2->window, l);
            c2->win_layer = l;
        }
    }
    g_list_free (list_of_windows);

    if (clientGetLastRaise (c->screen_info) == c)
    {
        clientClearLastRaise (c->screen_info);
    }

    c2 = clientGetFocusOrPending ();
    if (c2 && (c2 != c) && (c2->win_layer == c->win_layer) && clientsHaveOverlap (c, c2))
    {
        TRACE ("placing %s under %s", c->name, c2->name);
        clientLower (c, c2->frame);
    }
    else
    {
       TRACE ("placing %s on top of its layer %lu", c->name, c->win_layer);
       clientRaise (c, None);
    }
}

void
clientShade (Client *c)
{
    XWindowChanges wc;
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    unsigned long mask;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (!CLIENT_HAS_FRAME(c))
    {
        TRACE ("cowardly refusing to shade \"%s\" (0x%lx) because it has no title", c->name, c->window);
        return;
    }
    else if (FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
    {
        TRACE ("client \"%s\" (0x%lx) is already shaded", c->name, c->window);
        return;
    }

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    FLAG_SET (c->flags, CLIENT_FLAG_SHADED);
    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MANAGED))
    {
        mask = (CWWidth | CWHeight);
        if (clientConstrainPos (c, FALSE))
        {
            wc.x = c->x;
            wc.y = c->y;
            mask |= (CWX | CWY);
        }

        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE))
        {
            c->ignore_unmap++;
        }
        /*
         * Shading unmaps the client window. We therefore have to transfer focus to its frame
         * so that focus doesn't return to root. clientSetFocus() will take care of focusing
         * the window frame since the SHADED flag is now set.
         */
        if (c == clientGetFocus ())
        {
            clientSetFocus (screen_info, c, myDisplayGetCurrentTime (display_info), FOCUS_FORCE);
        }

        myDisplayErrorTrapPush (display_info);
        XUnmapWindow (display_info->dpy, c->window);
        myDisplayErrorTrapPopIgnored (display_info);

        wc.width = c->width;
        wc.height = c->height;
        clientConfigure (c, &wc, mask, CFG_FORCE_REDRAW);
    }
    clientSetNetState (c);
}

void
clientUnshade (Client *c)
{
    XWindowChanges wc;
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (!FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
    {
        TRACE ("\"%s\" (0x%lx) is not shaded", c->name, c->window);
        return;
    }

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    FLAG_UNSET (c->flags, CLIENT_FLAG_SHADED);
    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MANAGED))
    {
        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE))
        {
            myDisplayErrorTrapPush (display_info);
            XMapWindow (display_info->dpy, c->window);
            myDisplayErrorTrapPopIgnored (display_info);
        }
        /*
         * Unshading will show the client window, so we need to focus it when unshading.
         */
        if (c == clientGetFocus ())
        {
            clientSetFocus (screen_info, c, myDisplayGetCurrentTime (display_info), FOCUS_FORCE);
        }

        wc.width = c->width;
        wc.height = c->height;
        clientConfigure (c, &wc, CWWidth | CWHeight, CFG_FORCE_REDRAW);
    }
    clientSetNetState (c);
}

void
clientToggleShaded (Client *c)
{
    if (FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
    {
        clientUnshade (c);
    }
    else
    {
        clientShade (c);
    }
}

void
clientStick (Client *c, gboolean include_transients)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Client *c2;
    GList *list_of_windows;
    GList *list;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if (include_transients)
    {
        list_of_windows = clientListTransientOrModal (c);
        for (list = list_of_windows; list; list = g_list_next (list))
        {
            c2 = (Client *) list->data;
            TRACE ("sticking client \"%s\" (0x%lx)", c2->name, c2->window);
            FLAG_SET (c2->flags, CLIENT_FLAG_STICKY);
            setHint (display_info, c2->window, NET_WM_DESKTOP, (unsigned long) ALL_WORKSPACES);
            frameQueueDraw (c2, FALSE);
        }
        g_list_free (list_of_windows);
    }
    else
    {
        TRACE ("sticking client \"%s\" (0x%lx)", c->name, c->window);
        FLAG_SET (c->flags, CLIENT_FLAG_STICKY);
        setHint (display_info, c->window, NET_WM_DESKTOP, (unsigned long) ALL_WORKSPACES);
    }
    clientSetWorkspace (c, screen_info->current_ws, TRUE);
    clientSetNetState (c);
}

void
clientUnstick (Client *c, gboolean include_transients)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Client *c2;
    GList *list_of_windows;
    GList *list;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if (include_transients)
    {
        list_of_windows = clientListTransientOrModal (c);
        for (list = list_of_windows; list; list = g_list_next (list))
        {
            c2 = (Client *) list->data;
            TRACE ("unsticking client \"%s\" (0x%lx)", c2->name, c2->window);
            FLAG_UNSET (c2->flags, CLIENT_FLAG_STICKY);
            setHint (display_info, c2->window, NET_WM_DESKTOP, (unsigned long) screen_info->current_ws);
            frameQueueDraw (c2, FALSE);
        }
        g_list_free (list_of_windows);
    }
    else
    {
        TRACE ("unsticking client \"%s\" (0x%lx)", c->name, c->window);
        FLAG_UNSET (c->flags, CLIENT_FLAG_STICKY);
        setHint (display_info, c->window, NET_WM_DESKTOP, (unsigned long) screen_info->current_ws);
    }
    clientSetWorkspace (c, screen_info->current_ws, TRUE);
    clientSetNetState (c);
}

void
clientToggleSticky (Client *c, gboolean include_transients)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
    {
        clientUnstick (c, include_transients);
    }
    else
    {
        clientStick (c, include_transients);
    }
}

void
clientUpdateFullscreenSize (Client *c)
{
    ScreenInfo *screen_info;
    XWindowChanges wc;
    GdkRectangle monitor, rect;
    int i;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;

    if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN_MONITORS))
        {
            /* Monitor numbering is from Xinerama */
            myScreenGetXineramaMonitorGeometry (screen_info, c->fullscreen_monitors[0], &rect);
            for (i = 1; i < 4; i++)
            {
                myScreenGetXineramaMonitorGeometry (screen_info, c->fullscreen_monitors[i], &monitor);
                gdk_rectangle_union (&rect, &monitor, &rect);
            }
        }
        else
        {
            int cx, cy;

            cx = frameX (c) + (frameWidth (c) / 2);
            cy = frameY (c) + (frameHeight (c) / 2);

            myScreenFindMonitorAtPoint (screen_info, cx, cy, &rect);
        }

        wc.x = rect.x;
        wc.y = rect.y;
        wc.width = rect.width;
        wc.height = rect.height;
    }
    else
    {
        wc.x = c->pre_fullscreen_geometry.x;
        wc.y = c->pre_fullscreen_geometry.y;
        wc.width = c->pre_fullscreen_geometry.width;
        wc.height = c->pre_fullscreen_geometry.height;
    }

    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MANAGED))
    {
        clientConfigure (c, &wc, CWX | CWY | CWWidth | CWHeight, CFG_FORCE_REDRAW);
    }
    else
    {
        c->x = wc.x;
        c->y = wc.y;
        c->height = wc.height;
        c->width = wc.width;
    }
}

void clientToggleFullscreen (Client *c)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    /*can we switch to full screen, does it make any sense? */
    if (!FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN) && (c->size->flags & PMaxSize))
    {
        GdkRectangle rect;
        int cx, cy;

        cx = frameX (c) + (frameWidth (c) / 2);
        cy = frameY (c) + (frameHeight (c) / 2);

        myScreenFindMonitorAtPoint (c->screen_info, cx, cy, &rect);

        if ((c->size->max_width < rect.width) || (c->size->max_height < rect.height))
        {
            return;
        }
    }

    if (!clientIsTransientOrModal (c) && (c->type == WINDOW_NORMAL) && !FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
    {
        FLAG_TOGGLE (c->flags, CLIENT_FLAG_FULLSCREEN);
        clientUpdateFullscreenState (c);
    }
}

void clientSetFullscreenMonitor (Client *c, gint top, gint bottom, gint left, gint right)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    gint num_monitors;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    num_monitors = xfwm_get_n_monitors (screen_info->gscr);
    if ((top >= 0)    && (top < num_monitors)    &&
        (bottom >= 0) && (bottom < num_monitors) &&
        (left >= 0)   && (left < num_monitors)   &&
        (right >= 0)  && (right < num_monitors))
    {
        c->fullscreen_monitors[0] = top;
        c->fullscreen_monitors[1] = bottom;
        c->fullscreen_monitors[2] = left;
        c->fullscreen_monitors[3] = right;
        FLAG_SET (c->flags, CLIENT_FLAG_FULLSCREEN_MONITORS);
    }
    else
    {
        c->fullscreen_monitors[0] = 0;
        c->fullscreen_monitors[1] = 0;
        c->fullscreen_monitors[2] = 0;
        c->fullscreen_monitors[3] = 0;
        FLAG_UNSET (c->flags, CLIENT_FLAG_FULLSCREEN_MONITORS);
    }
    if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        clientUpdateFullscreenSize (c);
    }
    if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN_MONITORS))
    {
        setNetFullscreenMonitors (display_info, c->window, top, bottom, left, right);
    }
}

void clientToggleLayerAbove (Client *c)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if ((c->type & WINDOW_REGULAR_FOCUSABLE) &&
        !clientIsValidTransientOrModal (c) &&
        !FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        FLAG_UNSET (c->flags, CLIENT_FLAG_BELOW);
        FLAG_TOGGLE (c->flags, CLIENT_FLAG_ABOVE);
        clientUpdateLayerState (c);
    }
}

void clientToggleLayerBelow (Client *c)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if ((c->type & WINDOW_REGULAR_FOCUSABLE) &&
        !clientIsValidTransientOrModal (c) &&
        !FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        FLAG_UNSET (c->flags, CLIENT_FLAG_ABOVE);
        FLAG_TOGGLE (c->flags, CLIENT_FLAG_BELOW);
        clientUpdateLayerState (c);
    }
}

void clientSetLayerNormal (Client *c)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (!FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        FLAG_UNSET (c->flags, CLIENT_FLAG_ABOVE | CLIENT_FLAG_BELOW);
        clientUpdateLayerState (c);
    }
}

void
clientUpdateMaximizeSize (Client *c)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    /* Recompute size and position of maximized windows */
    if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED))
    {
        clientRecomputeMaximizeSize (c);
        clientReconfigure (c, CFG_NOTIFY);
    }
}

void
clientRemoveMaximizeFlag (Client *c)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    FLAG_UNSET (c->flags, CLIENT_FLAG_MAXIMIZED | CLIENT_FLAG_RESTORE_SIZE_POS);
    frameQueueDraw (c, FALSE);
    clientSetNetActions (c);
    clientSetNetState (c);
}

static void
clientNewMaxState (Client *c, XWindowChanges *wc, int mode)
{
    if (FLAG_TEST_ALL (mode, CLIENT_FLAG_MAXIMIZED))
    {
        /*
         * We need to test specifically for full de-maximization
         * otherwise it's too confusing when the window changes
         * from horiz to vertical maximization or vice-versa.
         */
        if (FLAG_TEST_ALL (c->flags, CLIENT_FLAG_MAXIMIZED))
        {
            FLAG_UNSET (c->flags, CLIENT_FLAG_MAXIMIZED | CLIENT_FLAG_RESTORE_SIZE_POS);
            wc->x = c->saved_geometry.x;
            wc->y = c->saved_geometry.y;
            wc->width = c->saved_geometry.width;
            wc->height = c->saved_geometry.height;

            return;
        }
        else if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_HORIZ))
        {
            FLAG_SET (c->flags, CLIENT_FLAG_MAXIMIZED_VERT);
            return;
        }
        else if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_VERT))
        {
            FLAG_SET (c->flags, CLIENT_FLAG_MAXIMIZED_HORIZ);
            return;
        }
    }

    if (FLAG_TEST (mode, CLIENT_FLAG_MAXIMIZED_HORIZ))
    {
        if (!FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_HORIZ))
        {
            FLAG_SET (c->flags, CLIENT_FLAG_MAXIMIZED_HORIZ | CLIENT_FLAG_RESTORE_SIZE_POS);
        }
        else
        {
            FLAG_UNSET (c->flags, CLIENT_FLAG_MAXIMIZED_HORIZ);
            if (!FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED))
            {
                FLAG_UNSET (c->flags, CLIENT_FLAG_RESTORE_SIZE_POS);
            }
            wc->x = c->saved_geometry.x;
            wc->y = c->saved_geometry.y;
            wc->width = c->saved_geometry.width;
            wc->height = c->saved_geometry.height;
        }
    }

    if (FLAG_TEST (mode, CLIENT_FLAG_MAXIMIZED_VERT))
    {
        if (!FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_VERT))
        {
            FLAG_SET (c->flags, CLIENT_FLAG_MAXIMIZED_VERT | CLIENT_FLAG_RESTORE_SIZE_POS);
        }
        else
        {
            FLAG_UNSET (c->flags, CLIENT_FLAG_MAXIMIZED_VERT);
            if (!FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED))
            {
                FLAG_UNSET (c->flags, CLIENT_FLAG_RESTORE_SIZE_POS);
            }
            wc->x = c->saved_geometry.x;
            wc->y = c->saved_geometry.y;
            wc->width = c->saved_geometry.width;
            wc->height = c->saved_geometry.height;
        }
    }
}

// @gaps@
#define GAPS_SIZE 15

static gboolean
clientNewTileSize (Client *c, XWindowChanges *wc, GdkRectangle *rect, tilePositionType tile)
{
    ScreenInfo *screen_info;
    int full_x, full_y, full_w, full_h;

    screen_info = c->screen_info;

    full_x = MAX (screen_info->params->xfwm_margins[STRUTS_LEFT], rect->x);
    full_y = MAX (screen_info->params->xfwm_margins[STRUTS_TOP], rect->y);
    full_w = MIN (screen_info->width - screen_info->params->xfwm_margins[STRUTS_RIGHT],
                  rect->x + rect->width) - full_x;
    full_h = MIN (screen_info->height - screen_info->params->xfwm_margins[STRUTS_BOTTOM],
                  rect->y + rect->height) - full_y;
    clientMaxSpace (c, &full_x, &full_y, &full_w, &full_h);

    switch (tile)
    {
        case TILE_UP:
            wc->x = full_x + frameExtentLeft (c);
            wc->y = full_y + frameExtentTop (c);
            wc->width = full_w - frameExtentLeft (c) - frameExtentRight (c);
            wc->height = full_h / 2 - frameExtentTop (c) - frameExtentBottom (c);
            break;
        case TILE_DOWN:
            wc->x = full_x + frameExtentLeft (c);
            wc->y = full_y + full_h / 2 + frameExtentTop (c);
            wc->width = full_w - frameExtentLeft (c) - frameExtentRight (c);
            wc->height = full_h - full_h / 2 - frameExtentTop (c) - frameExtentBottom (c);
            break;
        case TILE_LEFT:
            wc->x = full_x + frameExtentLeft (c);
            wc->y = full_y + frameExtentTop (c);
            wc->width = full_w / 2 - frameExtentLeft (c) - frameExtentRight (c);
            wc->height = full_h - frameExtentTop (c) - frameExtentBottom (c);

            wc->x += GAPS_SIZE;
            wc->y += GAPS_SIZE;
            wc->width -= GAPS_SIZE + (GAPS_SIZE / 2);
            wc->height -= GAPS_SIZE * 2;

            break;
        case TILE_RIGHT:
            wc->x = full_x + full_w / 2 + frameExtentLeft (c);
            wc->y = full_y + frameExtentTop (c);
            wc->width = full_w - full_w / 2 - frameExtentLeft (c) - frameExtentRight (c);
            wc->height = full_h - frameExtentTop (c) - frameExtentBottom (c);

            wc->x += GAPS_SIZE / 2;
            wc->y += GAPS_SIZE;
            wc->width -= GAPS_SIZE + (GAPS_SIZE / 2);
            wc->height -= GAPS_SIZE * 2;

            break;
        case TILE_DOWN_LEFT:
            wc->x = full_x + frameExtentLeft (c);
            wc->y = full_y + full_h / 2 + frameExtentTop (c);
            wc->width = full_w / 2 - frameExtentLeft (c) - frameExtentRight (c);
            wc->height = full_h - full_h / 2 - frameExtentTop (c) - frameExtentBottom (c);

            wc->x += GAPS_SIZE;
            wc->y += GAPS_SIZE / 2;
            wc->width -= GAPS_SIZE + (GAPS_SIZE / 2);
            wc->height -= GAPS_SIZE + (GAPS_SIZE / 2);

            break;
        case TILE_DOWN_RIGHT:
            wc->x = full_x + full_w /2 + frameExtentLeft (c);
            wc->y = full_y + full_h / 2 + frameExtentTop (c);
            wc->width = full_w - full_w / 2 - frameExtentLeft (c) - frameExtentRight (c);
            wc->height = full_h - full_h / 2 - frameExtentTop (c) - frameExtentBottom (c);

            wc->x += GAPS_SIZE / 2;
            wc->y += GAPS_SIZE / 2;
            wc->width -= GAPS_SIZE + (GAPS_SIZE / 2);
            wc->height -= GAPS_SIZE + (GAPS_SIZE / 2);

            break;
        case TILE_UP_LEFT:
            wc->x = full_x + frameExtentLeft (c);
            wc->y = full_y + frameExtentTop (c);
            wc->width = full_w / 2 - frameExtentLeft (c) - frameExtentRight (c);
            wc->height = full_h / 2 - frameExtentTop (c) - frameExtentBottom (c);

            wc->x += GAPS_SIZE;
            wc->y += GAPS_SIZE;
            wc->width -= GAPS_SIZE + (GAPS_SIZE / 2);
            wc->height -= GAPS_SIZE + (GAPS_SIZE / 2);

            break;
        case TILE_UP_RIGHT:
            wc->x = full_x + full_w /2 + frameExtentLeft (c);
            wc->y = full_y + frameExtentTop (c);
            wc->width = full_w - full_w / 2 - frameExtentLeft (c) - frameExtentRight (c);
            wc->height = full_h / 2 - frameExtentTop (c) - frameExtentBottom (c);

            wc->x += GAPS_SIZE / 2;
            wc->y += GAPS_SIZE;
            wc->width -= GAPS_SIZE + (GAPS_SIZE / 2);
            wc->height -= GAPS_SIZE + (GAPS_SIZE / 2);

            break;
        default:
            break;
    }

    return ((wc->height >= c->size->min_height) && (wc->height <= c->size->max_height) &&
            (wc->width >= c->size->min_width) && (wc->width <= c->size->max_width));
}

static gboolean
clientNewMaxSize (Client *c, XWindowChanges *wc, GdkRectangle *rect)
{
    ScreenInfo *screen_info;
    int full_x, full_y, full_w, full_h;

    screen_info = c->screen_info;

    full_x = MAX (screen_info->params->xfwm_margins[STRUTS_LEFT], rect->x);
    full_y = MAX (screen_info->params->xfwm_margins[STRUTS_TOP], rect->y);
    full_w = MIN (screen_info->width - screen_info->params->xfwm_margins[STRUTS_RIGHT],
                  rect->x + rect->width) - full_x;
    full_h = MIN (screen_info->height - screen_info->params->xfwm_margins[STRUTS_BOTTOM],
                  rect->y + rect->height) - full_y;
    clientMaxSpace (c, &full_x, &full_y, &full_w, &full_h);

    if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_HORIZ))
    {
        /* Adjust size to the widest size available, for the current vertical position/height */
        wc->x = full_x + frameExtentLeft (c);
        wc->width = full_w - frameExtentLeft (c) - frameExtentRight (c);
    }

    if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_VERT))
    {
        /* Adjust size to the tallest size available, for the current horizontal position/width */
        wc->y = full_y + frameExtentTop (c);
        wc->height = full_h - frameExtentTop (c) - frameExtentBottom (c);
    }

    wc->x += GAPS_SIZE;
    wc->y += GAPS_SIZE;
    wc->width -= GAPS_SIZE * 2;
    wc->height -= GAPS_SIZE * 2;

    return ((wc->height >= c->size->min_height) && (wc->height <= c->size->max_height) &&
            (wc->width >= c->size->min_width) && (wc->width <= c->size->max_width));
}

gboolean
clientToggleMaximized (Client *c, int mode, gboolean restore_position)
{
    g_return_val_if_fail (c != NULL, FALSE);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (!CLIENT_CAN_MAXIMIZE_WINDOW (c))
    {
        return FALSE;
    }

    return clientToggleMaximizedAtPoint(c,
                                frameX (c) + (frameWidth (c) / 2),
                                frameY (c) + (frameHeight (c) / 2),
                                mode, restore_position);
}

gboolean
clientToggleMaximizedAtPoint (Client *c, gint cx, gint cy, int mode, gboolean restore_position)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;
    XWindowChanges wc;
    GdkRectangle rect;
    unsigned long old_flags;

    g_return_val_if_fail (c != NULL, FALSE);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (!CLIENT_CAN_MAXIMIZE_WINDOW (c))
    {
        return FALSE;
    }

    if (c->tile_mode != TILE_NONE)
    {
        clientUntile (c);
    }

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    myScreenFindMonitorAtPoint (screen_info, cx, cy, &rect);

    wc.x = c->x;
    wc.y = c->y;
    wc.width = c->width;
    wc.height = c->height;

    if (restore_position &&
        FLAG_TEST (mode, CLIENT_FLAG_MAXIMIZED))
    {
        clientSaveSizePos (c);
    }

    old_flags = c->flags;

    /* 1) Compute the new state */
    clientNewMaxState (c, &wc, mode);

    /* 2) Compute the new size, based on the state */
    if (!clientNewMaxSize (c, &wc, &rect))
    {
        c->flags = old_flags;
        return FALSE;
    }

    /* 3) Update size and position fields */
    c->x = wc.x;
    c->y = wc.y;
    c->height = wc.height;
    c->width = wc.width;

    /* Maximizing may remove decoration on the side, update NET_FRAME_EXTENTS accordingly */
    setNetFrameExtents (display_info,
                        c->window,
                        frameTop (c),
                        frameLeft (c),
                        frameRight (c),
                        frameBottom (c));

    /* Maximized windows w/out border cannot be resized, update allowed actions */
    clientSetNetActions (c);
    if (restore_position && FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MANAGED))
    {
        if (FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
        {
            /* It's a shame, we are configuring the same client twice in a row */
            clientUnshade (c);
        }
        clientConfigure (c, &wc, CWWidth | CWHeight | CWX | CWY, CFG_FORCE_REDRAW);
    }
    /* Do not update the state while moving/resizing, CSD windows may resize */
    if (!FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING))
    {
        clientSetNetState (c);
    }

    return TRUE;
}

gboolean
clientTile (Client *c, gint cx, gint cy, tilePositionType tile, gboolean send_configure, gboolean restore_position)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;
    XWindowChanges wc;
    GdkRectangle rect;
    unsigned long old_flags;

    g_return_val_if_fail (c != NULL, FALSE);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (!CLIENT_CAN_TILE_WINDOW (c))
    {
        return FALSE;
    }

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    myScreenFindMonitorAtPoint (screen_info, cx, cy, &rect);

    wc.x = c->x;
    wc.y = c->y;
    wc.width = c->width;
    wc.height = c->height;

    if (restore_position)
    {
        clientSaveSizePos (c);
    }

    old_flags = c->flags;
    FLAG_UNSET (c->flags, CLIENT_FLAG_MAXIMIZED);
    if (!clientNewTileSize (c, &wc, &rect, tile))
    {
        c->flags = old_flags;
        return FALSE;
    }
    FLAG_SET (c->flags, CLIENT_FLAG_RESTORE_SIZE_POS);
    c->tile_mode = tile;

    c->x = wc.x;
    c->y = wc.y;
    c->height = wc.height;
    c->width = wc.width;

    if (send_configure)
    {
        setNetFrameExtents (display_info,
                            c->window,
                            frameTop (c),
                            frameLeft (c),
                            frameRight (c),
                            frameBottom (c));

        clientSetNetActions (c);
        clientReconfigure (c, CFG_FORCE_REDRAW);
    }
    /* Do not update the state while moving/resizing, CSD windows may resize */
    if (!FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING))
    {
        clientSetNetState (c);
    }

    return TRUE;
}

void
clientUntile (Client *c)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    c->tile_mode = TILE_NONE;
}

gboolean
clientToggleTile (Client *c, tilePositionType tile)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;

    g_return_val_if_fail (c != NULL, FALSE);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if (c->tile_mode == tile)
    {
        clientUntile (c);
        clientRestoreSizePos (c);
        setNetFrameExtents (display_info,
                            c->window,
                            frameTop (c),
                            frameLeft (c),
                            frameRight (c),
                            frameBottom (c));

        clientSetNetActions (c);
        clientReconfigure (c, CFG_FORCE_REDRAW);

        return TRUE;
    }
    else
    {
        return clientTile (c,
                           frameX (c) + frameWidth (c) / 2,
                           frameY (c) + frameHeight (c) / 2,
                           tile,
                           TRUE,
                           TRUE);
    }
}


static void
clientRecomputeTileSize (Client *c)
{
    ScreenInfo *screen_info;
    XWindowChanges wc;
    GdkRectangle rect;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;

    myScreenFindMonitorAtPoint (screen_info,
                                frameX (c) + frameWidth (c) / 2,
                                frameY (c) + frameHeight (c) / 2,
                                &rect);

    if (!clientNewTileSize (c, &wc, &rect, c->tile_mode))
    {
        return;
    }

    c->x = wc.x;
    c->y = wc.y;
    c->width = wc.width;
    c->height = wc.height;
}

void
clientUpdateTileSize (Client *c)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    /* Recompute size and position of maximized windows */
    if (c->tile_mode != TILE_NONE)
    {
        clientRecomputeTileSize (c);
        clientReconfigure (c, CFG_NOTIFY);
    }
}

void
clientUpdateOpacity (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Client *focused;
    gboolean opaque;

    g_return_if_fail (c != NULL);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    if (!compositorIsUsable (display_info))
    {
        return;
    }

    focused = clientGetFocus ();
    opaque = (FLAG_TEST(c->type, WINDOW_TYPE_DONT_PLACE | WINDOW_TYPE_DONT_FOCUS)
              || (focused == c));

    clientSetOpacity (c, c->opacity, OPACITY_INACTIVE, opaque ? 0 : OPACITY_INACTIVE);
}

void
clientUpdateAllOpacity (ScreenInfo *screen_info)
{
    DisplayInfo *display_info;
    Client *c;
    guint i;

    g_return_if_fail (screen_info != NULL);

    display_info = screen_info->display_info;
    if (!compositorIsUsable (display_info))
    {
        return;
    }

    for (c = screen_info->clients, i = 0; i < screen_info->client_count; c = c->next, ++i)
    {
        clientUpdateOpacity (c);
    }
}

void
clientSetOpacity (Client *c, guint32 opacity, guint32 clear, guint32 xor)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    guint32 applied;

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if (!compositorIsUsable (display_info))
    {
        return;
    }

    c->opacity_flags = (c->opacity_flags & ~clear) ^ xor;

    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_OPACITY_LOCKED))
    {
        applied = c->opacity;
    }
    else
    {
        long long multiplier = 1, divisor = 1;

        c->opacity = applied = opacity;

        if (FLAG_TEST (c->opacity_flags, OPACITY_MOVE))
        {
            multiplier *= c->screen_info->params->move_opacity;
            divisor *= 100;
        }
        if (FLAG_TEST (c->opacity_flags, OPACITY_RESIZE))
        {
            multiplier *= c->screen_info->params->resize_opacity;
            divisor *= 100;
        }
        if (FLAG_TEST (c->opacity_flags, OPACITY_INACTIVE))
        {
            multiplier *= c->screen_info->params->inactive_opacity;
            divisor *= 100;
        }

        applied = (guint32) (((long long) applied * multiplier / divisor) & G_MAXUINT32);
    }

    if (applied != c->opacity_applied)
    {
        c->opacity_applied = applied;
        compositorWindowSetOpacity (display_info, c->frame, applied);
    }
}

void
clientDecOpacity (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if (!compositorIsUsable (display_info))
    {
        return;
    }

    if ((c->opacity > OPACITY_SET_MIN) && !(FLAG_TEST (c->xfwm_flags, XFWM_FLAG_OPACITY_LOCKED)))
    {
         clientSetOpacity (c, c->opacity - OPACITY_SET_STEP, 0, 0);
    }
}

void
clientIncOpacity (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if (!compositorIsUsable (display_info))
    {
        return;
    }

    if ((c->opacity < NET_WM_OPAQUE) && !(FLAG_TEST (c->xfwm_flags, XFWM_FLAG_OPACITY_LOCKED)))
    {
         guint opacity = c->opacity + OPACITY_SET_STEP;

         if (opacity < OPACITY_SET_MIN)
         {
             opacity = NET_WM_OPAQUE;
         }
         clientSetOpacity (c, opacity, 0, 0);
    }
}

/* Xrandr stuff: on screen size change, make sure all clients are still visible */
void
clientScreenResize(ScreenInfo *screen_info, gboolean fully_visible)
{
    Client *c = NULL;
    GList *list, *list_of_windows;
    XWindowChanges wc;
    unsigned short configure_flags;

    list_of_windows = clientGetStackList (screen_info);

    if (!list_of_windows)
    {
        return;
    }

    /* Revalidate client struts */
    for (list = list_of_windows; list; list = g_list_next (list))
    {
        c = (Client *) list->data;
        if (FLAG_TEST (c->flags, CLIENT_FLAG_HAS_STRUT))
        {
            clientValidateNetStrut (c);
        }
    }

    for (list = list_of_windows; list; list = g_list_next (list))
    {
        c = (Client *) list->data;
        if (!CONSTRAINED_WINDOW (c))
        {
            continue;
        }

        if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
        {
            clientUpdateFullscreenSize (c);
        }
        else if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED))
        {
            clientUpdateMaximizeSize (c);
        }
        else if (c->tile_mode != TILE_NONE)
        {
            clientUpdateTileSize (c);
        }
        else
        {
            configure_flags = CFG_CONSTRAINED | CFG_REQUEST;
            if (fully_visible)
            {
                configure_flags |= CFG_KEEP_VISIBLE;
            }
            if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_SAVED_POS))
            {
                wc.x = c->pre_relayout_x;
                wc.y = c->pre_relayout_y;
            }
            else
            {
                FLAG_SET (c->xfwm_flags, XFWM_FLAG_SAVED_POS);

                c->pre_relayout_x = c->x;
                c->pre_relayout_y = c->y;

                wc.x = c->x;
                wc.y = c->y;
            }

            clientConfigure (c, &wc, CWX | CWY, configure_flags);
        }
    }

    g_list_free (list_of_windows);
}

void
clientUpdateCursor (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    guint i;

    g_return_if_fail (c != NULL);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    for (i = 0; i < SIDE_COUNT; i++)
    {
        xfwmWindowSetCursor (&c->sides[i],
            myDisplayGetCursorResize(display_info, CORNER_COUNT + i));
    }

    for (i = 0; i < CORNER_COUNT; i++)
    {
        xfwmWindowSetCursor (&c->corners[i],
            myDisplayGetCursorResize(display_info, i));
    }
}

void
clientUpdateAllCursor (ScreenInfo *screen_info)
{
    Client *c;
    guint i;

    g_return_if_fail (screen_info != NULL);

    for (c = screen_info->clients, i = 0; i < screen_info->client_count; c = c->next, ++i)
    {
        clientUpdateCursor (c);
    }
}

static eventFilterStatus
clientButtonPressEventFilter (XfwmEvent *event, gpointer data)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Client *c;
    ButtonPressData *passdata;
    eventFilterStatus status;
    int b;
    gboolean pressed;

    passdata = (ButtonPressData *) data;
    c = passdata->c;
    b = passdata->b;

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    /* Update the display time */
    myDisplayUpdateCurrentTime (display_info, event);

    status = EVENT_FILTER_STOP;
    pressed = TRUE;

    switch (event->meta.type)
    {
        case XFWM_EVENT_BUTTON:
            if (!event->button.pressed)
            {
                pressed = FALSE;
            }
            break;
        case XFWM_EVENT_CROSSING:
            if ((event->crossing.mode == NotifyGrab) ||
                (event->crossing.mode == NotifyUngrab) ||
                (event->meta.window != MYWINDOW_XWINDOW (c->buttons[b])))
            {
                break;
            }
            if (event->crossing.enter)
            {
                c->button_status[b] = BUTTON_STATE_PRESSED;
                frameQueueDraw (c, FALSE);
            }
            else
            {
                c->button_status[b] = BUTTON_STATE_NORMAL;
                frameQueueDraw (c, FALSE);
            }
            break;
        case XFWM_EVENT_XEVENT:
            if ((event->meta.xevent->type == UnmapNotify) && (event->meta.window == c->window))
            {
                pressed = FALSE;
                c->button_status[b] = BUTTON_STATE_NORMAL;
            }
            else
            {
                status = EVENT_FILTER_CONTINUE;
            }
            break;
        default:
            break;
    }

    if (!pressed)
    {
        TRACE ("event loop now finished");
        gtk_main_quit ();
    }

    return status;
}

void
clientButtonPress (Client *c, Window w, XfwmEventButton *event)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    ButtonPressData passdata;
    gint b;
    gboolean g1;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    for (b = 0; b < BUTTON_COUNT; b++)
    {
        if (MYWINDOW_XWINDOW (c->buttons[b]) == w)
        {
            break;
        }
    }

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    myDisplayErrorTrapPush (display_info);
    g1 = xfwm_device_grab (display_info->devices, &display_info->devices->pointer,
                           display_info->dpy, w, FALSE,
                           ButtonReleaseMask | EnterWindowMask | LeaveWindowMask,
                           GrabModeAsync, screen_info->xroot, None,
                           myDisplayGetCurrentTime (display_info));
    myDisplayErrorTrapPopIgnored (display_info);

    if (!g1)
    {
        TRACE ("grab failed in clientButtonPress");
        myDisplayBeep (display_info);
        return;
    }

    passdata.c = c;
    passdata.b = b;

    c->button_status[b] = BUTTON_STATE_PRESSED;
    frameQueueDraw (c, FALSE);

    TRACE ("entering button press loop");
    eventFilterPush (display_info->xfilter, clientButtonPressEventFilter, &passdata);
    gtk_main ();
    eventFilterPop (display_info->xfilter);
    TRACE ("leaving button press loop");

    myDisplayErrorTrapPush (display_info);
    xfwm_device_ungrab (display_info->devices, &display_info->devices->pointer,
                        display_info->dpy, myDisplayGetCurrentTime (display_info));
    myDisplayErrorTrapPopIgnored (display_info);

    if (c->button_status[b] == BUTTON_STATE_PRESSED)
    {
        /*
         * Button was pressed at the time, means the pointer was still within
         * the button, so return to prelight if available, normal otherwise.
         */
        if (!xfwmPixmapNone(clientGetButtonPixmap(c, b, PRELIGHT)))
        {
            c->button_status[b] = BUTTON_STATE_PRELIGHT;
        }
        else
        {
            c->button_status[b] = BUTTON_STATE_NORMAL;
        }

        switch (b)
        {
            case HIDE_BUTTON:
                if (CLIENT_CAN_HIDE_WINDOW (c))
                {
                    clientWithdraw (c, c->win_workspace, TRUE);
                }
                break;
            case CLOSE_BUTTON:
                clientClose (c);
                break;
            case MAXIMIZE_BUTTON:
                if (CLIENT_CAN_MAXIMIZE_WINDOW (c))
                {
                    if (event->button == Button1)
                    {
                        clientToggleMaximized (c, CLIENT_FLAG_MAXIMIZED, TRUE);
                    }
                    else if (event->button == Button2)
                    {
                        clientToggleMaximized (c, CLIENT_FLAG_MAXIMIZED_VERT, TRUE);
                    }
                    else if (event->button == Button3)
                    {
                        clientToggleMaximized (c, CLIENT_FLAG_MAXIMIZED_HORIZ, TRUE);
                    }
                }
                break;
            case SHADE_BUTTON:
                clientToggleShaded (c);
                break;
            case STICK_BUTTON:
                clientToggleSticky (c, TRUE);
                break;
            default:
                break;
        }
        frameQueueDraw (c, FALSE);
    }
}

xfwmPixmap *
clientGetButtonPixmap (Client *c, int button, int state)
{
    ScreenInfo *screen_info;

    TRACE ("button=%i, state=%i", button, state);
    screen_info = c->screen_info;
    switch (button)
    {
        case MENU_BUTTON:
            if ((screen_info->params->show_app_icon)
                && (!xfwmPixmapNone(&c->appmenu[state])))
            {
                return &c->appmenu[state];
            }
            break;
        case SHADE_BUTTON:
            if (FLAG_TEST (c->flags, CLIENT_FLAG_SHADED)
                && (!xfwmPixmapNone(&screen_info->buttons[SHADE_BUTTON][state + STATE_TOGGLED])))
            {
                return &screen_info->buttons[SHADE_BUTTON][state + STATE_TOGGLED];
            }
            return &screen_info->buttons[SHADE_BUTTON][state];
            break;
        case STICK_BUTTON:
            if (FLAG_TEST (c->flags, CLIENT_FLAG_STICKY)
                && (!xfwmPixmapNone(&screen_info->buttons[STICK_BUTTON][state + STATE_TOGGLED])))
            {
                return &screen_info->buttons[STICK_BUTTON][state + STATE_TOGGLED];
            }
            return &screen_info->buttons[STICK_BUTTON][state];
            break;
        case MAXIMIZE_BUTTON:
            if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED)
                && (!xfwmPixmapNone(&screen_info->buttons[MAXIMIZE_BUTTON][state + STATE_TOGGLED])))
            {
                return &screen_info->buttons[MAXIMIZE_BUTTON][state + STATE_TOGGLED];
            }
            return &screen_info->buttons[MAXIMIZE_BUTTON][state];
            break;
        default:
            break;
    }
    return &screen_info->buttons[button][state];
}

int
clientGetButtonState (Client *c, int button, int state)
{
    if (state == INACTIVE)
    {
        return (state);
    }

    if ((c->button_status[button] == BUTTON_STATE_PRESSED) &&
        clientGetButtonPixmap (c, button, PRESSED))
    {
        return (PRESSED);
    }

    if ((c->button_status[button] == BUTTON_STATE_PRELIGHT) &&
        clientGetButtonPixmap (c, button, PRELIGHT))
    {
        return (PRELIGHT);
    }

    return (ACTIVE);
}


Client *
clientGetLeader (Client *c)
{
    g_return_val_if_fail (c != NULL, NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (c->group_leader != None)
    {
        return myScreenGetClientFromWindow (c->screen_info, c->group_leader, SEARCH_WINDOW);
    }
    else if (c->client_leader != None)
    {
        return myScreenGetClientFromWindow (c->screen_info, c->client_leader, SEARCH_WINDOW);
    }
    return NULL;
}

gboolean
clientGetGtkFrameExtents (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    gboolean value_changed = FALSE;
    gulong *extents;
    unsigned long old_value;
    int nitems;
    int i;

    g_return_val_if_fail (c != NULL, FALSE);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    extents = NULL;

    old_value = FLAG_TEST (c->flags, CLIENT_FLAG_HAS_FRAME_EXTENTS);
    FLAG_UNSET (c->flags, CLIENT_FLAG_HAS_FRAME_EXTENTS);

    if (getCardinalList (display_info, c->window, GTK_FRAME_EXTENTS, &extents, &nitems))
    {
        if (nitems == SIDE_COUNT)
        {
            FLAG_SET (c->flags, CLIENT_FLAG_HAS_FRAME_EXTENTS);
            for (i = 0; i < SIDE_COUNT; i++)
            {
                if (c->frame_extents[i] != (int) extents[i])
                {
                    value_changed = TRUE;
                    c->frame_extents[i] = (int) extents[i];
                }
            }
        }
    }

    if (extents)
    {
        XFree (extents);
    }

    /* Adding or removing the property also counts as a change */
    value_changed |= (old_value ^ FLAG_TEST (c->flags, CLIENT_FLAG_HAS_FRAME_EXTENTS));

    return value_changed;
}

gboolean
clientGetGtkHideTitlebar (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    unsigned long old_value;
    long val;

    g_return_val_if_fail (c != NULL, FALSE);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    old_value = FLAG_TEST (c->flags, CLIENT_FLAG_HIDE_TITLEBAR);
    FLAG_UNSET (c->flags, CLIENT_FLAG_HIDE_TITLEBAR);

    if (getHint (display_info, c->window, GTK_HIDE_TITLEBAR_WHEN_MAXIMIZED, &val) &&( val != 0))
    {
        FLAG_SET (c->flags, CLIENT_FLAG_HIDE_TITLEBAR);
    }
    return old_value != FLAG_TEST (c->flags, CLIENT_FLAG_HIDE_TITLEBAR);
}

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
char *
clientGetStartupId (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    gboolean got_startup_id;

    g_return_val_if_fail (c != NULL, NULL);
    g_return_val_if_fail (c->window != None, NULL);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if (c->startup_id)
    {
        return (c->startup_id);
    }

    got_startup_id = getWindowStartupId (display_info, c->window, &c->startup_id);

    if (!got_startup_id && (c->client_leader))
    {
        got_startup_id = getWindowStartupId (display_info, c->client_leader, &c->startup_id);
    }

    if (!got_startup_id && (c->group_leader))
    {
        getWindowStartupId (display_info, c->group_leader, &c->startup_id);
    }

    return (c->startup_id);
}
#endif /* HAVE_LIBSTARTUP_NOTIFICATION */
