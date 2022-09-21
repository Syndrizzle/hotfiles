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


        Metacity - (c) 2001 Havoc Pennington
        libwnck  - (c) 2001 Havoc Pennington
        xfwm4    - (c) 2002-2015 Olivier Fourdan
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo/cairo-xlib.h>
#ifdef HAVE_COMPOSITOR
#include <cairo/cairo-xlib-xrender.h>
#endif
#include <libxfce4util/libxfce4util.h>

#include "icons.h"
#include "display.h"
#include "screen.h"
#include "client.h"
#include "compositor.h"
#include "hints.h"

static void
downsize_ratio (guint *width, guint *height, guint dest_w, guint dest_h)
{
    gdouble ratio;
    guint size;

    g_return_if_fail (width != NULL);
    g_return_if_fail (height != NULL);
    g_return_if_fail (dest_w > 0 && dest_w > 0);

    size = MIN (dest_w, dest_h);
    if (*width > *height)
    {
        ratio = ((gdouble) *width) / size;
        *width = (guint) size;
        *height = (guint) (((gdouble) *height) / ratio);
    }
    else
    {
        ratio = ((gdouble) *height) / size;
        *height = (guint) size;
        *width = (guint) (((gdouble) *width) / ratio);
    }
}

/*
 * create a GdkPixbuf from inline data and scale it to a given size
 */
static GdkPixbuf *
default_icon_at_size (GdkScreen *screen, guint width, guint height)
{
    GtkIconTheme *icon_theme;
    GdkPixbuf *base;
    GdkPixbuf *scaled;

    icon_theme = gtk_icon_theme_get_for_screen (screen);

    g_return_val_if_fail (icon_theme != NULL, NULL);

    if (width <= 0 || height <= 0)
    {
        width = 160;
        height = 160;
    }

    base = gtk_icon_theme_load_icon (icon_theme, "xfwm4-default",
                                     MAX (width, height), 0, NULL);

    if (base != NULL && width != height)
    {
        scaled = gdk_pixbuf_scale_simple (base, width, height, GDK_INTERP_BILINEAR);
        g_object_unref (G_OBJECT (base));
        return scaled;
    }

    return base;
}


static gboolean
find_largest_sizes (gulong * data, gulong nitems, guint *width, guint *height)
{
    guint w, h;

    *width = 0;
    *height = 0;

    while (nitems > 0)
    {
        if (nitems < 3)
        {
            return FALSE;       /* no space for w, h */
        }

        w = data[0];
        h = data[1];

        if (nitems < (gulong) ((w * h) + 2))
        {
            return FALSE;       /* not enough data */
        }

        *width = MAX (w, *width);
        *height = MAX (h, *height);

        data += (w * h) + 2;
        nitems -= (w * h) + 2;
    }

    return TRUE;
}

static gboolean
find_best_size (gulong * data, gulong nitems, gint ideal_width, gint ideal_height,
                guint *width, guint *height, gulong ** start)
{
    gulong *best_start;
    guint ideal_size;
    guint w, h, best_size, this_size;
    guint best_w, best_h, max_width, max_height;

    *width = 0;
    *height = 0;
    *start = NULL;

    if (!find_largest_sizes (data, nitems, &max_width, &max_height))
    {
        return FALSE;
    }

    if (ideal_width < 0)
    {
        ideal_width = max_width;
    }
    if (ideal_height < 0)
    {
        ideal_height = max_height;
    }

    best_w = 0;
    best_h = 0;
    best_start = NULL;

    while (nitems > 0)
    {
        gboolean replace;

        replace = FALSE;

        if (nitems < 3)
        {
            return FALSE;       /* no space for w, h */
        }

        w = data[0];
        h = data[1];

        if (nitems < (gulong) ((w * h) + 2))
        {
            break;              /* not enough data */
        }

        if (best_start == NULL)
        {
            replace = TRUE;
        }
        else
        {
            /* work with averages */
            ideal_size = (ideal_width + ideal_height) / 2;
            best_size = (best_w + best_h) / 2;
            this_size = (w + h) / 2;

            if ((best_size < ideal_size) && (this_size >= ideal_size))
            {
                /* larger than desired is always better than smaller */
                replace = TRUE;
            }
            else if ((best_size < ideal_size) && (this_size > best_size))
            {
                /* if we have too small, pick anything bigger */
                replace = TRUE;
            }
            else if ((best_size > ideal_size) && (this_size >= ideal_size) && (this_size < best_size))
            {
                /* if we have too large, pick anything smaller but still >= the ideal */
                replace = TRUE;
            }
        }

        if (replace)
        {
            best_start = data + 2;
            best_w = w;
            best_h = h;
        }

        data += (w * h) + 2;
        nitems -= (w * h) + 2;
    }

    if (best_start)
    {
        *start = best_start;
        *width = best_w;
        *height = best_h;
        return TRUE;
    }

    return FALSE;
}

