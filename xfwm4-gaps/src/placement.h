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

#ifndef INC_PLACEMENT_H
#define INC_PLACEMENT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include "client.h"

#define CLIENT_CONSTRAINED_TOP     1<<0
#define CLIENT_CONSTRAINED_BOTTOM  1<<1
#define CLIENT_CONSTRAINED_LEFT    1<<2
#define CLIENT_CONSTRAINED_RIGHT   1<<3

gboolean                 strutsToRectangles                     (Client *,
                                                                 GdkRectangle * /* left */,
                                                                 GdkRectangle * /* right */,
                                                                 GdkRectangle * /* top */,
                                                                 GdkRectangle * /* bottom */);
void                     clientMaxSpace                         (Client *,
                                                                 int *,
                                                                 int *,
                                                                 int *,
                                                                 int *);
gboolean                 clientsHaveOverlap                     (Client *,
                                                                 Client *);
unsigned int             clientConstrainPos                     (Client *,
                                                                 gboolean);
void                     clientInitPosition                     (Client *);
void                     clientFill                             (Client *,
                                                                 int);

#endif /* INC_PLACEMENT_H */
