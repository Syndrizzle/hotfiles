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


        metacity - (c) 2001 Anders Carlsson, Havoc Pennington
        xfwm4    - (c) 2002-2015 Olivier Fourdan

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include "screen.h"
#include "client.h"
#include "compositor.h"
#include "frame.h"
#include "ui_style.h"
#include "wireframe.h"

#ifndef OUTLINE_WIDTH
#define OUTLINE_WIDTH 5
#endif

#ifndef OUTLINE_WIDTH_CAIRO
#define OUTLINE_WIDTH_CAIRO 2
#endif

static void
wireframeDrawXlib (WireFrame *wireframe, int width, int height)
{
    ScreenInfo *screen_info = wireframe->screen_info;

    wireframe->mapped = FALSE;
    XUnmapWindow (myScreenGetXDisplay (screen_info), wireframe->xwindow);
    XMoveResizeWindow (myScreenGetXDisplay (screen_info), wireframe->xwindow,
                       wireframe->x, wireframe->y, width, height);

    wireframe->width = width;
    wireframe->height = height;

    if ((wireframe->width > OUTLINE_WIDTH * 2) && (wireframe->height > OUTLINE_WIDTH * 2))
    {
        XRectangle xrect;
        Region inner_xregion;
        Region outer_xregion;

        inner_xregion = XCreateRegion ();
        outer_xregion = XCreateRegion ();

        xrect.x = 0;
        xrect.y = 0;
        xrect.width = wireframe->width;
        xrect.height = wireframe->height;
        XUnionRectWithRegion (&xrect, outer_xregion, outer_xregion);

        xrect.x += OUTLINE_WIDTH;
        xrect.y += OUTLINE_WIDTH;
        xrect.width -= OUTLINE_WIDTH * 2;
        xrect.height -= OUTLINE_WIDTH * 2;

        XUnionRectWithRegion (&xrect, inner_xregion, inner_xregion);

        XSubtractRegion (outer_xregion, inner_xregion, outer_xregion);

        XShapeCombineRegion (myScreenGetXDisplay (screen_info), wireframe->xwindow, ShapeBounding,
                             0, 0, outer_xregion, ShapeSet);

        XDestroyRegion (outer_xregion);
        XDestroyRegion (inner_xregion);
        XMapWindow (myScreenGetXDisplay (screen_info), wireframe->xwindow);
        wireframe->mapped = TRUE;

        XDrawRectangle (myScreenGetXDisplay (screen_info), wireframe->xwindow,
                        screen_info->box_gc,
                        0, 0, wireframe->width - 1, wireframe->height - 1);

        XDrawRectangle (myScreenGetXDisplay (screen_info), wireframe->xwindow,
                        screen_info->box_gc,
                        OUTLINE_WIDTH - 1, OUTLINE_WIDTH - 1,
                        wireframe->width - 2 * (OUTLINE_WIDTH - 1) - 1,
                        wireframe->height- 2 * (OUTLINE_WIDTH - 1) - 1);
    }
    else
    {
        /* Unset the shape */
        XShapeCombineMask (myScreenGetXDisplay (screen_info), wireframe->xwindow,
                           ShapeBounding, 0, 0, None, ShapeSet);
        XMapWindow (myScreenGetXDisplay (screen_info), wireframe->xwindow);
        wireframe->mapped = TRUE;

        XDrawRectangle (myScreenGetXDisplay (screen_info), wireframe->xwindow,
                        screen_info->box_gc,
                        0, 0, wireframe->width - 1, wireframe->height - 1);
    }
}

static void
wireframeDrawCairo (WireFrame *wireframe, int width, int height)
{
    ScreenInfo *screen_info = wireframe->screen_info;

    XMoveResizeWindow (myScreenGetXDisplay (screen_info), wireframe->xwindow,
                       wireframe->x, wireframe->y, width, height);
    if (!wireframe->mapped)
    {
        XMapWindow (myScreenGetXDisplay (screen_info), wireframe->xwindow);
        wireframe->mapped = TRUE;
    }
    if ((width == wireframe->width) && (height == wireframe->height))
    {
        /* Moving only */
        return;
    }
    wireframe->width = width;
    wireframe->height = height;

    cairo_xlib_surface_set_size(wireframe->surface, wireframe->width, wireframe->height);
    XClearWindow (myScreenGetXDisplay (screen_info), wireframe->xwindow);
    cairo_set_source_rgba (wireframe->cr, wireframe->red, wireframe->green, wireframe->blue, wireframe->alpha);
    cairo_paint (wireframe->cr);

    cairo_set_source_rgba (wireframe->cr, wireframe->red, wireframe->green, wireframe->blue, 1.0);
    cairo_rectangle (wireframe->cr,
                     OUTLINE_WIDTH_CAIRO / 2, OUTLINE_WIDTH_CAIRO / 2,
                     wireframe->width - OUTLINE_WIDTH_CAIRO,
                     wireframe->height - OUTLINE_WIDTH_CAIRO);
    cairo_stroke (wireframe->cr);
    /* Force a resize of the compositor window to avoid flickering */
    compositorResizeWindow (screen_info->display_info, wireframe->xwindow,
                            wireframe->x, wireframe->y,
                            wireframe->width, wireframe->height);
}

