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


#ifndef INC_SESSION_H
#define INC_SESSION_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include "client.h"
#include "display.h"
#include "screen.h"

/*
 *  Save window states to file which name is given in argument.
 */
gboolean                sessionSaveWindowStates                 (DisplayInfo *,
                                                                 const gchar *);

/*
 *  Load window states to file which name is given in argument.
 */
gboolean                sessionLoadWindowStates                 (const gchar *);

/*
 * Free allocated structure. Should be called before xfwm4 dies
 */
void                    sessionFreeWindowStates                 (void);

/*
 * Search for existing client in saved session and update
 * relevant client fields if found.
 */
gboolean                sessionMatchWinToSM                     (Client *);

/*
 * Initiate session, connect to session manager and
 * load saved states if the connection succeeds.
 */
int                     sessionStart                            (DisplayInfo *);

#endif /* INC_CLIENT_H */
