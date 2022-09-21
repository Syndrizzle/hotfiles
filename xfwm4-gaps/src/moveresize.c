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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/extensions/shape.h>

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "client.h"
#include "compositor.h"
#include "focus.h"
#include "frame.h"
#include "moveresize.h"
#include "netwm.h"
#include "placement.h"
#include "poswin.h"
#include "screen.h"
#include "settings.h"
#include "transients.h"
#include "event_filter.h"
#include "wireframe.h"
#include "workspaces.h"
#include "xsync.h"

#define MOVERESIZE_POINTER_EVENT_MASK \
    PointerMotionMask | \
    ButtonMotionMask | \
    ButtonReleaseMask | \
    LeaveWindowMask

#define MOVERESIZE_KEYBOARD_EVENT_MASK \
    KeyPressMask

#define TILE_DISTANCE 10
#define BORDER_TILE_LENGTH_RELATIVE 5
#define use_xor_move(screen_info) (screen_info->params->box_move && !compositorIsActive (screen_info))
#define use_xor_resize(screen_info) (screen_info->params->box_resize && !compositorIsActive (screen_info))

typedef struct _MoveResizeData MoveResizeData;
struct _MoveResizeData
{
    Client *c;
    WireFrame *wireframe;
    gboolean use_keys;
    gboolean grab;
    gboolean is_transient;
    gboolean move_resized;
    gboolean released;
    gboolean client_gone;
    guint button;
    gint cancel_x, cancel_y;
    gint cancel_w, cancel_h;
    unsigned long cancel_flags;
    unsigned long configure_flags;
    guint cancel_workspace;
    gint mx, my;
    double pxratio, pyratio; /* pointer relative position ratio */
    gint ox, oy;
    gint ow, oh;
    gint oldw, oldh;
    gint handle;
    Poswin *poswin;
};

static int
clientCheckSize (Client * c, int size, int base, int min, int max, int incr, gboolean source_is_application)
{
    int size_return;

    g_return_val_if_fail (c != NULL, size);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    size_return = size;

    /* Bypass resize increment and max sizes for fullscreen and maximized */
    if (!FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN)
        && !(FLAG_TEST_ALL (c->flags, CLIENT_FLAG_MAXIMIZED)
             && (c->screen_info->params->borderless_maximize)))
    {

        if (!source_is_application && (c->size->flags & PResizeInc) && (incr))
        {
            int a;
            int b = 0;

            if (c->size->flags & PBaseSize)
            {
                b = base;
            }

            a = (size_return - b) / incr;
            size_return = b + (a * incr);
        }
        if (c->size->flags & PMaxSize)
        {
            if (size_return > max)
            {
                size_return = max;
            }
        }
    }

    if (c->size->flags & PMinSize)
    {
        if (size_return < min)
        {
            size_return = min;
        }
    }
    if (size_return < 1)
    {
        size_return = 1;
    }
    return size_return;
}

int
clientCheckWidth (Client * c, int w, gboolean source_is_application)
{
    g_return_val_if_fail (c != NULL, w);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    return clientCheckSize (c, w,
                            c->size->base_width,
                            c->size->min_width,
                            c->size->max_width,
                            c->size->width_inc,
                            source_is_application);
}

int
clientCheckHeight (Client * c, int h, gboolean source_is_application)
{
    g_return_val_if_fail (c != NULL, h);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    return clientCheckSize (c, h,
                            c->size->base_height,
                            c->size->min_height,
                            c->size->max_height,
                            c->size->height_inc,
                            source_is_application);
}

static void
clientMovePointer (DisplayInfo *display_info, gint dx, gint dy, guint repeat)
{
    guint i;
    for (i = 0; i < repeat; ++i)
    {
        XWarpPointer (display_info->dpy, None, None, 0, 0, 0, 0, dx, dy);
    }
}

static gboolean
clientKeyPressIsModifier (XfwmEventKey *event)
{
    int keysym = XkbKeycodeToKeysym (event->meta.xevent->xany.display, event->keycode, 0, 0);
    return (gboolean) IsModifierKey(keysym);
}

static void
clientSetHandle(MoveResizeData *passdata, int handle)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Client *c;
    int px, py;

    c = passdata->c;
    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    switch (handle)
    {
        case CORNER_BOTTOM_LEFT:
            px = frameExtentX (c) + frameExtentLeft(c) / 2;
            py = frameExtentY (c) + frameExtentHeight (c) - frameExtentBottom(c) / 2;
            break;
        case CORNER_BOTTOM_RIGHT:
            px = frameExtentX (c) + frameExtentWidth (c) - frameExtentRight(c) / 2;
            py = frameExtentY (c) + frameExtentHeight (c) - frameExtentBottom(c) / 2;
            break;
        case CORNER_TOP_LEFT:
            px = frameExtentX (c) + frameExtentLeft(c) / 2;
            py = frameExtentY (c);
            break;
        case CORNER_TOP_RIGHT:
            px = frameExtentX (c) + frameExtentWidth (c) - frameExtentRight(c) / 2;
            py = frameExtentY (c);
            break;
        case CORNER_COUNT + SIDE_LEFT:
            px = frameExtentX (c) + frameExtentLeft(c) / 2;
            py = frameExtentY (c) + frameExtentHeight (c) / 2;
            break;
        case CORNER_COUNT + SIDE_RIGHT:
            px = frameExtentX (c) + frameExtentWidth (c) - frameExtentRight(c) / 2;
            py = frameExtentY (c) + frameExtentHeight (c) / 2;
            break;
        case CORNER_COUNT + SIDE_TOP:
            px = frameExtentX (c) + frameExtentWidth (c) / 2;
            py = frameExtentY (c);
            break;
        case CORNER_COUNT + SIDE_BOTTOM:
            px = frameExtentX (c) + frameExtentWidth (c) / 2;
            py = frameExtentY (c) + frameExtentHeight (c) - frameExtentBottom(c) / 2;
            break;
        default:
            px = frameExtentX (c) + frameExtentWidth (c) / 2;
            py = frameExtentY (c) + frameExtentHeight (c) / 2;
            break;
    }

    XWarpPointer (display_info->dpy, None, screen_info->xroot, 0, 0, 0, 0, px, py);
    /* Update internal data */
    passdata->handle = handle;
    passdata->mx = px;
    passdata->my = py;
    passdata->pxratio = (passdata->mx - frameExtentX (c)) / (double) frameExtentWidth (c);
    passdata->pyratio = (passdata->my - frameExtentY (c)) / (double) frameExtentHeight (c);
    passdata->ox = c->x;
    passdata->oy = c->y;
    passdata->ow = c->width;
    passdata->oh = c->height;
}

/* clientConstrainRatio - adjust the given width and height to account for
   the constraints imposed by size hints

   The aspect ratio stuff, is borrowed from uwm's CheckConsistency routine.
 */

