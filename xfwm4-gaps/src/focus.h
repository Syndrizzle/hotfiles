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

#ifndef INC_FOCUS_H
#define INC_FOCUS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/X.h>
#include <X11/Xlib.h>
#include <glib.h>

#include "screen.h"
#include "client.h"

#define NO_FOCUS_FLAG                   0
#define FOCUS_SORT                      (1<<0)
#define FOCUS_IGNORE_MODAL              (1<<1)
#define FOCUS_FORCE                     (1<<2)
#define FOCUS_TRANSITION                (1<<3)

void                     clientFocusTop                         (ScreenInfo *,
                                                                 guint,
                                                                 guint32);
gboolean                 clientFocusNew                         (Client *);
gboolean                 clientSelectMask                       (Client *,
                                                                 Client *,
                                                                 guint,
                                                                 guint);
Client                  *clientGetNext                          (Client *,
                                                                 guint,
                                                                 guint);
Client                  *clientGetPrevious                      (Client *,
                                                                 guint,
                                                                 guint);
void                     clientPassFocus                        (ScreenInfo *,
                                                                 Client *,
                                                                 GList *);
gboolean                 clientAcceptFocus                      (Client *);
void                     clientSortRing                         (Client *);
void                     clientSetLast                          (Client *);
void                     clientUpdateFocus                      (ScreenInfo *,
                                                                 Client *,
                                                                 unsigned short);
void                     clientSetFocus                         (ScreenInfo *,
                                                                 Client *,
                                                                 guint32,
                                                                 unsigned short);
void                     clientInitFocusFlag                    (Client *);
Client                  *clientGetFocus                         (void);
Client                  *clientGetFocusPending                  (void);
Client                  *clientGetFocusOrPending                (void);
Client                  *clientGetUserFocus                     (void);
void                     clientClearFocus                       (Client *);
void                     clientGrabMouseButton                  (Client *);
void                     clientUngrabMouseButton                (Client *);
void                     clientGrabMouseButtonForAll            (ScreenInfo *);
void                     clientUngrabMouseButtonForAll          (ScreenInfo *);
void                     clientClearDelayedFocus                (void);
void                     clientAddDelayedFocus                  (Client *,
                                                                 guint32);
Client                  *clientGetDelayedFocus                  (void);

#endif /* INC_FOCUS_H */
