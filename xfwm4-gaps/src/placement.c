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
#include <libxfce4util/libxfce4util.h>

#include <common/xfwm-common.h>

#include "screen.h"
#include "misc.h"
#include "client.h"
#include "placement.h"
#include "transients.h"
#include "workspaces.h"
#include "frame.h"
#include "netwm.h"

#define USE_CLIENT_STRUTS(c) (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE) && \
                              FLAG_TEST (c->flags, CLIENT_FLAG_HAS_STRUT))

/* Compute rectangle overlap area */

static inline unsigned long
segment_overlap (int x0, int x1, int tx0, int tx1)
{
    if (tx0 > x0)
    {
        x0 = tx0;
    }
    if (tx1 < x1)
    {
        x1 = tx1;
    }
    if (x1 <= x0)
    {
        return 0;
    }
    return (x1 - x0);
}

static inline unsigned long
overlap (int x0, int y0, int x1, int y1, int tx0, int ty0, int tx1, int ty1)
{
    /* Compute overlapping box */
    return (segment_overlap (x0, x1, tx0, tx1)
            * segment_overlap (y0, y1, ty0, ty1));
}

static void
set_rectangle (GdkRectangle * rect, gint x, gint y, gint width, gint height)
{
    g_return_if_fail (rect != NULL);

    rect->x = x;
    rect->y = y;
    rect->width = width;
    rect->height = height;
}

gboolean
strutsToRectangles (Client *c,
                    GdkRectangle *left,
                    GdkRectangle *right,
                    GdkRectangle *top,
                    GdkRectangle *bottom)
{
    ScreenInfo *screen_info;

    g_return_val_if_fail (c != NULL, FALSE);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;

    if (!USE_CLIENT_STRUTS (c))
    {
        return FALSE;
    }

    if (left)
    {
        set_rectangle (left,
                       0,
                       c->struts[STRUTS_LEFT_START_Y],
                       c->struts[STRUTS_LEFT],
                       c->struts[STRUTS_LEFT_END_Y] - c->struts[STRUTS_LEFT_START_Y]);
    }

    if (right)
    {
        set_rectangle (right,
                       screen_info->width - c->struts[STRUTS_RIGHT],
                       c->struts[STRUTS_RIGHT_START_Y],
                       c->struts[STRUTS_RIGHT],
                       c->struts[STRUTS_RIGHT_END_Y] - c->struts[STRUTS_RIGHT_START_Y]);
    }

    if (top)
    {
        set_rectangle (top,
                       c->struts[STRUTS_TOP_START_X],
                       0,
                       c->struts[STRUTS_TOP_END_X] - c->struts[STRUTS_TOP_START_X],
                       c->struts[STRUTS_TOP]);
    }

    if (bottom)
    {
        set_rectangle (bottom,
                       c->struts[STRUTS_BOTTOM_START_X],
                       screen_info->height - c->struts[STRUTS_BOTTOM],
                       c->struts[STRUTS_BOTTOM_END_X] - c->struts[STRUTS_BOTTOM_START_X],
                       c->struts[STRUTS_BOTTOM]);
    }

    return TRUE;
}

static gboolean
clientsOnSameMonitor (Client *c1, Client *c2)
{
    ScreenInfo *screen_info;
    GdkRectangle monitor;
    GdkRectangle win1;
    GdkRectangle win2;
    int num_monitors, i;

    if (c1->screen_info != c2->screen_info)
    {
        return FALSE;
    }

    screen_info = c1->screen_info;

    set_rectangle (&win1,
                   frameExtentX (c1),
                   frameExtentY (c1),
                   frameExtentWidth (c1),
                   frameExtentHeight (c1));

    set_rectangle (&win2,
                   frameExtentX (c2),
                   frameExtentY (c2),
                   frameExtentWidth (c2),
                   frameExtentHeight (c2));

    num_monitors = xfwm_get_n_monitors (screen_info->gscr);
    for (i = 0; i < num_monitors; i++)
    {
        xfwm_get_monitor_geometry (screen_info->gscr, i, &monitor, TRUE);
        if (gdk_rectangle_intersect (&win1, &monitor, NULL) &&
            gdk_rectangle_intersect (&win2, &monitor, NULL))
        {
            return TRUE;
        }
    }

    return FALSE;
}

