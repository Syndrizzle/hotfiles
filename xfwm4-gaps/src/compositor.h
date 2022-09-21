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
        xfwm4    - (c) 2005-2011 Olivier Fourdan

 */

#ifndef INC_COMPOSITOR_H
#define INC_COMPOSITOR_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>

#include "display.h"
#include "screen.h"
#include "client.h"

gboolean                 compositorIsUsable                     (DisplayInfo *);
gboolean                 compositorIsActive                     (ScreenInfo *);
void                     compositorAddWindow                    (DisplayInfo *,
                                                                 Window,
                                                                 Client *);
gboolean                 compositorSetClient                    (DisplayInfo *,
                                                                 Window,
                                                                 Client *);
void                     compositorRemoveWindow                 (DisplayInfo *,
                                                                 Window);
void                     compositorDamageWindow                 (DisplayInfo *,
                                                                 Window);
void                     compositorResizeWindow                 (DisplayInfo *,
                                                                 Window,
                                                                 int,
                                                                 int,
                                                                 int,
                                                                 int);
Pixmap                   compositorGetWindowPixmapAtSize        (ScreenInfo *,
                                                                 Window,
                                                                 guint *,
                                                                 guint *);
void                     compositorHandleEvent                  (DisplayInfo *,
                                                                 XEvent *);
void                     compositorZoomIn                       (ScreenInfo *,
                                                                 XfwmEventButton *);
void                     compositorZoomOut                      (ScreenInfo *,
                                                                 XfwmEventButton *);
void                     compositorInitDisplay                  (DisplayInfo *);
void                     compositorSetCompositeMode             (DisplayInfo *,
                                                                 gboolean);

gboolean                 compositorManageScreen                 (ScreenInfo *);
void                     compositorUnmanageScreen               (ScreenInfo *);
void                     compositorAddAllWindows                (ScreenInfo *);
gboolean                 compositorActivateScreen               (ScreenInfo *,
                                                                 gboolean);
void                     compositorUpdateScreenSize             (ScreenInfo *);

void                     compositorWindowSetOpacity             (DisplayInfo *,
                                                                 Window,
                                                                 guint32);
void                     compositorRebuildScreen                (ScreenInfo *);
gboolean                 compositorTestServer                   (DisplayInfo *);

vblankMode               compositorParseVblankMode              (const gchar *);
void                     compositorSetVblankMode                (ScreenInfo *,
                                                                 vblankMode);


#endif /* INC_COMPOSITOR_H */
