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

#include <glib.h>
#include <cairo/cairo.h>
#include "screen.h"

#ifdef HAVE_RENDER
#include <X11/extensions/Xrender.h>
#endif

#ifndef INC_MYPIXMAP_H
#define INC_MYPIXMAP_H

#define MYPIXMAP_XPIXMAP(p) (p.pixmap)

typedef struct
{
    gchar *name;
    const gchar *value;
}
xfwmColorSymbol;

struct _xfwmPixmap
{
    ScreenInfo *screen_info;
    Pixmap pixmap, mask;
#ifdef HAVE_RENDER
    XRenderPictFormat *pict_format;
    Picture pict;
#endif
    gint width, height;
};

gboolean                 xfwmPixmapRenderGdkPixbuf              (xfwmPixmap *,
                                                                 GdkPixbuf *);
gboolean                 xfwmPixmapLoad                         (ScreenInfo *,
                                                                 xfwmPixmap *,
                                                                 const gchar *,
                                                                 const gchar *,
                                                                 xfwmColorSymbol *);
void                     xfwmPixmapCreate                       (ScreenInfo *,
                                                                 xfwmPixmap *,
                                                                 gint,
                                                                 gint);
void                     xfwmPixmapInit                         (ScreenInfo *,
                                                                 xfwmPixmap *);
void                     xfwmPixmapFree                         (xfwmPixmap *);
gboolean                 xfwmPixmapNone                         (xfwmPixmap *);
void                     xfwmPixmapFill                         (xfwmPixmap *,
                                                                 xfwmPixmap *,
                                                                 gint,
                                                                 gint,
                                                                 gint,
                                                                 gint);
void                     xfwmPixmapDuplicate                    (xfwmPixmap *,
                                                                 xfwmPixmap *);
cairo_surface_t         *xfwmPixmapCreateSurface                (xfwmPixmap *,
                                                                 gboolean);
#endif /* INC_MYPIXMAP_H */
