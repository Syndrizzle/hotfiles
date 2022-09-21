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


        xcompmgr - (c) 2003 Keith Packard
        metacity - (c) 2003, 2004 Red Hat, Inc.
        xfwm4    - (c) 2005-2021 Olivier Fourdan

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include <glib.h>
#include <math.h>
#include <string.h>
#include <libxfce4util/libxfce4util.h>

#ifdef HAVE_EPOXY
#include <epoxy/gl.h>
#include <epoxy/glx.h>
#ifdef HAVE_XSYNC_EXTENSION
#include <X11/extensions/sync.h>
#endif /* HAVE_XSYNC_EXTENSION */
#endif /* HAVE_EPOXY */

#ifdef HAVE_PRESENT_EXTENSION
#ifndef PRESENT_FUTURE_VERSION
#define PRESENT_FUTURE_VERSION 0
#endif /* PRESENT_FUTURE_VERSION */
#include <X11/extensions/Xpresent.h>
#endif /* HAVE_PRESENT_EXTENSION */

#include "display.h"
#include "screen.h"
#include "client.h"
#include "frame.h"
#include "hints.h"
#include "compositor.h"

#ifdef HAVE_COMPOSITOR
#include "common/xfwm-common.h"

#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>

#ifndef SHADOW_RADIUS
#define SHADOW_RADIUS   12
#endif /* SHADOW_RADIUS */

#ifndef SHADOW_OFFSET_X
#define SHADOW_OFFSET_X (-3 * SHADOW_RADIUS / 2)
#endif /* SHADOW_OFFSET_X */

#ifndef SHADOW_OFFSET_Y
#define SHADOW_OFFSET_Y (-3 * SHADOW_RADIUS / 2)
#endif /* SHADOW_OFFSET_Y */

/* Some convenient macros */
#define WIN_HAS_CLIENT(cw)              (cw->c)
#define WIN_HAS_FRAME(cw)               (WIN_HAS_CLIENT(cw) && CLIENT_HAS_FRAME(cw->c))
#define WIN_NO_SHADOW(cw)               ((cw->c) && \
                                           (FLAG_TEST (cw->c->flags, CLIENT_FLAG_FULLSCREEN | CLIENT_FLAG_BELOW) || \
                                            (cw->c->type & WINDOW_DESKTOP)))
#define WIN_IS_DOCK(cw)                 (WIN_HAS_CLIENT(cw) && (cw->c->type & WINDOW_DOCK))
#define WIN_IS_OVERRIDE(cw)             (cw->attr.override_redirect)
#define WIN_IS_ARGB(cw)                 (cw->argb)
#define WIN_IS_OPAQUE(cw)               ((cw->opacity == NET_WM_OPAQUE) && !WIN_IS_ARGB(cw))
#define WIN_IS_NATIVE_OPAQUE(cw)        ((cw->native_opacity) && !WIN_IS_ARGB(cw))
#define WIN_IS_FULLSCREEN(cw)           ((cw->attr.x <= 0) && \
                                           (cw->attr.y <= 0) && \
                                           (cw->attr.width + 2 * cw->attr.border_width >= cw->screen_info->width) && \
                                           (cw->attr.height + 2 * cw->attr.border_width >= cw->screen_info->height))
#define WIN_IS_SHAPED(cw)               ((WIN_HAS_CLIENT(cw) && FLAG_TEST (cw->c->flags, CLIENT_FLAG_HAS_SHAPE)) || \
                                           (WIN_IS_OVERRIDE(cw) && (cw->shaped)))
#define WIN_IS_MAXIMIZED(cw)            (WIN_HAS_CLIENT(cw) && FLAG_TEST_ALL (cw->c->flags, CLIENT_FLAG_MAXIMIZED))
#define WIN_IS_VIEWABLE(cw)             (cw->viewable)
#define WIN_HAS_DAMAGE(cw)              (cw->damage)
#define WIN_IS_VISIBLE(cw)              (WIN_IS_VIEWABLE(cw) && WIN_HAS_DAMAGE(cw))
#define WIN_IS_DAMAGED(cw)              (cw->damaged)
#define WIN_IS_REDIRECTED(cw)           (cw->redirected)
#define WIN_IS_SHADED(cw)               (WIN_HAS_CLIENT(cw) && FLAG_TEST (cw->c->flags, CLIENT_FLAG_SHADED))

#ifndef TIMEOUT_REPAINT_PRIORITY
#define TIMEOUT_REPAINT_PRIORITY   G_PRIORITY_DEFAULT
#endif /* TIMEOUT_REPAINT_PRIORITY */

#ifndef TIMEOUT_THROTTLED_REPAINT_PRIORITY
#define TIMEOUT_THROTTLED_REPAINT_PRIORITY   G_PRIORITY_LOW
#endif /* TIMEOUT_THROTTLED_REPAINT_PRIORITY */

#ifndef TIMEOUT_REPAINT_MS
#define TIMEOUT_REPAINT_MS   1
#endif /* TIMEOUT_REPAINT_MS */

#ifndef TIMEOUT_THROTTLED_REPAINT_MS
#define TIMEOUT_THROTTLED_REPAINT_MS   500
#endif /* TIMEOUT_THROTTLED_REPAINT_MS */

#ifndef MONITOR_ROOT_PIXMAP
#define MONITOR_ROOT_PIXMAP   1
#endif /* MONITOR_ROOT_PIXMAP */

typedef struct _CWindow CWindow;
struct _CWindow
{
    ScreenInfo *screen_info;
    Client *c;
    Window id;
    XWindowAttributes attr;

    gboolean damaged;
    gboolean viewable;
    gboolean shaped;
    gboolean redirected;
    gboolean fulloverlay;
    gboolean argb;
    gboolean skipped;
    gboolean native_opacity;
    gboolean opacity_locked;

    Damage damage;
#if HAVE_NAME_WINDOW_PIXMAP
    Pixmap name_window_pixmap;
#endif /* HAVE_NAME_WINDOW_PIXMAP */
    Picture picture;
    Picture saved_picture;
    Picture shadow;
    Picture alphaPict;
    Picture shadowPict;
    Picture alphaBorderPict;

    XserverRegion borderSize;
    XserverRegion clientSize;
    XserverRegion borderClip;
    XserverRegion extents;
    XserverRegion opaque_region;

    gint shadow_dx;
    gint shadow_dy;
    gint shadow_width;
    gint shadow_height;

    guint32 opacity;
    guint32 bypass_compositor;
};

static CWindow*
find_cwindow_in_screen (ScreenInfo *screen_info, Window id)
{
    g_return_val_if_fail (id != None, NULL);
    g_return_val_if_fail (screen_info != NULL, NULL);
    TRACE ("window 0x%lx", id);

    return g_hash_table_lookup(screen_info->cwindow_hash, (gpointer) id);
}

static CWindow*
find_cwindow_in_display (DisplayInfo *display_info, Window id)
{
    GSList *list;

    g_return_val_if_fail (id != None, NULL);
    g_return_val_if_fail (display_info != NULL, NULL);
    TRACE ("window 0x%lx", id);

    for (list = display_info->screens; list; list = g_slist_next (list))
    {
        ScreenInfo *screen_info = (ScreenInfo *) list->data;
        CWindow *cw;

        if (!compositorIsActive (screen_info))
        {
            continue;
        }

        cw = find_cwindow_in_screen (screen_info, id);
        if (cw)
        {
            return (cw);
        }
    }
    return NULL;
}

static gboolean
is_output (DisplayInfo *display_info, Window id)
{
    GSList *list;

    g_return_val_if_fail (id != None, FALSE);
    g_return_val_if_fail (display_info != NULL, FALSE);
    TRACE ("window 0x%lx", id);

    for (list = display_info->screens; list; list = g_slist_next (list))
    {
        ScreenInfo *screen_info = (ScreenInfo *) list->data;
#if HAVE_OVERLAYS
        if (id == screen_info->output || id == screen_info->overlay)
#else
        if (id == screen_info->output)
#endif
        {
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean
is_shaped (DisplayInfo *display_info, Window id)
{
    int xws, yws, xbs, ybs;
    unsigned wws, hws, wbs, hbs;
    int boundingShaped, clipShaped;
    int result;

    g_return_val_if_fail (display_info != NULL, FALSE);

    if (display_info->have_shape)
    {
        myDisplayErrorTrapPush (display_info);
        XShapeQueryExtents (display_info->dpy, id, &boundingShaped, &xws, &yws, &wws,
                            &hws, &clipShaped, &xbs, &ybs, &wbs, &hbs);
        result = myDisplayErrorTrapPop (display_info);

        return ((result == Success) && (boundingShaped != 0));
    }
    return FALSE;
}

static gboolean
is_fullscreen (CWindow *cw)
{
    GdkRectangle rect;

    /* First, check the good old way, the window is larger than the screen size */
    if (WIN_IS_FULLSCREEN(cw))
    {
        return TRUE;
    }

    /* Next check against the monitors which compose the entire screen */
    myScreenFindMonitorAtPoint (cw->screen_info,
                                cw->attr.x + (cw->attr.width + 2 * cw->attr.border_width) / 2,
                                cw->attr.y + (cw->attr.height + 2 * cw->attr.border_width) / 2,
                                &rect);

    return ((cw->attr.x == rect.x) &&
            (cw->attr.y == rect.y) &&
            (cw->attr.width + 2 * cw->attr.border_width == rect.width) &&
            (cw->attr.height + 2 * cw->attr.border_width == rect.height));
}

static gboolean
is_on_compositor (CWindow *cw)
{
    return (cw && compositorIsActive (cw->screen_info));
}

static gdouble
gaussian (gdouble r, gdouble x, gdouble y)
{
    return ((1 / (sqrt (2 * G_PI * r))) *
            exp ((- (x * x + y * y)) / (2 * r * r)));
}

static gaussian_conv *
make_gaussian_map (gdouble r)
{
    gaussian_conv *c;
    gint size, center;
    gint x, y;
    gdouble t;
    gdouble g;

    TRACE ("entering");

    size = ((gint) ceil ((r * 3)) + 1) & ~1;
    center = size / 2;
    c = g_malloc0 (sizeof (gaussian_conv) + size * size * sizeof (gdouble));
    c->size = size;
    c->data = (gdouble *) (c + 1);
    t = 0.0;

    for (y = 0; y < size; y++)
    {
        for (x = 0; x < size; x++)
        {
            g = gaussian (r, (gdouble) (x - center), (gdouble) (y - center));
            t += g;
            c->data[y * size + x] = g;
        }
    }

    for (y = 0; y < size; y++)
    {
        for (x = 0; x < size; x++)
        {
            c->data[y*size + x] /= t;
        }
    }

    return c;
}

/*
* A picture will help
*
*      -center   0                width  width+center
*  -center +-----+-------------------+-----+
*          |     |                   |     |
*          |     |                   |     |
*        0 +-----+-------------------+-----+
*          |     |                   |     |
*          |     |                   |     |
*          |     |                   |     |
*   height +-----+-------------------+-----+
*          |     |                   |     |
* height+  |     |                   |     |
*  center  +-----+-------------------+-----+
*/

static guchar
sum_gaussian (gaussian_conv *map, gdouble opacity, gint x, gint y, gint width, gint height)
{
    gdouble *g_data, *g_line;
    gdouble v;
    gint fx, fy;
    gint fx_start, fx_end;
    gint fy_start, fy_end;
    gint g_size, center;

    g_return_val_if_fail (map != NULL, (guchar) 255.0);
    TRACE ("(%i,%i) [%i×%i]", x, y, width, height);

    g_line = map->data;
    g_size = map->size;
    center = g_size / 2;
    fx_start = center - x;
    if (fx_start < 0)
    {
        fx_start = 0;
    }
    fx_end = width + center - x;
    if (fx_end > g_size)
    {
        fx_end = g_size;
    }

    fy_start = center - y;
    if (fy_start < 0)
    {
        fy_start = 0;
    }
    fy_end = height + center - y;
    if (fy_end > g_size)
    {
        fy_end = g_size;
    }
    g_line = g_line + fy_start * g_size + fx_start;

    v = 0;
    for (fy = fy_start; fy < fy_end; fy++)
    {
        g_data = g_line;
        g_line += g_size;

        for (fx = fx_start; fx < fx_end; fx++)
        {
            v += *g_data++;
        }
    }
    if (v > 1)
    {
        v = 1;
    }

    return ((guchar) (v * opacity * 255.0));
}

/* precompute shadow corners and sides to save time for large windows */
static void
presum_gaussian (ScreenInfo *screen_info)
{
    gint center;
    gint opacity, x, y;
    gaussian_conv * map;

    g_return_if_fail (screen_info != NULL);
    g_return_if_fail (screen_info->gaussianMap != NULL);
    TRACE ("entering");

    map = screen_info->gaussianMap;
    screen_info->gaussianSize = map->size;
    center = map->size / 2;

    if (screen_info->shadowCorner)
    {
        g_free (screen_info->shadowCorner);
    }
    if (screen_info->shadowTop)
    {
        g_free (screen_info->shadowTop);
    }

    screen_info->shadowCorner = (guchar *) (g_malloc0 ((screen_info->gaussianSize + 1)
                                                    * (screen_info->gaussianSize + 1) * 26));
    screen_info->shadowTop = (guchar *) (g_malloc0 ((screen_info->gaussianSize + 1) * 26));

    for (x = 0; x <= screen_info->gaussianSize; x++)
    {
        screen_info->shadowTop[25 * (screen_info->gaussianSize + 1) + x] =
            sum_gaussian (map, 1, x - center, center,
                          screen_info->gaussianSize * 2,
                          screen_info->gaussianSize * 2);

        for(opacity = 0; opacity < 25; opacity++)
        {
            screen_info->shadowTop[opacity * (screen_info->gaussianSize + 1) + x] =
                screen_info->shadowTop[25 * (screen_info->gaussianSize + 1) + x] * opacity / 25;
        }

        for(y = 0; y <= x; y++)
        {
            screen_info->shadowCorner[25 * (screen_info->gaussianSize + 1)
                                         * (screen_info->gaussianSize + 1)
                                     + y * (screen_info->gaussianSize + 1) + x]
                = sum_gaussian (map, 1, x - center,
                                        y - center,
                                        screen_info->gaussianSize * 2,
                                        screen_info->gaussianSize * 2);
            screen_info->shadowCorner[25 * (screen_info->gaussianSize + 1)
                                         * (screen_info->gaussianSize + 1)
                                     + x * (screen_info->gaussianSize + 1) + y]
                = screen_info->shadowCorner[25 * (screen_info->gaussianSize + 1)
                                               * (screen_info->gaussianSize + 1)
                                           + y * (screen_info->gaussianSize + 1) + x];

            for(opacity = 0; opacity < 25; opacity++)
            {
                screen_info->shadowCorner[opacity * (screen_info->gaussianSize + 1)
                                                  * (screen_info->gaussianSize + 1)
                                              + y * (screen_info->gaussianSize + 1) + x]
                    = screen_info->shadowCorner[opacity * (screen_info->gaussianSize + 1)
                                                        * (screen_info->gaussianSize + 1)
                                                    + x * (screen_info->gaussianSize + 1) + y]
                    = screen_info->shadowCorner[25 * (screen_info->gaussianSize + 1)
                                                   * (screen_info->gaussianSize + 1)
                                               + y * (screen_info->gaussianSize + 1) + x] * opacity / 25;
            }
        }
    }
}

static XImage *
make_shadow (ScreenInfo *screen_info, gdouble opacity, gint width, gint height)
{
    DisplayInfo *display_info;
    XImage *ximage;
    guchar *data;
    guchar d;
    gint gaussianSize;
    gint ylimit, xlimit;
    gint swidth;
    gint sheight;
    gint center;
    gint x, y;
    gint x_diff;
    gint opacity_int;
    gint x_swidth;
    gint y_swidth;

    g_return_val_if_fail (screen_info != NULL, NULL);
    TRACE ("entering");

    display_info = screen_info->display_info;
    gaussianSize = screen_info->gaussianMap->size;
    swidth = width + gaussianSize - screen_info->params->shadow_delta_width - screen_info->params->shadow_delta_x;
    sheight = height + gaussianSize - screen_info->params->shadow_delta_height - screen_info->params->shadow_delta_y;
    center = gaussianSize / 2;
    opacity_int = (gint) (opacity * 25);

    if ((swidth < 1) || (sheight < 1))
    {
        return NULL;
    }

    data = g_malloc0 (swidth * sheight * sizeof (guchar));

    ximage = XCreateImage (display_info->dpy,
                        DefaultVisual(display_info->dpy, screen_info->screen),
                        8, ZPixmap, 0, (char *) data,
                        swidth, sheight, 8, swidth * sizeof (guchar));
    if (ximage == NULL)
    {
        g_free (data);
        g_warning ("(ximage != NULL) failed");
        return NULL;
    }

    /*
    * Build the gaussian in sections
    */

    if (screen_info->gaussianSize > 0)
    {
        d = screen_info->shadowTop[opacity_int * (screen_info->gaussianSize + 1) + screen_info->gaussianSize];
    }
    else
    {
        d = sum_gaussian (screen_info->gaussianMap, opacity, center, center, width, height);
    }
    memset(data, d, sheight * swidth);

    /*
    * corners
    */

    ylimit = gaussianSize;
    if (ylimit > sheight / 2)
    {
        ylimit = (sheight + 1) / 2;
    }
    xlimit = gaussianSize;
    if (xlimit > swidth / 2)
    {
        xlimit = (swidth + 1) / 2;
    }
    for (y = 0; y < ylimit; y++)
    {
        for (x = 0; x < xlimit; x++)
        {
            if ((xlimit == screen_info->gaussianSize) && (ylimit == screen_info->gaussianSize))
            {
                d = screen_info->shadowCorner[opacity_int * (screen_info->gaussianSize + 1)
                                                        * (screen_info->gaussianSize + 1)
                                                    + y * (screen_info->gaussianSize + 1) + x];
            }
            else
            {
                d = sum_gaussian (screen_info->gaussianMap, opacity,
                                x - center, y - center, width, height);
            }

            data[y * swidth + x] = d;
            data[(sheight - y - 1) * swidth + x] = d;
            data[(sheight - y - 1) * swidth + (swidth - x - 1)] = d;
            data[y * swidth + (swidth - x - 1)] = d;
        }
    }

    /*
    * top/bottom
    */

    x_diff = swidth - (gaussianSize * 2);
    if (x_diff > 0 && ylimit > 0)
    {
        for (y = 0; y < ylimit; y++)
        {
            if (ylimit == screen_info->gaussianSize)
            {
                d = screen_info->shadowTop[opacity_int * (screen_info->gaussianSize + 1) + y];
            }
            else
            {
                d = sum_gaussian (screen_info->gaussianMap, opacity, center, y - center, width, height);
            }
            memset (&data[y * swidth + gaussianSize], d, x_diff);
            memset (&data[(sheight - y - 1) * swidth + gaussianSize], d, x_diff);
        }
    }

    /*
    * sides
    */

    for (x = 0; x < xlimit; x++)
    {
        if (xlimit == screen_info->gaussianSize)
        {
            d = screen_info->shadowTop[opacity_int * (screen_info->gaussianSize + 1) + x];
        }
        else
        {
            d = sum_gaussian (screen_info->gaussianMap, opacity, x - center, center, width, height);
        }
        x_swidth = swidth - x - 1;
        for (y = gaussianSize; y < sheight - gaussianSize; y++)
        {
            y_swidth = y * swidth;
            data[y_swidth + x] = d;
            data[y_swidth + x_swidth] = d;
        }
    }

    return ximage;
}

static Picture
shadow_picture (ScreenInfo *screen_info, gdouble opacity,
                gint width, gint height, gint *wp, gint *hp)
{
    DisplayInfo *display_info;
    XImage *shadowImage;
    Pixmap shadowPixmap;
    Picture shadowPicture;
    XRenderPictFormat *render_format;
    GC gc;

    g_return_val_if_fail (screen_info != NULL, None);
    TRACE ("entering");

    display_info = screen_info->display_info;
    render_format = XRenderFindStandardFormat (display_info->dpy, PictStandardA8);
    g_return_val_if_fail (render_format != NULL, None);

    shadowImage = make_shadow (screen_info, opacity, width, height);
    if (shadowImage == NULL)
    {
        *wp = *hp = 0;
        g_warning ("(shadowImage != NULL) failed");
        return (None);
    }

    shadowPixmap = XCreatePixmap (display_info->dpy, screen_info->output,
                                shadowImage->width, shadowImage->height, 8);
    if (shadowPixmap == None)
    {
        XDestroyImage (shadowImage);
        g_warning ("(shadowPixmap != None) failed");
        return None;
    }

    shadowPicture = XRenderCreatePicture (display_info->dpy,
                                        shadowPixmap, render_format, 0, NULL);
    if (shadowPicture == None)
    {
        XDestroyImage (shadowImage);
        XFreePixmap (display_info->dpy, shadowPixmap);
        g_warning ("(shadowPicture != None) failed");
        return None;
    }

    gc = XCreateGC (display_info->dpy, shadowPixmap, 0, NULL);
    XPutImage (display_info->dpy, shadowPixmap, gc, shadowImage, 0, 0, 0, 0,
            shadowImage->width, shadowImage->height);

    *wp = shadowImage->width;
    *hp = shadowImage->height;

    XFreeGC (display_info->dpy, gc);
    XDestroyImage (shadowImage);
    XFreePixmap (display_info->dpy, shadowPixmap);

    return shadowPicture;
}

static Picture
solid_picture (ScreenInfo *screen_info, gboolean argb,
               gdouble a, gdouble r, gdouble g, gdouble b)
{
    DisplayInfo *display_info;
    Pixmap pixmap;
    Picture picture;
    XRenderPictureAttributes pa;
    XRenderPictFormat *render_format;
    XRenderColor c;

    g_return_val_if_fail (screen_info, None);
    TRACE ("entering");

    display_info = screen_info->display_info;
    render_format = XRenderFindStandardFormat (display_info->dpy,
                                    argb ? PictStandardARGB32 : PictStandardA8);
    g_return_val_if_fail (render_format != NULL , None);

    pixmap = XCreatePixmap (display_info->dpy,
                            screen_info->output, 1, 1, argb ? 32 : 8);
    g_return_val_if_fail (pixmap != None, None);

    pa.repeat = TRUE;
    picture = XRenderCreatePicture (display_info->dpy, pixmap,
                                    render_format, CPRepeat, &pa);
    if (picture == None)
    {
        XFreePixmap (display_info->dpy, pixmap);
        g_warning ("(picture != None) failed");
        return None;
    }

    c.alpha = a * 0xffff;
    c.red   = r * 0xffff;
    c.green = g * 0xffff;
    c.blue  = b * 0xffff;

    XRenderFillRectangle (display_info->dpy, PictOpSrc,
                          picture, &c, 0, 0, 1, 1);
    XFreePixmap (display_info->dpy, pixmap);

    return picture;
}

static void
translate_to_client_region (CWindow *cw, XserverRegion region)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;
    int x, y;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx", cw->id);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;

    if (WIN_HAS_FRAME(cw))
    {
        x = frameX (cw->c) + frameLeft (cw->c);
        y = frameY (cw->c) + frameTop (cw->c);
    }
    else
    {
        x = cw->attr.x + cw->attr.border_width;
        y = cw->attr.y + cw->attr.border_width;
    }

    XFixesTranslateRegion (display_info->dpy, region, x, y);
}

static XserverRegion
client_size (CWindow *cw)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;
    XserverRegion border;

    g_return_val_if_fail (cw != NULL, None);
    TRACE ("window 0x%lx", cw->id);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;
    border = None;

    if (WIN_HAS_FRAME(cw))
    {
        XRectangle  r;
        Client *c;

        c = cw->c;
        r.x = frameX (c) + frameLeft (c);
        r.y = frameY (c) + frameTop (c);
        r.width = frameWidth (c) - frameLeft (c) - frameRight (c);
        r.height = frameHeight (c) - frameTop (c) - frameBottom (c);
        border = XFixesCreateRegion (display_info->dpy, &r, 1);
    }

    return border;
}

static XserverRegion
border_size (CWindow *cw)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;
    XserverRegion border;

    g_return_val_if_fail (cw != NULL, None);
    TRACE ("window 0x%lx", cw->id);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;
    myDisplayErrorTrapPush (display_info);
    border = XFixesCreateRegionFromWindow (display_info->dpy,
                                           cw->id, WindowRegionBounding);
    XFixesSetPictureClipRegion (display_info->dpy, cw->picture, 0, 0, border);
    XFixesTranslateRegion (display_info->dpy, border,
                           cw->attr.x + cw->attr.border_width,
                           cw->attr.y + cw->attr.border_width);

    if (myDisplayErrorTrapPop (display_info) != Success)
    {
        return None;
    }

    return border;
}