#define MAKE_MULT(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
static void
clientConstrainRatio (Client * c, int handle)
{

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (c->size->flags & PAspect)
    {
        int xinc, yinc, minx, miny, maxx, maxy, delta;

        xinc = c->size->width_inc;
        yinc = c->size->height_inc;
        minx = c->size->min_aspect.x;
        miny = c->size->min_aspect.y;
        maxx = c->size->max_aspect.x;
        maxy = c->size->max_aspect.y;

        if ((minx * c->height > miny * c->width) && (miny) &&
            ((handle == CORNER_COUNT + SIDE_TOP) || (handle == CORNER_COUNT + SIDE_BOTTOM)))
        {
            /* Change width to match */
            delta = MAKE_MULT (minx * c->height /  miny - c->width, xinc);
            if (!(c->size->flags & PMaxSize) ||
                (c->width + delta <= c->size->max_width))
            {
                c->width += delta;
            }
        }
        if ((minx * c->height > miny * c->width) && (minx))
        {
            delta = MAKE_MULT (c->height - c->width * miny / minx, yinc);
            if (!(c->size->flags & PMinSize) ||
                (c->height - delta >= c->size->min_height))
            {
                c->height -= delta;
            }
            else
            {
                delta = MAKE_MULT (minx * c->height / miny - c->width, xinc);
                if (!(c->size->flags & PMaxSize) ||
                    (c->width + delta <= c->size->max_width))
                  c->width += delta;
            }
        }

        if ((maxx * c->height < maxy * c->width) && (maxx) &&
            ((handle == CORNER_COUNT + SIDE_LEFT) || (handle == CORNER_COUNT + SIDE_RIGHT)))
        {
            delta = MAKE_MULT (c->width * maxy / maxx - c->height, yinc);
            if (!(c->size->flags & PMaxSize) ||
                (c->height + delta <= c->size->max_height))
            {
                c->height += delta;
            }
        }
        if ((maxx * c->height < maxy * c->width) && (maxy))
        {
            delta = MAKE_MULT (c->width - maxx * c->height / maxy, xinc);
            if (!(c->size->flags & PMinSize) ||
                (c->width - delta >= c->size->min_width))
            {
                c->width -= delta;
            }
            else
            {
                delta = MAKE_MULT (c->width * maxy / maxx - c->height, yinc);
                if (!(c->size->flags & PMaxSize) ||
                    (c->height + delta <= c->size->max_height))
                {
                    c->height += delta;
                }
            }
        }
    }
}

static void
clientDrawOutline (Client * c)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    XDrawRectangle (clientGetXDisplay (c), c->screen_info->xroot, c->screen_info->box_gc, frameExtentX (c), frameExtentY (c),
        frameExtentWidth (c) - 1, frameExtentHeight (c) - 1);
    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_BORDER)
        &&!FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN | CLIENT_FLAG_SHADED))
    {
        XDrawRectangle (clientGetXDisplay (c), c->screen_info->xroot, c->screen_info->box_gc, c->x, c->y, c->width - 1,
            c->height - 1);
    }
}

static gboolean
clientCheckOverlap (int s1, int e1, int s2, int e2)
{
    /* Simple overlap test for an arbitary axis. -Cliff */
    if ((s1 >= s2 && s1 <= e2) ||
        (e1 >= s2 && e1 <= e2) ||
        (s2 >= s1 && s2 <= e1) ||
        (e2 >= s1 && e2 <= e1))
    {
        return TRUE;
    }
    return FALSE;
}

static int
clientFindClosestEdgeX (Client *c, int edge_pos)
{
    /* Find the closest edge of anything that we can snap to, taking
       frames into account, or just return the original value if nothing
       is within the snapping range. -Cliff */

    Client *c2;
    ScreenInfo *screen_info;
    guint i;
    int snap_width, closest;

    screen_info = c->screen_info;
    snap_width = screen_info->params->snap_width;
    closest = edge_pos + snap_width + 2; /* This only needs to be out of the snap range to work. -Cliff */

    for (c2 = screen_info->clients, i = 0; i < screen_info->client_count; c2 = c2->next, i++)
    {
        if (FLAG_TEST (c2->xfwm_flags, XFWM_FLAG_VISIBLE)  && (c2 != c) &&
            (((screen_info->params->snap_to_windows) && (c2->win_layer == c->win_layer))
             || ((screen_info->params->snap_to_border)
                  && FLAG_TEST (c2->flags, CLIENT_FLAG_HAS_STRUT)
                  && FLAG_TEST (c2->xfwm_flags, XFWM_FLAG_VISIBLE))))
        {

            if (clientCheckOverlap (c->y - frameExtentTop (c) - 1, c->y + c->height + frameExtentBottom (c) + 1, c2->y - frameExtentTop (c) - 1, c2->y + c2->height + frameExtentBottom (c) + 1))
            {
                if (abs (c2->x - frameExtentLeft (c2) - edge_pos) < abs (closest - edge_pos))
                {
                    closest = c2->x - frameExtentLeft (c2);
                }
                if (abs ((c2->x + c2->width) + frameExtentRight (c2) - edge_pos) < abs (closest - edge_pos))
                {
                    closest = (c2->x + c2->width) + frameExtentRight (c2);
                }
            }
        }
    }

    if (abs (closest - edge_pos) > snap_width)
    {
        closest = edge_pos;
    }

    return closest;
}

static int
clientFindClosestEdgeY (Client *c, int edge_pos)
{
    /* This function is mostly identical to the one above, but swaps the
       axes. If there's a better way to do it than this, I'd like to
       know. -Cliff */

    Client *c2;
    ScreenInfo *screen_info;
    guint i;
    int snap_width, closest;

    screen_info = c->screen_info;
    snap_width = screen_info->params->snap_width;
    closest = edge_pos + snap_width + 1; /* This only needs to be out of the snap range to work. -Cliff */

    for (c2 = screen_info->clients, i = 0; i < screen_info->client_count; c2 = c2->next, i++)
    {
        if (FLAG_TEST (c2->xfwm_flags, XFWM_FLAG_VISIBLE)  && (c2 != c) &&
            (((screen_info->params->snap_to_windows) && (c2->win_layer == c->win_layer))
             || ((screen_info->params->snap_to_border)
                  && FLAG_TEST (c2->flags, CLIENT_FLAG_HAS_STRUT)
                  && FLAG_TEST (c2->xfwm_flags, XFWM_FLAG_VISIBLE))))
        {

            if (clientCheckOverlap (c->x - frameExtentLeft (c) - 1, c->x + c->width + frameExtentRight (c) + 1, c2->x - frameExtentLeft (c) - 1, c2->x + c2->width + frameExtentRight (c) + 1))
            {
                if (abs (c2->y - frameExtentTop(c2) - edge_pos) < abs (closest - edge_pos))
                {
                    closest = c2->y - frameExtentTop (c2);
                }
                if (abs ((c2->y + c2->height) + frameExtentBottom (c2) - edge_pos) < abs (closest - edge_pos))
                {
                    closest = (c2->y + c2->height) + frameExtentBottom (c2);
                }
            }
        }
    }

    if (abs (closest - edge_pos) > snap_width)
    {
        closest = edge_pos;
    }

    return closest;
}

