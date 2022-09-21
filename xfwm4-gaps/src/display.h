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


        xfwm4    - (c) 2002-2020 Olivier Fourdan

 */

#ifndef INC_DISPLAY_H
#define INC_DISPLAY_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/extensions/shape.h>

#ifndef ShapeInput
#define ShapeInput 2
#endif

#ifdef HAVE_RANDR
#include <X11/extensions/Xrandr.h>
#endif /* HAVE_RANDR */

#ifdef HAVE_XSYNC
#include <X11/extensions/sync.h>
#endif /* HAVE_XSYNC */

#ifdef HAVE_COMPOSITOR
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>
#if COMPOSITE_MAJOR > 0 || COMPOSITE_MINOR >= 2
#ifndef HAVE_NAME_WINDOW_PIXMAP
#define HAVE_NAME_WINDOW_PIXMAP 1
#endif /* HAVE_NAME_WINDOW_PIXMAP */
#endif /* COMPOSITE_MAJOR > 0 || COMPOSITE_MINOR >= 2 */
#if COMPOSITE_MAJOR > 0 || COMPOSITE_MINOR >= 3
#ifndef HAVE_OVERLAYS
#define HAVE_OVERLAYS 1
#endif /* HAVE_OVERLAYS */
#endif /* COMPOSITE_MAJOR > 0 || COMPOSITE_MINOR >= 3 */
#endif /* HAVE_COMPOSITOR */

#include <gtk/gtk.h>
#include <glib.h>
#include <libxfce4ui/libxfce4ui.h>

#include "event_filter.h"

/*
 * The following macro is taken straight from metacity,
 * if that needs some explanation, please refer to metacity's
 * display.h source where it is explaned
 */
#define TIMESTAMP_IS_BEFORE_REAL(time1, time2)  (((time1 < time2) && (time2 - time1 < (G_MAXUINT32 >> 1))) || \
                                                 ((time1 > time2) && (time1 - time2 > (G_MAXUINT32 >> 1))))
#define TIMESTAMP_IS_BEFORE(time1, time2)       ((time1 == 0) ||                                                    \
                                                (TIMESTAMP_IS_BEFORE_REAL(time1, time2) &&                          \
                                                (time2 != 0)))

enum
{
    SEARCH_WINDOW         = (1 << 0),
    SEARCH_FRAME          = (1 << 1),
    SEARCH_BUTTON         = (1 << 2),
    SEARCH_WIN_USER_TIME  = (1 << 3)
};

enum
{
    TITLE_1 = 0,
    TITLE_2,
    TITLE_3,
    TITLE_4,
    TITLE_5,
    TITLE_COUNT
};

enum
{
    CORNER_BOTTOM_LEFT = 0,
    CORNER_BOTTOM_RIGHT,
    CORNER_TOP_LEFT,
    CORNER_TOP_RIGHT,
    CORNER_COUNT
};

enum
{
    SIDE_LEFT = 0,
    SIDE_RIGHT,
    SIDE_TOP,
    SIDE_BOTTOM,
    SIDE_COUNT
};
#define NO_HANDLE -1
#define HANDLES_COUNT (CORNER_COUNT + SIDE_COUNT - 1)

enum
{
    MENU_BUTTON = 0,
    STICK_BUTTON,
    SHADE_BUTTON,
    HIDE_BUTTON,
    MAXIMIZE_BUTTON,
    CLOSE_BUTTON,
    TITLE_SEPARATOR,
    BUTTON_STRING_COUNT
};
#define BUTTON_COUNT (BUTTON_STRING_COUNT - 1)

enum
{
    ACTIVE = 0,
    INACTIVE,
    PRELIGHT,
    PRESSED,
    T_ACTIVE,
    T_INACTIVE,
    T_PRELIGHT,
    T_PRESSED,
    STATE_COUNT
};
#define STATE_TOGGLED (STATE_COUNT / 2)

enum
{
    BUTTON_STATE_NORMAL = 0,
    BUTTON_STATE_PRELIGHT,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_COUNT
};

