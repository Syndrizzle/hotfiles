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

#ifndef INC_WORKSPACES_H
#define INC_WORKSPACES_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#include <glib.h>

#include "screen.h"
#include "placement.h"
#include "client.h"

gboolean                workspaceMove                           (ScreenInfo *,
                                                                 int,
                                                                 int,
                                                                 Client *,
                                                                 guint32);
void                    workspaceSwitch                         (ScreenInfo *,
                                                                 int,
                                                                 Client *,
                                                                 gboolean,
                                                                 guint32);
void                    workspaceSetNames                       (ScreenInfo *,
                                                                 gchar **,
                                                                 int);
void                    workspaceSetCount                       (ScreenInfo *,
                                                                 guint);
void                    workspaceUpdateArea                     (ScreenInfo *);

void                    workspaceInsert                         (ScreenInfo *,
                                                                 guint);

void                    workspaceDelete                         (ScreenInfo *,
                                                                 guint);

#endif /* INC_WORKSPACES_H */
