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
        xfwm4    - (c) 2002-2014 Olivier Fourdan

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "client.h"

#ifdef HAVE_XSYNC
#include <X11/extensions/sync.h>
#endif /* HAVE_XSYNC */

#ifndef INC_XSYNC_H
#define INC_XSYNC_H

#ifdef HAVE_XSYNC
gboolean                 clientCreateXSyncAlarm                 (Client *);
void                     clientDestroyXSyncAlarm                (Client *);
gboolean                 clientGetXSyncCounter                  (Client *);
void                     clientXSyncClearTimeout                (Client *);
void                     clientXSyncRequest                     (Client *);
void                     clientXSyncUpdateValue                 (Client *,
                                                                 XSyncValue);

#endif /* HAVE_XSYNC */

#endif /* INC_XSYNC_H */