gboolean
clientsHaveOverlap (Client *c1, Client *c2)
{
    GdkRectangle win1;
    GdkRectangle win2;

    if (c1->screen_info != c2->screen_info)
    {
        return FALSE;
    }

    set_rectangle (&win1,
                   frameExtentX (c1),
                   frameExtentY (c1),
                   frameExtentWidth (c1),
                   frameExtentHeight (c1));

    set_rectangle (&win2,
                   frameExtentX (c2),
                   frameExtentY (c2),
                   frameExtentWidth (c2),
                   frameExtentHeight (c2));

    return gdk_rectangle_intersect (&win1, &win2, NULL);
}

void
clientMaxSpace (Client *c, int *x, int *y, int *w, int *h)
{
    ScreenInfo *screen_info;
    Client *c2;
    guint i;
    GdkRectangle top, left, right, bottom, area, initial, intersect;

    g_return_if_fail (c != NULL);
    g_return_if_fail (x != NULL);
    g_return_if_fail (y != NULL);
    g_return_if_fail (w != NULL);
    g_return_if_fail (h != NULL);

    TRACE ("client \"%s\" (0x%lx) %s", c->name, c->window);

    screen_info = c->screen_info;

    set_rectangle (&area, *x, *y, *w, *h);
    set_rectangle (&initial, *x, *y, *w, *h);

    for (c2 = screen_info->clients, i = 0; i < screen_info->client_count; c2 = c2->next, i++)
    {
        if (!USE_CLIENT_STRUTS (c2))
        {
            continue;
        }

        if (!clientsOnSameMonitor (c, c2))
        {
            continue;
        }

        if (strutsToRectangles (c2, &left, &right, &top, &bottom))
        {
            /* Left */
            if (gdk_rectangle_intersect (&left, &area, &intersect))
            {
                *x = *x + intersect.width;
                *w = *w - intersect.width;
                 set_rectangle (&area, *x, *y, *w, *h);
            }

            /* Right */
            if (gdk_rectangle_intersect (&right, &area, &intersect))
            {
                *w = *w - intersect.width;
                set_rectangle (&area, *x, *y, *w, *h);
            }

            /* Top */
            if (gdk_rectangle_intersect (&top, &area, &intersect))
            {
                *y = *y + intersect.height;
                *h = *h - intersect.height;
                set_rectangle (&area, *x, *y, *w, *h);
            }

            /* Bottom */
            if (gdk_rectangle_intersect (&bottom, &area, &intersect))
            {
                *h = *h - intersect.height;
                set_rectangle (&area, *x, *y, *w, *h);
            }
        }
    }
}

/* clientConstrainPos() is used when moving windows
   to ensure that the window stays accessible to the user

   Returns the position in which the window was constrained.
    CLIENT_CONSTRAINED_TOP    = 1<<0
    CLIENT_CONSTRAINED_BOTTOM = 1<<1
    CLIENT_CONSTRAINED_LEFT   = 1<<2
    CLIENT_CONSTRAINED_RIGHT  = 1<<3

 */
