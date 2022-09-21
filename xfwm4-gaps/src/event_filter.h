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

#ifndef INC_EVENT_FILTER_H
#define INC_EVENT_FILTER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <X11/Xlib.h>

#include "device.h"

/* this formatting is needed by glib-mkenums */
typedef enum {
    EVENT_FILTER_STOP     = 0x0,
    EVENT_FILTER_PASS     = 0x0,
    EVENT_FILTER_REMOVE   = 0x1,
    EVENT_FILTER_CONTINUE = 0x1,
}
eventFilterStatus;

typedef eventFilterStatus (*XfwmFilter) (XfwmEvent *event, gpointer data);

typedef struct eventFilterStack
{
    XfwmFilter filter;
    gpointer data;
    struct eventFilterStack *next;
}
eventFilterStack;

typedef struct eventFilterSetup
{
    eventFilterStack *filterstack;
    XfwmDevices *devices;
}
eventFilterSetup;

GdkWindow               *eventFilterAddWin                      (GdkScreen *,
                                                                 XfwmDevices *,
                                                                 long);
eventFilterStack        *eventFilterPush                        (eventFilterSetup *,
                                                                 XfwmFilter,
                                                                 gpointer );
eventFilterStack        *eventFilterPop                         (eventFilterSetup *);
eventFilterSetup        *eventFilterInit                        (XfwmDevices *,
                                                                 gpointer);
void                     eventFilterClose                       (eventFilterSetup *);

#endif /* INC_EVENT_FILTER_H */