enum
{
    NET_WM_MOVERESIZE_SIZE_TOPLEFT = 0,
    NET_WM_MOVERESIZE_SIZE_TOP,
    NET_WM_MOVERESIZE_SIZE_TOPRIGHT,
    NET_WM_MOVERESIZE_SIZE_RIGHT,
    NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT,
    NET_WM_MOVERESIZE_SIZE_BOTTOM,
    NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT,
    NET_WM_MOVERESIZE_SIZE_LEFT,
    NET_WM_MOVERESIZE_MOVE,
    NET_WM_MOVERESIZE_SIZE_KEYBOARD,
    NET_WM_MOVERESIZE_MOVE_KEYBOARD,
    NET_WM_MOVERESIZE_CANCEL
};

enum
{
    COMPOSITING_MANAGER = 0,
    GTK_FRAME_EXTENTS,
    GTK_HIDE_TITLEBAR_WHEN_MAXIMIZED,
    GTK_SHOW_WINDOW_MENU,
    KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR,
    KWM_WIN_ICON,
    MOTIF_WM_HINTS,
    MOTIF_WM_INFO,
    NET_ACTIVE_WINDOW,
    NET_CLIENT_LIST,
    NET_CLIENT_LIST_STACKING,
    NET_CLOSE_WINDOW,
    NET_CURRENT_DESKTOP,
    NET_DESKTOP_GEOMETRY,
    NET_DESKTOP_LAYOUT,
    NET_DESKTOP_NAMES,
    NET_DESKTOP_VIEWPORT,
    NET_FRAME_EXTENTS,
    NET_MOVERESIZE_WINDOW,
    NET_NUMBER_OF_DESKTOPS,
    NET_REQUEST_FRAME_EXTENTS,
    NET_SHOWING_DESKTOP,
    NET_STARTUP_ID,
    NET_SUPPORTED,
    NET_SUPPORTING_WM_CHECK,
    NET_SYSTEM_TRAY_OPCODE,
    NET_WM_ACTION_ABOVE,
    NET_WM_ACTION_BELOW,
    NET_WM_ACTION_CHANGE_DESKTOP,
    NET_WM_ACTION_CLOSE,
    NET_WM_ACTION_FULLSCREEN,
    NET_WM_ACTION_MAXIMIZE_HORZ,
    NET_WM_ACTION_MAXIMIZE_VERT,
    NET_WM_ACTION_MINIMIZE,
    NET_WM_ACTION_MOVE,
    NET_WM_ACTION_RESIZE,
    NET_WM_ACTION_SHADE,
    NET_WM_ACTION_STICK,
    NET_WM_ALLOWED_ACTIONS,
    NET_WM_BYPASS_COMPOSITOR,
    NET_WM_CONTEXT_HELP,
    NET_WM_DESKTOP,
    NET_WM_FULLSCREEN_MONITORS,
    NET_WM_ICON,
    NET_WM_ICON_GEOMETRY,
    NET_WM_ICON_NAME,
    NET_WM_MOVERESIZE,
    NET_WM_NAME,
    NET_WM_OPAQUE_REGION,
    NET_WM_PID,
    NET_WM_PING,
    NET_WM_WINDOW_OPACITY,
    NET_WM_WINDOW_OPACITY_LOCKED,
    NET_WM_STATE,
    NET_WM_STATE_ABOVE,
    NET_WM_STATE_BELOW,
    NET_WM_STATE_DEMANDS_ATTENTION,
    NET_WM_STATE_FOCUSED,
    NET_WM_STATE_FULLSCREEN,
    NET_WM_STATE_HIDDEN,
    NET_WM_STATE_MAXIMIZED_HORZ,
    NET_WM_STATE_MAXIMIZED_VERT,
    NET_WM_STATE_MODAL,
    NET_WM_STATE_SHADED,
    NET_WM_STATE_SKIP_PAGER,
    NET_WM_STATE_SKIP_TASKBAR,
    NET_WM_STATE_STICKY,
    NET_WM_STRUT,
    NET_WM_STRUT_PARTIAL,
    NET_WM_SYNC_REQUEST,
    NET_WM_SYNC_REQUEST_COUNTER,
    NET_WM_USER_TIME,
    NET_WM_USER_TIME_WINDOW,
    NET_WM_WINDOW_TYPE,
    NET_WM_WINDOW_TYPE_DESKTOP,
    NET_WM_WINDOW_TYPE_DIALOG,
    NET_WM_WINDOW_TYPE_DOCK,
    NET_WM_WINDOW_TYPE_MENU,
    NET_WM_WINDOW_TYPE_NORMAL,
    NET_WM_WINDOW_TYPE_SPLASH,
    NET_WM_WINDOW_TYPE_TOOLBAR,
    NET_WM_WINDOW_TYPE_UTILITY,
    NET_WM_WINDOW_TYPE_NOTIFICATION,
    NET_WORKAREA,
    MANAGER,
    PIXMAP,
    SM_CLIENT_ID,
    UTF8_STRING,
    WM_CHANGE_STATE,
    WM_CLIENT_LEADER,
    WM_CLIENT_MACHINE,
    WM_COLORMAP_WINDOWS,
    WM_DELETE_WINDOW,
    WM_HINTS,
    WM_PROTOCOLS,
    WM_STATE,
    WM_TAKE_FOCUS,
    WM_TRANSIENT_FOR,
    WM_WINDOW_ROLE,
    XFWM4_COMPOSITING_MANAGER,
    XFWM4_TIMESTAMP_PROP,
    XROOTPMAP,
    XSETROOT,
    GTK_READ_RCFILES,
    ATOM_COUNT
};