unsigned int
clientConstrainPos (Client * c, gboolean show_full)
{
    Client *c2;
    ScreenInfo *screen_info;
    guint i;
    gint cx, cy;
    gint frame_top, frame_left;
    gint title_visible;
    gint screen_width, screen_height;
    guint ret;
    GdkRectangle top, left, right, bottom, win, monitor;
    gint min_visible;

    g_return_val_if_fail (c != NULL, 0);

    TRACE ("client \"%s\" (0x%lx) %s", c->name, c->window,
        show_full ? "(with show full)" : "(w/out show full)");

    screen_info = c->screen_info;

    /* We use a bunch of local vars to reduce the overhead of calling other functions all the time */
    frame_top = frameExtentTop (c);
    frame_left = frameExtentLeft (c);
    set_rectangle (&win, frameExtentX (c), frameExtentY (c), frameExtentWidth (c), frameExtentHeight (c));

    title_visible = frame_top;
    if (title_visible <= 0)
    {
        /* CSD window, use the title height from the theme */
        title_visible = frameDecorationTop (screen_info);
    }
    min_visible = MAX (title_visible, CLIENT_MIN_VISIBLE);
    ret = 0;

    cx = win.x + (win.width / 2);
    cy = win.y + (win.height / 2);
    myScreenFindMonitorAtPoint (screen_info, cx, cy, &monitor);

    screen_width = screen_info->width;
    screen_height = screen_info->height;

    if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        TRACE ("ignoring constrained for client \"%s\" (0x%lx)", c->name,
            c->window);
        return 0;
    }
    if (show_full)
    {
        for (c2 = screen_info->clients, i = 0; i < screen_info->client_count; c2 = c2->next, i++)
        {
            if ((c2 == c) || !strutsToRectangles (c2, &left, &right, &top, &bottom))
            {
                continue;
            }

            if (!clientsOnSameMonitor (c, c2))
            {
                continue;
            }

            /* right */
            if (gdk_rectangle_intersect (&right, &win, NULL))
            {
                c->x = screen_width - c2->struts[STRUTS_RIGHT] - win.width + frame_left;
                win.x = frameExtentX (c);
                ret |= CLIENT_CONSTRAINED_RIGHT;
            }

            /* Bottom */
            if (gdk_rectangle_intersect (&bottom, &win, NULL))
            {
                c->y = screen_height - c2->struts[STRUTS_BOTTOM] - win.height + frame_top;
                win.y = frameExtentY (c);
                ret |= CLIENT_CONSTRAINED_BOTTOM;
            }
        }

        if (win.x + win.width >= monitor.x + monitor.width)
        {
            c->x = monitor.x + monitor.width - win.width + frame_left;
            win.x = frameExtentX (c);
            ret |= CLIENT_CONSTRAINED_RIGHT;
        }
        if (win.x <= monitor.x)
        {
            c->x = monitor.x + frame_left;
            win.x = frameExtentX (c);
            ret |= CLIENT_CONSTRAINED_LEFT;
        }
        if (win.y + win.height >= monitor.y + monitor.height)
        {
            c->y = monitor.y + monitor.height - win.height + frame_top;
            win.y = frameExtentY (c);
            ret |= CLIENT_CONSTRAINED_BOTTOM;
        }
        if (win.y <= monitor.y)
        {
            c->y = monitor.y + frame_top;
            win.y = frameExtentY (c);
            ret |= CLIENT_CONSTRAINED_TOP;
        }

        for (c2 = screen_info->clients, i = 0; i < screen_info->client_count; c2 = c2->next, i++)
        {
            if ((c2 == c) || !strutsToRectangles (c2, &left, &right, &top, &bottom))
            {
                continue;
            }

            if (!clientsOnSameMonitor (c, c2))
            {
                continue;
            }

            /* Left */
            if (gdk_rectangle_intersect (&left, &win, NULL))
            {
                c->x = c2->struts[STRUTS_LEFT] + frame_left;
                win.x = frameExtentX (c);
                ret |= CLIENT_CONSTRAINED_LEFT;
            }

            /* Top */
            if (gdk_rectangle_intersect (&top, &win, NULL))
            {
                c->y = c2->struts[STRUTS_TOP] + frame_top;
                win.y = frameExtentY (c);
                ret |= CLIENT_CONSTRAINED_TOP;
            }
        }
    }
    else
    {
        if (win.x + win.width <= monitor.x + min_visible)
        {
            c->x = monitor.x + min_visible - win.width + frame_left;
            win.x = frameExtentX (c);
            ret |= CLIENT_CONSTRAINED_LEFT;
        }
        if (win.x + min_visible >= monitor.x + monitor.width)
        {
            c->x = monitor.x + monitor.width - min_visible + frame_left;
            win.x = frameExtentX (c);
            ret |= CLIENT_CONSTRAINED_RIGHT;
        }
        if (win.y + win.height <= monitor.y + min_visible)
        {
            c->y = monitor.y + min_visible - win.height + frame_top;
            win.y = frameExtentY (c);
            ret |= CLIENT_CONSTRAINED_TOP;
        }
        if (win.y + min_visible >= monitor.y + monitor.height)
        {
            c->y = monitor.y + monitor.height - min_visible + frame_top;
            win.y = frameExtentY (c);
            ret |= CLIENT_CONSTRAINED_BOTTOM;
        }
        if ((win.y <= monitor.y) && (win.y >= monitor.y - frame_top))
        {
            c->y = monitor.y + frame_top;
            win.y = frameExtentY (c);
            ret |= CLIENT_CONSTRAINED_TOP;
        }

        /* Struts and other partial struts */
        for (c2 = screen_info->clients, i = 0; i < screen_info->client_count; c2 = c2->next, i++)
        {
            if ((c2 == c) || !strutsToRectangles (c2, &left, &right, &top, &bottom))
            {
                continue;
            }

            if (!clientsOnSameMonitor (c, c2))
            {
                continue;
            }

            /* Right */
            if (gdk_rectangle_intersect (&right, &win, NULL))
            {
                if (win.x >= screen_width - c2->struts[STRUTS_RIGHT] - min_visible)
                {
                    c->x = screen_width - c2->struts[STRUTS_RIGHT] - min_visible + frame_left;
                    win.x = frameExtentX (c);
                    ret |= CLIENT_CONSTRAINED_RIGHT;
                }
            }

            /* Left */
            if (gdk_rectangle_intersect (&left, &win, NULL))
            {
                if (win.x + win.width <= c2->struts[STRUTS_LEFT] + min_visible)
                {
                    c->x = c2->struts[STRUTS_LEFT] + min_visible - win.width + frame_left;
                    win.x = frameExtentX (c);
                    ret |= CLIENT_CONSTRAINED_LEFT;
                }
            }

            /* Bottom */
            if (gdk_rectangle_intersect (&bottom, &win, NULL))
            {
                if (win.y >= screen_height - c2->struts[STRUTS_BOTTOM] - min_visible)
                {
                    c->y = screen_height - c2->struts[STRUTS_BOTTOM] - min_visible + frame_top;
                    win.y = frameExtentY (c);
                    ret |= CLIENT_CONSTRAINED_BOTTOM;
                }
            }

            /* Top */
            if (gdk_rectangle_intersect (&top, &win, NULL))
            {
                if (segment_overlap (win.y, win.y + title_visible, 0, c2->struts[STRUTS_TOP]))
                {
                    c->y = c2->struts[STRUTS_TOP] + frame_top;
                    win.y = frameExtentY (c);
                    ret |= CLIENT_CONSTRAINED_TOP;
                }
                if (win.y + win.height <= c2->struts[STRUTS_TOP] + min_visible)
                {
                    c->y = c2->struts[STRUTS_TOP] + min_visible - win.height + frame_top;
                    win.y = frameExtentY (c);
                    ret |= CLIENT_CONSTRAINED_TOP;
                }
            }
        }
    }
    return ret;
}