static void
clientSnapPosition (Client * c, int prev_x, int prev_y)
{
    ScreenInfo *screen_info;
    Client *c2;
    guint i;
    int cx, cy, delta;
    int disp_x, disp_y, disp_max_x, disp_max_y;
    int frame_x, frame_y, frame_height, frame_width;
    int frame_top, frame_left;
    int frame_x2, frame_y2;
    int best_frame_x, best_frame_y;
    int best_delta_x, best_delta_y;
    int c_frame_x1, c_frame_x2, c_frame_y1, c_frame_y2;
    GdkRectangle rect;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    best_delta_x = screen_info->params->snap_width + 1;
    best_delta_y = screen_info->params->snap_width + 1;

    frame_x = frameExtentX (c);
    frame_y = frameExtentY (c);
    frame_height = frameExtentHeight (c);
    frame_width = frameExtentWidth (c);
    frame_top = frameExtentTop (c);
    frame_left = frameExtentLeft (c);

    cx = frame_x + (frame_width / 2);
    cy = frame_y + (frame_height / 2);

    frame_x2 = frame_x + frame_width;
    frame_y2 = frame_y + frame_height;
    best_frame_x = frame_x;
    best_frame_y = frame_y;

    myScreenFindMonitorAtPoint (screen_info, cx, cy, &rect);

    disp_x = rect.x;
    disp_y = rect.y;
    disp_max_x = rect.x + rect.width;
    disp_max_y = rect.y + rect.height;

    if (screen_info->params->snap_to_border)
    {
        if (abs (disp_x - frame_x) < abs (disp_max_x - frame_x2))
        {
            if (!screen_info->params->snap_resist || ((frame_x <= disp_x) && (c->x < prev_x)))
            {
                best_delta_x = abs (disp_x - frame_x);
                best_frame_x = disp_x;
            }
        }
        else
        {
            if (!screen_info->params->snap_resist || ((frame_x2 >= disp_max_x) && (c->x > prev_x)))
            {
                best_delta_x = abs (disp_max_x - frame_x2);
                best_frame_x = disp_max_x - frame_width;
            }
        }

        if (abs (disp_y - frame_y) < abs (disp_max_y - frame_y2))
        {
            if (!screen_info->params->snap_resist || ((frame_y <= disp_y) && (c->y < prev_y)))
            {
                best_delta_y = abs (disp_y - frame_y);
                best_frame_y = disp_y;
            }
        }
        else
        {
            if (!screen_info->params->snap_resist || ((frame_y2 >= disp_max_y) && (c->y > prev_y)))
            {
                best_delta_y = abs (disp_max_y - frame_y2);
                best_frame_y = disp_max_y - frame_height;
            }
        }
    }

    for (c2 = screen_info->clients, i = 0; i < screen_info->client_count; c2 = c2->next, i++)
    {
        if (FLAG_TEST (c2->xfwm_flags, XFWM_FLAG_VISIBLE)  && (c2 != c) &&
            (((screen_info->params->snap_to_windows) && (c2->win_layer == c->win_layer))
             || ((screen_info->params->snap_to_border)
                  && FLAG_TEST (c2->flags, CLIENT_FLAG_HAS_STRUT)
                  && FLAG_TEST (c2->xfwm_flags, XFWM_FLAG_VISIBLE))))
        {
            c_frame_x1 = frameExtentX (c2);
            c_frame_x2 = c_frame_x1 + frameExtentWidth (c2);
            c_frame_y1 = frameExtentY (c2);
            c_frame_y2 = c_frame_y1 + frameExtentHeight (c2);

            if ((c_frame_y1 <= frame_y2) && (c_frame_y2 >= frame_y))
            {
                delta = abs (c_frame_x2 - frame_x);
                if (delta < best_delta_x)
                {
                    if (!screen_info->params->snap_resist || ((frame_x <= c_frame_x2) && (c->x < prev_x)))
                    {
                        best_delta_x = delta;
                        best_frame_x = c_frame_x2;
                    }
                }

                delta = abs (c_frame_x1 - frame_x2);
                if (delta < best_delta_x)
                {
                    if (!screen_info->params->snap_resist || ((frame_x2 >= c_frame_x1) && (c->x > prev_x)))
                    {
                        best_delta_x = delta;
                        best_frame_x = c_frame_x1 - frame_width;
                    }
                }
            }

            if ((c_frame_x1 <= frame_x2) && (c_frame_x2 >= frame_x))
            {
                delta = abs (c_frame_y2 - frame_y);
                if (delta < best_delta_y)
                {
                    if (!screen_info->params->snap_resist || ((frame_y <= c_frame_y2) && (c->y < prev_y)))
                    {
                        best_delta_y = delta;
                        best_frame_y = c_frame_y2;
                    }
                }

                delta = abs (c_frame_y1 - frame_y2);
                if (delta < best_delta_y)
                {
                    if (!screen_info->params->snap_resist || ((frame_y2 >= c_frame_y1) && (c->y > prev_y)))
                    {
                        best_delta_y = delta;
                        best_frame_y = c_frame_y1 - frame_height;
                    }
                }
            }
        }
    }

    if (best_delta_x <= screen_info->params->snap_width)
    {
        c->x = best_frame_x + frame_left;
    }
    if (best_delta_y <= screen_info->params->snap_width)
    {
        c->y = best_frame_y + frame_top;
    }
}

static eventFilterStatus
clientButtonReleaseFilter (XfwmEvent *event, gpointer data)
{
    MoveResizeData *passdata = (MoveResizeData *) data;
    ScreenInfo *screen_info;
    Client *c;

    c = passdata->c;
    screen_info = c->screen_info;

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if ((event->meta.type == XFWM_EVENT_BUTTON && !event->button.pressed &&
        (passdata->button == AnyButton || passdata->button == event->button.button)) ||
        (event->meta.type == XFWM_EVENT_KEY && event->key.pressed &&
         event->key.keycode == screen_info->params->keys[KEY_CANCEL].keycode))
    {
        gtk_main_quit ();
        return EVENT_FILTER_STOP;
    }

    return EVENT_FILTER_CONTINUE;
}