typedef struct _Client            Client;
typedef struct _DisplayInfo       DisplayInfo;
typedef struct _xfwmPixmap        xfwmPixmap;
typedef struct _XfwmParams        XfwmParams;
typedef struct _ScreenInfo        ScreenInfo;
typedef struct _Settings          Settings;

struct _DisplayInfo
{
    GdkDisplay *gdisplay;
    Display *dpy;

    XfceSMClient *session;
    gboolean quit;
    gboolean reload;

    Window timestamp_win;
    Cursor busy_cursor;
    Cursor move_cursor;
    Cursor root_cursor;
    Cursor resize_cursor[SIDE_COUNT + CORNER_COUNT];

    Atom atoms[ATOM_COUNT];

    eventFilterSetup *xfilter;
    XfwmDevices *devices;
    GSList *screens;
    GSList *clients;

    gboolean have_shape;
    gboolean have_render;
    gboolean have_xrandr;
    gboolean have_xsync;
    gboolean have_xres;
    gint shape_version;
    gint shape_event_base;
    gint double_click_time;
    gint double_click_distance;
    gint xgrabcount;
    gint nb_screens;
    gchar* hostname;

    guint32 current_time;
    guint32 last_user_time;

    gboolean enable_compositor;

#ifdef HAVE_RENDER
    gint render_error_base;
    gint render_event_base;
#endif /* HAVE_RENDER */
#ifdef HAVE_RANDR
    gint xrandr_error_base;
    gint xrandr_event_base;
#endif /* HAVE_RANDR */
#ifdef HAVE_XSYNC
    gint xsync_event_base;
    gint xsync_error_base;
#endif /* HAVE_XSYNC */
#ifdef HAVE_XRES
    gint xres_event_base;
    gint xres_error_base;
#endif /* HAVE_XRES */
#ifdef HAVE_COMPOSITOR
    gint composite_error_base;
    gint composite_event_base;
    gint damage_error_base;
    gint damage_event_base;
    gint fixes_error_base;
    gint fixes_event_base;

    gboolean have_composite;
    gboolean have_damage;
    gboolean have_fixes;

#if HAVE_NAME_WINDOW_PIXMAP
    gboolean have_name_window_pixmap;
#endif /* HAVE_NAME_WINDOW_PIXMAP */

#if HAVE_OVERLAYS
    gboolean have_overlays;
#endif /* HAVE_OVERLAYS */

#ifdef HAVE_PRESENT_EXTENSION
    gboolean have_present;
    gint present_opcode;
    gint present_error_base;
    gint present_event_base;
#endif /* HAVE_PRESENT_EXTENSION */

#endif /* HAVE_COMPOSITOR */
};