/* clientKeepVisible is used at initial mapping, to make sure
   the window is visible on screen. It also does coordonate
   translation in Xinerama to center window on physical screen
   Not to be confused with clientConstrainPos()
 */
static void
clientKeepVisible (Client * c, gint n_monitors, GdkRectangle *monitor_rect)
{
    gboolean centered;
    int diff_x, diff_y;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    centered = FALSE;
    /* We only center dialogs */
    if (c->type & (WINDOW_TYPE_DIALOG))
    {
        if ((c->size->x == 0) && (c->size->y == 0))
        {
            /* Dialogs that place themselves in (0,0) will be centered */
            centered = TRUE;
        }
        else if ((n_monitors > 1) && (c->size->x > 0) && (c->size->y > 0))
        {
            /* Check if the window is centered on the whole screen */
            diff_x = ABS(c->size->x - ((c->screen_info->width - c->size->width) / 2));
            diff_y = ABS(c->size->y - ((c->screen_info->height - c->size->height) / 2));
            centered = ((diff_x < 25) && (diff_y < 25));
        }
    }
    if (centered)
    {
        /* We consider that the windows is centered on screen,
         * Thus, will move it so its center on the current
         * physical screen
         */
        c->x = monitor_rect->x + (monitor_rect->width - c->width) / 2;
        c->y = monitor_rect->y + (monitor_rect->height - c->height) / 2;
    }
    clientConstrainPos (c, TRUE);
}

static void
clientAutoMaximize (Client * c, int full_w, int full_h)
{
    if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN) ||
        !FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_BORDER))
    {
        /*
         * Fullscreen or undecorated windows should not be
         * automatically maximized...
         */
        return;
    }

    if (!FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_HORIZ) &&
        (frameExtentWidth (c) >= full_w))
    {
        DBG ("The application \"%s\" has requested a window width "
             "(%u) equal or larger than the actual width available in the workspace (%u), "
             "the window will be maximized horizontally.", c->name, frameExtentWidth (c), full_w);
        FLAG_SET (c->flags, CLIENT_FLAG_MAXIMIZED_HORIZ | CLIENT_FLAG_RESTORE_SIZE_POS);
    }

    if (!FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED_VERT) &&
        (frameExtentHeight (c) >= full_h))
    {
        DBG ("The application \"%s\" has requested a window height "
             "(%u) equal or larger than the actual height available in the workspace (%u), "
             "the window will be maximized vertically.", c->name, frameExtentHeight (c), full_h);
        FLAG_SET (c->flags, CLIENT_FLAG_MAXIMIZED_VERT | CLIENT_FLAG_RESTORE_SIZE_POS);
    }
}