void
wireframeUpdate (Client *c, WireFrame *wireframe)
{
    ScreenInfo *screen_info;

    g_return_if_fail (c != NULL);
    g_return_if_fail (wireframe != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    wireframe->x = frameExtentX (c);
    wireframe->y = frameExtentY (c);

    screen_info = wireframe->screen_info;
    if (compositorIsActive (screen_info))
    {
         wireframeDrawCairo (wireframe, frameExtentWidth (c), frameExtentHeight (c));
    }
    else
    {
         wireframeDrawXlib (wireframe, frameExtentWidth (c), frameExtentHeight (c));
    }
    XFlush (myScreenGetXDisplay (screen_info));
}

static void
wireframeInitColor (WireFrame *wireframe)
{
    GdkRGBA rgba;

    if (getUIStyleColor (myScreenGetGtkWidget (wireframe->screen_info), "bg", "selected", &rgba))
    {
        wireframe->red = rgba.red;
        wireframe->green = rgba.green;
        wireframe->blue = rgba.blue;
    }
}

WireFrame *
wireframeCreate (Client *c)
{
    ScreenInfo *screen_info;
    WireFrame *wireframe;
    XSetWindowAttributes attrs;
    XVisualInfo xvisual_info;
    Visual *xvisual;
    int depth;

    g_return_val_if_fail (c != NULL, None);

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    wireframe = g_new0 (WireFrame, 1);
    wireframe->screen_info = screen_info;
    wireframe->mapped = FALSE;
    wireframe->width = 0;
    wireframe->height = 0;
    wireframe->cr = NULL;
    wireframe->surface = NULL;
    wireframe->alpha = (compositorIsActive (screen_info) ? 0.5 : 1.0);

    if (compositorIsActive (screen_info) &&
        XMatchVisualInfo (myScreenGetXDisplay (screen_info), screen_info->screen,
                          32, TrueColor, &xvisual_info))
    {
        xvisual = xvisual_info.visual;
        depth = xvisual_info.depth;
        wireframe->xcolormap = XCreateColormap (myScreenGetXDisplay (screen_info),
                                                screen_info->xroot,
                                                xvisual, AllocNone);
    }
    else
    {
        xvisual = screen_info->visual;
        depth = screen_info->depth;
        wireframe->xcolormap = screen_info->cmap;
    }

    attrs.override_redirect = True;
    attrs.colormap = wireframe->xcolormap;
    attrs.background_pixel = BlackPixel (myScreenGetXDisplay (screen_info),
                                         screen_info->screen);
    attrs.border_pixel = BlackPixel (myScreenGetXDisplay (screen_info),
                                     screen_info->screen);
    wireframe->xwindow = XCreateWindow (myScreenGetXDisplay (screen_info), screen_info->xroot,
                                        frameExtentX (c), frameExtentY (c),
                                        frameExtentWidth (c), frameExtentHeight (c),
                                        0, depth, InputOutput, xvisual,
                                        CWOverrideRedirect | CWColormap | CWBackPixel | CWBorderPixel,
                                        &attrs);

    if (compositorIsActive (screen_info))
    {
        /* Cairo */
        wireframeInitColor (wireframe);
        wireframe->surface = cairo_xlib_surface_create (myScreenGetXDisplay (screen_info),
                                                        wireframe->xwindow, xvisual,
                                                        frameExtentWidth (c), frameExtentHeight (c));
        wireframe->cr = cairo_create (wireframe->surface);
        cairo_set_line_width (wireframe->cr, OUTLINE_WIDTH_CAIRO);
        cairo_set_line_join (wireframe->cr, CAIRO_LINE_JOIN_MITER);
    }

    wireframeUpdate (c, wireframe);

    return (wireframe);
}

void
wireframeDelete (WireFrame *wireframe)
{
    ScreenInfo *screen_info;

    g_return_if_fail (wireframe != None);
    TRACE ("entering");

    screen_info = wireframe->screen_info;
    XUnmapWindow (myScreenGetXDisplay (screen_info), wireframe->xwindow);
    if (wireframe->cr)
    {
        cairo_destroy (wireframe->cr);
    }
    if (wireframe->surface)
    {
        cairo_surface_destroy (wireframe->surface);
    }
    if (wireframe->xcolormap != screen_info->cmap)
    {
        XFreeColormap (myScreenGetXDisplay (screen_info), wireframe->xcolormap);
    }
    XDestroyWindow (myScreenGetXDisplay (screen_info), wireframe->xwindow);
    g_free (wireframe);
}
