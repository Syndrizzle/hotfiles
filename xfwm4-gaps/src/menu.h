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


        Metacity - (c) 2001 Havoc Pennington
        xfwm4    - (c) 2002-2011 Olivier Fourdan

 */

#ifndef INC_MENU_H
#define INC_MENU_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "event_filter.h"

typedef enum
{
    MENU_OP_SEPARATOR    = 0,
    MENU_OP_MAXIMIZE     = 1 << 0,
    MENU_OP_UNMAXIMIZE   = 1 << 1,
    MENU_OP_MINIMIZE     = 1 << 2,
    MENU_OP_MINIMIZE_ALL = 1 << 3,
    MENU_OP_UNMINIMIZE   = 1 << 4,
    MENU_OP_MOVE         = 1 << 5,
    MENU_OP_RESIZE       = 1 << 6,
    MENU_OP_SHADE        = 1 << 7,
    MENU_OP_UNSHADE      = 1 << 8,
    MENU_OP_STICK        = 1 << 9,
    MENU_OP_DELETE       = 1 << 10,
    MENU_OP_DESTROY      = 1 << 11,
    MENU_OP_WORKSPACES   = 1 << 12,
    MENU_OP_QUIT         = 1 << 13,
    MENU_OP_RESTART      = 1 << 14,
    MENU_OP_ABOVE        = 1 << 15,
    MENU_OP_BELOW        = 1 << 16,
    MENU_OP_NORMAL       = 1 << 17,
    MENU_OP_FULLSCREEN   = 1 << 18,
    MENU_OP_UNFULLSCREEN = 1 << 19,
    MENU_OP_CONTEXT_HELP = 1 << 20,
    MENU_OP_OTHER        = 1 << 21
}
MenuOp;

typedef enum
{
    MENU_TYPE_SEPARATOR = 0,
    MENU_TYPE_REGULAR,
    MENU_TYPE_RADIO,
    MENU_TYPE_CHECKBOX,
}
MenuType;

typedef struct _Menu Menu;
typedef struct _MenuItem MenuItem;
typedef struct _MenuData MenuData;

typedef void (*MenuFunc) (Menu * menu, MenuOp op, Window xid,
                          gpointer menu_data, gpointer item_data);

struct _MenuItem
{
    MenuOp op;
    MenuType type;
    const char *label;
};

struct _MenuData
{
    Menu *menu;
    MenuOp op;
    gpointer data;
};

struct _Menu
{
    GdkScreen *screen;
    GtkWidget *menu;
    eventFilterSetup *filter_setup;
    MenuFunc func;
    MenuOp ops;
    MenuOp insensitive;
    Window xid;
    gpointer data;
};

Menu                    *menu_default                           (GdkScreen *,
                                                                 Window,
                                                                 MenuOp,
                                                                 MenuOp,
                                                                 MenuFunc,
                                                                 gint,
                                                                 gint,
                                                                 gchar **,
                                                                 gint,
                                                                 eventFilterSetup*,
                                                                 gpointer);
GtkWidget               *menu_item_connect                      (GtkWidget *,
                                                                 MenuData *);
gboolean                 menu_is_opened                         (void);
gboolean                 menu_check_and_close                   (void);
gboolean                 menu_popup                             (Menu *,
                                                                 gint,
                                                                 gint,
                                                                 guint,
                                                                 guint32);
void                     menu_free                              (Menu *);

#endif /* INC_MENU_H */