static void
free_win_data (CWindow *cw, gboolean delete)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;

    myDisplayErrorTrapPush (display_info);
#if HAVE_NAME_WINDOW_PIXMAP
    if (cw->name_window_pixmap)
    {
        XFreePixmap (display_info->dpy, cw->name_window_pixmap);
        cw->name_window_pixmap = None;
    }
#endif

    if (cw->shadow)
    {
        XRenderFreePicture (display_info->dpy, cw->shadow);
        cw->shadow = None;
    }

    if (cw->alphaPict)
    {
        XRenderFreePicture (display_info->dpy, cw->alphaPict);
        cw->alphaPict = None;
    }

    if (cw->shadowPict)
    {
        XRenderFreePicture (display_info->dpy, cw->shadowPict);
        cw->shadowPict = None;
    }

    if (cw->alphaBorderPict)
    {
        XRenderFreePicture (display_info->dpy, cw->alphaBorderPict);
        cw->alphaBorderPict = None;
    }

    if (cw->borderSize)
    {
        XFixesDestroyRegion (display_info->dpy, cw->borderSize);
        cw->borderSize = None;
    }

    if (cw->clientSize)
    {
        XFixesDestroyRegion (display_info->dpy, cw->clientSize);
        cw->clientSize = None;
    }

    if (cw->borderClip)
    {
        XFixesDestroyRegion (display_info->dpy, cw->borderClip);
        cw->borderClip = None;
    }

    if (cw->extents)
    {
        XFixesDestroyRegion (display_info->dpy, cw->extents);
        cw->extents = None;
    }

    if (delete)
    {
        if (cw->picture)
        {
            XRenderFreePicture (display_info->dpy, cw->picture);
            cw->picture = None;
        }
        /* No need to keep this around */
        if (cw->saved_picture)
        {
            XRenderFreePicture (display_info->dpy, cw->saved_picture);
            cw->saved_picture = None;
        }

        if (cw->damage)
        {
            XDamageDestroy (display_info->dpy, cw->damage);
            cw->damage = None;
        }

        if (cw->opaque_region)
        {
            XFixesDestroyRegion (display_info->dpy, cw->opaque_region);
            cw->opaque_region = None;
        }

        g_slice_free (CWindow, cw);
    }
    else
    {
        if (cw->saved_picture)
        {
            XRenderFreePicture (display_info->dpy, cw->saved_picture);
        }
        cw->saved_picture = cw->picture;
        cw->picture = None;
    }
    myDisplayErrorTrapPopIgnored (display_info);
}

static Picture
root_tile (ScreenInfo *screen_info)
{
    DisplayInfo *display_info;
    Display *dpy;
    Picture picture = None;
#if MONITOR_ROOT_PIXMAP
    Pixmap pixmap;
    XRenderPictureAttributes pa;
    XRenderPictFormat *format;
    gint p;
    Atom backgroundProps[2];
#endif

    g_return_val_if_fail (screen_info != NULL, None);
    TRACE ("entering");

    display_info = screen_info->display_info;
    dpy = display_info->dpy;
#if MONITOR_ROOT_PIXMAP
    backgroundProps[0] = display_info->atoms[XROOTPMAP];
    backgroundProps[1] = display_info->atoms[XSETROOT];

    for (p = 0; p < 2; p++)
    {
        Atom actual_type;
        gint actual_format;
        unsigned long nitems;
        unsigned long bytes_after;
        guchar *prop;
        gint result;

        result = XGetWindowProperty (dpy, screen_info->xroot, backgroundProps[p],
                                0, 4, False, AnyPropertyType,
                                &actual_type, &actual_format, &nitems, &bytes_after, &prop);

        if ((result == Success) &&
            (actual_type == display_info->atoms[PIXMAP]) &&
            (actual_format == 32) &&
            (nitems == 1))
        {
            memcpy (&pixmap, prop, 4);
            XFree (prop);
            pa.repeat = TRUE;
            format = XRenderFindVisualFormat (dpy, screen_info->visual);
            g_return_val_if_fail (format != NULL, None);
            picture = XRenderCreatePicture (dpy, pixmap, format, CPRepeat, &pa);
            break;
        }
    }
#endif
    if (picture == None)
    {
        XRenderColor c;

        /* Background default to just plain black */
        c.red   = 0x0000;
        c.green = 0x0000;
        c.blue  = 0x0000;
        c.alpha = 0xffff;
        picture = XRenderCreateSolidFill (dpy, &c);
    }
    return picture;
}

static Pixmap
safe_create_pixmap (ScreenInfo *screen_info,
                    Drawable d,
                    unsigned int width,
                    unsigned int height,
                    unsigned int depth)
{
    Display *dpy;
    Pixmap pixmap;
    XGCValues gv;
    GC gc;

    myDisplayErrorTrapPush (screen_info->display_info);
    dpy = myScreenGetXDisplay (screen_info);
    pixmap = XCreatePixmap (dpy, d, width, height, depth);
    gv.foreground = BlackPixel (dpy, screen_info->screen);
    gc = XCreateGC (dpy, pixmap, GCForeground, &gv);
    XFillRectangle (dpy, pixmap, gc, 0, 0, width, height);
    XFreeGC (dpy, gc);
    myDisplayErrorTrapPopIgnored (screen_info->display_info);

    return pixmap;
}

static Pixmap
create_root_pixmap (ScreenInfo *screen_info)
{
    return safe_create_pixmap (screen_info,
                               screen_info->xroot,
                               screen_info->width,
                               screen_info->height,
                               screen_info->depth);
}

static Picture
create_root_buffer (ScreenInfo *screen_info, Pixmap pixmap)
{
    DisplayInfo *display_info;
    Picture pict;
    XRenderPictFormat *format;

    g_return_val_if_fail (screen_info != NULL, None);
    g_return_val_if_fail (pixmap != None, None);
    TRACE ("entering");

    display_info = screen_info->display_info;
    format = XRenderFindVisualFormat (display_info->dpy, screen_info->visual);
    g_return_val_if_fail (format != NULL, None);

    pict = XRenderCreatePicture (display_info->dpy,
                                 pixmap, format, 0, NULL);

    return pict;
}

static Picture
cursor_to_picture (ScreenInfo *screen_info, XFixesCursorImage *cursor)
{
    DisplayInfo       *display_info;
    XRenderPictFormat *render_format;
    XImage            *ximage;
    guint32           *data;
    Pixmap             pixmap;
    Picture            picture;
    GC                 gc;
    gint               i;

    g_return_val_if_fail (screen_info != NULL, None);
    TRACE ("entering");

    display_info = screen_info->display_info;

    /* XFixesGetCursorImage() returns an array of long but actual data is 32bit */
    data = g_malloc0 (cursor->width * cursor->height * sizeof (guint32));
    for (i = 0; i < cursor->width * cursor->height; i++)
    {
        data[i] = cursor->pixels[i];
    }

    ximage = XCreateImage (display_info->dpy,
                           screen_info->visual,
                           32, ZPixmap, 0, (char *) data,
                           cursor->width, cursor->height, 32,
                           cursor->width * sizeof (guint32));

    if (!ximage)
    {
        g_warning ("Failed to create the cursor image");
        return None;
    }

    pixmap = XCreatePixmap (display_info->dpy,
                            screen_info->output,
                            cursor->width,
                            cursor->height,
                            32);

    gc = XCreateGC (display_info->dpy, pixmap, 0, NULL);
    XPutImage (display_info->dpy, pixmap, gc, ximage,
                0, 0, 0, 0, cursor->width, cursor->height);
    XFreeGC (display_info->dpy, gc);
    XDestroyImage (ximage);

    render_format = XRenderFindStandardFormat (display_info->dpy,
                                               PictStandardARGB32);
    picture = XRenderCreatePicture (display_info->dpy,
                                    pixmap,
                                    render_format,
                                    0, NULL);
    XFreePixmap (display_info->dpy, pixmap);

    return picture;
}

#ifdef HAVE_EPOXY
static gboolean
check_gl_error (void)
{
    gboolean clean = TRUE;
#ifdef DEBUG /* glGetError() is expensive, keep it for debug only */
    GLenum error;

     error = glGetError();
     while (error != GL_NO_ERROR)
     {
        clean = FALSE;
        switch (error)
        {
            case GL_INVALID_ENUM:
                g_warning ("GL error: Invalid enum");
                break;
            case GL_INVALID_VALUE:
                g_warning ("GL error: Invalid value");
                break;
            case GL_INVALID_OPERATION:
                g_warning ("GL error: Invalid operation");
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                g_warning ("GL error: Invalid frame buffer operation");
                break;
            case GL_OUT_OF_MEMORY:
                g_warning ("GL error: Out of memory");
                break;
            default:
                break;
        }
        error = glGetError();
    }
#endif
    return clean;
}

static gboolean
check_glx_renderer (ScreenInfo *screen_info)
{
    const char *glRenderer;
    const char *blacklisted[] = {
        "Software Rasterizer",
        "Mesa X11",
        "llvmpipe",
        "SVGA3D",
        "virgl",
        NULL
    };
#if HAVE_PRESENT_EXTENSION
    const char *prefer_xpresent[] = {
        "Intel",
        /* Cannot add AMD and Radeon until the fix for
         * https://gitlab.freedesktop.org/xorg/driver/xf86-video-amdgpu/-/issues/10
         * is included in a release.
         */
        /* "AMD", */
        /* "Radeon", */
        NULL
    };
#endif /* HAVE_PRESENT_EXTENSION */
    int i;

    g_return_val_if_fail (screen_info != NULL, FALSE);
    TRACE ("entering");

    glRenderer = (const char *) glGetString (GL_RENDERER);
    if (glRenderer == NULL)
    {
        g_warning ("Cannot identify GLX renderer.");
        return FALSE;
    }
    DBG ("Using GL renderer: %s", glRenderer);

#if HAVE_PRESENT_EXTENSION
    if (screen_info->vblank_mode == VBLANK_AUTO)
    {
        i = 0;
        while (prefer_xpresent[i] && !strcasestr (glRenderer, prefer_xpresent[i]))
            i++;
        if (prefer_xpresent[i])
        {
            g_info ("Prefer XPresent with %s", glRenderer);
            return FALSE;
        }
    }
#endif /* HAVE_PRESENT_EXTENSION */

    i = 0;
    while (blacklisted[i] && !strcasestr (glRenderer, blacklisted[i]))
        i++;
    if (blacklisted[i])
    {
        g_warning ("Unsupported GL renderer (%s).", glRenderer);
        return FALSE;
    }

    return TRUE;
}

static gboolean
check_gl_extensions (ScreenInfo *screen_info)
{
    g_return_val_if_fail (screen_info != NULL, FALSE);

    /* Note: the GL context must be current for this to work */
    if (screen_info->texture_type == GL_TEXTURE_2D)
    {
        /*
         * If all we have is GL_TEXTURE_2D then we ought to have
         * GL_ARB_texture_non_power_of_two, otherwise we'll fail.
         */
        return (epoxy_has_gl_extension ("GL_ARB_texture_non_power_of_two"));
    }

    return TRUE;
}

static gboolean
choose_glx_settings (ScreenInfo *screen_info)
{
    static GLint visual_attribs[] = {
        GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT | GLX_WINDOW_BIT,
        GLX_X_RENDERABLE,  True,
        GLX_DOUBLEBUFFER,  True,
        GLX_CONFIG_CAVEAT, GLX_NONE,
        GLX_DEPTH_SIZE,    1,
        GLX_RED_SIZE,      1,
        GLX_GREEN_SIZE,    1,
        GLX_BLUE_SIZE,     1,
        GLX_RENDER_TYPE,   GLX_RGBA_BIT,
        None
    };
    GLint texture_type = 0;
    GLint texture_target = 0;
    GLint texture_format = 0;
    gboolean texture_inverted = FALSE;
    int n_configs, i;
    int value, status;
    GLXFBConfig *configs, fb_config;
    XVisualInfo *visual_info;
    gboolean fb_match;
    VisualID xvisual_id;

    g_return_val_if_fail (screen_info != NULL, FALSE);
    TRACE ("entering");

    configs = glXChooseFBConfig (myScreenGetXDisplay (screen_info),
                                 screen_info->screen,
                                 visual_attribs,
                                 &n_configs);
    if (configs == NULL)
    {
        g_warning ("Cannot retrieve GLX frame buffer config.");
        return FALSE;
    }

    fb_match = FALSE;
    xvisual_id = XVisualIDFromVisual (screen_info->visual);
    for (i = 0; i < n_configs; i++)
    {
        visual_info = glXGetVisualFromFBConfig (myScreenGetXDisplay (screen_info),
                                                configs[i]);
        if (!visual_info)
        {
            DBG ("%i/%i: no visual info, skipped", i + 1, n_configs);
            continue;
        }

        if (visual_info->visualid != xvisual_id)
        {
            DBG ("%i/%i: xvisual id 0x%lx != 0x%lx, skipped", i + 1, n_configs, visual_info->visualid, xvisual_id);
            XFree (visual_info);
            continue;
        }
        XFree (visual_info);

        status = glXGetFBConfigAttrib (myScreenGetXDisplay (screen_info),
                                       configs[i],
                                       GLX_DRAWABLE_TYPE, &value);

        if (status != Success || !(value & GLX_PIXMAP_BIT))
        {
            DBG ("%i/%i: No GLX_PIXMAP_BIT, skipped", i + 1, n_configs);
            continue;
        }

        status = glXGetFBConfigAttrib (myScreenGetXDisplay (screen_info),
                                       configs[i],
                                       GLX_BIND_TO_TEXTURE_TARGETS_EXT,
                                       &value);
        if (status != Success)
        {
            DBG ("%i/%i: No GLX_BIND_TO_TEXTURE_TARGETS_EXT, skipped", i + 1, n_configs);
            continue;
        }

        if (value & GLX_TEXTURE_RECTANGLE_BIT_EXT)
        {
            DBG ("Using texture GL_TEXTURE_RECTANGLE_ARB target GLX_TEXTURE_RECTANGLE_EXT");
            texture_type = GL_TEXTURE_RECTANGLE_ARB;
            texture_target = GLX_TEXTURE_RECTANGLE_EXT;
        }
        else if (value & GLX_TEXTURE_2D_BIT_EXT)
        {
            DBG ("Using texture GL_TEXTURE_2D target GLX_TEXTURE_2D_EXT");
            texture_type = GL_TEXTURE_2D;
            texture_target = GLX_TEXTURE_2D_EXT;
        }
        else
        {
            DBG ("%i/%i: No GLX_TEXTURE_*_BIT_EXT, skipped", i + 1, n_configs);
            continue;
        }

        status = glXGetFBConfigAttrib (myScreenGetXDisplay (screen_info),
                                       configs[i],
                                       GLX_RED_SIZE,
                                       &value);
        if (status == Success && value > 8)
        {
            DBG ("%i/%i: RGB10 config, skipped", i + 1, n_configs);
            continue;
        }

        status = glXGetFBConfigAttrib (myScreenGetXDisplay (screen_info),
                                       configs[i],
                                       GLX_BIND_TO_TEXTURE_RGBA_EXT,
                                       &value);
        if (status == Success && value == TRUE)
        {
            DBG ("Using texture format GLX_TEXTURE_FORMAT_RGBA_EXT");
            texture_format = GLX_TEXTURE_FORMAT_RGBA_EXT;
        }
        else
        {
            status = glXGetFBConfigAttrib (myScreenGetXDisplay (screen_info),
                                           configs[i],
                                           GLX_BIND_TO_TEXTURE_RGB_EXT,
                                           &value);
            if (status == Success && value == TRUE)
            {
                DBG ("Using texture format GLX_TEXTURE_FORMAT_RGB_EXT");
                texture_format = GLX_TEXTURE_FORMAT_RGB_EXT;
            }
            else
            {
                DBG ("%i/%i: No GLX_BIND_TO_TEXTURE_RGB/RGBA_EXT, skipped", i + 1, n_configs);
                continue;
            }
        }
#if 0
        status = glXGetFBConfigAttrib (myScreenGetXDisplay (screen_info),
                                       configs[i],
                                       GLX_Y_INVERTED_EXT,
                                       &value);
        texture_inverted = (status == Success && value == True);
        if (texture_inverted)
        {
            DBG ("Using texture attribute GLX_Y_INVERTED_EXT");
        }
#endif
        fb_config = configs[i];
        fb_match = TRUE;
        break;
    }
    XFree(configs);

    if (fb_match == FALSE)
    {
        g_warning ("Cannot find a matching visual for the frame buffer config.");
        return FALSE;
    }
    DBG ("Selected texture type 0x%x target 0x%x format 0x%x (%s)",
         texture_type, texture_target, texture_format,
         texture_inverted ? "inverted" : "non inverted");

    screen_info->texture_type = texture_type;
    screen_info->texture_target = texture_target;
    screen_info->texture_format = texture_format;
    screen_info->texture_inverted = texture_inverted;
    screen_info->glx_fbconfig = fb_config;

    return TRUE;
}