void
clientMoveWarp (Client * c, ScreenInfo * screen_info, int * x_root, int * y_root, guint32 timestamp)
{
    static guint32 lastresist = 0;
    static int edge_scroll_x = 0;
    static int edge_scroll_y = 0;
    static int original_x = 0;
    static int original_y = 0;
    gboolean warp_pointer = FALSE;
    int maxx, maxy;
    int rx, ry, delta;

    g_return_if_fail (screen_info != NULL);
    g_return_if_fail (x_root != NULL);
    g_return_if_fail (y_root != NULL);
    TRACE ("entering");

    if ((c != NULL) && !(screen_info->params->wrap_windows))
    {
        return;
    }

    if (!(screen_info->params->wrap_resistance) || !(screen_info->workspace_count > 1))
    {
        return;
    }

    maxx = screen_info->width - 1;
    maxy = screen_info->height - 1;
    rx = 0;
    ry = 0;
    warp_pointer = FALSE;

    if (edge_scroll_x == 0)
    {
        original_y = *y_root;
    }
    if (edge_scroll_y == 0)
    {
        original_x = *x_root;
    }
    if ((*x_root == 0) || (*x_root == maxx))
    {
        if ((timestamp - lastresist) > 250)  /* ms */
        {
            edge_scroll_x = 0;
        }
        else
        {
            edge_scroll_x++;
        }
        if (*x_root == 0)
        {
            rx = 1;
        }
        else
        {
            rx = -1;
        }
        warp_pointer = TRUE;
        lastresist = timestamp;
    }
    if ((*y_root == 0) || (*y_root == maxy))
    {
        if ((timestamp - lastresist) > 250)  /* ms */
        {
            edge_scroll_y = 0;
        }
        else
        {
            edge_scroll_y++;
        }
        if (*y_root == 0)
        {
            ry = 1;
        }
        else
        {
            ry = -1;
        }
        warp_pointer = TRUE;
        lastresist = timestamp;
    }

    if (edge_scroll_x > screen_info->params->wrap_resistance)
    {
        edge_scroll_x = 0;
        if ((ABS(*y_root - original_y) < MAX_SNAP_DRIFT) && ((*x_root == 0) || (*x_root == maxx)))
        {
            delta = MAX (9 * maxx / 10, maxx - 5 * screen_info->params->wrap_resistance);
            if (*x_root == 0)
            {
                if (workspaceMove (screen_info, 0, -1, c, timestamp))
                {
                    rx = delta;
                }
            }
            else
            {
                if (workspaceMove (screen_info, 0, 1, c, timestamp))
                {
                    rx = -delta;
                }
            }
            warp_pointer = TRUE;
        }
        lastresist = 0;
    }
    if (edge_scroll_y > screen_info->params->wrap_resistance)
    {
        edge_scroll_y = 0;
        if ((ABS(*x_root - original_x) < MAX_SNAP_DRIFT) && ((*y_root == 0) || (*y_root == maxy)))
        {
            delta = MAX (9 * maxy / 10, maxy - 5 * screen_info->params->wrap_resistance);
            if (*y_root == 0)
            {
                if (workspaceMove (screen_info, -1, 0, c, timestamp))
                {
                    ry = delta;
                }
            }
            else
            {
                if (workspaceMove (screen_info, 1, 0, c, timestamp))
                {
                    ry = -delta;
                }
            }
            warp_pointer = TRUE;
        }
        lastresist = 0;
    }

    if (warp_pointer)
    {
        XWarpPointer (myScreenGetXDisplay(screen_info), None, None, 0, 0, 0, 0, rx, ry);
        *x_root += rx;
        *y_root += ry;
    }
}

static gboolean
clientMoveTile (Client *c, XfwmEventMotion *event)
{
    ScreenInfo *screen_info;
    GdkRectangle rect;
    int x, y, disp_x, disp_y, disp_max_x, disp_max_y, dist, dist_corner;

    screen_info = c->screen_info;

    /* We cannot tile windows if wrapping is enabled */
    if (!screen_info->params->tile_on_move || screen_info->params->wrap_windows)
    {
        return FALSE;
    }

    x = event->x;
    y = event->y;

    myScreenFindMonitorAtPoint (screen_info, x, y, &rect);
    disp_x = rect.x;
    disp_y = rect.y;
    disp_max_x = rect.x + rect.width;
    disp_max_y = rect.y + rect.height;

    dist = MIN (TILE_DISTANCE, frameDecorationTop (screen_info) / 2);
    dist_corner = (MIN (disp_max_x, disp_max_y)) / BORDER_TILE_LENGTH_RELATIVE;

    /* make sure the mouse position is inside the screen edges */
    if ((x >= disp_x - 1) && (x < disp_max_x + 1) &&
        (y >= disp_y - 1) && (y < disp_max_y + 1))
    {
        /* tile window depending on the mouse position on the screen */

        if ((y >= disp_y + dist_corner) && (y < disp_max_y - dist_corner))
        {
            /* mouse pointer on left edge excluding corners */
            if (x < disp_x + dist)
            {
                return clientTile (c, x, y, TILE_LEFT, !screen_info->params->box_move, FALSE);
            }
            /* mouse pointer on right edge excluding corners */
            if (x >= disp_max_x - dist)
            {
                return clientTile (c, x, y, TILE_RIGHT, !screen_info->params->box_move, FALSE);
            }
        }

        if ((x >= disp_x + dist_corner) && (x < disp_max_x - dist_corner))
        {
            /* mouse pointer on top edge excluding corners */
            if (y < disp_y + dist)
            {
                return clientToggleMaximizedAtPoint (c, x, y, CLIENT_FLAG_MAXIMIZED, FALSE);
            }
        }

        /* mouse pointer on top left corner */
        if (((x < disp_x + dist_corner) && (y < disp_y + dist))
            || ((x < disp_x + dist) && (y < disp_y + dist_corner)))
        {
            return clientTile (c, x, y, TILE_UP_LEFT, !screen_info->params->box_move, FALSE);
        }
        /* mouse pointer on top right corner */
        if (((x >= disp_max_x - dist_corner) && (y < disp_y + dist))
            || ((x >= disp_max_x - dist) && (y < disp_y + dist_corner)))
        {
            return clientTile (c, x, y, TILE_UP_RIGHT, !screen_info->params->box_move, FALSE);
        }
        /* mouse pointer on bottom left corner */
        if (((x < disp_x + dist_corner) && (y >= disp_max_y - dist))
            || ((x < disp_x + dist) && (y >= disp_max_y - dist_corner)))
        {
            return clientTile (c, x, y, TILE_DOWN_LEFT, !screen_info->params->box_move, FALSE);
        }
        /* mouse pointer on bottom right corner */
        if (((x >= disp_max_x - dist_corner) && (y >= disp_max_y - dist))
            || ((x >= disp_max_x - dist) && (y >= disp_max_y - dist_corner)))
        {
            return clientTile (c, x, y, TILE_DOWN_RIGHT, !screen_info->params->box_move, FALSE);
        }
    }

    return FALSE;
}

