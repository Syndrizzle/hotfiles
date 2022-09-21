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


        xfwm4    - (c) 2002-2015 Olivier Fourdan

 */

#ifndef INC_NETWM_H
#define INC_NETWM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glib.h>
#include "screen.h"
#include "client.h"

void                     clientSetNetState                      (Client *);
void                     clientGetNetState                      (Client *);
void                     clientUpdateNetWmDesktop               (Client *,
                                                                 XClientMessageEvent *);
void                     clientUpdateNetState                   (Client *,
                                                                 XClientMessageEvent *);
void                     clientNetMoveResize                    (Client *,
                                                                 XClientMessageEvent *);
void                     clientNetMoveResizeWindow              (Client *,
                                                                 XClientMessageEvent *);
void                     clientUpdateFullscreenState            (Client *);
void                     clientGetNetWmType                     (Client *);
void                     clientGetInitialNetWmDesktop           (Client *);
void                     clientSetNetClientList                 (ScreenInfo *,
                                                                 Atom,
                                                                 GList *);
gboolean                 clientValidateNetStrut                 (Client *);
gboolean                 clientGetNetStruts                     (Client *);
void                     clientSetNetActions                    (Client *);
void                     clientWindowType                       (Client *);
void                     clientUpdateLayerState                 (Client *);
void                     clientSetNetActiveWindow               (ScreenInfo *,
                                                                 Client *,
                                                                 guint32);
void                     clientHandleNetActiveWindow            (Client *,
                                                                 guint32,
                                                                 gboolean);
void                     clientRemoveNetWMPing                  (Client *);
gboolean                 clientSendNetWMPing                    (Client *,
                                                                 guint32);
void                     clientReceiveNetWMPong                 (ScreenInfo *,
                                                                 guint32);
gboolean                 clientGetUserTime                      (Client *);
void                     clientAddUserTimeWin                   (Client *);
void                     clientRemoveUserTimeWin                (Client *);

#endif /* INC_NETWM_H */