static void
argbdata_to_pixdata (gulong * argb_data, guint len, guchar ** pixdata)
{
    guchar *p;
    guint argb;
    guint rgba;
    guint i;

    *pixdata = g_new0 (guchar, len * 4);
    p = *pixdata;

    i = 0;
    while (i < len)
    {
        argb = argb_data[i];
        rgba = (argb << 8) | (argb >> 24);

        *p = rgba >> 24; ++p;
        *p = (rgba >> 16) & 0xff; ++p;
        *p = (rgba >> 8) & 0xff; ++p;
        *p = rgba & 0xff; ++p;

        ++i;
    }
}

static gboolean
read_rgb_icon (DisplayInfo *display_info, Window window, guint ideal_width, guint ideal_height,
               guint *width, guint *height, guchar ** pixdata)
{
    gulong nitems;
    gulong *data;
    gulong *best;
    guint w, h;

    data = NULL;

    if (!getRGBIconData (display_info, window, &data, &nitems))
    {
        return FALSE;
    }

    if (!find_best_size (data, nitems, ideal_width, ideal_height, &w, &h, &best))
    {
        XFree (data);
        return FALSE;
    }

    *width = w;
    *height = h;

    argbdata_to_pixdata (best, w * h, pixdata);

    XFree (data);

    return TRUE;
}

static void
get_pixmap_geometry (ScreenInfo *screen_info, Pixmap pixmap, guint *out_width, guint *out_height, guint *out_depth)
{
    Window root;
    guint border_width;
    gint x, y;
    guint width, height;
    guint depth;
    Status rc;
    int result;

    myDisplayErrorTrapPush (screen_info->display_info);
    rc = XGetGeometry (myScreenGetXDisplay(screen_info), pixmap, &root,
                       &x, &y, &width, &height, &border_width, &depth);
    result = myDisplayErrorTrapPop (screen_info->display_info);

    if ((rc == 0) || (result != Success))
    {
        return;
    }

    if (out_width != NULL)
    {
        *out_width = width;
    }
    if (out_height != NULL)
    {
        *out_height = height;
    }
    if (out_depth != NULL)
    {
        *out_depth = depth;
    }
}

static cairo_surface_t *
get_surface_from_pixmap (ScreenInfo *screen_info, Pixmap xpixmap, guint width, guint height, guint depth)
{
    cairo_surface_t *surface;
#ifdef HAVE_COMPOSITOR
    XRenderPictFormat *render_format;
#endif

    if (depth == 1)
    {
        surface = cairo_xlib_surface_create_for_bitmap (screen_info->display_info->dpy,
                                                        xpixmap,
                                                        screen_info->xscreen,
                                                        width, height);
    }
#ifdef HAVE_COMPOSITOR
    else if (depth == 32)
    {
        render_format = XRenderFindStandardFormat (screen_info->display_info->dpy,
                                                   PictStandardARGB32);
        surface = cairo_xlib_surface_create_with_xrender_format (screen_info->display_info->dpy,
                                                                 xpixmap,
                                                                 screen_info->xscreen,
                                                                 render_format,
                                                                 width, height);
    }
#endif
    else
    {
        surface = cairo_xlib_surface_create (screen_info->display_info->dpy,
                                             xpixmap,
                                             screen_info->visual,
                                             width, height);
    }

    return surface;
}