static eventFilterStatus
clientMoveEventFilter (XfwmEvent *event, gpointer data)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    eventFilterStatus status = EVENT_FILTER_STOP;
    MoveResizeData *passdata = (MoveResizeData *) data;
    Client *c = NULL;
    gboolean moving;
    XWindowChanges wc;
    int prev_x, prev_y;
    unsigned long cancel_maximize_flags;
    unsigned long cancel_restore_size_flags;

    TRACE ("entering");

    c = passdata->c;
    prev_x=c->x;
    prev_y=c->y;
    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    /*
     * Clients may choose to end the move operation,
     * we use XFWM_FLAG_MOVING_RESIZING for that.
     */
    moving = FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING);

    /* Update the display time */
    myDisplayUpdateCurrentTime (display_info, event);

    if (event->meta.type == XFWM_EVENT_KEY && event->key.pressed)
    {
        int key_move;

        while (xfwm_device_check_mask_event (display_info->devices, display_info->dpy,
                                             KeyPressMask, event))
        {
            /* Update the display time */
            myDisplayUpdateCurrentTime (display_info, event);
        }

        key_move = 16;
        if ((screen_info->params->snap_to_border) || (screen_info->params->snap_to_windows))
        {
            key_move = MAX (key_move, screen_info->params->snap_width + 1);
        }
        if (event->key.keycode == screen_info->params->keys[KEY_LEFT].keycode)
        {
            clientMovePointer (display_info, -1, 0, key_move);
        }
        else if (event->key.keycode == screen_info->params->keys[KEY_RIGHT].keycode)
        {
            clientMovePointer (display_info, 1, 0, key_move);
        }
        else if (event->key.keycode == screen_info->params->keys[KEY_UP].keycode)
        {
            clientMovePointer (display_info, 0, -1, key_move);
        }
        else if (event->key.keycode == screen_info->params->keys[KEY_DOWN].keycode)
        {
            clientMovePointer (display_info, 0, 1, key_move);
        }
        else if (event->key.keycode == screen_info->params->keys[KEY_CANCEL].keycode)
        {
            moving = FALSE;
            passdata->released = passdata->use_keys;

            if (screen_info->params->box_move)
            {
                if (passdata->wireframe)
                {
                    wireframeUpdate  (c, passdata->wireframe);
                }
                else
                {
                    clientDrawOutline (c);
                }
            }

            c->x = passdata->cancel_x;
            c->y = passdata->cancel_y;
            /* Restore the width height to correct the outline */
            if (c->width != passdata->cancel_w ||
                c->height != passdata->cancel_h)
            {
                c->width = passdata->cancel_w;
                c->height = passdata->cancel_h;
                passdata->move_resized = TRUE;
            }

            if (screen_info->current_ws != passdata->cancel_workspace)
            {
                workspaceSwitch (screen_info, passdata->cancel_workspace, c, FALSE, event->key.time);
            }
            cancel_maximize_flags = passdata->cancel_flags & CLIENT_FLAG_MAXIMIZED;
            if (!FLAG_TEST_AND_NOT(c->flags, cancel_maximize_flags, CLIENT_FLAG_MAXIMIZED))
            {
                /* Toggle maximize on the differences between the current state and the cancelled state */
                cancel_maximize_flags ^= c->flags & CLIENT_FLAG_MAXIMIZED;
                if (clientToggleMaximized (c, cancel_maximize_flags, FALSE))
                {
                    passdata->configure_flags = CFG_FORCE_REDRAW;
                    passdata->move_resized = TRUE;
                }
            }
            cancel_restore_size_flags = passdata->cancel_flags & CLIENT_FLAG_RESTORE_SIZE_POS;
            if (!FLAG_TEST_AND_NOT(c->flags, cancel_restore_size_flags, CLIENT_FLAG_RESTORE_SIZE_POS))
            {
                FLAG_TOGGLE (c->flags, CLIENT_FLAG_RESTORE_SIZE_POS);
            }
            if (screen_info->params->box_move)
            {
                if (passdata->wireframe)
                {
                    wireframeUpdate  (c, passdata->wireframe);
                }
                else
                {
                    clientDrawOutline (c);
                }
            }
        }
        else if (passdata->use_keys)
        {
            moving = clientKeyPressIsModifier(&event->key);
        }
    }
    else if (event->meta.type == XFWM_EVENT_BUTTON && !event->button.pressed)
    {
        moving = FALSE;
        passdata->released = (passdata->use_keys ||
                              passdata->button == AnyButton ||
                              passdata->button == event->button.button);
    }
    else if (event->meta.type == XFWM_EVENT_MOTION)
    {
        while (xfwm_device_check_mask_event (display_info->devices, display_info->dpy,
                                             PointerMotionMask | ButtonMotionMask, event))
        {
            /* Update the display time */
            myDisplayUpdateCurrentTime (display_info, event);
        }

        if (!passdata->grab && use_xor_move(screen_info))
        {
            myDisplayGrabServer (display_info);
            passdata->grab = TRUE;
            clientDrawOutline (c);
        }
        if (use_xor_move(screen_info))
        {
            clientDrawOutline (c);
        }
        if ((screen_info->workspace_count > 1) && !(passdata->is_transient))
        {
            clientMoveWarp (c, screen_info,
                            &event->motion.x_root,
                            &event->motion.y_root,
                            event->motion.time);
        }

        if (FLAG_TEST (c->flags, CLIENT_FLAG_RESTORE_SIZE_POS))
        {
            gboolean size_changed;

            size_changed = clientToggleMaximized (c, c->flags & CLIENT_FLAG_MAXIMIZED, FALSE);
            if (clientRestoreSizePos (c))
            {
                size_changed = TRUE;
            }
            if (size_changed)
            {
                passdata->move_resized = TRUE;
                clientUntile (c);
                if (!screen_info->params->box_move)
                {
                    clientSetNetState (c);
                }

                /* to keep the distance from the edges of the window proportional. */
                passdata->ox = c->x;
                passdata->mx = frameExtentX (c) + passdata->pxratio * frameExtentWidth (c);
                passdata->oy = c->y;
                passdata->my = frameExtentY (c) + passdata->pyratio * frameExtentHeight (c);

                passdata->configure_flags = CFG_FORCE_REDRAW;
            }
        }

        c->x = passdata->ox + (event->motion.x_root - passdata->mx);
        c->y = passdata->oy + (event->motion.y_root - passdata->my);

        clientSnapPosition (c, prev_x, prev_y);
        if (clientMoveTile (c, &event->motion))
        {
            passdata->configure_flags = CFG_FORCE_REDRAW;
            passdata->move_resized = TRUE;
        }
        else
        {
            clientConstrainPos(c, FALSE);
        }

#ifdef SHOW_POSITION
        if (passdata->poswin)
        {
            poswinSetPosition (passdata->poswin, c);
        }
#endif /* SHOW_POSITION */
        if (screen_info->params->box_move)
        {
            if (passdata->wireframe)
            {
                wireframeUpdate  (c, passdata->wireframe);
            }
            else
            {
                clientDrawOutline (c);
            }
        }
        else
        {
            int changes = CWX | CWY;

            if (passdata->move_resized)
            {
                wc.width = c->width;
                wc.height = c->height;
                changes |= CWWidth | CWHeight;
                passdata->move_resized = FALSE;
            }

            wc.x = c->x;
            wc.y = c->y;
            clientConfigure (c, &wc, changes, passdata->configure_flags);
            /* Configure applied, clear the flags */
            passdata->configure_flags = NO_CFG_FLAG;
        }
    }
    else if ((event->meta.xevent->type == UnmapNotify) && (event->meta.window == c->window))
    {
        moving = FALSE;
        status = EVENT_FILTER_CONTINUE;
        passdata->client_gone = TRUE;
        if (use_xor_move(screen_info))
        {
            clientDrawOutline (c);
        }
    }
    else if (event->meta.type == XFWM_EVENT_CROSSING && event->crossing.enter)
    {
        /* Ignore enter events */
    }
    else
    {
        status = EVENT_FILTER_CONTINUE;
    }

    TRACE ("leaving");

    if (!moving)
    {
        TRACE ("event loop now finished");
        clientSaveSizePos (c);
        gtk_main_quit ();
    }

    return status;
}

