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


        device.h - (c) 2017 Viktor Odintsev

 */

#ifndef INC_DEVICE_H
#define INC_DEVICE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>
#include <gdk/gdk.h>

typedef enum
{
    XFWM_EVENT_XEVENT,
    XFWM_EVENT_KEY,
    XFWM_EVENT_BUTTON,
    XFWM_EVENT_MOTION,
    XFWM_EVENT_CROSSING
} XfwmEventType;

typedef struct
{
    XfwmEventType type;
    Window window;
    gint device;
    XEvent *xevent;
} XfwmEventMeta;

typedef struct
{
    XfwmEventMeta meta;

    Window root;
    gboolean pressed;
    guint keycode;
    guint state;
    Time time;
} XfwmEventKey;

typedef struct
{
    XfwmEventMeta meta;

    Window root;
    Window subwindow;
    gboolean pressed;
    guint button;
    guint state;
    gint x;
    gint y;
    gint x_root;
    gint y_root;
    Time time;
} XfwmEventButton;

typedef struct
{
    XfwmEventMeta meta;

    gint x;
    gint y;
    gint x_root;
    gint y_root;
    Time time;
} XfwmEventMotion;

typedef struct
{
    XfwmEventMeta meta;

    Window root;
    gboolean enter;
    gint mode;
    gint detail;
    gint x_root;
    gint y_root;
    Time time;
} XfwmEventCrossing;

typedef union
{
    XfwmEventMeta meta;
    XfwmEventKey key;
    XfwmEventButton button;
    XfwmEventMotion motion;
    XfwmEventCrossing crossing;
} XfwmEvent;

typedef struct
{
    gboolean keyboard;
    gint xi2_device;
} XfwmDevice;

typedef struct {
    XfwmDevice pointer;
    XfwmDevice keyboard;

    gboolean xi2_available;
    gint xi2_opcode;
} XfwmDevices;

XfwmEvent               *xfwm_device_translate_event            (XfwmDevices *,
                                                                 XEvent *,
                                                                 XfwmEvent *);
void                     xfwm_device_free_event                 (XfwmEvent *);
void                     xfwm_device_button_update_window       (XfwmEventButton *,
                                                                 Window);
#ifdef HAVE_XI2
void                     xfwm_device_configure_xi2_event_mask   (XfwmDevices *,
                                                                 Display *,
                                                                 Window,
                                                                 gulong);
#endif
gboolean                 xfwm_device_grab                       (XfwmDevices *,
                                                                 XfwmDevice *,
                                                                 Display *,
                                                                 Window,
                                                                 gboolean,
                                                                 guint,
                                                                 gint,
                                                                 Window,
                                                                 Cursor,
                                                                 Time);
void                     xfwm_device_ungrab                     (XfwmDevices *,
                                                                 XfwmDevice *,
                                                                 Display *,
                                                                 Time);
gboolean                 xfwm_device_grab_button                (XfwmDevices *,
                                                                 Display *,
                                                                 guint,
                                                                 guint,
                                                                 Window,
                                                                 gboolean,
                                                                 guint,
                                                                 gint,
                                                                 gint,
                                                                 Window,
                                                                 Cursor);
void                     xfwm_device_ungrab_button              (XfwmDevices *,
                                                                 Display *,
                                                                 guint,
                                                                 guint,
                                                                 Window);
gboolean                 xfwm_device_grab_keycode               (XfwmDevices *,
                                                                 Display *,
                                                                 gint,
                                                                 guint,
                                                                 Window,
                                                                 gboolean,
                                                                 guint,
                                                                 gint,
                                                                 gint);
void                     xfwm_device_ungrab_keycode             (XfwmDevices *,
                                                                 Display *,
                                                                 gint,
                                                                 guint,
                                                                 Window);
gboolean                 xfwm_device_check_mask_event           (XfwmDevices *,
                                                                 Display *,
                                                                 guint,
                                                                 XfwmEvent *);
XfwmDevices             *xfwm_devices_new                       (GdkDisplay *);

#endif /* INC_DEVICE_H */
