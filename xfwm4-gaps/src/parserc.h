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

#ifndef INC_PARSERC_H
#define INC_PARSERC_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include "settings.h"

gboolean                 parseRc                                (const gchar *,
                                                                 const gchar *,
                                                                 Settings *);
gboolean                 checkRc                                (Settings *);
GValue                  *getGValue                              (const gchar *,
                                                                 Settings *);
const gchar             *getStringValue                         (const gchar *,
                                                                 Settings *);
gint                     getIntValue                            (const gchar *,
                                                                 Settings *);
gboolean                 getBoolValue                           (const gchar *,
                                                                 Settings *);
gboolean                 setValue                               (const gchar *,
                                                                 const gchar *,
                                                                 Settings *);
gboolean                 setStringValue                         (const gchar *,
                                                                 const gchar *,
                                                                 Settings *);
gboolean                 setIntValue                            (const gchar *,
                                                                 gint,
                                                                 Settings *rc);
gboolean                 setBooleanValue                        (const gchar *,
                                                                 gboolean,
                                                                 Settings *rc);
gboolean                 setBooleanValueFromInt                 (const gchar *,
                                                                 int,
                                                                 Settings *);
gboolean                 setIntValueFromInt                     (const gchar *,
                                                                 int,
                                                                 Settings *);
gchar                   *getSystemThemeDir                      (void);
gchar                   *getThemeDir                            (const gchar *,
                                                                 const gchar *);
void                     freeRc                                 (Settings *);

#endif /* INC_PARSERC_H */
