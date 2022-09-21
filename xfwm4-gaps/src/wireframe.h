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
        xfwm4    - (c) 2002-2011 Olivier Fourdan

 */

#ifndef INC_WIREFRAME_H
#define INC_WIREFRAME_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <X11/Xlib.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include "screen.h"
#include "client.h"

typedef struct _WireFrame WireFrame;
struct _WireFrame
{
    ScreenInfo *screen_info;
    Window xwindow;
    gboolean mapped;
    int x;
    int y;
    int width;
    int height;
    Colormap xcolormap;
    cairo_surface_t *surface;
    cairo_t *cr;
    gdouble red;
    gdouble green;
    gdouble blue;
    gdouble alpha;
};

void                     wireframeUpdate                        (Client *,
                                                                 WireFrame *);
WireFrame *              wireframeCreate                        (Client *);
void                     wireframeDelete                        (WireFrame *);

#endif /* INC_WIREFRAME_H */