static void
free_glx_data (ScreenInfo *screen_info)
{
    DisplayInfo *display_info;

    g_return_if_fail (screen_info != NULL);

    display_info = screen_info->display_info;
    myDisplayErrorTrapPush (display_info);

    glXMakeCurrent (myScreenGetXDisplay (screen_info), None, NULL);

    if (screen_info->glx_context)
    {
        glXDestroyContext (myScreenGetXDisplay (screen_info), screen_info->glx_context);
        screen_info->glx_context = None;
    }

    if (screen_info->glx_window)
    {
        glXDestroyWindow (myScreenGetXDisplay (screen_info), screen_info->glx_window);
        screen_info->glx_window = None;
    }

    if (screen_info->has_ext_arb_sync && screen_info->gl_sync)
    {
#if defined (glDeleteSync)
        glDeleteSync (screen_info->gl_sync);
#else
#warning glDeleteSync() not supported by libepoxy, please update your version of libepoxy
#endif
        screen_info->gl_sync = 0;
    }
    myDisplayErrorTrapPopIgnored (display_info);
}

static gboolean
init_glx (ScreenInfo *screen_info)
{
    int error_base, event_base;
    int version;

    g_return_val_if_fail (screen_info != NULL, FALSE);
    TRACE ("entering");

    if (!xfwm_is_default_screen (screen_info->gscr))
    {
        g_warning ("GLX not enabled in multi-screen setups");
        return FALSE;
    }

    if (!glXQueryExtension (myScreenGetXDisplay (screen_info), &error_base, &event_base))
    {
        g_warning ("GLX extension missing, GLX support disabled.");
        return FALSE;
    }

    version = epoxy_glx_version (myScreenGetXDisplay (screen_info), screen_info->screen);
    DBG ("Using GLX version %d", version);
    if (version < 13)
    {
        g_warning ("GLX version %d is too old, GLX support disabled.", version);
        return FALSE;
    }

    if (!choose_glx_settings (screen_info))
    {
        g_warning ("Cannot find a matching GLX config, vsync disabled.");
        return FALSE;
    }

    screen_info->glx_context = glXCreateNewContext (myScreenGetXDisplay (screen_info),
                                                    screen_info->glx_fbconfig,
                                                    GLX_RGBA_TYPE,
                                                    0,
                                                    TRUE);
    if (!screen_info->glx_context)
    {
        g_warning ("Could not create GLX context.");
        return FALSE;
    }

    screen_info->glx_window = glXCreateWindow (myScreenGetXDisplay (screen_info),
                                               screen_info->glx_fbconfig,
                                               screen_info->output,
                                               NULL);
    if (!screen_info->glx_window)
    {
        g_warning ("Could not create GLX window.");
        free_glx_data (screen_info);

        return FALSE;
    }

    if (!glXMakeCurrent (myScreenGetXDisplay (screen_info),
                         screen_info->glx_window,
                         screen_info->glx_context))
    {
        g_warning ("Could not make GL context current.");
        free_glx_data (screen_info);

        return FALSE;
    }

    if (!check_glx_renderer (screen_info))
    {
        free_glx_data (screen_info);

        return FALSE;
    }

    if (!check_gl_extensions (screen_info))
    {
        g_warning ("Screen is missing required GL extension, GL support disabled.");
        free_glx_data (screen_info);

        return FALSE;
    }
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glDisable(GL_BLEND);

    glLoadIdentity();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Swap control methods available */
    screen_info->has_mesa_swap_control =
        epoxy_has_glx_extension (myScreenGetXDisplay (screen_info),
                                 screen_info->screen, "GLX_MESA_swap_control");
    screen_info->has_ext_swap_control =
        epoxy_has_glx_extension (myScreenGetXDisplay (screen_info),
                                 screen_info->screen, "GLX_EXT_swap_control");
    screen_info->has_ext_swap_control_tear =
        epoxy_has_glx_extension (myScreenGetXDisplay (screen_info),
                                 screen_info->screen, "GLX_EXT_swap_control_tear");
    /* Sync */
    screen_info->has_ext_arb_sync = epoxy_has_gl_extension ("GL_ARB_sync");

    check_gl_error();

    return TRUE;
}

static void
enable_glx_texture (ScreenInfo *screen_info)
{
    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    glBindTexture(screen_info->texture_type, screen_info->rootTexture);
    glEnable(screen_info->texture_type);
}

static void
disable_glx_texture (ScreenInfo *screen_info)
{
    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    glBindTexture(screen_info->texture_type, None);
    glDisable(screen_info->texture_type);
}

static void
fence_create (ScreenInfo *screen_info, gushort buffer)
{
#ifdef HAVE_XSYNC
    if (screen_info->fence[buffer] == None)
    {
        DBG ("Create fence for buffer %i", buffer);
        screen_info->fence[buffer] =
            XSyncCreateFence (myScreenGetXDisplay (screen_info),
                              screen_info->rootPixmap[buffer], FALSE);
    }
#endif /* HAVE_XSYNC */
}

static void
fence_reset (ScreenInfo *screen_info, gushort buffer)
{
#ifdef HAVE_XSYNC
    if (screen_info->fence[buffer] == None)
    {
        return;
    }
    DBG ("Reset fence for buffer %i", buffer);
    XSyncResetFence(myScreenGetXDisplay (screen_info),
                    screen_info->fence[buffer]);
#endif /* HAVE_XSYNC */
}

static gboolean
is_fence_triggered (ScreenInfo *screen_info, gushort buffer)
{
#ifdef HAVE_XSYNC
    Bool triggered = False;

    if (screen_info->fence[buffer] == None)
    {
        DBG ("No fence for buffer %i", buffer);
        return TRUE;
    }

    if (!XSyncQueryFence(myScreenGetXDisplay (screen_info),
                         screen_info->fence[buffer], &triggered))
    {
        DBG ("Cannot query fence for buffer %i", buffer);
        return TRUE;
    }

    DBG ("Fence for buffer %i is %striggered", buffer, triggered ? "" : "not ");
    return (gboolean) triggered;
#else
    return TRUE;
#endif /* HAVE_XSYNC */
}

static void
fence_sync (ScreenInfo *screen_info, gushort buffer)
{
#ifdef HAVE_XSYNC
#ifdef DEBUG
    gint64 t1, t2;
#endif /* DEBUG */
    if (is_fence_triggered (screen_info, buffer))
    {
        DBG ("Fence for buffer %i already triggered", buffer);
        return;
    }

    TRACE ("Awaiting fence for buffer %i", buffer);
#ifdef DEBUG
    t1 = g_get_monotonic_time ();
#endif /* DEBUG */
    XSyncTriggerFence(myScreenGetXDisplay (screen_info),
                      screen_info->fence[buffer]);
    XSyncAwaitFence(myScreenGetXDisplay (screen_info),
                    &screen_info->fence[buffer], 1);
#ifdef DEBUG
    t2 = g_get_monotonic_time ();
    DBG ("Fence sync time %luμs", t2 - t1);
#endif /* DEBUG */

    fence_reset (screen_info, buffer);
#else
    XSync (myScreenGetXDisplay (screen_info), FALSE);
    glXWaitX ();
#endif /* HAVE_XSYNC */
}

static void
fence_destroy (ScreenInfo *screen_info, gushort buffer)
{
#ifdef HAVE_XSYNC
    if (screen_info->fence[buffer])
    {
        DBG ("Destroying fence for buffer %i", buffer);
        XSyncDestroyFence (myScreenGetXDisplay (screen_info),
                           screen_info->fence[buffer]);
        screen_info->fence[buffer] = None;
    }
#endif /* HAVE_XSYNC */
}

static void
set_swap_interval (ScreenInfo *screen_info, gushort buffer)
{
#if defined (glXSwapIntervalEXT)
    if (screen_info->has_ext_swap_control)
    {
        if (screen_info->has_ext_swap_control_tear)
        {
            DBG ("Setting adaptive vsync using GLX_EXT_swap_control");
            glXSwapIntervalEXT (myScreenGetXDisplay (screen_info),
                                screen_info->glx_drawable[buffer], -1);
        }
        else
        {
            DBG ("Setting swap interval using GLX_EXT_swap_control");
            glXSwapIntervalEXT (myScreenGetXDisplay (screen_info),
                                screen_info->glx_drawable[buffer], 1);
        }
        return;
    }
#else
#warning glXSwapIntervalEXT() not supported by libepoxy, please update your version of libepoxy
#endif

#if defined (glXSwapIntervalMESA)
    if (screen_info->has_mesa_swap_control)
    {
        DBG ("Setting swap interval using GLX_MESA_swap_control");
        glXSwapIntervalMESA (1);
        return;
    }
#else
#warning glXSwapIntervalMESA() not supported by libepoxy, please update your version of libepoxy
#endif

    DBG ("No swap control available");
}

static void
create_glx_drawable (ScreenInfo *screen_info, gushort buffer)
{
    int pixmap_attribs[] = {
        GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
        GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
        None
    };

    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    pixmap_attribs[1] = screen_info->texture_target;
    pixmap_attribs[3] = screen_info->texture_format;

    screen_info->glx_drawable[buffer] =
        glXCreatePixmap (myScreenGetXDisplay (screen_info),
                         screen_info->glx_fbconfig,
                         screen_info->rootPixmap[buffer],
                         pixmap_attribs);
    check_gl_error();
    TRACE ("created GLX pixmap 0x%lx for buffer %i",
           screen_info->glx_drawable[buffer], buffer);
}

static void
bind_glx_texture (ScreenInfo *screen_info, gushort buffer)
{
    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    if (screen_info->rootTexture == None)
    {
        glGenTextures(1, &screen_info->rootTexture);
        TRACE ("generated texture 0x%x", screen_info->rootTexture);
    }
    if (screen_info->glx_drawable[buffer] == None)
    {
        create_glx_drawable (screen_info, buffer);
        set_swap_interval (screen_info, buffer);
    }
    TRACE ("(re)Binding GLX pixmap 0x%lx to texture 0x%x",
           screen_info->glx_drawable[buffer], screen_info->rootTexture);
    enable_glx_texture (screen_info);
    glXBindTexImageEXT (myScreenGetXDisplay (screen_info),
                        screen_info->glx_drawable[buffer], GLX_FRONT_EXT, NULL);

    check_gl_error();
}

static void
unbind_glx_texture (ScreenInfo *screen_info, gushort buffer)
{
    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    if (screen_info->glx_drawable[buffer])
    {
        TRACE ("unbinding GLX drawable 0x%lx", screen_info->glx_drawable[buffer]);
        glXReleaseTexImageEXT (myScreenGetXDisplay (screen_info),
                               screen_info->glx_drawable[buffer], GLX_FRONT_EXT);
    }
}

static void
destroy_glx_drawable (ScreenInfo *screen_info, gushort buffer)
{
    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    if (screen_info->glx_drawable[buffer])
    {
        unbind_glx_texture (screen_info, buffer);
        glXDestroyPixmap(myScreenGetXDisplay (screen_info),
                         screen_info->glx_drawable[buffer]);
        screen_info->glx_drawable[buffer] = None;
    }

    if (screen_info->rootTexture)
    {
        disable_glx_texture (screen_info);
        glDeleteTextures (1, &screen_info->rootTexture);
        screen_info->rootTexture = None;
    }
    check_gl_error();
}

static void
set_glx_scale (ScreenInfo *screen_info, gint width, gint height, double scale)
{
    if (screen_info->texture_type == GL_TEXTURE_RECTANGLE_ARB)
    {
        glScaled ((double) width * scale, (double) height * scale, 1.0);
    }
    else
    {
        glScaled(1.0 * scale, 1.0 * scale, 1.0);
    }
}

static void
redraw_glx_rects (ScreenInfo *screen_info, XRectangle *rects, int nrects)
{
    int i;

    glBegin(GL_QUADS);
    for (i = 0; i < nrects; i++)
    {
        double texture_x1 = (double) (rects[i].x) / screen_info->width;
        double texture_y1 = (double) (rects[i].y) / screen_info->height;
        double texture_x2 = (double) (rects[i].x + rects[i].width) / screen_info->width;
        double texture_y2 = (double) (rects[i].y + rects[i].height) / screen_info->height;
        double vertice_x1 = 2 * texture_x1 - 1.0;
        double vertice_y1 = -2 * texture_y1 + 1.0;
        double vertice_x2 = 2 * texture_x2 - 1.0;
        double vertice_y2 =  -2 * texture_y2 + 1.0;

        if (screen_info->texture_inverted)
        {
            texture_y1 = 1.0 - texture_y1;
            texture_y2 = texture_y2 - 1.0;
        }

        TRACE ("Rect#%i: Texture (%.2f,%.2f,%.2f,%.2f) to vertice (%.2f,%.2f,%.2f,%.2f)", i,
               texture_x1, texture_y1, texture_x2, texture_y2,
               vertice_x1, vertice_y1, vertice_x2, vertice_y2);

        glTexCoord2f (texture_x1, texture_y1);
        glVertex2f (vertice_x1, vertice_y1);
        glTexCoord2f (texture_x2, texture_y1);
        glVertex2f (vertice_x2, vertice_y1);
        glTexCoord2f (texture_x2, texture_y2);
        glVertex2f (vertice_x2, vertice_y2);
        glTexCoord2f (texture_x1, texture_y2);
        glVertex2f (vertice_x1, vertice_y2);
    }
    glEnd();
}

static void
redraw_glx_screen (ScreenInfo *screen_info)
{
    XRectangle root_rect = { 0, 0, screen_info->width, screen_info->height};

    redraw_glx_rects (screen_info, &root_rect, 1);
}

static void
redraw_glx_texture (ScreenInfo *screen_info, gushort buffer)
{
#ifdef DEBUG
    gint64 t1, t2;
#endif /* DEBUG */
    g_return_if_fail (screen_info != NULL);
    TRACE ("(re)Drawing GLX pixmap 0x%lx/texture 0x%x",
           screen_info->glx_drawable[buffer], screen_info->rootTexture);

    fence_sync (screen_info, buffer);
#ifdef DEBUG
    t1 = g_get_monotonic_time ();
#endif /* DEBUG */

    if (screen_info->has_ext_arb_sync)
    {
#if defined (glDeleteSync)
        glDeleteSync (screen_info->gl_sync);
#else
#warning glDeleteSync() not supported by libepoxy, please update your version of libepoxy
#endif
        screen_info->gl_sync = 0;
    }

    bind_glx_texture (screen_info, buffer);

    glDrawBuffer (GL_BACK);
    glViewport(0, 0, screen_info->width, screen_info->height);

    glMatrixMode(GL_TEXTURE);
    glPushMatrix();

    if (screen_info->zoomed)
    {
        /* Reuse the values from the XRender matrix */
        XFixed zf = screen_info->transform.matrix[0][0];
        XFixed xp = screen_info->transform.matrix[0][2];
        XFixed yp = screen_info->transform.matrix[1][2];

        double zoom = XFixedToDouble (zf);
        double x = XFixedToDouble (xp) / (screen_info->width * zoom);
        double y = XFixedToDouble (yp) / (screen_info->height * zoom);

        glTexParameteri(screen_info->texture_type,
                        GL_TEXTURE_MIN_FILTER,
                        screen_info->texture_filter);
        glTexParameteri(screen_info->texture_type,
                        GL_TEXTURE_MAG_FILTER,
                        screen_info->texture_filter);

        set_glx_scale (screen_info, screen_info->width, screen_info->height, zoom);
        glTranslated (x, y, 0.0);
    }
    else
    {
        set_glx_scale (screen_info, screen_info->width, screen_info->height, 1.0);
        glTranslated (0.0, 0.0, 0.0);
    }

    redraw_glx_screen (screen_info);

    glXSwapBuffers (myScreenGetXDisplay (screen_info),
                    screen_info->glx_window);

    glPopMatrix();

    unbind_glx_texture (screen_info, buffer);

    if (screen_info->has_ext_arb_sync)
    {
#if defined (glFenceSync)
        screen_info->gl_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
#else
#warning glFenceSync() not supported by libepoxy, please update your version of libepoxy
#endif
    }

#ifdef DEBUG
    t2 = g_get_monotonic_time ();
    DBG ("Swap buffer time %luμs", t2 - t1);
#endif /* DEBUG */

    check_gl_error();
}
#endif /* HAVE_EPOXY */

#ifdef HAVE_PRESENT_EXTENSION
static void
present_error (ScreenInfo *screen_info, int error_code)
{
    char buf[64];

    XGetErrorText (myScreenGetXDisplay (screen_info), error_code, buf, 63);
    g_warning ("Dismissing XPresent as unusable, error %i (%s)", error_code, buf);

    screen_info->present_pending = FALSE;
    screen_info->use_present = FALSE;
}

static void
present_flip (ScreenInfo *screen_info, XserverRegion region, gushort buffer)
{
    static guint32 present_serial;
    DisplayInfo *display_info;
    int result;

    g_return_if_fail (screen_info != NULL);
    g_return_if_fail (region != None);
    TRACE ("serial %d", present_serial);

    display_info = screen_info->display_info;
    myDisplayErrorTrapPush (display_info);
    XPresentPixmap (display_info->dpy, screen_info->output,
                    screen_info->rootPixmap[buffer],
                    present_serial++, None, region, 0, 0, None, None, None,
                    PresentOptionNone, 0, 1, 0, NULL, 0);
    result = myDisplayErrorTrapPop (display_info);

    if (result != 0)
    {
        present_error (screen_info, result);
        return;
    }

    screen_info->present_pending = TRUE;
    DBG ("present flip requested, present pending...");
}
#endif /* HAVE_PRESENT_EXTENSION */