static void
smartPlacement (Client * c, int full_x, int full_y, int full_w, int full_h)
{
    Client *c2;
    ScreenInfo *screen_info;
    gfloat best_overlaps;
    guint i;
    gint test_x, test_y, xmax, ymax, best_x, best_y;
    gint frame_height, frame_width, frame_left, frame_top;
    gint c2_x, c2_y;
    gint xmin, ymin;

    g_return_if_fail (c != NULL);

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    frame_height = frameExtentHeight (c);
    frame_width = frameExtentWidth (c);
    frame_left = frameExtentLeft(c);
    frame_top = frameExtentTop (c);

    /* max coordinates (bottom-right) */
    xmax = full_x + full_w - c->width - frameExtentRight (c);
    ymax = full_y + full_h - c->height - frameExtentBottom (c);

    /* min coordinates (top-left) */
    xmin = full_x + frameExtentLeft (c);
    ymin = full_y + frameExtentTop (c);

    /* start with worst-case position at top-left */
    best_overlaps = G_MAXFLOAT;
    best_x = xmin;
    best_y = ymin;

    TRACE ("analyzing %i clients", screen_info->client_count);

    test_y = ymin;
    do
    {
        gint next_test_y = G_MAXINT;
        gboolean first_test_x = TRUE;

        TRACE ("testing y position %d", test_y);

        test_x = xmin;
        do
        {
            gfloat count_overlaps = 0.0;
            gint next_test_x = G_MAXINT;
            gint c2_next_test_x;
            gint c2_next_test_y;
            gint c2_frame_height;
            gint c2_frame_width;

            TRACE ("testing x position %d", test_x);

            for (c2 = screen_info->clients, i = 0; i < screen_info->client_count; c2 = c2->next, i++)
            {
                if ((c2 != c) && (c2->type != WINDOW_DESKTOP)
                    && (c->win_workspace == c2->win_workspace)
                    && FLAG_TEST (c2->xfwm_flags, XFWM_FLAG_VISIBLE))
                {
                    c2_x = frameExtentX (c2);
                    c2_frame_width = frameExtentWidth (c2);
                    if (c2_x >= full_x + full_w
                        || c2_x + c2_frame_width < full_x)
                    {
                        /* skip clients on right-of or left-of monitor */
                        continue;
                    }

                    c2_y = frameExtentY (c2);
                    c2_frame_height = frameExtentHeight (c2);
                    if (c2_y >= full_y + full_h
                        || c2_y + c2_frame_height < full_y)
                    {
                        /* skip clients on above-of or below-of monitor */
                        continue;
                    }

                    count_overlaps += overlap (test_x - frame_left,
                                               test_y - frame_top,
                                               test_x - frame_left + frame_width,
                                               test_y - frame_top + frame_height,
                                               c2_x,
                                               c2_y,
                                               c2_x + c2_frame_width,
                                               c2_y + c2_frame_height);

                    /* find the next x boundy for the step */
                    if (test_x > c2_x)
                    {
                        /* test location is beyond the x of the window,
                         * take the window right corner as next target */
                        c2_x += c2_frame_width;
                    }
                    c2_next_test_x = MIN (c2_x, xmax);
                    if (c2_next_test_x < next_test_x
                        && c2_next_test_x > test_x)
                    {
                        /* set new optimal next x step position */
                        next_test_x = c2_next_test_x;
                    }

                    if (first_test_x)
                    {
                        /* find the next y boundry step */
                        if (test_y > c2_y)
                        {
                            /* test location is beyond the y of the window,
                             * take the window bottom corner as next target */
                            c2_y += c2_frame_height;
                        }
                        c2_next_test_y = MIN (c2_y, ymax);
                        if (c2_next_test_y < next_test_y
                            && c2_next_test_y > test_y)
                        {
                            /* set new optimal next y step position */
                            next_test_y = c2_next_test_y;
                        }
                    }
                }
            }

            /* don't look for the next y boundry this x row */
            first_test_x = FALSE;

            if (count_overlaps < best_overlaps)
            {
                /* found position with less overlap */
                best_x = test_x;
                best_y = test_y;
                best_overlaps = count_overlaps;

                if (count_overlaps == 0.0f)
                {
                    /* overlap is ideal, stop searching */
                    TRACE ("found position without overlap");
                    goto found_best;
                }
            }

            if (G_LIKELY (next_test_x != G_MAXINT))
            {
                test_x = MAX (next_test_x, next_test_x + frameExtentLeft (c));
                if (test_x > xmax)
                {
                   /* always clamp on the monitor */
                   test_x = xmax;
                }
            }
            else
            {
                test_x++;
            }
        }
        while (test_x <= xmax);

        if (G_LIKELY (next_test_y != G_MAXINT))
        {
            test_y = MAX (next_test_y, next_test_y + frameExtentTop (c));
            if (test_y > ymax)
            {
                /* always clamp on the monitor */
                test_y = ymax;
            }
        }
        else
        {
            test_y++;
        }
    }
    while (test_y <= ymax);

    found_best:

    TRACE ("overlaps %f at %d,%d (x,y)", best_overlaps, best_x, best_y);

    c->x = best_x;
    c->y = best_y;
}