void
clientMove (Client * c, XfwmEventButton *event)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    XWindowChanges wc;
    MoveResizeData passdata;
    int changes;
    gboolean g1, g2;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING) ||
        !FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_MOVE))
    {
        return;
    }

    if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        return;
    }

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);
    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    changes = CWX | CWY;

    passdata.c = c;
    passdata.cancel_x = passdata.ox = c->x;
    passdata.cancel_y = passdata.oy = c->y;
    passdata.cancel_w = c->width;
    passdata.cancel_h = c->height;
    passdata.cancel_flags = c->flags;
    passdata.configure_flags = NO_CFG_FLAG;
    passdata.cancel_workspace = c->win_workspace;
    passdata.use_keys = FALSE;
    passdata.grab = FALSE;
    passdata.released = FALSE;
    passdata.client_gone = FALSE;
    passdata.button = AnyButton;
    passdata.is_transient = clientIsValidTransientOrModal (c);
    passdata.move_resized = FALSE;
    passdata.wireframe = NULL;

    clientSaveSizePos (c);

    if (event && event->pressed)
    {
        passdata.button = event->button;
        passdata.mx = event->x_root;
        passdata.my = event->y_root;
        passdata.pxratio = (passdata.mx - frameExtentX (c)) / (double) frameExtentWidth (c);
        passdata.pyratio = (passdata.my - frameExtentY (c)) / (double) frameExtentHeight (c);
    }
    else
    {
        clientSetHandle(&passdata, NO_HANDLE);
        passdata.released = passdata.use_keys = TRUE;
    }

    g1 = myScreenGrabKeyboard (screen_info, MOVERESIZE_KEYBOARD_EVENT_MASK,
                               myDisplayGetCurrentTime (display_info));
    g2 = myScreenGrabPointer (screen_info, FALSE, MOVERESIZE_POINTER_EVENT_MASK,
                              myDisplayGetCursorMove (display_info),
                              myDisplayGetCurrentTime (display_info));
    if (!g1 || !g2)
    {
        TRACE ("grab failed in clientMove");

        myDisplayBeep (display_info);
        myScreenUngrabKeyboard (screen_info, myDisplayGetCurrentTime (display_info));
        myScreenUngrabPointer (screen_info, myDisplayGetCurrentTime (display_info));

        return;
    }

    if (screen_info->params->box_move && compositorIsActive (screen_info))
    {
        passdata.wireframe = wireframeCreate (c);
    }

    passdata.poswin = NULL;
#ifdef SHOW_POSITION
    passdata.poswin = poswinCreate(screen_info->gscr);
    poswinSetPosition (passdata.poswin, c);
    poswinShow (passdata.poswin);
#endif /* SHOW_POSITION */

    /* Set window translucent while moving */
    if ((screen_info->params->move_opacity < 100) &&
        !(screen_info->params->box_move) &&
        !FLAG_TEST (c->xfwm_flags, XFWM_FLAG_OPACITY_LOCKED))
    {
        clientSetOpacity (c, c->opacity, OPACITY_MOVE, OPACITY_MOVE);
    }

    /*
     * Need to remove the sidewalk windows while moving otherwise
     * the motion events aren't reported on screen edges
     */
    placeSidewalks(screen_info, FALSE);

    /* Clear any previously saved pos flag from screen resize */
    FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_SAVED_POS);

    FLAG_SET (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING);
    TRACE ("entering move loop");
    eventFilterPush (display_info->xfilter, clientMoveEventFilter, &passdata);
    gtk_main ();
    eventFilterPop (display_info->xfilter);
    TRACE ("leaving move loop");
    if (passdata.client_gone)
    {
        goto move_cleanup;
    }
    FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING);

    if (passdata.grab && screen_info->params->box_move)
    {
        clientDrawOutline (c);
    }

    /* Set window opacity to its original value */
    clientSetOpacity (c, c->opacity, OPACITY_MOVE, 0);

    clientSetNetState (c);

    wc.x = c->x;
    wc.y = c->y;
    if (passdata.move_resized)
    {
        wc.width = c->width;
        wc.height = c->height;
        changes |= CWWidth | CWHeight;
    }
    clientConfigure (c, &wc, changes, passdata.configure_flags);

    if (passdata.button != AnyButton && !passdata.released)
    {
        /* If this is a drag-move, wait for the button to be released.
         * If we don't, we might get release events in the wrong place.
         */
        eventFilterPush (display_info->xfilter, clientButtonReleaseFilter, &passdata);
        gtk_main ();
        eventFilterPop (display_info->xfilter);
    }

move_cleanup:
    /* Put back the sidewalks as they ought to be */
    placeSidewalks (screen_info, screen_info->params->wrap_workspaces);

#ifdef SHOW_POSITION
    if (passdata.poswin)
    {
        poswinDestroy (passdata.poswin);
    }
#endif /* SHOW_POSITION */

    if (passdata.wireframe)
    {
        wireframeDelete (passdata.wireframe);
    }

    myScreenUngrabKeyboard (screen_info, myDisplayGetCurrentTime (display_info));
    myScreenUngrabPointer (screen_info, myDisplayGetCurrentTime (display_info));

    if (passdata.grab && screen_info->params->box_move)
    {
        myDisplayUngrabServer (display_info);
    }
}

static gboolean
clientChangeHandle (MoveResizeData *passdata, XfwmEvent *event, int handle)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Client *c;
    Cursor cursor;
    gboolean grab;

    c = passdata->c;
    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    clientSetHandle(passdata, handle);
    if ((handle > NO_HANDLE) && (handle <= HANDLES_COUNT))
    {
        cursor = myDisplayGetCursorResize (display_info, handle);
    }
    else
    {
        cursor = myDisplayGetCursorMove (display_info);
    }
    grab = myScreenChangeGrabPointer (screen_info, FALSE, MOVERESIZE_POINTER_EVENT_MASK,
                                      cursor, myDisplayGetCurrentTime (display_info));

    return (grab);
}

static void
clientResizeConfigure (Client *c, int pw, int ph)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    if (pw == c->width && ph == c->height)
    {
        /* Not a resize */
        return;
    }
#ifdef HAVE_XSYNC
    if (!FLAG_TEST (c->flags, CLIENT_FLAG_XSYNC_WAITING))
    {
        if ((display_info->have_xsync) &&
            (FLAG_TEST (c->flags, CLIENT_FLAG_XSYNC_ENABLED)) &&
            (c->xsync_counter))
        {
            clientXSyncRequest (c);
        }
#endif /* HAVE_XSYNC */
        clientReconfigure (c, NO_CFG_FLAG);
#ifdef HAVE_XSYNC
    }
#endif /* HAVE_XSYNC */
}