static XserverRegion
win_extents (CWindow *cw)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;
    XRectangle r;

    g_return_val_if_fail (cw != NULL, None);
    TRACE ("window 0x%lx", cw->id);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;
    r.x = cw->attr.x;
    r.y = cw->attr.y;
    r.width = cw->attr.width + cw->attr.border_width * 2;
    r.height = cw->attr.height + cw->attr.border_width * 2;

    /*
       We apply a shadow to the window if:
       - It's a window with a frame and the user asked for shadows under regular
         windows,
       - it's an override redirect window that is not shaped, not an argb and
         the user asked for shadows on so called "popup" windows.
     */

    if ((screen_info->params->show_popup_shadow &&
              WIN_IS_OVERRIDE(cw) &&
              !(WIN_IS_ARGB(cw) || WIN_IS_SHAPED(cw)) &&
              !WIN_IS_FULLSCREEN(cw)) ||
          (screen_info->params->show_frame_shadow &&
              !WIN_IS_OVERRIDE(cw) &&
              !WIN_NO_SHADOW(cw) &&
              !WIN_IS_DOCK(cw) &&
              !WIN_IS_MAXIMIZED(cw) &&
              (WIN_HAS_FRAME(cw) || !(WIN_IS_ARGB(cw) || WIN_IS_SHAPED(cw)))) ||
          (screen_info->params->show_dock_shadow &&
              WIN_IS_DOCK(cw) &&
              !WIN_NO_SHADOW(cw) &&
              !WIN_IS_OVERRIDE(cw) &&
              (!WIN_IS_SHAPED(cw))))
    {
        XRectangle sr;

        TRACE ("window 0x%lx has extents", cw->id);
        cw->shadow_dx = SHADOW_OFFSET_X + screen_info->params->shadow_delta_x;
        cw->shadow_dy = SHADOW_OFFSET_Y + screen_info->params->shadow_delta_y;

        if (!(cw->shadow))
        {
            double shadow_opacity;
            shadow_opacity = (double) screen_info->params->frame_opacity
                           * (screen_info->params->shadow_opacity / 100.0)
                           * cw->opacity
                           / (NET_WM_OPAQUE * 100.0);

            cw->shadow = shadow_picture (screen_info, shadow_opacity,
                                         cw->attr.width + 2 * cw->attr.border_width,
                                         cw->attr.height + 2 * cw->attr.border_width,
                                         &cw->shadow_width, &cw->shadow_height);
        }

        sr.x = cw->attr.x + cw->shadow_dx;
        sr.y = cw->attr.y + cw->shadow_dy;
        sr.width = cw->shadow_width;
        sr.height = cw->shadow_height;

        if (sr.x < r.x)
        {
            r.width = (r.x + r.width) - sr.x;
            r.x = sr.x;
        }
        if (sr.y < r.y)
        {
            r.height = (r.y + r.height) - sr.y;
            r.y = sr.y;
        }
        if (sr.x + sr.width > r.x + r.width)
        {
            r.width = sr.x + sr.width - r.x;
        }
        if (sr.y + sr.height > r.y + r.height)
        {
            r.height = sr.y + sr.height - r.y;
        }
    }
    else if (cw->shadow)
    {
        XRenderFreePicture (display_info->dpy, cw->shadow);
        cw->shadow = None;
    }
    return XFixesCreateRegion (display_info->dpy, &r, 1);
}

static void
get_paint_bounds (CWindow *cw, gint *x, gint *y, guint *w, guint *h)
{
    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx", cw->id);

#if HAVE_NAME_WINDOW_PIXMAP
    *x = cw->attr.x;
    *y = cw->attr.y;
    *w = cw->attr.width + 2 * cw->attr.border_width;
    *h = cw->attr.height + 2 * cw->attr.border_width;
#else
    *x = cw->attr.x + cw->attr.border_width;
    *y = cw->attr.y + cw->attr.border_width;
    *w = cw->attr.width;
    *h = cw->attr.height;
#endif
}

static XRenderPictFormat *
get_window_format (CWindow *cw)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;
    XRenderPictFormat *format;

    g_return_val_if_fail (cw != NULL, NULL);
    TRACE ("window 0x%lx", cw->id);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;
    format = XRenderFindVisualFormat (display_info->dpy, cw->attr.visual);
    if (!format)
    {
        format = XRenderFindVisualFormat (display_info->dpy,
                                          DefaultVisual (display_info->dpy,
                                                         screen_info->screen));
    }

    return format;
}

static Picture
get_window_picture (CWindow *cw)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;
    XRenderPictureAttributes pa;
    XRenderPictFormat *format;
    Drawable draw;

    g_return_val_if_fail (cw != NULL, None);
    TRACE ("window 0x%lx", cw->id);

    draw = cw->id;
    screen_info = cw->screen_info;
    display_info = screen_info->display_info;

#if HAVE_NAME_WINDOW_PIXMAP
    myDisplayErrorTrapPush (display_info);
    if ((display_info->have_name_window_pixmap) && (cw->name_window_pixmap == None))
    {
        cw->name_window_pixmap = XCompositeNameWindowPixmap (display_info->dpy, cw->id);
    }
    if ((myDisplayErrorTrapPop (display_info) == Success) && (cw->name_window_pixmap != None))
    {
        draw = cw->name_window_pixmap;
    }
#endif
    format = get_window_format (cw);
    if (format)
    {
        Picture pict;

        myDisplayErrorTrapPush (display_info);
        pa.subwindow_mode = IncludeInferiors;
        pict = XRenderCreatePicture (display_info->dpy, draw, format, CPSubwindowMode, &pa);
        if (myDisplayErrorTrapPop (display_info) == Success)
        {
            return pict;
        }
    }

    return None;
}

static XserverRegion
get_screen_region (ScreenInfo *screen_info)
{
    DisplayInfo *display_info;
    XserverRegion region;
    XRectangle  r;

    display_info = screen_info->display_info;
    if (screen_info->width > 0 && screen_info->height > 0)
    {
        r.x = 0;
        r.y = 0;
        r.width = screen_info->width;
        r.height = screen_info->height;
        region = XFixesCreateRegion (display_info->dpy, &r, 1);
    }
    else
    {
        region = XFixesCreateRegion (display_info->dpy, NULL, 0);
    }

    return region;
}

static void
unredirect_win (CWindow *cw)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx", cw->id);

    if (WIN_IS_REDIRECTED(cw))
    {
        screen_info = cw->screen_info;
        display_info = screen_info->display_info;

        myDisplayErrorTrapPush (display_info);
        XCompositeUnredirectWindow (display_info->dpy, cw->id, CompositeRedirectManual);
        myDisplayErrorTrapPopIgnored (display_info);

        free_win_data (cw, FALSE);
        cw->redirected = FALSE;
        TRACE ("window 0x%lx unredirected, wins_unredirected is %i",
               cw->id, screen_info->wins_unredirected);
    }
}

static void
paint_root (ScreenInfo *screen_info, Picture paint_buffer)
{
    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    if (screen_info->rootTile == None)
    {
        screen_info->rootTile = root_tile (screen_info);
        g_return_if_fail (screen_info->rootTile != None);
    }

    XRenderComposite (myScreenGetXDisplay (screen_info),
                      PictOpSrc,
                      screen_info->rootTile,
                      None, paint_buffer,
                      0, 0, 0, 0, 0, 0,
                      screen_info->width,
                      screen_info->height);
}

static void
paint_cursor (ScreenInfo *screen_info, XserverRegion region, Picture paint_buffer)
{
    XFixesSetPictureClipRegion (myScreenGetXDisplay (screen_info),
                                paint_buffer, 0, 0, region);
    XRenderComposite (myScreenGetXDisplay (screen_info),
                      PictOpOver,
                      screen_info->cursorPicture,
                      None, paint_buffer,
                      0, 0, 0, 0,
                      screen_info->cursorLocation.x,
                      screen_info->cursorLocation.y,
                      screen_info->cursorLocation.width,
                      screen_info->cursorLocation.height);
}

static void
paint_win (CWindow *cw, XserverRegion region, Picture paint_buffer, gboolean solid_part)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    gboolean paint_solid;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx", cw->id);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;
    paint_solid = (solid_part && WIN_IS_OPAQUE(cw));

    if (WIN_HAS_FRAME(cw) && (screen_info->params->frame_opacity < 100))
    {
        int frame_x, frame_y, frame_width, frame_height;
        int frame_top, frame_bottom, frame_left, frame_right;
        frame_x = cw->attr.x;
        frame_y = cw->attr.y;
        frame_width = cw->attr.width;
        frame_height = cw->attr.height;
        frame_top = frameTop (cw->c);
        frame_bottom = frameBottom (cw->c);
        frame_left = frameLeft (cw->c);
        frame_right = frameRight (cw->c);
        if (!solid_part)
        {
            if (!cw->alphaBorderPict)
            {
                double frame_opacity;
                frame_opacity = (double) cw->opacity
                                         * screen_info->params->frame_opacity
                                         / (NET_WM_OPAQUE * 100.0);

                cw->alphaBorderPict = solid_picture (screen_info,
                                                     FALSE,
                                                     frame_opacity,
                                                     0.0, /* red   */
                                                     0.0, /* green */
                                                     0.0  /* blue  */);
            }

            /* Top Border (title bar) */
            XRenderComposite (display_info->dpy, PictOpOver, cw->picture, cw->alphaBorderPict,
                              paint_buffer,
                              0, 0,
                              0, 0,
                              frame_x, frame_y,
                              frame_width, frame_top);

            /* Bottom Border */
            XRenderComposite (display_info->dpy, PictOpOver, cw->picture, cw->alphaBorderPict,
                              paint_buffer,
                              0, frame_height - frame_bottom,
                              0, 0,
                              frame_x, frame_y + frame_height - frame_bottom,
                              frame_width, frame_bottom);
            /* Left Border */
            XRenderComposite (display_info->dpy, PictOpOver, cw->picture, cw->alphaBorderPict,
                              paint_buffer,
                              0, frame_top,
                              0, 0,
                              frame_x, frame_y + frame_top,
                              frame_left, frame_height - frame_top - frame_bottom);

            /* Right Border */
            XRenderComposite (display_info->dpy, PictOpOver, cw->picture, cw->alphaBorderPict,
                              paint_buffer,
                              frame_width - frame_right, frame_top,
                              0, 0,
                              frame_x + frame_width - frame_right,
                              frame_y + frame_top, frame_right,
                              frame_height - frame_top - frame_bottom);
        }
        /* Client Window */
        if (paint_solid)
        {
            XFixesSetPictureClipRegion (display_info->dpy, paint_buffer, 0, 0, region);
            XRenderComposite (display_info->dpy, PictOpSrc, cw->picture, None,
                              paint_buffer,
                              frame_left, frame_top,
                              0, 0,
                              frame_x + frame_left, frame_y + frame_top,
                              frame_width - frame_left - frame_right, frame_height - frame_top - frame_bottom);

            /* clientSize is set in paint_all() prior to calling paint_win() */
            XFixesSubtractRegion (display_info->dpy, region, region, cw->clientSize);
        }
        else if (!solid_part)
        {
            XRenderComposite (display_info->dpy, PictOpOver, cw->picture, cw->alphaPict,
                              paint_buffer,
                              frame_left, frame_top,
                              0, 0,
                              frame_x + frame_left, frame_y + frame_top,
                              frame_width - frame_left - frame_right, frame_height - frame_top - frame_bottom);
        }
    }
    else
    {
        gint x, y;
        guint w, h;

        get_paint_bounds (cw, &x, &y, &w, &h);
        if (paint_solid)
        {
            XFixesSetPictureClipRegion (display_info->dpy, paint_buffer, 0, 0, region);
            XRenderComposite (display_info->dpy, PictOpSrc,
                              cw->picture, None,
                              paint_buffer,
                              0, 0, 0, 0, x, y, w, h);
            XFixesSubtractRegion (display_info->dpy, region, region, cw->borderSize);
        }
        else if (!solid_part)
        {
            XRenderComposite (display_info->dpy, PictOpOver,
                              cw->picture, cw->alphaPict,
                              paint_buffer,
                              0, 0, 0, 0, x, y, w, h);
        }
    }
}

static void
clip_opaque_region (CWindow *cw, XserverRegion region)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    XserverRegion opaque_region;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx", cw->id);

    if (cw->opaque_region == None)
    {
        TRACE ("window 0x%lx has no opaque region", cw->id);
        return;
    }

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;

    opaque_region = XFixesCreateRegion (display_info->dpy, NULL, 0);
    XFixesCopyRegion (display_info->dpy, opaque_region, cw->opaque_region);
    translate_to_client_region (cw, opaque_region);
    /* cw->borderSize and cw->clientSize are already updated in paint_all() */
    if (cw->clientSize)
    {
        XFixesIntersectRegion (display_info->dpy, opaque_region, opaque_region, cw->clientSize);
    }
    XFixesIntersectRegion (display_info->dpy, opaque_region, opaque_region, cw->borderSize);
    XFixesSubtractRegion (display_info->dpy, region, region, opaque_region);
    XFixesDestroyRegion (display_info->dpy, opaque_region);
}

static int
get_region_bounds (Display *dpy, XserverRegion region, XRectangle *bounds)
{
    XRectangle *rects;
    int nrects;

    rects = XFixesFetchRegionAndBounds (dpy, region, &nrects, bounds);
    XFree (rects);

    return nrects;
}

static gboolean
is_region_empty (Display *dpy, XserverRegion region)
{
    XRectangle bounds;
    int nrects;

    nrects = get_region_bounds (dpy, region, &bounds);

    return (nrects == 0 || bounds.width == 0 || bounds.height == 0);
}

static void
paint_all (ScreenInfo *screen_info, XserverRegion region, gushort buffer)
{
    DisplayInfo *display_info;
    XserverRegion paint_region;
    XRectangle region_bounds;
    Picture paint_buffer;
    Display *dpy;
    GList *list;
    gint screen_width;
    gint screen_height;
    CWindow *cw;

    TRACE ("buffer %d", buffer);
    g_return_if_fail (screen_info);

    display_info = screen_info->display_info;
    dpy = display_info->dpy;
    screen_width = screen_info->width;
    screen_height = screen_info->height;

    myDisplayErrorTrapPush (display_info);

    /* Create root buffer if not done yet */
    if (screen_info->rootPixmap[buffer] == None)
    {
        screen_info->rootPixmap[buffer] = create_root_pixmap (screen_info);
#ifdef HAVE_EPOXY
        if (screen_info->use_glx)
        {
            fence_create (screen_info, buffer);
        }
#endif /* HAVE_EPOXY */
    }

    if (screen_info->rootBuffer[buffer] == None)
    {
        screen_info->rootBuffer[buffer] =
            create_root_buffer (screen_info, screen_info->rootPixmap[buffer]);
    }

    if (screen_info->zoomed && !screen_info->use_glx)
    {
        if (screen_info->zoomBuffer == None)
        {
            Pixmap pixmap;

            pixmap = create_root_pixmap (screen_info);
            screen_info->zoomBuffer = create_root_buffer (screen_info, pixmap);
            XFreePixmap (display_info->dpy, pixmap);
        }
        paint_buffer = screen_info->zoomBuffer;
    }
    else
    {
        paint_buffer = screen_info->rootBuffer[buffer];
    }
    /* Copy the original given region */
    paint_region = XFixesCreateRegion (dpy, NULL, 0);
    XFixesCopyRegion (dpy, paint_region, region);

    /*
     * Painting from top to bottom, reducing the clipping area at each iteration.
     * Only the opaque windows are painted 1st.
     */
    for (list = screen_info->cwindows; list; list = g_list_next (list))
    {
        cw = (CWindow *) list->data;
        TRACE ("painting forward 0x%lx", cw->id);
        if (!WIN_IS_VISIBLE(cw) || !WIN_IS_DAMAGED(cw))
        {
            TRACE ("skipped, not damaged or not viewable 0x%lx", cw->id);
            cw->skipped = TRUE;
            continue;
        }

        if (!WIN_IS_REDIRECTED(cw))
        {
            TRACE ("skipped, not redirected 0x%lx", cw->id);
            cw->skipped = TRUE;
            continue;
        }

        if ((cw->attr.x + cw->attr.width < 1) || (cw->attr.y + cw->attr.height < 1) ||
            (cw->attr.x >= screen_width) || (cw->attr.y >= screen_height))
        {
            TRACE ("skipped, off screen 0x%lx", cw->id);
            cw->skipped = TRUE;
            continue;
        }

        if (cw->extents == None)
        {
            cw->extents = win_extents (cw);
        }
        if (cw->picture == None)
        {
            cw->picture = get_window_picture (cw);
        }
        if (cw->borderSize == None)
        {
            cw->borderSize = border_size (cw);
        }
        if (cw->clientSize == None)
        {
            cw->clientSize = client_size (cw);
        }
        if (WIN_IS_OPAQUE(cw))
        {
            paint_win (cw, paint_region, paint_buffer, TRUE);
        }

        if (cw->borderClip == None)
        {
            cw->borderClip = XFixesCreateRegion (dpy, NULL, 0);
            XFixesCopyRegion (dpy, cw->borderClip, paint_region);
        }

        if ((cw->opacity == NET_WM_OPAQUE) && !WIN_IS_SHADED(cw))
        {
            clip_opaque_region (cw, paint_region);
        }

        cw->skipped = FALSE;
    }

    /*
     * region has changed because of the XFixesSubtractRegion (),
     * reapply clipping for the last iteration.
     */
    XFixesSetPictureClipRegion (dpy, paint_buffer, 0, 0, paint_region);
    if (!is_region_empty (dpy, paint_region))
    {
        paint_root (screen_info, paint_buffer);
    }

    /*
     * Painting from bottom to top, translucent windows and shadows are painted now...
     */
    for (list = g_list_last(screen_info->cwindows); list; list = g_list_previous (list))
    {
        XserverRegion shadowClip;

        cw = (CWindow *) list->data;
        shadowClip = None;
        TRACE ("painting backward 0x%lx", cw->id);

        if (cw->skipped)
        {
            TRACE ("skipped 0x%lx", cw->id);
            continue;
        }

        if (cw->shadow)
        {
            shadowClip = XFixesCreateRegion (dpy, NULL, 0);
            XFixesSubtractRegion (dpy, shadowClip, cw->borderClip, cw->borderSize);

            XFixesSetPictureClipRegion (dpy, paint_buffer, 0, 0, shadowClip);
            XRenderComposite (dpy, PictOpOver, screen_info->blackPicture, cw->shadow,
                              paint_buffer, 0, 0, 0, 0,
                              cw->attr.x + cw->shadow_dx,
                              cw->attr.y + cw->shadow_dy,
                              cw->shadow_width, cw->shadow_height);
        }

        if (cw->picture)
        {
            if ((cw->opacity != NET_WM_OPAQUE) && !(cw->alphaPict))
            {
                cw->alphaPict = solid_picture (screen_info, FALSE,
                                               (double) cw->opacity / NET_WM_OPAQUE,
                                               0.0, /* red   */
                                               0.0, /* green */
                                               0.0  /* blue  */);
            }
            XFixesIntersectRegion (dpy, cw->borderClip, cw->borderClip, cw->borderSize);
            XFixesSetPictureClipRegion (dpy, paint_buffer, 0, 0, cw->borderClip);
            paint_win (cw, paint_region, paint_buffer, FALSE);
        }

        if (shadowClip)
        {
            XFixesDestroyRegion (dpy, shadowClip);
        }

        if (cw->borderClip)
        {
            XFixesDestroyRegion (dpy, cw->borderClip);
            cw->borderClip = None;
        }
    }

    TRACE ("copying data back to screen");
#ifdef HAVE_EPOXY
    if (screen_info->use_glx)
    {
        if (screen_info->zoomed && screen_info->cursor_is_zoomed)
        {
            paint_cursor (screen_info, region, paint_buffer);
        }
        fence_reset (screen_info, buffer);
    }
    else
#endif /* HAVE_EPOXY */
    {
        if (screen_info->zoomed)
        {
            if (screen_info->cursor_is_zoomed)
            {
                paint_cursor (screen_info, region, paint_buffer);
            }
            /* Fixme: copy back whole screen if zoomed
               It would be better to scale the clipping region if possible. */
            XFixesSetPictureClipRegion (dpy, screen_info->rootBuffer[buffer], 0, 0, None);
            XFixesSetPictureClipRegion (dpy, paint_buffer, 0, 0, None);
        }
        else
        {
            /* Set clipping back to the given region */
            XFixesSetPictureClipRegion (dpy, screen_info->rootBuffer[buffer], 0, 0, region);
        }
    }

#ifdef HAVE_PRESENT_EXTENSION
    if (screen_info->use_present)
    {
        if (screen_info->zoomed)
        {
            XRenderComposite (dpy, PictOpSrc,
                              screen_info->zoomBuffer,
                              None, screen_info->rootBuffer[buffer],
                              0, 0, 0, 0, 0, 0, screen_width, screen_height);
        }
        present_flip (screen_info, region, buffer);
    }
    else
#endif /* HAVE_PRESENT_EXTENSION */
#ifdef HAVE_EPOXY
    if (screen_info->use_glx)
    {
        redraw_glx_texture (screen_info, buffer);
    }
    else
#endif /* HAVE_EPOXY */
    {
        if (screen_info->zoomed)
        {
            XRenderComposite (dpy, PictOpSrc,
                              screen_info->zoomBuffer,
                              None,  screen_info->rootPicture,
                              0, 0, 0, 0, 0, 0, screen_width, screen_height);
        }
        else
        {
            get_region_bounds (dpy, region, &region_bounds);
            XRenderComposite (dpy, PictOpSrc, paint_buffer,
                              None, screen_info->rootPicture,
                              region_bounds.x, region_bounds.y,
                              region_bounds.x, region_bounds.y,
                              region_bounds.x, region_bounds.y,
                              region_bounds.width, region_bounds.height);
        }
        XFlush (dpy);
    }

    XFixesDestroyRegion (dpy, paint_region);

    myDisplayErrorTrapPopIgnored (display_info);
}