static void
centerPlacement (Client * c, int full_x, int full_y, int full_w, int full_h)
{
    g_return_if_fail (c != NULL);

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    c->x = MAX (full_x + frameExtentLeft(c) + (full_w - frameExtentWidth(c)) / 2, full_x + frameExtentLeft(c));
    c->y = MAX (full_y + frameExtentTop(c) + (full_h - frameExtentHeight(c)) / 2, full_y + frameExtentTop(c));
}

static void
mousePlacement (Client * c, int full_x, int full_y, int full_w, int full_h, int mx, int my)
{
    g_return_if_fail (c != NULL);

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    c->x = mx + frameExtentLeft(c) - frameExtentWidth(c) / 2;
    c->y = my + frameExtentTop(c) - frameExtentHeight(c) / 2;

    c->x = MIN (c->x, full_x + full_w - frameExtentWidth(c) + frameExtentLeft(c));
    c->y = MIN (c->y, full_y + full_h - frameExtentHeight(c) + frameExtentTop(c));

    c->x = MAX (c->x, full_x + frameExtentLeft(c));
    c->y = MAX (c->y, full_y + frameExtentTop(c));
}

void
clientInitPosition (Client * c)
{
    ScreenInfo *screen_info;
    Client *c2;
    GdkRectangle rect;
    int full_x, full_y, full_w, full_h, msx, msy;
    gint n_monitors;
    gboolean place;
    gboolean position;
    gboolean is_transient;

    g_return_if_fail (c != NULL);

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    msx = 0;
    msy = 0;
    position = (c->size->flags & (PPosition | USPosition));

    n_monitors = myScreenGetNumMonitors (c->screen_info);
    getMouseXY (screen_info, &msx, &msy);
    myScreenFindMonitorAtPoint (screen_info, msx, msy, &rect);
    is_transient = clientIsTransient (c);

    if (position || is_transient || (c->type & (WINDOW_TYPE_DONT_PLACE | WINDOW_TYPE_DIALOG)))
    {
        if (!position && is_transient && (c2 = clientGetTransient (c)))
        {
            /* Center transient relative to their parent window */
            c->x = c2->x + (c2->width - c->width) / 2;
            c->y = c2->y + (c2->height - c->height) / 2;
        }

        if (position && n_monitors > 1)
        {
            msx = frameExtentX (c) + (frameExtentWidth (c) / 2);
            msy = frameExtentY (c) + (frameExtentHeight (c) / 2);
            myScreenFindMonitorAtPoint (screen_info, msx, msy, &rect);
        }

        if (CONSTRAINED_WINDOW (c))
        {
            clientKeepVisible (c, n_monitors, &rect);
        }
        place = FALSE;
    }
    else
    {
        place = TRUE;
    }

    full_x = MAX (screen_info->params->xfwm_margins[STRUTS_LEFT], rect.x);
    full_y = MAX (screen_info->params->xfwm_margins[STRUTS_TOP], rect.y);
    full_w = MIN (screen_info->width - screen_info->params->xfwm_margins[STRUTS_RIGHT],
                  rect.x + rect.width) - full_x;
    full_h = MIN (screen_info->height - screen_info->params->xfwm_margins[STRUTS_BOTTOM],
                  rect.y + rect.height) - full_y;

    /* Adjust size to the widest size available, not covering struts */
    clientMaxSpace (c, &full_x, &full_y, &full_w, &full_h);

    /*
       If the windows is smaller than the given ratio of the available screen area,
       or if the window is larger than the screen area or if the given ratio is higher
       than 100% place the window at the center.
       Otherwise, place the window "smartly", using the good old CPU consuming algorithm...
     */
    if (place)
    {
        if ((screen_info->params->placement_ratio >= 100) ||
            (100 * frameExtentWidth(c) * frameExtentHeight(c)) < (screen_info->params->placement_ratio * full_w * full_h))
        {
            if (screen_info->params->placement_mode == PLACE_MOUSE)
            {
                mousePlacement (c, full_x, full_y, full_w, full_h, msx, msy);
            }
            else
            {
                centerPlacement (c, full_x, full_y, full_w, full_h);
            }
        }
        else if ((frameExtentWidth(c) >= full_w) && (frameExtentHeight(c) >= full_h))
        {
            centerPlacement (c, full_x, full_y, full_w, full_h);
        }
        else
        {
            smartPlacement (c, full_x, full_y, full_w, full_h);
        }
    }

    if (c->type & WINDOW_REGULAR_FOCUSABLE)
    {
        clientAutoMaximize (c, full_w, full_h);
    }
}

