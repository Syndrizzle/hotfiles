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


        Metacity - (c) 2003 Havoc Pennington
        xfwm4    - (c) 2002-2011 Olivier Fourdan

 */

#ifndef INC_STARTUP_NOTIFICATION_H
#define INC_STARTUP_NOTIFICATION_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN

#include <libsn/sn.h>

#include "screen.h"
#include "client.h"

void                     sn_client_startup_properties           (Client *);
void                     sn_init_display                        (ScreenInfo *);
void                     sn_close_display                       (void);
void                     sn_process_event                       (XEvent *);

#else /* HAVE_LIBSTARTUP_NOTIFICATION */

#define                  sn_client_startup_properties(c) ;
#define                  sn_init_display(d) ;
#define                  sn_close_display() ;
#define                  sn_process_event(e) ;

#endif /* HAVE_LIBSTARTUP_NOTIFICATION */

#endif /* INC_STARTUP_NOTIFICATION_H */