static void
remove_timeouts (ScreenInfo *screen_info)
{
    if (screen_info->compositor_timeout_id != 0)
    {
        g_source_remove (screen_info->compositor_timeout_id);
        screen_info->compositor_timeout_id = 0;
    }
}

static gboolean
repair_screen (ScreenInfo *screen_info)
{
    DisplayInfo *display_info;
    XserverRegion damage;

    g_return_val_if_fail (screen_info, FALSE);
    TRACE ("entering");

    if (!screen_info->compositor_active)
    {
        return FALSE;
    }

    if (screen_info->width == 0 || screen_info->height == 0)
    {
        return FALSE;
    }

#ifdef HAVE_PRESENT_EXTENSION
    /*
     * We do not paint the screen because we are waiting for
     * a pending present notification, do not cancel the callback yet...
     */
     if (screen_info->use_present && screen_info->present_pending)
     {
         DBG ("Waiting for Present");
         return (screen_info->allDamage != None);
     }
#endif /* HAVE_PRESENT_EXTENSION */

#ifdef HAVE_EPOXY
    /*
     * We do not paint the screen because we are waiting for
     * the GL pipeline to complete, do not cancel the callback yet...
     */
     if (screen_info->use_glx && screen_info->gl_sync)
     {
#if defined (GL_SIGNALED)
         GLint status = GL_SIGNALED;
#if defined (glGetSynciv)
         glGetSynciv(screen_info->gl_sync, GL_SYNC_STATUS, sizeof(GLint), NULL, &status);
#else
#warning glGetSynciv() not supported by libepoxy, please update your version of libepoxy
#endif
         if (status != GL_SIGNALED)
         {
             DBG ("Waiting for GL pipeline");
             return (screen_info->allDamage != None);
         }
     }
#else
#warning GL_SIGNALED not supported by libepoxy, please update your version of libepoxy
#endif
#endif /* HAVE_EPOXY */
    display_info = screen_info->display_info;
    damage = screen_info->allDamage;
    if (damage)
    {
        if (screen_info->use_n_buffers > 1)
        {
            if (screen_info->prevDamage)
            {
                XFixesUnionRegion(display_info->dpy,
                                  screen_info->prevDamage,
                                  screen_info->prevDamage,
                                  damage);
                damage = screen_info->prevDamage;
            }
        }

        remove_timeouts (screen_info);
        paint_all (screen_info, damage, screen_info->current_buffer);

        if (screen_info->use_n_buffers > 1)
        {
            screen_info->current_buffer =
                (screen_info->current_buffer + 1) % screen_info->use_n_buffers;

            if (screen_info->prevDamage)
            {
                XFixesDestroyRegion (display_info->dpy, screen_info->prevDamage);
            }

            screen_info->prevDamage = screen_info->allDamage;
        }
        else
        {
            XFixesDestroyRegion (display_info->dpy, screen_info->allDamage);
        }
        screen_info->allDamage = None;
    }

    return FALSE;
}

static gboolean
compositor_timeout_cb (gpointer data)
{
    static guint number_of_retries = 0;
    ScreenInfo *screen_info;
    gboolean retry;

    screen_info = (ScreenInfo *) data;
    retry = repair_screen (screen_info);

    if (retry)
    {
        if (number_of_retries <= 100)
        {
            number_of_retries++;
        }
        if (number_of_retries == 100)
        {
            DBG ("Throttling repaint after 100 unsuccessful retries");
            /* Stop the current timeout repain */
            g_source_remove (screen_info->compositor_timeout_id);
            retry = FALSE;
            /* Recreate a throttled timeout instead */
            screen_info->compositor_timeout_id =
                g_timeout_add_full (TIMEOUT_THROTTLED_REPAINT_PRIORITY,
                                    TIMEOUT_THROTTLED_REPAINT_MS,
                                    compositor_timeout_cb, screen_info, NULL);
        }
    }
    else
    {
        screen_info->compositor_timeout_id = 0;
        number_of_retries = 0;
    }

    return retry;
}

static void
add_repair (ScreenInfo *screen_info)
{
    if (screen_info->compositor_timeout_id == 0)
    {
        screen_info->compositor_timeout_id =
            g_timeout_add_full (TIMEOUT_REPAINT_PRIORITY,
                                TIMEOUT_REPAINT_MS,
                                compositor_timeout_cb, screen_info, NULL);
    }
}

static void
add_damage (ScreenInfo *screen_info, XserverRegion damage)
{
    DisplayInfo *display_info;

    TRACE ("entering");

    if (damage == None)
    {
        return;
    }

    if (screen_info->screenRegion == None)
    {
        screen_info->screenRegion = get_screen_region (screen_info);
    }

    display_info = screen_info->display_info;
    XFixesIntersectRegion (display_info->dpy,
                           damage,
                           damage,
                           screen_info->screenRegion);
    if (screen_info->allDamage != None)
    {
        XFixesUnionRegion (display_info->dpy,
                           screen_info->allDamage,
                           screen_info->allDamage,
                           damage);
        XFixesDestroyRegion (display_info->dpy, damage);
    }
    else
    {
        screen_info->allDamage = damage;
    }

    /* The per-screen allDamage region is freed by repair_screen () */
    add_repair (screen_info);
}

static void
fix_region (CWindow *cw, XserverRegion region)
{
    GList *list;
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;

    /* Exclude opaque windows in front of the given area */
    for (list = screen_info->cwindows; list; list = g_list_next (list))
    {
        CWindow *cw2;

        cw2 = (CWindow *) list->data;
        if (cw2 == cw)
        {
            break;
        }
        else if (WIN_IS_OPAQUE(cw2) && WIN_IS_VISIBLE(cw2))
        {
            /* Make sure the window's areas are up-to-date... */
            if (cw2->picture == None)
            {
                cw2->picture = get_window_picture (cw2);
            }
            if (cw2->borderSize == None)
            {
                cw2->borderSize = border_size (cw2);
            }
            if (cw2->clientSize == None)
            {
                cw2->clientSize = client_size (cw2);
            }
            /* ...before subtracting them from the damaged zone. */
            if ((cw2->clientSize) && (screen_info->params->frame_opacity < 100))
            {
                XFixesSubtractRegion (display_info->dpy, region,
                                     region, cw2->clientSize);
            }
            else if (cw2->borderSize)
            {
                XFixesSubtractRegion (display_info->dpy, region,
                                     region, cw2->borderSize);
            }
        }
    }
}

static void
repair_win (CWindow *cw, XRectangle *r)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;
    XserverRegion parts;

    g_return_if_fail (cw != NULL);

    TRACE ("entering");
    screen_info = cw->screen_info;
    display_info = screen_info->display_info;

    if (!(cw->damage))
    {
        /* Input Only have no damage */
        return;
    }

    myDisplayErrorTrapPush (display_info);
    if (cw->damaged)
    {
        parts = XFixesCreateRegion (display_info->dpy, NULL, 0);
        /* Copy the damage region to parts, subtracting it from the window's damage */
        XDamageSubtract (display_info->dpy, cw->damage, None, parts);
        XFixesTranslateRegion (display_info->dpy, parts,
                               cw->attr.x + cw->attr.border_width,
                               cw->attr.y + cw->attr.border_width);
    }
    else
    {
        parts = win_extents (cw);
        /* Subtract all damage from the window's damage */
        XDamageSubtract (display_info->dpy, cw->damage, None, None);
    }
    myDisplayErrorTrapPopIgnored (display_info);

    if (parts)
    {
        fix_region (cw, parts);
        /* parts region will be destroyed by add_damage () */
        add_damage (cw->screen_info, parts);
        cw->damaged = TRUE;
    }
}

static void
damage_screen (ScreenInfo *screen_info)
{
    XserverRegion region;

    region = get_screen_region (screen_info);
    /* region will be freed by add_damage () */
    add_damage (screen_info, region);
}

static void
damage_win (CWindow *cw)
{
    XserverRegion extents;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx", cw->id);

    extents = win_extents (cw);
    fix_region (cw, extents);
    /* extents region will be freed by add_damage () */
    add_damage (cw->screen_info, extents);
}

static void
update_extents (CWindow *cw)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx", cw->id);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;

    if (WIN_IS_VISIBLE(cw))
    {
        damage_win (cw);
    }

    if (cw->extents)
    {
        XFixesDestroyRegion (display_info->dpy, cw->extents);
        cw->extents = None;
    }
}

static void
determine_mode (CWindow *cw)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;
    XRenderPictFormat *format;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx", cw->id);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;
    format = NULL;

    if (cw->alphaPict)
    {
        XRenderFreePicture (display_info->dpy, cw->alphaPict);
        cw->alphaPict = None;
    }
    if (cw->shadowPict)
    {
        XRenderFreePicture (display_info->dpy, cw->shadowPict);
        cw->shadowPict = None;
    }

    if (cw->alphaBorderPict)
    {
        XRenderFreePicture (display_info->dpy, cw->alphaBorderPict);
        cw->alphaBorderPict = None;
    }

    format = XRenderFindVisualFormat (display_info->dpy, cw->attr.visual);
    cw->argb = ((format) && (format->type == PictTypeDirect) && (format->direct.alphaMask));

    if (cw->extents)
    {
        XserverRegion damage;

        damage = XFixesCreateRegion (display_info->dpy, NULL, 0);
        XFixesCopyRegion (display_info->dpy, damage, cw->extents);
        fix_region (cw, damage);
        /* damage region will be destroyed by add_damage () */
        add_damage (screen_info, damage);
    }
}

static void
expose_area (ScreenInfo *screen_info, XRectangle *rects, gint nrects)
{
    DisplayInfo *display_info;
    XserverRegion region;

    g_return_if_fail (rects != NULL);
    g_return_if_fail (nrects > 0);
    TRACE ("entering");

    display_info = screen_info->display_info;
    region = XFixesCreateRegion (display_info->dpy, rects, nrects);
    /* region will be destroyed by add_damage () */
    add_damage (screen_info, region);
}

static void
set_win_opacity (CWindow *cw, guint32 opacity)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx", cw->id);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;

    cw->opacity = opacity;
    determine_mode(cw);
    if (cw->shadow)
    {
        XRenderFreePicture (display_info->dpy, cw->shadow);
        cw->shadow = None;
        if (cw->extents)
        {
            XFixesDestroyRegion (display_info->dpy, cw->extents);
        }
        cw->extents = win_extents (cw);
        add_repair (screen_info);
    }
}

static void
map_win (CWindow *cw)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx", cw->id);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;

    if (!WIN_IS_REDIRECTED(cw))
    {
        cw->fulloverlay = is_fullscreen (cw);
        if (cw->fulloverlay)
        {
            /*
             * To be safe, we only count the fullscreen un-redirected windows.
             * We do not want a smaller override redirect such as a tooltip
             * for example to prevent the overlay to be remapped and leave
             * a black screen until the tooltip is unmapped...
             */
            screen_info->wins_unredirected++;
            TRACE ("mapping fullscreen window 0x%lx, wins_unredirected increased to %i", cw->id, screen_info->wins_unredirected);
        }
        TRACE ("mapping unredirected window 0x%lx, wins_unredirected is now %i", cw->id, screen_info->wins_unredirected);
#if HAVE_OVERLAYS
        if ((screen_info->wins_unredirected == 1) && (display_info->have_overlays))
        {
            TRACE ("unmapping overlay window");
            XUnmapWindow (myScreenGetXDisplay (screen_info), screen_info->overlay);
        }
#endif /* HAVE_OVERLAYS */
        return;
    }

    cw->viewable = TRUE;
    cw->damaged = FALSE;

    /* Check for new windows to un-redirect. */
    if (WIN_HAS_DAMAGE(cw) && WIN_IS_NATIVE_OPAQUE(cw) &&
        WIN_IS_REDIRECTED(cw) && !WIN_IS_SHAPED(cw) &&
        ((screen_info->wins_unredirected > 0) || is_fullscreen (cw)))
    {
        /* Make those opaque, we don't want them to be transparent */
        cw->opacity = NET_WM_OPAQUE;

        /* For NET_WM_BYPASS_COMPOSITOR, 0 indicates no preference, 1 hints
         * the compositor to disabling compositing.
         */
        if ((cw->bypass_compositor == 1) ||
            (screen_info->params->unredirect_overlays &&
             WIN_IS_OVERRIDE(cw) && cw->bypass_compositor == 0))
        {
            TRACE ("unredirecting toplevel window 0x%lx", cw->id);
            unredirect_win (cw);
        }
    }
}

static void
unmap_win (CWindow *cw)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx", cw->id);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;

    if (!WIN_IS_REDIRECTED(cw) && (screen_info->wins_unredirected > 0))
    {
        if (cw->fulloverlay)
        {
            screen_info->wins_unredirected--;
            TRACE ("unmapping fullscreen window 0x%lx, wins_unredirected decreased to %i",
                   cw->id, screen_info->wins_unredirected);
        }
        TRACE ("unmapped window 0x%lx, wins_unredirected is now %i", cw->id, screen_info->wins_unredirected);
        if (!screen_info->wins_unredirected)
        {
            /* Restore the overlay if that was the last unredirected window */
#if HAVE_OVERLAYS
            if (display_info->have_overlays)
            {
                TRACE ("remapping overlay window");
                XMapWindow (myScreenGetXDisplay (screen_info), screen_info->overlay);
            }
#endif /* HAVE_OVERLAYS */
            damage_screen (screen_info);
       }
    }
    else if (WIN_IS_VISIBLE(cw))
    {
        damage_win (cw);
    }

    cw->viewable = FALSE;
    cw->damaged = FALSE;
    cw->redirected = TRUE;
    cw->fulloverlay = FALSE;

    free_win_data (cw, FALSE);
}

static void
init_opacity (CWindow *cw)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Client *c;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx", cw->id);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;
    c = cw->c;

    cw->native_opacity = FALSE;
    if (c)
    {
        cw->opacity_locked = FLAG_TEST (c->xfwm_flags, XFWM_FLAG_OPACITY_LOCKED);
        cw->opacity = c->opacity_applied;
        cw->native_opacity = WIN_IS_OPAQUE(cw);
    }
    else if (getOpacity (display_info, cw->id, &cw->opacity))
    {
        cw->native_opacity = WIN_IS_OPAQUE(cw);
        cw->opacity_locked = getOpacityLock (display_info, cw->id);
    }
    else
    {
        cw->opacity = (double) (screen_info->params->popup_opacity / 100.0) * NET_WM_OPAQUE;
        cw->native_opacity = TRUE;
        cw->opacity_locked = getOpacityLock (display_info, cw->id);
    }
}

static void
update_opaque_region (CWindow *cw, Window id)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;
    XRectangle *rects = NULL;
    unsigned int nrects;
    XserverRegion old_opaque_region;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx", cw->id);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;

    old_opaque_region = cw->opaque_region;

    nrects = getOpaqueRegionRects (display_info, id, &rects);
    if (nrects)
    {
        cw->opaque_region = XFixesCreateRegion (display_info->dpy, rects, nrects);
        g_free (rects);
    }
    else
    {
        cw->opaque_region = None;
    }

    if (old_opaque_region != None)
    {
        if (!WIN_IS_VISIBLE(cw) || !WIN_IS_REDIRECTED(cw))
        {
            XFixesDestroyRegion (display_info->dpy, old_opaque_region);
        }
        else
        {
            if (cw->opaque_region)
            {
                XFixesSubtractRegion (display_info->dpy, old_opaque_region,
                                      old_opaque_region, cw->opaque_region);
            }
            translate_to_client_region (cw, old_opaque_region);
            /* old_opaque_region region will be destroyed by add_damage () */
            add_damage (screen_info, old_opaque_region);
        }
    }
    else
    {
        damage_win (cw);
    }
}

static void
add_win (DisplayInfo *display_info, Window id, Client *c)
{
    ScreenInfo *screen_info;
    CWindow *new;
    int result, status;

    TRACE ("window 0x%lx", id);

    if (is_output (display_info, id))
    {
        TRACE ("not adding output window 0x%lx", id);
        return;
    }

    new = find_cwindow_in_display (display_info, id);
    if (new)
    {
        TRACE ("window 0x%lx already added", id);
        return;
    }

    new = g_slice_alloc0 (sizeof(CWindow));
    myDisplayGrabServer (display_info);
    myDisplayErrorTrapPush (display_info);
    status = XGetWindowAttributes (display_info->dpy, id, &new->attr);
    result = myDisplayErrorTrapPop (display_info);

    if ((result != Success) || !status)
    {
        g_slice_free (CWindow, new);
        myDisplayUngrabServer (display_info);
        TRACE ("an error occurred getting window attributes, 0x%lx not added", id);
        return;
    }

    if (c)
    {
        screen_info = c->screen_info;
    }
    else
    {
        screen_info = myDisplayGetScreenFromRoot (display_info, new->attr.root);
    }

    if (!screen_info)
    {
        g_slice_free (CWindow, new);
        myDisplayUngrabServer (display_info);
        TRACE ("couldn't get screen from window, 0x%lx not added", id);
        return;
    }

    if (!screen_info->compositor_active)
    {
        g_slice_free (CWindow, new);
        myDisplayUngrabServer (display_info);
        TRACE ("compositor not active on screen %i, 0x%lx not added", screen_info->screen, id);
        return;
    }

    myDisplayErrorTrapPush (display_info);
    if (c == NULL)
    {
        /* We must be notified of property changes for transparency, even if the win is not managed */
        XSelectInput (display_info->dpy, id, new->attr.your_event_mask | PropertyChangeMask | StructureNotifyMask);
    }

    /* Listen for XShape events if applicable */
    if (display_info->have_shape)
    {
        XShapeSelectInput (display_info->dpy, id, ShapeNotifyMask);
    }
    myDisplayErrorTrapPopIgnored (display_info);

    new->c = c;
    new->screen_info = screen_info;
    new->id = id;
    new->damaged = FALSE;
    new->redirected = TRUE;
    new->fulloverlay = FALSE;
    new->shaped = is_shaped (display_info, id);
    new->viewable = (new->attr.map_state == IsViewable);

    if (new->attr.class != InputOnly)
    {
        myDisplayErrorTrapPush (display_info);
        new->damage = XDamageCreate (display_info->dpy, id, XDamageReportNonEmpty);
        if (myDisplayErrorTrapPop (display_info) != Success)
        {
            new->damage = None;
        }
    }
    else
    {
        new->damage = None;
    }
#if HAVE_NAME_WINDOW_PIXMAP
    new->name_window_pixmap = None;
#endif
    new->picture = None;
    new->saved_picture = None;
    new->alphaPict = None;
    new->alphaBorderPict = None;
    new->shadowPict = None;
    new->borderSize = None;
    new->clientSize = None;
    new->extents = None;
    new->shadow = None;
    new->shadow_dx = 0;
    new->shadow_dy = 0;
    new->shadow_width = 0;
    new->shadow_height = 0;
    new->borderClip = None;

    if (c)
    {
        update_opaque_region (new, c->window);
    }
    else
    {
        update_opaque_region (new, id);
    }
    getBypassCompositor (display_info, id, &new->bypass_compositor);
    init_opacity (new);
    determine_mode (new);

    /* Insert window at top of stack */
    screen_info->cwindows = g_list_prepend (screen_info->cwindows, new);
    g_hash_table_insert(screen_info->cwindow_hash, (gpointer) new->id, new);

    if (WIN_IS_VISIBLE(new))
    {
        map_win (new);
    }

    TRACE ("window 0x%lx added", id);

    myDisplayUngrabServer (display_info);
}

