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
          based on a patch from Joshua Blanton <jblanton@irg.cs.ohiou.edu>
 */

#ifndef INC_POSWIN_H
#define INC_POSWIN_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "client.h"

typedef struct _Poswin Poswin;
typedef struct _PoswinWidgetClass PoswinWidgetClass;

struct _Poswin
{
    GtkWindow __parent__;
    GtkWidget *label;
};

struct _PoswinWidgetClass
{
    GtkWindowClass __parent__;
};

Poswin                  *poswinCreate                           (GdkScreen *);
void                     poswinSetPosition                      (Poswin *,
                                                                 Client *);
void                     poswinDestroy                          (Poswin *);
void                     poswinShow                             (Poswin *);
void                     poswinHide                             (Poswin *);

#endif /* INC_POSWIN_H */