DisplayInfo             *myDisplayInit                          (GdkDisplay *);
DisplayInfo             *myDisplayClose                         (DisplayInfo *);
DisplayInfo             *myDisplayGetDefault                    (void);
gboolean                 myDisplayHaveShape                     (DisplayInfo *);
gboolean                 myDisplayHaveShapeInput                (DisplayInfo *);
gboolean                 myDisplayHaveRender                    (DisplayInfo *);
void                     myDisplayCreateCursor                  (DisplayInfo *);
void                     myDisplayFreeCursor                    (DisplayInfo *);
Cursor                   myDisplayGetCursorBusy                 (DisplayInfo *);
Cursor                   myDisplayGetCursorMove                 (DisplayInfo *);
Cursor                   myDisplayGetCursorRoot                 (DisplayInfo *);
Cursor                   myDisplayGetCursorResize               (DisplayInfo *,
                                                                 guint);
void                     myDisplayGrabServer                    (DisplayInfo *);
void                     myDisplayUngrabServer                  (DisplayInfo *);
void                     myDisplayAddClient                     (DisplayInfo *,
                                                                 Client *);
void                     myDisplayRemoveClient                  (DisplayInfo *,
                                                                 Client *);
Client                  *myDisplayGetClientFromWindow           (DisplayInfo *,
                                                                 Window,
                                                                 unsigned short);
void                     myDisplayAddScreen                     (DisplayInfo *,
                                                                 ScreenInfo *);
void                     myDisplayRemoveScreen                  (DisplayInfo *,
                                                                 ScreenInfo *);
ScreenInfo              *myDisplayGetScreenFromRoot             (DisplayInfo *,
                                                                 Window);
ScreenInfo              *myDisplayGetScreenFromOutput           (DisplayInfo *,
                                                                 Window);
ScreenInfo              *myDisplayGetScreenFromNum              (DisplayInfo *,
                                                                 int);
Window                   myDisplayGetRootFromWindow             (DisplayInfo *,
                                                                 Window w);
ScreenInfo              *myDisplayGetScreenFromWindow           (DisplayInfo *,
                                                                 Window w);
#ifdef ENABLE_KDE_SYSTRAY_PROXY
ScreenInfo              *myDisplayGetScreenFromSystray          (DisplayInfo *,
                                                                 Window);
#endif /* ENABLE_KDE_SYSTRAY_PROXY */
#ifdef HAVE_XSYNC
Client                  *myDisplayGetClientFromXSyncAlarm       (DisplayInfo *,
                                                                 XSyncAlarm);
#endif /* HAVE_XSYNC */
ScreenInfo              *myDisplayGetDefaultScreen              (DisplayInfo *);
guint32                  myDisplayUpdateCurrentTime             (DisplayInfo *,
                                                                 XfwmEvent *);
guint32                  myDisplayGetCurrentTime                (DisplayInfo *);
guint32                  myDisplayGetTime                       (DisplayInfo *,
                                                                 guint32);
guint32                  myDisplayGetLastUserTime               (DisplayInfo *);
void                     myDisplaySetLastUserTime               (DisplayInfo *,
                                                                 guint32);
void                     myDisplayUpdateLastUserTime            (DisplayInfo *,
                                                                 guint32);
gboolean                 myDisplayTestXrender                   (DisplayInfo *,
                                                                 gdouble);
void                     myDisplayErrorTrapPush                 (DisplayInfo *);
gint                     myDisplayErrorTrapPop                  (DisplayInfo *);
void                     myDisplayErrorTrapPopIgnored           (DisplayInfo *);
void                     myDisplayBeep                          (DisplayInfo *);
GdkKeymap               *myDisplayGetKeymap                     (DisplayInfo *);
#endif /* INC_DISPLAY_H */