static void
restack_win (CWindow *cw, Window above)
{
    ScreenInfo *screen_info;
    Window previous_above;
    GList *sibling;
    GList *next;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx above 0x%lx", cw->id, above);

    screen_info = cw->screen_info;
    sibling = g_list_find (screen_info->cwindows, (gconstpointer) cw);
    next = g_list_next (sibling);
    previous_above = None;

    if (next)
    {
        CWindow *ncw = (CWindow *) next;
        previous_above = ncw->id;
    }

    /* If above is set to None, the window whose state was changed is on
     * the bottom of the stack with respect to sibling.
     */
    if (above == None)
    {
        /* Insert at bottom of window stack */
        screen_info->cwindows = g_list_delete_link (screen_info->cwindows, sibling);
        screen_info->cwindows = g_list_append (screen_info->cwindows, cw);
    }
    else if (previous_above != above)
    {
        GList *list;

        for (list = screen_info->cwindows; list; list = g_list_next (list))
        {
            CWindow *cw2 = (CWindow *) list->data;
            if (cw2->id == above)
            {
                break;
            }
        }

        if (list != NULL)
        {
            screen_info->cwindows = g_list_delete_link (screen_info->cwindows, sibling);
            screen_info->cwindows = g_list_insert_before (screen_info->cwindows, list, cw);
        }
    }
}

static void
resize_win (CWindow *cw, gint x, gint y, gint width, gint height, gint bw)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;
    XserverRegion damage;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx, (%i,%i) %ix%i", cw->id, x, y, width, height);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;
    damage = None;

    myDisplayErrorTrapPush (display_info);

    if (WIN_IS_VISIBLE(cw))
    {
        damage = XFixesCreateRegion (display_info->dpy, NULL, 0);
        if (cw->extents)
        {
            XFixesCopyRegion (display_info->dpy, damage, cw->extents);
        }
    }

    if (cw->extents)
    {
        XFixesDestroyRegion (display_info->dpy, cw->extents);
        cw->extents = None;
    }

    if ((cw->attr.width != width) || (cw->attr.height != height))
    {
#if HAVE_NAME_WINDOW_PIXMAP
        if (cw->name_window_pixmap)
        {
            XFreePixmap (display_info->dpy, cw->name_window_pixmap);
            cw->name_window_pixmap = None;
        }
#endif
        if (cw->picture)
        {
            XRenderFreePicture (display_info->dpy, cw->picture);
            cw->picture = None;
        }

        if (cw->saved_picture)
        {
            XRenderFreePicture (display_info->dpy, cw->saved_picture);
            cw->saved_picture = None;
        }

        if (cw->shadow)
        {
            XRenderFreePicture (display_info->dpy, cw->shadow);
            cw->shadow = None;
        }
    }

    if ((cw->attr.width != width) || (cw->attr.height != height) ||
        (cw->attr.x != x) || (cw->attr.y != y))
    {
        if (cw->borderSize)
        {
            XFixesDestroyRegion (display_info->dpy, cw->borderSize);
            cw->borderSize = None;
        }

        if (cw->clientSize)
        {
            XFixesDestroyRegion (display_info->dpy, cw->clientSize);
            cw->clientSize = None;
        }
    }

    cw->attr.x = x;
    cw->attr.y = y;
    cw->attr.width = width;
    cw->attr.height = height;
    cw->attr.border_width = bw;

    if (damage)
    {
        cw->extents = win_extents (cw);
        XFixesUnionRegion (display_info->dpy, damage, damage, cw->extents);

        fix_region (cw, damage);
        /* damage region will be destroyed by add_damage () */
        add_damage (screen_info, damage);
    }

    myDisplayErrorTrapPopIgnored (display_info);
}

static void
reshape_win (CWindow *cw)
{
    DisplayInfo *display_info;
    ScreenInfo *screen_info;
    XserverRegion damage;

    g_return_if_fail (cw != NULL);
    TRACE ("window 0x%lx", cw->id);

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;

    damage = None;

    myDisplayErrorTrapPush (display_info);

    if (WIN_IS_VISIBLE(cw))
    {
        damage = XFixesCreateRegion (display_info->dpy, NULL, 0);
        if (cw->extents)
        {
            XFixesCopyRegion (display_info->dpy, damage, cw->extents);
        }
    }

    if (cw->extents)
    {
        XFixesDestroyRegion (display_info->dpy, cw->extents);
        cw->extents = None;
    }

    if (cw->shadow)
    {
        XRenderFreePicture (display_info->dpy, cw->shadow);
        cw->shadow = None;
    }

    if (cw->borderSize)
    {
        XFixesDestroyRegion (display_info->dpy, cw->borderSize);
        cw->borderSize = None;
    }

    if (cw->clientSize)
    {
        XFixesDestroyRegion (display_info->dpy, cw->clientSize);
        cw->clientSize = None;
    }

    if (damage)
    {
        cw->extents = win_extents (cw);
        XFixesUnionRegion (display_info->dpy, damage, damage, cw->extents);

        /* A shape notify will likely change the shadows too, so clear the extents */
        XFixesDestroyRegion (display_info->dpy, cw->extents);
        cw->extents = None;

        fix_region (cw, damage);
        /* damage region will be destroyed by add_damage () */
        add_damage (screen_info, damage);
    }

    myDisplayErrorTrapPopIgnored (display_info);
}

static void
destroy_win (DisplayInfo *display_info, Window id)
{
    CWindow *cw;

    g_return_if_fail (display_info != NULL);
    g_return_if_fail (id != None);
    TRACE ("window 0x%lx", id);

    cw = find_cwindow_in_display (display_info, id);
    if (is_on_compositor (cw))
    {
        ScreenInfo *screen_info;

        if (WIN_IS_VIEWABLE (cw))
        {
            unmap_win (cw);
        }
        screen_info = cw->screen_info;
        g_hash_table_remove(screen_info->cwindow_hash, (gpointer) cw->id);
        screen_info->cwindows = g_list_remove (screen_info->cwindows, (gconstpointer) cw);

        free_win_data (cw, TRUE);
    }
}

static void
update_cursor (ScreenInfo *screen_info)
{
    XFixesCursorImage *cursor;

    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    cursor = XFixesGetCursorImage (screen_info->display_info->dpy);
    if (cursor == NULL)
    {
        g_warning ("Failed to retrieve cursor image!");
        return;
    }

    if (screen_info->cursorSerial != cursor->cursor_serial)
    {
        if (screen_info->zoomed)
        {
            expose_area (screen_info, &screen_info->cursorLocation, 1);
        }

        if (screen_info->cursorPicture)
        {
            XRenderFreePicture (screen_info->display_info->dpy, screen_info->cursorPicture);
        }
        screen_info->cursorPicture = cursor_to_picture (screen_info, cursor);

        screen_info->cursorSerial = cursor->cursor_serial;
        screen_info->cursorOffsetX = cursor->xhot;
        screen_info->cursorOffsetY = cursor->yhot;
        screen_info->cursorLocation.x = cursor->x - cursor->xhot;
        screen_info->cursorLocation.y = cursor->y - cursor->yhot;
        screen_info->cursorLocation.width = cursor->width;
        screen_info->cursorLocation.height = cursor->height;

        if (screen_info->zoomed)
        {
            expose_area (screen_info, &screen_info->cursorLocation, 1);
        }
    }

    XFree (cursor);
}

static void
recenter_zoomed_area (ScreenInfo *screen_info, int x_root, int y_root)
{
    int zf = screen_info->transform.matrix[0][0];
    double zoom = XFixedToDouble (zf);

    if (screen_info->zoomed)
    {
        int xp = x_root * (1 - zoom);
        int yp = y_root * (1 - zoom);
        screen_info->transform.matrix[0][2] = (xp << 16);
        screen_info->transform.matrix[1][2] = (yp << 16);
    }

    if (zf > (1 << 14) && zf < (1 << 16))
    {
#ifdef HAVE_EPOXY
        if (screen_info->use_glx)
        {
            screen_info->texture_filter = GL_LINEAR;
        }
#endif /* HAVE_EPOXY */
        if (screen_info->zoomBuffer)
        {
            XRenderSetPictureFilter (myScreenGetXDisplay (screen_info),
                                     screen_info->zoomBuffer,
                                     FilterBilinear, NULL, 0);
        }
    }
    else
    {
#ifdef HAVE_EPOXY
        if (screen_info->use_glx)
        {
            screen_info->texture_filter = GL_NEAREST;
        }
#endif /* HAVE_EPOXY */
        if (screen_info->zoomBuffer)
        {
            XRenderSetPictureFilter (myScreenGetXDisplay (screen_info),
                                     screen_info->zoomBuffer,
                                     FilterNearest, NULL, 0);
        }
    }
    /* zoomBuffer might be None if we're using GLX */
    if (screen_info->zoomBuffer)
    {
        XRenderSetPictureTransform (myScreenGetXDisplay (screen_info),
                                    screen_info->zoomBuffer,
                                    &screen_info->transform);
    }

    damage_screen (screen_info);
}

static gboolean
zoom_timeout_cb (gpointer data)
{
    ScreenInfo   *screen_info;
    int          x_root, y_root;

    screen_info = (ScreenInfo *) data;

    if (!screen_info->zoomed)
    {
        screen_info->zoom_timeout_id = 0;

        if (screen_info->zoomBuffer)
        {
            XRenderFreePicture (myScreenGetXDisplay (screen_info),
                                screen_info->zoomBuffer);
            screen_info->zoomBuffer = None;
        }

        return FALSE; /* stop calling this callback */
    }

    getMouseXY (screen_info, &x_root, &y_root);

    if (screen_info->cursorLocation.x + screen_info->cursorOffsetX != x_root ||
        screen_info->cursorLocation.y + screen_info->cursorOffsetY != y_root)
    {
        screen_info->cursorLocation.x = x_root - screen_info->cursorOffsetX;
        screen_info->cursorLocation.y = y_root - screen_info->cursorOffsetY;
        recenter_zoomed_area (screen_info, x_root, y_root);
    }

    return TRUE;
}

static void
compositorHandleDamage (DisplayInfo *display_info, XDamageNotifyEvent *ev)
{
    ScreenInfo *screen_info;
    CWindow *cw;

    g_return_if_fail (display_info != NULL);
    g_return_if_fail (ev != NULL);
    TRACE ("drawable 0x%lx", ev->drawable);

    /*
      ev->drawable is the window ID of the damaged window
      ev->geometry is the geometry of the damaged window
      ev->area     is the bounding rect for the damaged area
      ev->damage   is the damage handle returned by XDamageCreate()
     */

    cw = find_cwindow_in_display (display_info, ev->drawable);
    if (is_on_compositor (cw) && WIN_IS_REDIRECTED(cw))
    {
        screen_info = cw->screen_info;
        repair_win (cw, &ev->area);
        screen_info->damages_pending = ev->more;
    }
}

static void
compositorHandlePropertyNotify (DisplayInfo *display_info, XPropertyEvent *ev)
{
#if MONITOR_ROOT_PIXMAP
    gint p;
    Atom backgroundProps[2];
#endif

    g_return_if_fail (display_info != NULL);
    g_return_if_fail (ev != NULL);
    TRACE ("window 0x%lx", ev->window);

#if MONITOR_ROOT_PIXMAP
    backgroundProps[0] = display_info->atoms[XROOTPMAP];
    backgroundProps[1] = display_info->atoms[XSETROOT];

    for (p = 0; p < 2; p++)
    {
        if (ev->atom == backgroundProps[p] && ev->state == PropertyNewValue)
        {
            ScreenInfo *screen_info = myDisplayGetScreenFromRoot (display_info, ev->window);
            if ((screen_info) && (screen_info->compositor_active) && (screen_info->rootTile))
            {
                myDisplayErrorTrapPush (display_info);
                XClearArea (display_info->dpy, screen_info->output, 0, 0, 0, 0, TRUE);
                XRenderFreePicture (display_info->dpy, screen_info->rootTile);
                myDisplayErrorTrapPopIgnored (display_info);

                screen_info->rootTile = None;
                damage_screen (screen_info);

                return;
            }
        }
    }
#endif

    /* check if Trans property was changed */
    if (ev->atom == display_info->atoms[NET_WM_WINDOW_OPACITY])
    {
        CWindow *cw = find_cwindow_in_display (display_info, ev->window);
        TRACE ("window 0x%lx", ev->window);

        if (is_on_compositor (cw))
        {
            Client *c = cw->c;

            TRACE ("NET_WM_WINDOW_OPACITY changed for 0x%lx", cw->id);
            if (!getOpacity (display_info, cw->id, &cw->opacity))
            {
                /* The property was removed */
                cw->opacity = NET_WM_OPAQUE;
            }
            set_win_opacity (cw, cw->opacity);
            cw->native_opacity = WIN_IS_OPAQUE(cw);

            /* Transset changes the property on the frame, not the client
               window. We need to check and update the client "opacity"
               value accordingly.
              */
            if (c)
            {
                if (c->opacity != cw->opacity)
                {
                    clientSetOpacity (c, cw->opacity, 0, 0);
                }
            }
        }
    }
    else if (ev->atom == display_info->atoms[NET_WM_WINDOW_OPACITY_LOCKED])
    {
        CWindow *cw = find_cwindow_in_display (display_info, ev->window);
        TRACE ("NET_WM_WINDOW_OPACITY_LOCKED changed for id 0x%lx", ev->window);

        if (is_on_compositor (cw))
        {
            cw->opacity_locked = getOpacityLock (display_info, cw->id);
            if (cw->c)
            {
                if (cw->opacity_locked)
                {
                    FLAG_SET (cw->c->xfwm_flags, XFWM_FLAG_OPACITY_LOCKED);
                }
                else
                {
                    FLAG_UNSET (cw->c->xfwm_flags, XFWM_FLAG_OPACITY_LOCKED);
                }
            }
        }
    }
    else if (ev->atom == display_info->atoms[NET_WM_BYPASS_COMPOSITOR])
    {
        CWindow *cw = find_cwindow_in_display (display_info, ev->window);
        TRACE ("NET_WM_BYPASS_COMPOSITOR changed for id 0x%lx", ev->window);

        if (is_on_compositor (cw))
        {
            getBypassCompositor (display_info, cw->id, &cw->bypass_compositor);
        }
    }
    else if (ev->atom == display_info->atoms[NET_WM_OPAQUE_REGION])
    {
        Client* c;
        CWindow *cw;

        c = myDisplayGetClientFromWindow (display_info, ev->window, SEARCH_WINDOW);
        if (c)
        {
            cw = find_cwindow_in_display (display_info, c->frame);
        }
        else
        {
            cw = find_cwindow_in_display (display_info, ev->window);
        }

        if (is_on_compositor (cw))
        {
            update_opaque_region (cw, ev->window);
        }
    }
    else
    {
        TRACE ("no compositor property changed for id 0x%lx", ev->window);
    }
}

static void
compositorHandleExpose (DisplayInfo *display_info, XExposeEvent *ev)
{
    ScreenInfo *screen_info;
    XRectangle rect[1];
    CWindow *cw;

    g_return_if_fail (display_info);
    TRACE ("window 0x%lx", ev->window);

    cw = find_cwindow_in_display (display_info, ev->window);
    if (cw != NULL)
    {
        screen_info = cw->screen_info;
    }
    else
    {
        /* Get the screen structure from the root of the event */
        screen_info = myDisplayGetScreenFromRoot (display_info, ev->window);
    }

    if (!screen_info || !compositorIsActive (screen_info))
    {
        return;
    }

    rect[0].x = ev->x;
    rect[0].y = ev->y;
    rect[0].width = ev->width;
    rect[0].height = ev->height;

    expose_area (screen_info, rect, 1);
}

static void
compositorHandleConfigureNotify (DisplayInfo *display_info, XConfigureEvent *ev)
{
    CWindow *cw;

    g_return_if_fail (display_info != NULL);
    g_return_if_fail (ev != NULL);
    TRACE ("window 0x%lx", ev->window);

    cw = find_cwindow_in_display (display_info, ev->window);
    if (is_on_compositor (cw))
    {
        restack_win (cw, ev->above);
        resize_win (cw, ev->x, ev->y, ev->width, ev->height, ev->border_width);
    }
}

static void
compositorHandleCirculateNotify (DisplayInfo *display_info, XCirculateEvent *ev)
{
    CWindow *cw;
    CWindow *top;
    GList *first;
    Window above;

    g_return_if_fail (display_info != NULL);
    g_return_if_fail (ev != NULL);
    TRACE ("window 0x%lx", ev->window);

    cw = find_cwindow_in_display (display_info, ev->window);
    if (!(is_on_compositor (cw)))
    {
        return;
    }

    first = cw->screen_info->cwindows;
    top = (CWindow *) first;

    if ((ev->place == PlaceOnTop) && (top))
    {
        above = top->id;
    }
    else
    {
        above = None;
    }
    restack_win (cw, above);
}

static void
compositorHandleCreateNotify (DisplayInfo *display_info, XCreateWindowEvent *ev)
{
    g_return_if_fail (display_info != NULL);
    g_return_if_fail (ev != NULL);
    TRACE ("window 0x%lx", ev->window);

    /*
       We are only interested in top level windows, other will
       be caught by the WM.
     */

    if (myDisplayGetScreenFromRoot (display_info, ev->parent) != NULL)
    {
        if (!find_cwindow_in_display (display_info, ev->window))
        {
            add_win (display_info, ev->window, NULL);
        }
    }
}

static void
compositorHandleReparentNotify (DisplayInfo *display_info, XReparentEvent *ev)
{
    g_return_if_fail (display_info != NULL);
    g_return_if_fail (ev != NULL);
    TRACE ("window 0x%lx", ev->window);

    if (myDisplayGetScreenFromRoot (display_info, ev->parent) != NULL)
    {
        add_win (display_info, ev->window, NULL);
    }
    else
    {
        destroy_win (display_info, ev->window);
    }
}

static void
compositorHandleDestroyNotify (DisplayInfo *display_info, XDestroyWindowEvent *ev)
{
    g_return_if_fail (display_info != NULL);
    g_return_if_fail (ev != NULL);
    TRACE ("window 0x%lx", ev->window);

    destroy_win (display_info, ev->window);
}

static void
compositorHandleMapNotify (DisplayInfo *display_info, XMapEvent *ev)
{
    CWindow *cw;

    g_return_if_fail (display_info != NULL);
    g_return_if_fail (ev != NULL);
    TRACE ("window 0x%lx", ev->window);

    cw = find_cwindow_in_display (display_info, ev->window);
    if (is_on_compositor (cw))
    {
        map_win (cw);
    }
}

