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

#ifndef INC_TABWIN_H
#define INC_TABWIN_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

typedef struct _Tabwin Tabwin;
typedef struct _TabwinWidget TabwinWidget;
typedef struct _TabwinWidgetClass TabwinWidgetClass;

typedef enum
{
    STANDARD_ICON_GRID,
    OVERFLOW_COLUMN_GRID,
}
CYCLE_TABWIN_MODE;

struct _Tabwin
{
    GList *tabwin_list;
    GList **client_list;
    GList *icon_list;
    GList *selected;
    gint monitor_width;
    gint monitor_height;
    gint client_count;
    gint grid_cols;
    gint grid_rows;
    gint icon_size;
    gint icon_scale;
    gint label_height;
    gboolean display_workspace;
};

struct _TabwinWidget
{
    GtkWindow __parent__;
    /* The below must be freed when destroying */
    GList *widgets;

    /* these don't have to be */
    Tabwin *tabwin;
    GtkWidget *label;
    GtkWidget *container;
    GtkWidget *selected;
    GtkWidget *hovered;

    gulong selected_callback;
    gint width;
    gint height;
    gint monitor_num;
};

struct _TabwinWidgetClass
{
    GtkWindowClass __parent__;
};

Tabwin                  *tabwinCreate                           (GList **,
                                                                 GList *,
                                                                 gboolean);
Client                  *tabwinGetSelected                      (Tabwin *);
Client                  *tabwinSelectHead                       (Tabwin *);
Client                  *tabwinSelectNext                       (Tabwin *);
Client                  *tabwinSelectPrev                       (Tabwin *);
Client                  *tabwinSelectDelta                      (Tabwin *, int, int);
Client                  *tabwinSelectHovered                    (Tabwin *);
Client                  *tabwinRemoveClient                     (Tabwin *,
                                                                 Client *);
void                    tabwinDestroy                           (Tabwin *);

#endif /* INC_TABWIN_H */
