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

#ifndef INC_MISC_H
#define INC_MISC_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/X.h>
#include <X11/Xlib.h>
#include <glib.h>
#include "screen.h"

#if defined (__GNUC__) && __GNUC__ >= 7
#define FALLTHROUGH __attribute__ ((fallthrough))
#else
#define FALLTHROUGH
#endif

/*
   Just for completion, being a bit pedantic, X seems to be missing
   those so far, so create them for now.
 */

#ifndef Button6
#define Button6 6
#endif

#ifndef Button7
#define Button7 7
#endif

#ifndef Button8
#define Button8 8
#endif

#ifndef Button9
#define Button9 9
#endif

guint                    getMouseXY                             (ScreenInfo *,
                                                                 gint *,
                                                                 gint *);
GC                       createGC                               (ScreenInfo *,
                                                                 char *,
                                                                 int,
                                                                 XFontStruct *,
                                                                 int,
                                                                 gboolean);
void                     sendClientMessage                      (ScreenInfo *,
                                                                 Window,
                                                                 int,
                                                                 guint32);
void                     sendRootMessage                        (ScreenInfo *,
                                                                 int,
                                                                 long,
                                                                 guint32);
gboolean                 checkWindowOnRoot                      (ScreenInfo *,
                                                                 Window);
void                     placeSidewalks                         (ScreenInfo *,
                                                                 gboolean);
gchar*                   get_atom_name                          (DisplayInfo *,
                                                                 Atom);

#endif /* INC_MISC_H */
