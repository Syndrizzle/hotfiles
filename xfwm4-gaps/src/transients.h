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

#ifndef INC_TRANSIENTS_H
#define INC_TRANSIENTS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include "client.h"

Client                  *clientGetTransient                     (Client *);
gboolean                 clientIsDirectTransient                (Client *);
gboolean                 clientIsTransientForGroup              (Client *);
gboolean                 clientIsTransient                      (Client *);
gboolean                 clientIsModal                          (Client *);
gboolean                 clientIsModalForGroup                  (Client *);
gboolean                 clientIsTransientOrModal               (Client *);
gboolean                 clientIsValidTransientOrModal          (Client *);
gboolean                 clientSameGroup                        (Client *,
                                                                 Client *);
gboolean                 clientSameLeader                       (Client *,
                                                                 Client *);
gboolean                 clientSameName                         (Client *,
                                                                 Client *);
gboolean                 clientSameApplication                  (Client *,
                                                                 Client *);
gboolean                 clientIsTransientFor                   (Client *,
                                                                 Client *);
gboolean                 clientIsModalFor                       (Client *,
                                                                 Client *);
gboolean                 clientIsTransientOrModalFor            (Client *,
                                                                 Client *);
gboolean                 clientIsTransientOrModalForGroup       (Client *);
gboolean                 clientTransientOrModalHasAncestor      (Client *,
                                                                 guint);
Client                  *clientGetModalFor                      (Client *);
Client                  *clientGetTransientFor                  (Client *);
GList                   *clientListTransient                    (Client *);
GList                   *clientListTransientOrModal             (Client *);
gboolean                 clientCheckTransientWindow             (Client *,
                                                                 Window);
#endif /* INC_TRANSIENTS_H */
