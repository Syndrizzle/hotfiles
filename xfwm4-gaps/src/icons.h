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

#ifndef INC_ICONS_H
#define INC_ICONS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "screen.h"
#include "client.h"

GdkPixbuf               *getAppIcon                             (Client *,
                                                                 guint,
                                                                 guint);
GdkPixbuf               *getClientIcon                          (Client *,
                                                                 guint,
                                                                 guint);

#endif /* INC_ICONS_H */
