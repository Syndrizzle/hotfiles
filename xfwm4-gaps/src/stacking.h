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

#ifndef INC_STACKING_H
#define INC_STACKING_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include "screen.h"
#include "client.h"

void                     clientApplyStackList                   (ScreenInfo *);
Client                  *clientGetLowestTransient               (Client *);
Client                  *clientGetHighestTransientOrModalFor    (Client *);
gboolean                 clientIsTopMost                        (Client *);
Client                  *clientGetTopMostForGroup               (Client *);
Client                  *clientGetNextTopMost                   (ScreenInfo *,
                                                                 guint,
                                                                 Client *);
Client                  *clientGetBottomMost                    (ScreenInfo *,
                                                                 guint,
                                                                 Client *);
Client                  *clientAtPosition                       (ScreenInfo *,
                                                                 int,
                                                                 int,
                                                                 GList *);
void                     clientRaise                            (Client *,
                                                                 Window);
void                     clientLower                            (Client *,
                                                                 Window);
gboolean                 clientAdjustFullscreenLayer            (Client *,
                                                                 gboolean);
void                     clientAddToList                        (Client *);
void                     clientRemoveFromList                   (Client *);
GList                   *clientGetStackList                     (ScreenInfo *);
void                     clientSetLastRaise                     (Client *);
Client                  *clientGetLastRaise                     (ScreenInfo *);
void                     clientClearLastRaise                   (ScreenInfo *);
void                     clientClearDelayedRaise                (void);
void                     clientResetDelayedRaise                (ScreenInfo *);

#endif /* INC_STACKING_H */