static eventFilterStatus
clientResizeEventFilter (XfwmEvent *event, gpointer data)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Client *c;
    GdkRectangle rect;
    MoveResizeData *passdata;
    eventFilterStatus status;
    int prev_width, prev_height;
    int cx, cy;
    int move_top, move_bottom, move_left, move_right;
    gboolean resizing;
    int right_edge; /* -Cliff */
    int bottom_edge; /* -Cliff */

    TRACE ("entering");

    passdata = (MoveResizeData *) data;
    c = passdata->c;
    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    status = EVENT_FILTER_STOP;

    /*
     * Clients may choose to end the resize operation,
     * we use XFWM_FLAG_MOVING_RESIZING for that.
     */
    resizing = FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING);

    cx = frameExtentX (c) + (frameExtentWidth (c) / 2);
    cy = frameExtentY (c) + (frameExtentHeight (c) / 2);

    move_top = ((passdata->handle == CORNER_TOP_RIGHT)
            || (passdata->handle == CORNER_TOP_LEFT)
            || (passdata->handle == CORNER_COUNT + SIDE_TOP)) ?
        1 : 0;
    move_bottom = ((passdata->handle == CORNER_BOTTOM_RIGHT)
            || (passdata->handle == CORNER_BOTTOM_LEFT)
            || (passdata->handle == CORNER_COUNT + SIDE_BOTTOM)) ?
        1 : 0;
    move_right = ((passdata->handle == CORNER_TOP_RIGHT)
            || (passdata->handle == CORNER_BOTTOM_RIGHT)
            || (passdata->handle == CORNER_COUNT + SIDE_RIGHT)) ?
        1 : 0;
    move_left = ((passdata->handle == CORNER_TOP_LEFT)
            || (passdata->handle == CORNER_BOTTOM_LEFT)
            || (passdata->handle == CORNER_COUNT + SIDE_LEFT)) ?
        1 : 0;

    myScreenFindMonitorAtPoint (screen_info, cx, cy, &rect);

    /* Store previous values in case the resize puts the window title off bounds */
    prev_width = c->width;
    prev_height = c->height;

    /* Update the display time */
    myDisplayUpdateCurrentTime (display_info, event);

    if (event->meta.type == XFWM_EVENT_KEY && event->key.pressed)
    {
        int key_width_inc, key_height_inc;

        while (xfwm_device_check_mask_event (display_info->devices, display_info->dpy,
                                             KeyPressMask, event))
        {
            /* Update the display time */
            myDisplayUpdateCurrentTime (display_info, event);
        }

        key_width_inc = c->size->width_inc;
        key_height_inc = c->size->height_inc;

        if (key_width_inc < 10)
        {
            key_width_inc = ((int) (10 / key_width_inc)) * key_width_inc;
        }
        if (key_height_inc < 10)
        {
            key_height_inc = ((int) (10 / key_height_inc)) * key_height_inc;
        }

        if (event->key.keycode == screen_info->params->keys[KEY_UP].keycode)
        {
            if ((passdata->handle == CORNER_COUNT + SIDE_BOTTOM) ||
                (passdata->handle == CORNER_COUNT + SIDE_TOP))
            {
                clientMovePointer (display_info, 0, -1, key_height_inc);
            }
            else
            {
                clientChangeHandle (passdata, event, CORNER_COUNT + SIDE_TOP);
            }
        }
        else if (event->key.keycode == screen_info->params->keys[KEY_DOWN].keycode)
        {
            if ((passdata->handle == CORNER_COUNT + SIDE_BOTTOM) ||
                (passdata->handle == CORNER_COUNT + SIDE_TOP))
            {
                clientMovePointer (display_info, 0, 1, key_height_inc);
            }
            else
            {
                clientChangeHandle (passdata, event, CORNER_COUNT + SIDE_BOTTOM);
            }
        }
        else if (event->key.keycode == screen_info->params->keys[KEY_LEFT].keycode)
        {
            if ((passdata->handle == CORNER_COUNT + SIDE_LEFT) ||
                (passdata->handle == CORNER_COUNT + SIDE_RIGHT))
            {
                clientMovePointer (display_info, -1, 0, key_width_inc);
            }
            else
            {
                clientChangeHandle (passdata, event, CORNER_COUNT + SIDE_LEFT);
            }
        }
        else if (event->key.keycode == screen_info->params->keys[KEY_RIGHT].keycode)
        {
            if ((passdata->handle == CORNER_COUNT + SIDE_LEFT) ||
                (passdata->handle == CORNER_COUNT + SIDE_RIGHT))
            {
                clientMovePointer (display_info, 1, 0, key_width_inc);
            }
            else
            {
                clientChangeHandle (passdata, event, CORNER_COUNT + SIDE_RIGHT);
            }
        }
        else if (event->key.keycode == screen_info->params->keys[KEY_CANCEL].keycode)
        {
            resizing = FALSE;
            passdata->released = passdata->use_keys;

            if (use_xor_resize(screen_info))
            {
                clientDrawOutline (c);
            }

            /* restore the pre-resize position & size */
            c->x = passdata->cancel_x;
            c->y = passdata->cancel_y;
            c->width = passdata->cancel_w;
            c->height = passdata->cancel_h;
            if (screen_info->params->box_resize)
            {
                if (passdata->wireframe)
                {
                    wireframeUpdate  (c, passdata->wireframe);
                }
                else
                {
                    clientDrawOutline (c);
                }
            }
            else
            {
                clientResizeConfigure (c, prev_width, prev_height);
            }
        }
        else if (passdata->use_keys)
        {
            resizing = clientKeyPressIsModifier(&event->key);
        }
    }
    else if (event->meta.type == XFWM_EVENT_MOTION)
    {
        while (xfwm_device_check_mask_event (display_info->devices, display_info->dpy,
                                             ButtonMotionMask | PointerMotionMask, event))
        {
            /* Update the display time */
            myDisplayUpdateCurrentTime (display_info, event);
        }

        if (event->meta.xevent->type == ButtonRelease)
        {
            resizing = FALSE;
        }
        if (!passdata->grab && use_xor_resize(screen_info))
        {
            myDisplayGrabServer (display_info);
            passdata->grab = TRUE;
            clientDrawOutline (c);
        }
        if (use_xor_resize(screen_info))
        {
            clientDrawOutline (c);
        }
        passdata->oldw = c->width;
        passdata->oldh = c->height;
        right_edge = c->x + c->width;
        bottom_edge = c->y + c->height;

        if (move_left)
        {
            c->width = passdata->ow - (event->motion.x_root - passdata->mx);
            c->x = c->x - (c->width - passdata->oldw);

            /* Snap the left edge to something. -Cliff */
            c->x = clientFindClosestEdgeX (c, c->x - frameExtentLeft (c)) + frameExtentLeft (c);
            c->width = right_edge - c->x;
        }
        else if (move_right)
        {
            c->width = passdata->ow + (event->motion.x_root - passdata->mx);

            /* Attempt to snap the right edge to something. -Cliff */
            c->width = clientFindClosestEdgeX (c, c->x + c->width + frameExtentRight (c)) - c->x - frameExtentRight (c);

        }
        if (!FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
        {
            if (move_top)
            {
                c->height = passdata->oh - (event->motion.y_root - passdata->my);
                c->y = c->y - (c->height - passdata->oldh);

                /* Snap the top edge to something. -Cliff */
                c->y = clientFindClosestEdgeY (c, c->y - frameExtentTop (c)) + frameExtentTop (c);
                c->height = bottom_edge - c->y;
            }
            else if (move_bottom)
            {
                c->height = passdata->oh + (event->motion.y_root - passdata->my);

                /* Attempt to snap the bottom edge to something. -Cliff */
                c->height = clientFindClosestEdgeY (c, c->y + c->height + frameExtentBottom (c)) - c->y - frameExtentBottom (c);
            }
        }

        /* Make sure the title remains visible on screen, adjust size if moved */
        cx = c->x;
        cy = c->y;
        clientConstrainPos (c, FALSE);
        c->height -= c->y - cy;
        c->width -= c->x - cx;

        /* Apply contrain ratio if any, only once the expected size is set */
        clientConstrainRatio (c, passdata->handle);

        c->width = clientCheckWidth (c, c->width, FALSE);
        if (move_left)
        {
            c->x = right_edge - c->width;
        }

        c->height = clientCheckHeight (c, c->height, FALSE);
        if (move_top && !FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
        {
            c->y =  bottom_edge - c->height;
        }

        if (passdata->poswin)
        {
            poswinSetPosition (passdata->poswin, c);
        }
        if (screen_info->params->box_resize)
        {
            if (passdata->wireframe)
            {
                wireframeUpdate  (c, passdata->wireframe);
            }
            else
            {
                clientDrawOutline (c);
            }
        }
        else
        {
            clientResizeConfigure (c, prev_width, prev_height);
        }
    }
    else if (event->meta.type == XFWM_EVENT_BUTTON && !event->button.pressed)
    {
        resizing = FALSE;
        passdata->released = (passdata->use_keys ||
                              passdata->button == AnyButton ||
                              passdata->button == event->button.button);
    }
    else if ((event->meta.xevent->type == UnmapNotify) && (event->meta.window == c->window))
    {
        resizing = FALSE;
        status = EVENT_FILTER_CONTINUE;
        passdata->client_gone = TRUE;
        if (use_xor_resize(screen_info))
        {
            clientDrawOutline (c);
        }
    }
    else if (event->meta.type == XFWM_EVENT_CROSSING && event->crossing.enter)
    {
        /* Ignore enter events */
    }
    else
    {
        status = EVENT_FILTER_CONTINUE;
    }

    TRACE ("leaving");

    if (!resizing)
    {
        TRACE ("event loop now finished");
        gtk_main_quit ();
    }

    return status;
}

void
clientResize (Client * c, int handle, XfwmEventButton *event)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    MoveResizeData passdata;
    int w_orig, h_orig;
    Cursor cursor;
    gboolean g1, g2;
#ifndef SHOW_POSITION
    gint scale;
#endif

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING))
    {
        return;
    }

    if (!FLAG_TEST_ALL (c->xfwm_flags, XFWM_FLAG_HAS_RESIZE | XFWM_FLAG_IS_RESIZABLE))
    {
        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_MOVE))
        {
            clientMove (c, event);
        }
        return;
    }

    if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        return;
    }

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    passdata.c = c;
    passdata.cancel_x = passdata.ox = c->x;
    passdata.cancel_y = passdata.oy = c->y;
    passdata.cancel_w = passdata.ow = c->width;
    passdata.cancel_h = passdata.oh = c->height;
    passdata.configure_flags = NO_CFG_FLAG;
    passdata.use_keys = FALSE;
    passdata.grab = FALSE;
    passdata.released = FALSE;
    passdata.client_gone = FALSE;
    passdata.button = AnyButton;
    passdata.handle = handle;
    passdata.wireframe = NULL;
    w_orig = c->width;
    h_orig = c->height;

    if (event && event->pressed)
    {
        passdata.button = event->button;
        passdata.mx = event->x_root;
        passdata.my = event->y_root;
    }
    else
    {
        clientSetHandle (&passdata, handle);
        passdata.released = passdata.use_keys = TRUE;
    }
    if ((handle > NO_HANDLE) && (handle <= HANDLES_COUNT))
    {
        cursor = myDisplayGetCursorResize (display_info, passdata.handle);
    }
    else
    {
        cursor = myDisplayGetCursorMove (display_info);
    }

    g1 = myScreenGrabKeyboard (screen_info, MOVERESIZE_KEYBOARD_EVENT_MASK,
                               myDisplayGetCurrentTime (display_info));
    g2 = myScreenGrabPointer (screen_info, FALSE, MOVERESIZE_POINTER_EVENT_MASK,
                              cursor, myDisplayGetCurrentTime (display_info));

    if (!g1 || !g2)
    {
        TRACE ("grab failed in clientResize");

        myDisplayBeep (display_info);
        myScreenUngrabKeyboard (screen_info, myDisplayGetCurrentTime (display_info));
        myScreenUngrabPointer (screen_info, myDisplayGetCurrentTime (display_info));

        return;
    }

    if (screen_info->params->box_resize && compositorIsActive (screen_info))
    {
        passdata.wireframe = wireframeCreate (c);
    }

    passdata.poswin = NULL;