void
clientFill (Client * c, int fill_type)
{
    ScreenInfo *screen_info;
    Client *east_neighbour;
    Client *west_neighbour;
    Client *north_neighbour;
    Client *south_neighbour;
    Client *c2;
    GdkRectangle rect;
    XWindowChanges wc;
    unsigned short mask;
    guint i;
    gint cx, cy, full_x, full_y, full_w, full_h;
    gint tmp_x, tmp_y, tmp_w, tmp_h;

    g_return_if_fail (c != NULL);

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (!CLIENT_CAN_FILL_WINDOW (c))
    {
        return;
    }

    screen_info = c->screen_info;
    mask = 0;
    east_neighbour = NULL;
    west_neighbour = NULL;
    north_neighbour = NULL;
    south_neighbour = NULL;

    for (c2 = screen_info->clients, i = 0; i < screen_info->client_count; c2 = c2->next, i++)
    {

        /* Filter out all windows which are not visible, or not on the same layer
         * as well as the client window itself
         */
        if ((c != c2) && FLAG_TEST (c2->xfwm_flags, XFWM_FLAG_VISIBLE) && (c2->win_layer == c->win_layer))
        {
            /* Fill horizontally */
            if (fill_type & CLIENT_FILL_HORIZ)
            {
                /*
                 * check if the neigbour client (c2) is located
                 * east or west of our client.
                 */
                if (segment_overlap (frameExtentY(c), frameExtentY(c) + frameExtentHeight(c), frameExtentY(c2), frameExtentY(c2) + frameExtentHeight(c2)))
                {
                    if ((frameExtentX(c2) + frameExtentWidth(c2)) <= frameExtentX(c))
                    {
                        if (west_neighbour)
                        {
                            /* Check if c2 is closer to the client
                             * then the west neighbour already found
                             */
                            if ((frameExtentX(west_neighbour) + frameExtentWidth(west_neighbour)) < (frameExtentX(c2) + frameExtentWidth(c2)))
                            {
                                west_neighbour = c2;
                            }
                        }
                        else
                        {
                            west_neighbour = c2;
                        }
                    }
                    if ((frameExtentX(c) + frameExtentWidth(c)) <= frameExtentX(c2))
                    {
                        /* Check if c2 is closer to the client
                         * then the west neighbour already found
                         */
                        if (east_neighbour)
                        {
                            if (frameExtentX(c2) < frameExtentX(east_neighbour))
                            {
                                east_neighbour = c2;
                            }
                        }
                        else
                        {
                            east_neighbour = c2;
                        }
                    }
                }
            }

            /* Fill vertically */
            if (fill_type & CLIENT_FILL_VERT)
            {
                /* check if the neigbour client (c2) is located
                 * north or south of our client.
                 */
                if (segment_overlap (frameExtentX(c), frameExtentX(c) + frameExtentWidth(c), frameExtentX(c2), frameExtentX(c2) + frameExtentWidth(c2)))
                {
                    if ((frameExtentY(c2) + frameExtentHeight(c2)) <= frameExtentY(c))
                    {
                        if (north_neighbour)
                        {
                            /* Check if c2 is closer to the client
                             * then the north neighbour already found
                             */
                            if ((frameExtentY(north_neighbour) + frameExtentHeight(north_neighbour)) < (frameExtentY(c2) + frameExtentHeight(c2)))
                            {
                                north_neighbour = c2;
                            }
                        }
                        else
                        {
                            north_neighbour = c2;
                        }
                    }
                    if ((frameExtentY(c) + frameExtentHeight(c)) <= frameExtentY(c2))
                    {
                        if (south_neighbour)
                        {
                            /* Check if c2 is closer to the client
                             * then the south neighbour already found
                             */
                            if (frameExtentY(c2) < frameExtentY(south_neighbour))
                            {
                                south_neighbour = c2;
                            }
                        }
                        else
                        {
                            south_neighbour = c2;
                        }
                    }
                }
            }
        }
    }

    /* Compute the largest size available, based on struts, margins and Xinerama layout */
    tmp_x = frameExtentX (c);
    tmp_y = frameExtentY (c);
    tmp_h = frameExtentHeight (c);
    tmp_w = frameExtentWidth (c);

    cx = tmp_x + (tmp_w / 2);
    cy = tmp_y + (tmp_h / 2);

    myScreenFindMonitorAtPoint (screen_info, cx, cy, &rect);

    full_x = MAX (screen_info->params->xfwm_margins[STRUTS_LEFT], rect.x);
    full_y = MAX (screen_info->params->xfwm_margins[STRUTS_TOP], rect.y);
    full_w = MIN (screen_info->width - screen_info->params->xfwm_margins[STRUTS_RIGHT],
                  rect.x + rect.width) - full_x;
    full_h = MIN (screen_info->height - screen_info->params->xfwm_margins[STRUTS_BOTTOM],
                  rect.y + rect.height) - full_y;

    if ((fill_type & CLIENT_FILL) == CLIENT_FILL)
    {
        mask = CWX | CWY | CWHeight | CWWidth;
        /* Adjust size to the largest size available, not covering struts */
        clientMaxSpace (c, &full_x, &full_y, &full_w, &full_h);
    }
    else if (fill_type & CLIENT_FILL_VERT)
    {
        mask = CWY | CWHeight;
        /* Adjust size to the tallest size available, for the current horizontal position/width */
        clientMaxSpace (c, &tmp_x, &full_y, &tmp_w, &full_h);
    }
    else if (fill_type & CLIENT_FILL_HORIZ)
    {
        mask = CWX | CWWidth;
        /* Adjust size to the widest size available, for the current vertical position/height */
        clientMaxSpace (c, &full_x, &tmp_y, &full_w, &tmp_h);
    }

    /* If there are neighbours, resize to their borders.
     * If not, resize to the largest size available that you just have computed.
     */

    wc.x = full_x + frameExtentLeft(c);
    if (west_neighbour)
    {
        wc.x += MAX (frameExtentX(west_neighbour) + frameExtentWidth(west_neighbour) - full_x, 0);
    }

    wc.width = full_w - frameExtentRight(c) - (wc.x - full_x);
    if (east_neighbour)
    {
        wc.width -= MAX (full_w - (frameExtentX(east_neighbour) - full_x), 0);
    }

    wc.y = full_y + frameExtentTop(c);
    if (north_neighbour)
    {
        wc.y += MAX (frameExtentY(north_neighbour) + frameExtentHeight(north_neighbour) - full_y, 0);
    }

    wc.height = full_h - frameExtentBottom(c) - (wc.y - full_y);
    if (south_neighbour)
    {
        wc.height -= MAX (full_h - (frameExtentY(south_neighbour) - full_y), 0);
    }

    TRACE ("fill size request: (%d,%d) %dx%d", wc.x, wc.y, wc.width, wc.height);
    if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MANAGED))
    {
        clientConfigure(c, &wc, mask, NO_CFG_FLAG);
    }
}