static GdkPixbuf *
try_pixmap_and_mask (ScreenInfo *screen_info, Pixmap src_pixmap, Pixmap src_mask, guint width, guint height)
{
    GdkPixbuf *unscaled;
    GdkPixbuf *icon;
    guint w, h, depth;
    cairo_surface_t *surface, *mask_surface, *image;
    cairo_t *cr;

    if (G_UNLIKELY (src_pixmap == None))
    {
        return NULL;
    }

    get_pixmap_geometry (screen_info, src_pixmap, &w, &h, &depth);
    surface = get_surface_from_pixmap (screen_info, src_pixmap, w, h, depth);

    if (surface && src_mask != None)
    {
        get_pixmap_geometry (screen_info, src_mask, &w, &h, &depth);
        mask_surface = get_surface_from_pixmap (screen_info, src_mask, w, h, depth);
    }
    else
    {
        mask_surface = NULL;
    }

    if (G_UNLIKELY (surface == NULL))
    {
        return NULL;
    }
    myDisplayErrorTrapPush (screen_info->display_info);
    image = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
    cr = cairo_create (image);

    /* Need special code for alpha-only surfaces. We only get those
     * for bitmaps. And in that case, it's a differentiation between
     * foreground (white) and background (black).
     */
    if (mask_surface &&
        (cairo_surface_get_content (surface) & CAIRO_CONTENT_ALPHA))
    {
        cairo_push_group (cr);

        /* black background */
        cairo_set_source_rgb (cr, 0, 0, 0);
        cairo_paint (cr);
        /* mask with white foreground */
        cairo_set_source_rgb (cr, 1, 1, 1);
        cairo_mask_surface (cr, surface, 0, 0);

        cairo_pop_group_to_source (cr);
    }
    else
    {
        cairo_set_source_surface (cr, surface, 0, 0);
    }

    if (mask_surface)
    {
        cairo_mask_surface (cr, mask_surface, 0, 0);
        cairo_surface_destroy (mask_surface);
    }
    else
    {
        cairo_paint (cr);
    }

    cairo_surface_destroy (surface);
    cairo_destroy (cr);
    if (myDisplayErrorTrapPop (screen_info->display_info) != Success)
    {
        cairo_surface_destroy (image);
        return NULL;
    }

    unscaled = gdk_pixbuf_get_from_surface (image, 0, 0, w, h);
    cairo_surface_destroy (image);

    if (unscaled)
    {
        downsize_ratio (&w, &h, width, height);
        icon = gdk_pixbuf_scale_simple (unscaled, w, h, GDK_INTERP_BILINEAR);
        g_object_unref (G_OBJECT (unscaled));
        return icon;
    }

    return NULL;
}

static void
free_pixels (guchar * pixels, gpointer data)
{
    g_free (pixels);
}

static GdkPixbuf *
scaled_from_pixdata (guchar * pixdata, guint w, guint h, guint dest_w, guint dest_h)
{
    GdkPixbuf *src;
    GdkPixbuf *dest;

    src = gdk_pixbuf_new_from_data (pixdata, GDK_COLORSPACE_RGB, TRUE, 8, w, h, w * 4, free_pixels, NULL);

    if (G_UNLIKELY (src == NULL))
    {
        return NULL;
    }

    if (w != dest_w || h != dest_h)
    {
        downsize_ratio (&w, &h, dest_w, dest_h);
        dest = gdk_pixbuf_scale_simple (src, w, h, GDK_INTERP_BILINEAR);
        g_object_unref (G_OBJECT (src));
    }
    else
    {
        dest = src;
    }

    return dest;
}