#ifndef SHOW_POSITION
    scale = gdk_window_get_scale_factor (myScreenGetGdkWindow (screen_info));
    if ((c->size->width_inc > scale) || (c->size->height_inc > scale))
#endif /* SHOW_POSITION */
    {
        passdata.poswin = poswinCreate(screen_info->gscr);
        poswinSetPosition (passdata.poswin, c);
        poswinShow (passdata.poswin);
    }

    /* Set window translucent while resizing */
    if ((screen_info->params->resize_opacity < 100) &&
        !(screen_info->params->box_resize) &&
        !FLAG_TEST (c->xfwm_flags, XFWM_FLAG_OPACITY_LOCKED))
    {
        clientSetOpacity (c, c->opacity, OPACITY_RESIZE, OPACITY_RESIZE);
    }

    /* Clear any previously saved pos flag from screen resize */
    FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_SAVED_POS);

    FLAG_SET (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING);
    TRACE ("entering resize loop");
    eventFilterPush (display_info->xfilter, clientResizeEventFilter, &passdata);
    gtk_main ();
    eventFilterPop (display_info->xfilter);
    TRACE ("leaving resize loop");
    if (passdata.client_gone)
    {
        goto resize_cleanup;
    }
    FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING);

    if (passdata.grab && screen_info->params->box_resize)
    {
        clientDrawOutline (c);
    }

    /* Set window opacity to its original value */
    clientSetOpacity (c, c->opacity, OPACITY_RESIZE, 0);

    if ((w_orig != c->width) || (h_orig != c->height))
    {
        if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED))
        {
            clientRemoveMaximizeFlag (c);
            passdata.configure_flags = CFG_FORCE_REDRAW;
        }
        if (FLAG_TEST (c->flags, CLIENT_FLAG_RESTORE_SIZE_POS))
        {
            FLAG_UNSET (c->flags, CLIENT_FLAG_RESTORE_SIZE_POS);
        }
        if (c->tile_mode != TILE_NONE)
        {
            clientUntile (c);
        }
    }
    clientReconfigure (c, passdata.configure_flags);

    if (passdata.button != AnyButton && !passdata.released)
    {
        /* If this is a drag-resize, wait for the button to be released.
         * If we don't, we might get release events in the wrong place.
         */
        eventFilterPush (display_info->xfilter, clientButtonReleaseFilter, &passdata);
        gtk_main ();
        eventFilterPop (display_info->xfilter);
    }

resize_cleanup:
    if (passdata.poswin)
    {
        poswinDestroy (passdata.poswin);
    }
    if (passdata.wireframe)
    {
        wireframeDelete (passdata.wireframe);
    }

    myScreenUngrabKeyboard (screen_info, myDisplayGetCurrentTime (display_info));
    myScreenUngrabPointer (screen_info, myDisplayGetCurrentTime (display_info));

    if (passdata.grab && screen_info->params->box_resize)
    {
        myDisplayUngrabServer (display_info);
    }
}