static void
compositorHandleUnmapNotify (DisplayInfo *display_info, XUnmapEvent *ev)
{
    CWindow *cw;

    g_return_if_fail (display_info != NULL);
    g_return_if_fail (ev != NULL);
    TRACE ("window 0x%lx", ev->window);

    if (ev->from_configure)
    {
        TRACE ("ignoring UnmapNotify caused by parent's resize");
        return;
    }

    cw = find_cwindow_in_display (display_info, ev->window);
    if (is_on_compositor (cw))
    {
        if (WIN_IS_VIEWABLE (cw))
        {
            unmap_win (cw);
        }
    }
}

static void
compositorHandleShapeNotify (DisplayInfo *display_info, XShapeEvent *ev)
{
    CWindow *cw;

    g_return_if_fail (display_info != NULL);
    g_return_if_fail (ev != NULL);
    TRACE ("window 0x%lx", ev->window);

    cw = find_cwindow_in_display (display_info, ev->window);
    if (is_on_compositor (cw))
    {
        if (ev->kind == ShapeBounding)
        {
            if (!(ev->shaped) && (cw->shaped))
            {
                cw->shaped = FALSE;
            }
            reshape_win  (cw);
            if ((ev->shaped) && !(cw->shaped))
            {
                cw->shaped = TRUE;
            }
        }
    }
}

static void
compositorHandleCursorNotify (DisplayInfo *display_info, XFixesCursorNotifyEvent *ev)
{
    ScreenInfo *screen_info;

    g_return_if_fail (display_info != NULL);
    g_return_if_fail (ev != NULL);
    TRACE ("window 0x%lx", ev->window);

    screen_info = myDisplayGetScreenFromRoot (display_info, ev->window);
    if (screen_info && screen_info->cursor_is_zoomed)
    {
        update_cursor (screen_info);
    }
}

static gboolean
compositorCheckCMSelection (ScreenInfo *screen_info)
{
    DisplayInfo *display_info;
    gchar selection[32];
    Atom a;

    display_info = screen_info->display_info;

    /* Newer EWMH standard property "_NET_WM_CM_S<n>" */
    g_snprintf (selection, sizeof (selection), "_NET_WM_CM_S%d", screen_info->screen);
    a = XInternAtom (display_info->dpy, selection, FALSE);
    if (XGetSelectionOwner (display_info->dpy, a) != None)
    {
        return TRUE;
    }

    /* Older property "COMPOSITING_MANAGER" */
    if (XGetSelectionOwner (display_info->dpy, display_info->atoms[COMPOSITING_MANAGER]) != None)
    {
        return TRUE;
    }

    return FALSE;
}

#ifdef HAVE_PRESENT_EXTENSION
static void
compositorHandlePresentCompleteNotify (DisplayInfo *display_info, XPresentCompleteNotifyEvent *ev)
{
    ScreenInfo *screen_info;
    GSList *list;

    g_return_if_fail (display_info != NULL);
    g_return_if_fail (ev != NULL);
    TRACE ("window 0x%lx", ev->window);

    for (list = display_info->screens; list; list = g_slist_next (list))
    {
        screen_info = (ScreenInfo *) list->data;
        if (screen_info->output == ev->window)
        {
             DBG ("present completed, present pending cleared");
             screen_info->present_pending = FALSE;
             break;
        }
    }
}

static void
compositorHandleGenericEvent(DisplayInfo *display_info, XGenericEvent *ev)
{
    XGenericEventCookie *ev_cookie = (XGenericEventCookie *) ev;

    g_return_if_fail (display_info != NULL);
    g_return_if_fail (ev != NULL);
    TRACE ("entering");

    if (ev_cookie->extension == display_info->present_opcode)
    {
        XGetEventData (display_info->dpy, ev_cookie);
        if (ev_cookie->evtype == PresentCompleteNotify)
        {
            compositorHandlePresentCompleteNotify (display_info,
                                                   (XPresentCompleteNotifyEvent *) ev_cookie->data);
        }
        XFreeEventData (display_info->dpy, ev_cookie);
    }
}
#endif /* HAVE_PRESENT_EXTENSION */

static void
compositorSetCMSelection (ScreenInfo *screen_info, Window w)
{
    DisplayInfo *display_info;
    gchar selection[32];
    Atom a;

    g_return_if_fail (screen_info != NULL);

    display_info = screen_info->display_info;
    /* Newer EWMH standard property "_NET_WM_CM_S<n>" */
    g_snprintf (selection, sizeof (selection), "_NET_WM_CM_S%d", screen_info->screen);
    a = XInternAtom (display_info->dpy, selection, FALSE);
    setXAtomManagerOwner (display_info, a, screen_info->xroot, w);
}

static Pixmap
compositorScaleWindowPixmap (CWindow *cw, guint *width, guint *height)
{
    Display *dpy;
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Picture srcPicture, tmpPicture, destPicture;
    Pixmap tmpPixmap, dstPixmap;
    XTransform transform;
    XRenderPictFormat *render_format;
    double scale;
    int tx, ty;
    int src_x, src_y;
    int src_size, dest_size;
    unsigned int src_w, src_h;
    unsigned int dst_w, dst_h;
    XRenderColor c = { 0x7fff, 0x7fff, 0x7fff, 0xffff };

    screen_info = cw->screen_info;
    display_info = screen_info->display_info;
    dpy = myScreenGetXDisplay (screen_info);

    srcPicture = cw->picture;
    if (!srcPicture)
    {
        srcPicture = cw->saved_picture;
    }
    /* Could not get a usable picture, bail out */
    if (!srcPicture)
    {
        return None;
    }

    /* Get the source pixmap size to compute the scale */
    get_paint_bounds (cw, &tx, &ty, &src_w, &src_h);
    if (WIN_HAS_CLIENT(cw))
    {
        src_x = ABS(frameExtentLeft (cw->c));
        src_y = ABS(frameExtentTop (cw->c));
        src_w = src_w - src_x - ABS(frameExtentRight (cw->c));
        src_h = src_h - src_y - ABS(frameExtentBottom (cw->c));
    }
    else
    {
        src_x = 0;
        src_y = 0;
    }
    src_size = MAX (src_w, src_h);

    /* Shaped windows have no height */
    if (src_w == 0 || src_h == 0)
    {
        return None;
    }

    /*/
     * Caller may pass either NULL or 0.
     * If 0, we return the actual unscalled size.
     */
    dst_w = (width != NULL && *width > 0) ? *width : src_w;
    dst_h = (height != NULL && *height > 0) ? *height : src_h;
    dest_size = MIN (dst_w, dst_h);

    scale = (double) dest_size / (double) src_size;
    dst_w = src_w * scale;
    dst_h = src_h * scale;

    transform.matrix[0][0] = XDoubleToFixed (1.0);
    transform.matrix[0][1] = XDoubleToFixed (0.0);
    transform.matrix[0][2] = XDoubleToFixed (0.0);
    transform.matrix[1][0] = XDoubleToFixed (0.0);
    transform.matrix[1][1] = XDoubleToFixed (1.0);
    transform.matrix[1][2] = XDoubleToFixed (0.0);
    transform.matrix[2][0] = XDoubleToFixed (0.0);
    transform.matrix[2][1] = XDoubleToFixed (0.0);
    transform.matrix[2][2] = XDoubleToFixed (scale);

    tmpPixmap = XCreatePixmap (dpy, screen_info->output, src_w, src_h, 32);
    if (!tmpPixmap)
    {
        return None;
    }

    dstPixmap = XCreatePixmap (dpy, screen_info->output, dst_w, dst_h, 32);
    if (!dstPixmap)
    {
        XFreePixmap (dpy, tmpPixmap);
        return None;
    }

    render_format = get_window_format (cw);
    if (!render_format)
    {
        XFreePixmap (dpy, dstPixmap);
        XFreePixmap (dpy, tmpPixmap);
        return None;
    }

    myDisplayErrorTrapPush (display_info);
    render_format = XRenderFindStandardFormat (dpy, PictStandardARGB32);
    tmpPicture = XRenderCreatePicture (dpy, tmpPixmap, render_format, 0, NULL);
    XRenderFillRectangle (dpy, PictOpSrc, tmpPicture, &c, 0, 0, src_w, src_h);
    XFixesSetPictureClipRegion (dpy, tmpPicture, 0, 0, None);
    XRenderComposite (dpy, PictOpOver, srcPicture, None, tmpPicture,
                      src_x, src_y, 0, 0, 0, 0, src_w, src_h);

    XRenderSetPictureFilter (dpy, tmpPicture, FilterBest, NULL, 0);
    XRenderSetPictureTransform (dpy, tmpPicture, &transform);

    destPicture = XRenderCreatePicture (dpy, dstPixmap, render_format, 0, NULL);
    XRenderComposite (dpy, PictOpOver, tmpPicture, None, destPicture,
                      0, 0, 0, 0, 0, 0, dst_w, dst_h);

    XRenderFreePicture (dpy, tmpPicture);
    XRenderFreePicture (dpy, destPicture);
    XFreePixmap (dpy, tmpPixmap);

    /* Update given size if requested */
    if (width != NULL)
    {
        *width = dst_w;
    }
    if (height != NULL)
    {
        *height = dst_h;
    }
    myDisplayErrorTrapPopIgnored (display_info);

    return dstPixmap;
}

#endif /* HAVE_COMPOSITOR */

gboolean
compositorIsUsable (DisplayInfo *display_info)
{
#ifdef HAVE_COMPOSITOR
    return display_info->enable_compositor;
#endif /* HAVE_COMPOSITOR */
    return FALSE;
}

gboolean
compositorIsActive (ScreenInfo *screen_info)
{
#ifdef HAVE_COMPOSITOR
    g_return_val_if_fail (screen_info != NULL, FALSE);

    return screen_info->compositor_active;
#endif /* HAVE_COMPOSITOR */
    return FALSE;
}

void
compositorAddWindow (DisplayInfo *display_info, Window id, Client *c)
{
#ifdef HAVE_COMPOSITOR
    g_return_if_fail (display_info != NULL);
    g_return_if_fail (id != None);
    TRACE ("window 0x%lx", id);

    if (!compositorIsUsable (display_info))
    {
        return;
    }

    if (!compositorSetClient (display_info, id, c))
    {
        add_win (display_info, id, c);
    }
#endif /* HAVE_COMPOSITOR */
}

gboolean
compositorSetClient (DisplayInfo *display_info, Window id, Client *c)
{
#ifdef HAVE_COMPOSITOR
    CWindow *cw;

    g_return_val_if_fail (display_info != NULL, FALSE);
    g_return_val_if_fail (id != None, FALSE);
    TRACE ("window 0x%lx", id);

    if (!compositorIsUsable (display_info))
    {
        return FALSE;
    }

    cw = find_cwindow_in_display (display_info, id);
    if (is_on_compositor (cw))
    {
        if (cw->c != c)
        {
            update_extents (cw);
            cw->c = c;
        }
        return TRUE;
    }
#endif /* HAVE_COMPOSITOR */
    return FALSE;
}

void
compositorRemoveWindow (DisplayInfo *display_info, Window id)
{
#ifdef HAVE_COMPOSITOR
    g_return_if_fail (display_info != NULL);
    g_return_if_fail (id != None);
    TRACE ("window 0x%lx", id);

    if (!compositorIsUsable (display_info))
    {
        return;
    }

    destroy_win (display_info, id);
#endif /* HAVE_COMPOSITOR */
}

void
compositorDamageWindow (DisplayInfo *display_info, Window id)
{
#ifdef HAVE_COMPOSITOR
    CWindow *cw;

    g_return_if_fail (display_info != NULL);
    g_return_if_fail (id != None);
    TRACE ("window 0x%lx", id);

    if (!compositorIsUsable (display_info))
    {
        return;
    }

    cw = find_cwindow_in_display (display_info, id);
    if (is_on_compositor (cw))
    {
        /* that will also damage the window */
        update_extents (cw);
    }
#endif /* HAVE_COMPOSITOR */
}

void
compositorResizeWindow (DisplayInfo *display_info, Window id, int x, int y, int width, int height)
{
#ifdef HAVE_COMPOSITOR
    CWindow *cw;

    g_return_if_fail (display_info != NULL);
    g_return_if_fail (id != None);
    TRACE ("window 0x%lx", id);

    if (!compositorIsUsable (display_info))
    {
        return;
    }

    cw = find_cwindow_in_display (display_info, id);
    if (is_on_compositor (cw))
    {
        resize_win (cw, x, y, width, height, 0);
    }
#endif /* HAVE_COMPOSITOR */
}

/* May return None if:
 * - The xserver does not support name window pixmaps
 * - The compositor is disabled at run time
 * - The compositor is disabled at build time
 * The pixmap may be an older copy of the last know good pixmap
 * if the window is unmapped.
 */
Pixmap
compositorGetWindowPixmapAtSize (ScreenInfo *screen_info, Window id, guint *width, guint *height)
{
#ifdef HAVE_NAME_WINDOW_PIXMAP
#ifdef HAVE_COMPOSITOR
    CWindow *cw;

    TRACE ("window 0x%lx", id);

    if (!screen_info->compositor_active)
    {
        return None;
    }

    g_return_val_if_fail (id != None, None);

    if (!compositorIsActive (screen_info))
    {
        return None;
    }

    cw = find_cwindow_in_screen (screen_info, id);
    if (is_on_compositor (cw))
    {
        return compositorScaleWindowPixmap (cw, width, height);
    }
#endif /* HAVE_COMPOSITOR */
#endif /* HAVE_NAME_WINDOW_PIXMAP */

    return None;
}

void
compositorHandleEvent (DisplayInfo *display_info, XEvent *ev)
{
#ifdef HAVE_COMPOSITOR
    g_return_if_fail (display_info != NULL);
    g_return_if_fail (ev != NULL);
    TRACE ("event type %i", ev->type);

    if (!compositorIsUsable (display_info))
    {
        return;
    }
    else if (ev->type == CreateNotify)
    {
        compositorHandleCreateNotify (display_info, (XCreateWindowEvent *) ev);
    }
    else if (ev->type == DestroyNotify)
    {
        compositorHandleDestroyNotify (display_info, (XDestroyWindowEvent *) ev);
    }
    else if (ev->type == ConfigureNotify)
    {
        compositorHandleConfigureNotify (display_info, (XConfigureEvent *) ev);
    }
    else if (ev->type == ReparentNotify)
    {
        compositorHandleReparentNotify (display_info, (XReparentEvent *) ev);
    }
    else if (ev->type == Expose)
    {
        compositorHandleExpose (display_info, (XExposeEvent *) ev);
    }
    else if (ev->type == CirculateNotify)
    {
        compositorHandleCirculateNotify (display_info, (XCirculateEvent *) ev);
    }
    else if (ev->type == PropertyNotify)
    {
        compositorHandlePropertyNotify (display_info, (XPropertyEvent *) ev);
    }
    else if (ev->type == MapNotify)
    {
        compositorHandleMapNotify (display_info, (XMapEvent *) ev);
    }
    else if (ev->type == UnmapNotify)
    {
        compositorHandleUnmapNotify (display_info, (XUnmapEvent *) ev);
    }
    else if (ev->type == (display_info->damage_event_base + XDamageNotify))
    {
        compositorHandleDamage (display_info, (XDamageNotifyEvent *) ev);
    }
    else if (ev->type == (display_info->shape_event_base + ShapeNotify))
    {
        compositorHandleShapeNotify (display_info, (XShapeEvent *) ev);
    }
    else if (ev->type == (display_info->fixes_event_base + XFixesCursorNotify))
    {
        compositorHandleCursorNotify (display_info, (XFixesCursorNotifyEvent *) ev);
    }
#ifdef HAVE_PRESENT_EXTENSION
    else if (ev->type == GenericEvent)
    {
        compositorHandleGenericEvent (display_info, (XGenericEvent *) ev);
    }
#endif /* HAVE_PRESENT_EXTENSION */

#endif /* HAVE_COMPOSITOR */
}

void
compositorZoomIn (ScreenInfo *screen_info, XfwmEventButton *event)
{
    TRACE ("entering");

#ifdef HAVE_COMPOSITOR
    if (!screen_info->compositor_active)
    {
        return;
    }

    screen_info->transform.matrix[0][0] -= 4096;
    screen_info->transform.matrix[1][1] -= 4096;

    if (screen_info->transform.matrix[0][0] < (1 << 10))
    {
        screen_info->transform.matrix[0][0] = (1 << 10);
        screen_info->transform.matrix[1][1] = (1 << 10);
    }

    if (!screen_info->zoomed)
    {
        screen_info->cursor_is_zoomed = screen_info->params->zoom_pointer;

        if (screen_info->cursor_is_zoomed)
        {
            XFixesHideCursor (screen_info->display_info->dpy, screen_info->xroot);
            screen_info->cursorLocation.x = event->x_root - screen_info->cursorOffsetX;
            screen_info->cursorLocation.y = event->y_root - screen_info->cursorOffsetY;
            update_cursor (screen_info);
        }
    }

    screen_info->zoomed = TRUE;
    if (!screen_info->zoom_timeout_id)
    {
        gint timeout_rate;

        timeout_rate = xfwm_get_primary_refresh_rate (screen_info->gscr);
        screen_info->zoom_timeout_id = g_timeout_add ((1000 / timeout_rate /* per second */),
                                                      zoom_timeout_cb, screen_info);
    }
    recenter_zoomed_area (screen_info, event->x_root, event->y_root);
#endif /* HAVE_COMPOSITOR */
}

void
compositorZoomOut (ScreenInfo *screen_info, XfwmEventButton *event)
{
    TRACE ("entering");

#ifdef HAVE_COMPOSITOR
    if (!screen_info->compositor_active)
    {
        return;
    }

    /* don't do anything if the user disabled the zoom feature */
    if (screen_info->zoomed)
    {
        screen_info->transform.matrix[0][0] += 4096;
        screen_info->transform.matrix[1][1] += 4096;

        if (screen_info->transform.matrix[0][0] >= (1 << 16))
        {
            screen_info->transform.matrix[0][0] = (1 << 16);
            screen_info->transform.matrix[1][1] = (1 << 16);
            screen_info->transform.matrix[0][2] = 0;
            screen_info->transform.matrix[1][2] = 0;
            screen_info->zoomed = FALSE;

            if (screen_info->cursor_is_zoomed)
            {
                XFixesShowCursor (screen_info->display_info->dpy, screen_info->xroot);
            }
        }
        recenter_zoomed_area (screen_info, event->x_root, event->y_root);
    }
#endif /* HAVE_COMPOSITOR */
}