GdkPixbuf *
getAppIcon (Client *c, guint width, guint height)
{
    ScreenInfo *screen_info;
    XWMHints *hints;
    Pixmap pixmap;
    Pixmap mask;
    guchar *pixdata;
    guint w, h;

    g_return_val_if_fail (c != NULL, NULL);

    pixdata = NULL;
    pixmap = None;
    mask = None;

    screen_info = c->screen_info;
    if (read_rgb_icon (screen_info->display_info, c->window, width, height, &w, &h, &pixdata))
    {
        return scaled_from_pixdata (pixdata, w, h, width, height);
    }

    myDisplayErrorTrapPush (screen_info->display_info);
    hints = XGetWMHints (myScreenGetXDisplay(screen_info), c->window);
    myDisplayErrorTrapPopIgnored (screen_info->display_info);

    if (hints)
    {
        if (hints->flags & IconPixmapHint)
        {
            pixmap = hints->icon_pixmap;
        }
        if (hints->flags & IconMaskHint)
        {
            mask = hints->icon_mask;
        }

        XFree (hints);
        hints = NULL;
    }

    if (pixmap != None)
    {
        GdkPixbuf *icon = try_pixmap_and_mask (screen_info, pixmap, mask, width, height);
        if (icon)
        {
            return icon;
        }
    }

    getKDEIcon (screen_info->display_info, c->window, &pixmap, &mask);
    if (pixmap != None)
    {
        GdkPixbuf *icon = try_pixmap_and_mask (screen_info, pixmap, mask, width, height);
        if (icon)
        {
            return icon;
        }
    }

    if (c->class.res_name != NULL)
    {
        GdkPixbuf *icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                    c->class.res_name,
                                                    MIN (width, height),
                                                    0, NULL);
        if (icon)
        {
            return icon;
        }
    }

    return default_icon_at_size (screen_info->gscr, width, height);
}

GdkPixbuf *
getClientIcon (Client *c, guint width, guint height)
{
    ScreenInfo *screen_info;
    GdkPixbuf *app_content;
    GdkPixbuf *small_icon;
    GdkPixbuf *icon_pixbuf;
    GdkPixbuf *icon_pixbuf_stated;
    guint small_icon_size;
    guint app_icon_width, app_icon_height;
    Pixmap pixmap;

    g_return_val_if_fail (c != NULL, NULL);

    screen_info = c->screen_info;
    icon_pixbuf = NULL;
    app_icon_width = width; /* Set to 0 to use gdk pixbuf scaling */
    app_icon_height = height; /* Set to 0 to use gdk pixbuf scaling */

    icon_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
    gdk_pixbuf_fill (icon_pixbuf, 0x00);

    pixmap = compositorGetWindowPixmapAtSize (screen_info, c->frame, &app_icon_width, &app_icon_height);
    if (pixmap != None)
    {
        app_content = try_pixmap_and_mask (screen_info, pixmap, None, width, height);
        XFreePixmap (myScreenGetXDisplay (screen_info), pixmap);
    }
    else
    {
        app_content = default_icon_at_size (screen_info->gscr, width, height);
    }

    app_icon_width = (guint) gdk_pixbuf_get_width (app_content);
    app_icon_height = (guint) gdk_pixbuf_get_height (app_content);

    gdk_pixbuf_copy_area (app_content, 0, 0, app_icon_width, app_icon_height, icon_pixbuf,
                          (width - app_icon_width) / 2, (height - app_icon_height) / 2);
    g_object_unref (app_content);

    small_icon_size = MIN (width / 4, height / 4);
    small_icon_size = MIN (small_icon_size, 48);

    small_icon = getAppIcon (c, small_icon_size, small_icon_size);

    gdk_pixbuf_composite (small_icon, icon_pixbuf,
                          (width - small_icon_size) / 2, height - small_icon_size,
                          small_icon_size, small_icon_size,
                          (width - small_icon_size) / 2, height - small_icon_size,
                          1.0, 1.0,
                          GDK_INTERP_BILINEAR,
                          0xff);

    g_object_unref (small_icon);

    if (FLAG_TEST (c->flags, CLIENT_FLAG_ICONIFIED))
    {
        icon_pixbuf_stated = gdk_pixbuf_copy (icon_pixbuf);
        gdk_pixbuf_saturate_and_pixelate (icon_pixbuf, icon_pixbuf_stated, 0.55, TRUE);
        g_object_unref (icon_pixbuf);
        icon_pixbuf = icon_pixbuf_stated;
    }

    return icon_pixbuf;
}
