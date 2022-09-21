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

#ifndef INC_FRAME_H
#define INC_FRAME_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "screen.h"
#include "mypixmap.h"
#include "client.h"

int                      frameDecorationLeft                    (ScreenInfo *);
int                      frameDecorationRight                   (ScreenInfo *);
int                      frameDecorationTop                     (ScreenInfo *);
int                      frameDecorationBottom                  (ScreenInfo *);
int                      frameLeft                              (Client *);
int                      frameRight                             (Client *);
int                      frameTop                               (Client *);
int                      frameBottom                            (Client *);
int                      frameX                                 (Client *);
int                      frameY                                 (Client *);
int                      frameWidth                             (Client *);
int                      frameHeight                            (Client *);
int                      frameExtentLeft                        (Client *);
int                      frameExtentRight                       (Client *);
int                      frameExtentTop                         (Client *);
int                      frameExtentBottom                      (Client *);
int                      frameExtentX                           (Client *);
int                      frameExtentY                           (Client *);
int                      frameExtentWidth                       (Client *);
int                      frameExtentHeight                      (Client *);
void                     frameSetShapeInput                     (Client *);
void                     frameClearQueueDraw                    (Client *);
void                     frameQueueDraw                         (Client *,
                                                                 gboolean);
void                     frameDraw                              (Client *,
                                                                 gboolean);

#endif /* INC_FRAME_H */