void
compositorInitDisplay (DisplayInfo *display_info)
{
#ifdef HAVE_COMPOSITOR
    int composite_major, composite_minor;

    composite_major = 0;
    composite_minor = 0;

    if (!XCompositeQueryExtension (display_info->dpy,
                                &display_info->composite_event_base,
                                &display_info->composite_error_base))
    {
        g_warning ("The display does not support the XComposite extension.");
        display_info->have_composite = FALSE;
        display_info->composite_event_base = 0;
        display_info->composite_error_base = 0;
    }
    else
    {
        display_info->have_composite = TRUE;
        XCompositeQueryVersion (display_info->dpy, &composite_major, &composite_minor);

        DBG ("composite event base: %i", display_info->composite_event_base);
        DBG ("composite error base: %i", display_info->composite_error_base);
        DBG ("composite version: %i.%i", composite_major, composite_minor);
    }

    if (!XDamageQueryExtension (display_info->dpy,
                            &display_info->damage_event_base,
                            &display_info->damage_error_base))
    {
        g_warning ("The display does not support the XDamage extension.");
        display_info->have_damage = FALSE;
        display_info->damage_event_base = 0;
        display_info->damage_error_base = 0;
    }
    else
    {
        display_info->have_damage = TRUE;

        DBG ("damage event base: %i", display_info->damage_event_base);
        DBG ("damage error base: %i", display_info->damage_error_base);
    }

    if (!XFixesQueryExtension (display_info->dpy,
                            &display_info->fixes_event_base,
                            &display_info->fixes_error_base))
    {
        g_warning ("The display does not support the XFixes extension.");
        display_info->have_fixes = FALSE;
        display_info->fixes_event_base = 0;
        display_info->fixes_error_base = 0;
    }
    else
    {
        display_info->have_fixes = TRUE;

        DBG ("fixes event base: %i", display_info->fixes_event_base);
        DBG ("fixes error base: %i", display_info->fixes_error_base);
    }

#ifdef HAVE_PRESENT_EXTENSION
    if (!XPresentQueryExtension (display_info->dpy,
                                 &display_info->present_opcode,
                                 &display_info->present_event_base,
                                 &display_info->present_error_base))
    {
        g_warning ("The display does not support the XPresent extension.");
        display_info->have_present = FALSE;
        display_info->present_opcode = 0;
        display_info->present_event_base = 0;
        display_info->present_error_base = 0;
    }
    else
    {
        display_info->have_present = TRUE;
        DBG ("present opcode:  %i", display_info->present_opcode);
        DBG ("present event base: %i", display_info->present_event_base);
        DBG ("present error base: %i", display_info->present_error_base);
    }
#endif /* HAVE_PRESENT_EXTENSION */

    display_info->enable_compositor = ((display_info->have_render)
                                    && (display_info->have_composite)
                                    && (display_info->have_damage)
                                    && (display_info->have_fixes));
    if (!display_info->enable_compositor)
    {
        g_warning ("Compositing manager disabled.");
    }

#if HAVE_NAME_WINDOW_PIXMAP
    display_info->have_name_window_pixmap = ((composite_major > 0) || (composite_minor >= 2));
#else  /* HAVE_NAME_WINDOW_PIXMAP */
    display_info->have_name_window_pixmap = FALSE;
#endif /* HAVE_NAME_WINDOW_PIXMAP */

#if HAVE_OVERLAYS
    display_info->have_overlays = ((composite_major > 0) || (composite_minor >= 3));
#endif /* HAVE_OVERLAYS */

#else /* HAVE_COMPOSITOR */
    display_info->enable_compositor = FALSE;
#endif /* HAVE_COMPOSITOR */
}

gboolean
compositorManageScreen (ScreenInfo *screen_info)
{
#ifdef HAVE_COMPOSITOR
    DisplayInfo *display_info;
    XRenderPictureAttributes pa;
    XRenderPictFormat *visual_format;
    gushort buffer;

    g_return_val_if_fail (screen_info != NULL, FALSE);
    TRACE ("entering compositorManageScreen");

    if (compositorCheckCMSelection (screen_info))
    {
        g_warning ("Another compositing manager is running on screen %i", screen_info->screen);
        return FALSE;
    }

    compositorSetCMSelection (screen_info, screen_info->xfwm4_win);
    display_info = screen_info->display_info;

    screen_info->output = screen_info->xroot;
#if HAVE_OVERLAYS
    if (display_info->have_overlays)
    {
        screen_info->overlay = XCompositeGetOverlayWindow (display_info->dpy, screen_info->xroot);
        if (screen_info->overlay != None)
        {
            XserverRegion region;
            XSetWindowAttributes attributes;

            XMapRaised (display_info->dpy, screen_info->overlay);

            region = XFixesCreateRegion (display_info->dpy, NULL, 0);
            XFixesSetWindowShapeRegion (display_info->dpy, screen_info->overlay,
                                        ShapeBounding, 0, 0, 0);
            XFixesSetWindowShapeRegion (display_info->dpy, screen_info->overlay,
                                        ShapeInput, 0, 0, region);
            XFixesDestroyRegion (display_info->dpy, region);

            screen_info->root_overlay = XCreateWindow (display_info->dpy, screen_info->overlay,
                                                       0, 0, screen_info->width, screen_info->height, 0, screen_info->depth,
                                                       InputOutput, screen_info->visual, 0, &attributes);
            XMapRaised (display_info->dpy, screen_info->root_overlay);

            screen_info->output = screen_info->root_overlay;
            DBG ("Overlay enabled (0x%lx -> 0x%lx)", screen_info->overlay, screen_info->root_overlay);
        }
        else
        {
            /* Something is wrong with overlay support */
            DBG ("Cannot get root window overlay, overlay support disabled");
            display_info->have_overlays = FALSE;
        }
    }
#endif /* HAVE_OVERLAYS */
    DBG ("Window used for output: 0x%lx (%s)", screen_info->output, display_info->have_overlays ? "overlay" : "root");

    XCompositeRedirectSubwindows (display_info->dpy, screen_info->xroot, CompositeRedirectManual);
    screen_info->compositor_active = TRUE;

    visual_format = XRenderFindVisualFormat (display_info->dpy,
                                             DefaultVisual (display_info->dpy,
                                                            screen_info->screen));
    if (!visual_format)
    {
        g_warning ("Cannot find visual format on screen %i", screen_info->screen);
        compositorUnmanageScreen (screen_info);
        return FALSE;
    }

    pa.subwindow_mode = IncludeInferiors;
    screen_info->rootPicture = XRenderCreatePicture (display_info->dpy, screen_info->output,
                                                     visual_format, CPSubwindowMode, &pa);

    if (screen_info->rootPicture == None)
    {
        g_warning ("Cannot create root picture on screen %i", screen_info->screen);
        compositorUnmanageScreen (screen_info);
        return FALSE;
    }

    screen_info->gaussianSize = -1;
    screen_info->gaussianMap = make_gaussian_map(SHADOW_RADIUS);
    presum_gaussian (screen_info);
    screen_info->cursorPicture = None;
    /* Change following argb values to play with shadow colors */
    screen_info->blackPicture = solid_picture (screen_info,
                                               TRUE,
                                               1.0, /* alpha */
                                               0.0, /* red   */
                                               0.0, /* green */
                                               0.0  /* blue  */);
    screen_info->rootTile = None;
    screen_info->allDamage = None;
    screen_info->prevDamage = None;
    screen_info->screenRegion = get_screen_region (screen_info);
    screen_info->cwindows = NULL;
    screen_info->cwindow_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
    screen_info->wins_unredirected = 0;
    screen_info->compositor_timeout_id = 0;
    screen_info->zoomed = FALSE;
    screen_info->zoom_timeout_id = 0;
    screen_info->damages_pending = FALSE;
    screen_info->current_buffer = 0;
    memset(screen_info->transform.matrix, 0, 9);
    screen_info->transform.matrix[0][0] = 1 << 16;
    screen_info->transform.matrix[1][1] = 1 << 16;
    screen_info->transform.matrix[2][2] = 1 << 16;
    screen_info->zoomBuffer = None;
    screen_info->use_n_buffers = 1;
    for (buffer = 0; buffer < N_BUFFERS; buffer++)
    {
        screen_info->rootPixmap[buffer] = None;
        screen_info->rootBuffer[buffer] = None;
#ifdef HAVE_EPOXY
        screen_info->glx_drawable[buffer] = None;
#ifdef HAVE_XSYNC
        screen_info->fence[buffer] = None;
#endif /* HAVE_XSYNC */
#endif /* HAVE_EPOXY */
    }
    XClearArea (display_info->dpy, screen_info->output, 0, 0, 0, 0, TRUE);
    TRACE ("manual compositing enabled");

#ifdef HAVE_EPOXY
    screen_info->use_glx = (screen_info->vblank_mode == VBLANK_AUTO ||
                            screen_info->vblank_mode == VBLANK_GLX);
#ifdef HAVE_XSYNC
    screen_info->use_glx &= display_info->have_xsync;
#endif /* HAVE_XSYNC */

    if (screen_info->use_glx)
    {
        screen_info->use_n_buffers = 1;
        screen_info->glx_context = None;
        screen_info->glx_window = None;
        screen_info->rootTexture = None;
        screen_info->texture_filter = GL_LINEAR;
        screen_info->gl_sync = 0;
        screen_info->use_glx = init_glx (screen_info);
    }
#else /* HAVE_EPOXY */
    screen_info->use_glx = FALSE;
#endif /* HAVE_EPOXY */

#ifdef HAVE_PRESENT_EXTENSION
    screen_info->use_present = display_info->have_present &&
#ifdef HAVE_EPOXY
                               !screen_info->use_glx &&
#endif /* HAVE_EPOXY */
                               (screen_info->vblank_mode == VBLANK_AUTO ||
                                screen_info->vblank_mode == VBLANK_XPRESENT);
    if (screen_info->use_present)
    {
        screen_info->use_n_buffers = N_BUFFERS;
        screen_info->present_pending = FALSE;
        XPresentSelectInput (display_info->dpy,
                             screen_info->output,
                             PresentCompleteNotifyMask);
    }
#else /* HAVE_PRESENT_EXTENSION */
    screen_info->use_present = FALSE;
#endif /* HAVE_PRESENT_EXTENSION */

    if (screen_info->use_present)
    {
        g_info ("Compositor using XPresent for vsync");
    }
    else if (screen_info->use_glx)
    {
        g_info ("Compositor using GLX for vsync");
    }
    else
    {
        g_info ("No vsync support in compositor");
    }

    XFixesSelectCursorInput (display_info->dpy,
                             screen_info->xroot,
                             XFixesDisplayCursorNotifyMask);

    return TRUE;
#else
    return FALSE;
#endif /* HAVE_COMPOSITOR */
}

void
compositorUnmanageScreen (ScreenInfo *screen_info)
{
#ifdef HAVE_COMPOSITOR
    DisplayInfo *display_info;
    GList *list;
    gushort buffer;

    g_return_if_fail (screen_info != NULL);
    TRACE ("entering compositorUnmanageScreen");

    display_info = screen_info->display_info;
    if (!compositorIsUsable (display_info))
    {
        return;
    }

    if (!screen_info->compositor_active)
    {
        TRACE ("compositor not active on screen %i", screen_info->screen);
        return;
    }
    screen_info->compositor_active = FALSE;

    remove_timeouts (screen_info);

    myDisplayErrorTrapPush (display_info);

    for (list = screen_info->cwindows; list; list = g_list_next (list))
    {
        CWindow *cw2 = (CWindow *) list->data;
        free_win_data (cw2, TRUE);
    }

    g_hash_table_destroy(screen_info->cwindow_hash);
    screen_info->cwindow_hash = NULL;
    g_list_free (screen_info->cwindows);
    screen_info->cwindows = NULL;

#ifdef HAVE_EPOXY
    if (screen_info->use_glx)
    {
        free_glx_data (screen_info);
        for (buffer = 0; buffer < screen_info->use_n_buffers; buffer++)
        {
            destroy_glx_drawable (screen_info, buffer);
        }
    }
#endif /* HAVE_EPOXY */

    for (buffer = 0; buffer < screen_info->use_n_buffers; buffer++)
    {
        if (screen_info->rootPixmap[buffer])
        {
            XFreePixmap (display_info->dpy, screen_info->rootPixmap[buffer]);
            screen_info->rootPixmap[buffer] = None;
        }
        if (screen_info->rootBuffer[buffer])
        {
            XRenderFreePicture (display_info->dpy, screen_info->rootBuffer[buffer]);
            screen_info->rootBuffer[buffer] = None;
        }
#ifdef HAVE_EPOXY
        fence_destroy (screen_info, buffer);
#endif /* HAVE_EPOXY */
    }

    if (screen_info->allDamage)
    {
        XFixesDestroyRegion (display_info->dpy, screen_info->allDamage);
        screen_info->allDamage = None;
    }

    if (screen_info->prevDamage)
    {
        XFixesDestroyRegion (display_info->dpy, screen_info->prevDamage);
        screen_info->prevDamage = None;
    }

    if (screen_info->screenRegion)
    {
        XFixesDestroyRegion (display_info->dpy, screen_info->screenRegion);
        screen_info->screenRegion = None;
    }

    if (screen_info->zoomBuffer)
    {
        XRenderFreePicture (display_info->dpy, screen_info->zoomBuffer);
        screen_info->zoomBuffer = None;
    }

    if (screen_info->rootPicture)
    {
        XRenderFreePicture (display_info->dpy, screen_info->rootPicture);
        screen_info->rootPicture = None;
    }

    if (screen_info->blackPicture)
    {
        XRenderFreePicture (display_info->dpy, screen_info->blackPicture);
        screen_info->blackPicture = None;
    }
    if (screen_info->cursorPicture)
    {
        XRenderFreePicture (display_info->dpy, screen_info->cursorPicture);
        screen_info->cursorPicture = None;
    }

    if (screen_info->shadowTop)
    {
        g_free (screen_info->shadowTop);
        screen_info->shadowTop = NULL;
    }

    if (screen_info->shadowCorner)
    {
        g_free (screen_info->shadowCorner);
        screen_info->shadowCorner = NULL;
    }

    if (screen_info->gaussianMap)
    {
        g_free (screen_info->gaussianMap);
        screen_info->gaussianMap = NULL;
    }

#if HAVE_OVERLAYS
    if (display_info->have_overlays)
    {
        XDestroyWindow (display_info->dpy, screen_info->root_overlay);
        screen_info->root_overlay = None;

        XCompositeReleaseOverlayWindow (display_info->dpy, screen_info->overlay);
        screen_info->overlay = None;
    }
#endif /* HAVE_OVERLAYS */

    screen_info->gaussianSize = -1;
    screen_info->wins_unredirected = 0;

    if (screen_info->zoomed)
    {
        XFixesShowCursor (display_info->dpy, screen_info->xroot);
    }

    XCompositeUnredirectSubwindows (display_info->dpy, screen_info->xroot,
                                    CompositeRedirectManual);
    screen_info->output = screen_info->xroot;

    compositorSetCMSelection (screen_info, None);

    myDisplayErrorTrapPopIgnored (display_info);
#endif /* HAVE_COMPOSITOR */
}

void
compositorAddAllWindows (ScreenInfo *screen_info)
{
#ifdef HAVE_COMPOSITOR
    DisplayInfo *display_info;
    Window w1, w2, *wins;
    unsigned int count, i;

    TRACE ("entering");

    display_info = screen_info->display_info;
    if (!compositorIsUsable (display_info))
    {
        return;
    }

    myDisplayGrabServer (display_info);
    XQueryTree (display_info->dpy, screen_info->xroot, &w1, &w2, &wins, &count);

    for (i = 0; i < count; i++)
    {
        Client *c;

        c = myScreenGetClientFromWindow (screen_info, wins[i], SEARCH_FRAME);
        compositorAddWindow (display_info, wins[i], c);
    }
    if (wins)
    {
        XFree (wins);
    }
    myDisplayUngrabServer (display_info);
#endif /* HAVE_COMPOSITOR */
}

gboolean
compositorActivateScreen (ScreenInfo *screen_info, gboolean active)
{
#ifdef HAVE_COMPOSITOR
    DisplayInfo *display_info;

    g_return_val_if_fail (screen_info != NULL, FALSE);
    TRACE ("active %s", active ? "TRUE" : "FALSE");

    display_info = screen_info->display_info;
    if (!compositorIsUsable (display_info))
    {
        return FALSE;
    }

    if (screen_info->compositor_active == active)
    {
        return FALSE;
    }

    if (active)
    {
        compositorManageScreen (screen_info);
        compositorAddAllWindows (screen_info);
    }
    else
    {
        compositorUnmanageScreen (screen_info);
    }

    return TRUE;
#else /* HAVE_COMPOSITOR */
    return FALSE;
#endif /* HAVE_COMPOSITOR */
}

void
compositorUpdateScreenSize (ScreenInfo *screen_info)
{
#ifdef HAVE_COMPOSITOR
    DisplayInfo *display_info;
    gushort buffer;

    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    display_info = screen_info->display_info;
    if (!compositorIsUsable (display_info))
    {
        return;
    }

    myDisplayErrorTrapPush (display_info);
#if HAVE_OVERLAYS
    if (display_info->have_overlays)
    {
        XResizeWindow (display_info->dpy, screen_info->overlay, screen_info->width, screen_info->height);
        XResizeWindow (display_info->dpy, screen_info->root_overlay, screen_info->width, screen_info->height);
    }
#endif
    if (screen_info->zoomBuffer)
    {
        XRenderFreePicture (display_info->dpy, screen_info->zoomBuffer);
        screen_info->zoomBuffer = None;
    }

#ifdef HAVE_EPOXY
    if (screen_info->use_glx)
    {
        for (buffer = 0; buffer < screen_info->use_n_buffers; buffer++)
        {
            destroy_glx_drawable (screen_info, buffer);
        }
    }
#endif /* HAVE_EPOXY */

    for (buffer = 0; buffer < screen_info->use_n_buffers; buffer++)
    {
        if (screen_info->rootPixmap[buffer])
        {
            XFreePixmap (display_info->dpy, screen_info->rootPixmap[buffer]);
            screen_info->rootPixmap[buffer] = None;
        }
        if (screen_info->rootBuffer[buffer])
        {
            XRenderFreePicture (display_info->dpy, screen_info->rootBuffer[buffer]);
            screen_info->rootBuffer[buffer] = None;
        }
#ifdef HAVE_EPOXY
        fence_destroy (screen_info, buffer);
#endif /* HAVE_EPOXY */
    }

    if (screen_info->screenRegion)
    {
        XFixesDestroyRegion (display_info->dpy, screen_info->screenRegion);
        screen_info->screenRegion = None;
    }

    damage_screen (screen_info);
    myDisplayErrorTrapPopIgnored (display_info);
#endif /* HAVE_COMPOSITOR */
}

void
compositorWindowSetOpacity (DisplayInfo *display_info, Window id, guint32 opacity)
{
#ifdef HAVE_COMPOSITOR
    CWindow *cw;

    g_return_if_fail (display_info != NULL);
    g_return_if_fail (id != None);
    TRACE ("window 0x%lx", id);

    if (!compositorIsUsable (display_info))
    {
        return;
    }

    cw = find_cwindow_in_display (display_info, id);
    if (cw)
    {
        set_win_opacity (cw, opacity);
    }
#endif /* HAVE_COMPOSITOR */
}

void
compositorRebuildScreen (ScreenInfo *screen_info)
{
#ifdef HAVE_COMPOSITOR
    DisplayInfo *display_info;
    GList *list;

    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    display_info = screen_info->display_info;
    if (!compositorIsUsable (display_info))
    {
        return;
    }

    for (list = screen_info->cwindows; list; list = g_list_next (list))
    {
        CWindow *cw2 = (CWindow *) list->data;
        free_win_data (cw2, FALSE);
        init_opacity (cw2);
    }
    damage_screen (screen_info);
#endif /* HAVE_COMPOSITOR */
}

gboolean
compositorTestServer (DisplayInfo *display_info)
{
#ifdef HAVE_COMPOSITOR
    char *vendor;

    g_return_val_if_fail (display_info != NULL, FALSE);
    TRACE ("entering");

    vendor = ServerVendor (display_info->dpy);

    if (vendor && (!strstr ("X.Org", vendor)))
    {
        /*
           Check the version, X.org 6.8.x has some bugs that makes
           it not suitable for use with xfwm4 compositor
         */
        if ((VendorRelease(display_info->dpy) / 10) <= 68)
        {
            return FALSE;
        }
    }
    return TRUE;
#else /* HAVE_COMPOSITOR */
    return FALSE;
#endif /* HAVE_COMPOSITOR */
}

vblankMode
compositorParseVblankMode (const gchar *vblank_setting)
{
#ifdef HAVE_PRESENT_EXTENSION
    if (g_ascii_strcasecmp (vblank_setting, "xpresent") == 0)
    {
        return VBLANK_XPRESENT;
    }
    else
#endif /* HAVE_PRESENT_EXTENSION */
#ifdef HAVE_EPOXY
    if (g_ascii_strcasecmp (vblank_setting, "glx") == 0)
    {
        return VBLANK_GLX;
    }
    else
#endif /* HAVE_EPOXY */
    if (g_ascii_strcasecmp (vblank_setting, "off") == 0)
    {
        return VBLANK_OFF;
    }

    return VBLANK_AUTO;
}

void
compositorSetVblankMode (ScreenInfo *screen_info,
                         vblankMode  vblank_mode)
{
#ifdef HAVE_COMPOSITOR
    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    screen_info->vblank_mode = vblank_mode;
#endif /* HAVE_COMPOSITOR */
}
