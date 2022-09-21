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
        xfwm4    - (c) 2002-2015 Olivier Fourdan

 */

#ifndef INC_EVENTS_H
#define INC_EVENTS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/X.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

#include "screen.h"
#include "event_filter.h"

void                     initMenuEventWin                       (void);
eventFilterStatus        xfwm4_event_filter                     (XfwmEvent *,
                                                                 gpointer);
void                     initPerScreenCallbacks                 (ScreenInfo *);
void                     initPerDisplayCallbacks                (DisplayInfo *);
#endif /* INC_EVENTS_H */
