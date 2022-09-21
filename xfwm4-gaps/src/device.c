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


        device.c - (c) 2017 Viktor Odintsev

 */

#include "device.h"

#include <gdk/gdkx.h>
#ifdef HAVE_XI2
#include <X11/extensions/XInput2.h>
#endif

#include "display.h"

#ifdef HAVE_XI2
static const struct
{
    guint core_mask;
    guint xi2_event;
} core_to_xi2[] =
{
    { KeyPressMask, XI_KeyPress },
    { KeyReleaseMask, XI_KeyRelease },
    { ButtonPressMask, XI_ButtonPress },
    { ButtonReleaseMask, XI_ButtonRelease },
    { PointerMotionMask | ButtonMotionMask, XI_Motion },
    { EnterWindowMask, XI_Enter },
    { LeaveWindowMask, XI_Leave }
};
#endif

#define xfwm_device_fill_meta(evtype, evwindow, evdevice) \
{ \
    if (event == NULL) \
    { \
        event = g_new0 (XfwmEvent, 1); \
    } \
    event->meta.type = evtype; \
    event->meta.window = evwindow; \
    event->meta.device = evdevice; \
    event->meta.xevent = xevent; \
}

#ifdef HAVE_XI2
static guint
xfwm_device_obtain_state_xi2 (XIButtonState *buttons, XIModifierState *mods, XIGroupState *group)
{
    guint result;
    gint i, count;

    result = mods->effective | (group->effective << 13);
    count = MIN (3, buttons->mask_len / 8);
    for (i = 0; i < count; i++)
    {
        /* check first 3 buttons as GDK does */
        if (XIMaskIsSet (buttons->mask, i + 1))
        {
            result |= 1 << (8 + i);
        }
    }

    return result;
}
#endif

static XfwmEvent *
xfwm_device_translate_event_key_core (XEvent *xevent, XfwmEvent *event)
{
    xfwm_device_fill_meta (XFWM_EVENT_KEY, xevent->xany.window, None);

    event->key.root = xevent->xkey.root;
    event->key.pressed = xevent->type == KeyPress;
    event->key.keycode = xevent->xkey.keycode;
    event->key.state = xevent->xkey.state;
    event->key.time = xevent->xkey.time;

    return (XfwmEvent *)event;
}

#ifdef HAVE_XI2
static XfwmEvent *
xfwm_device_translate_event_key_xi2 (XEvent *xevent, XIDeviceEvent *xievent, XfwmEvent *event)
{
    xfwm_device_fill_meta (XFWM_EVENT_KEY, xievent->event, xievent->deviceid);

    event->key.root = xievent->root;
    event->key.pressed = xievent->evtype == XI_KeyPress;
    event->key.keycode = xievent->detail;
    event->key.state = xfwm_device_obtain_state_xi2 (&xievent->buttons,
                                                     &xievent->mods,
                                                     &xievent->group);
    event->key.time = xievent->time;

    return (XfwmEvent *)event;
}
#endif

static XfwmEvent *
xfwm_device_translate_event_button_core (XEvent *xevent, XfwmEvent *event)
{
    xfwm_device_fill_meta (XFWM_EVENT_BUTTON, xevent->xany.window, None);

    event->button.root = xevent->xbutton.root;
    event->button.subwindow = xevent->xbutton.subwindow;
    event->button.pressed = xevent->type == ButtonPress;
    event->button.button = xevent->xbutton.button;
    event->button.state = xevent->xbutton.state;
    event->button.x = xevent->xbutton.x;
    event->button.y = xevent->xbutton.y;
    event->button.x_root = xevent->xbutton.x_root;
    event->button.y_root = xevent->xbutton.y_root;
    event->button.time = xevent->xbutton.time;

    return (XfwmEvent *)event;
}

#ifdef HAVE_XI2
static XfwmEvent *
xfwm_device_translate_event_button_xi2 (XEvent *xevent, XIDeviceEvent *xievent, XfwmEvent *event)
{
    xfwm_device_fill_meta (XFWM_EVENT_BUTTON, xievent->event, xievent->deviceid);

    event->button.root = xievent->root;
    event->button.subwindow = xievent->child;
    event->button.pressed = xievent->evtype == XI_ButtonPress;
    event->button.button = xievent->detail;
    event->button.state = xfwm_device_obtain_state_xi2 (&xievent->buttons,
                                                        &xievent->mods,
                                                        &xievent->group);
    event->button.x = xievent->event_x;
    event->button.y = xievent->event_y;
    event->button.x_root = xievent->root_x;
    event->button.y_root = xievent->root_y;
    event->button.time = xievent->time;

    return (XfwmEvent *)event;
}
#endif

static XfwmEvent *
xfwm_device_translate_event_motion_core (XEvent *xevent, XfwmEvent *event)
{
    xfwm_device_fill_meta (XFWM_EVENT_MOTION, xevent->xany.window, None);

    event->motion.x = xevent->xbutton.x;
    event->motion.y = xevent->xbutton.y;
    event->motion.x_root = xevent->xbutton.x_root;
    event->motion.y_root = xevent->xbutton.y_root;
    event->motion.time = xevent->xbutton.time;

    return (XfwmEvent *)event;
}

#ifdef HAVE_XI2
static XfwmEvent *
xfwm_device_translate_event_motion_xi2 (XEvent *xevent, XIDeviceEvent *xievent, XfwmEvent *event)
{
    xfwm_device_fill_meta (XFWM_EVENT_MOTION, xievent->event, xievent->deviceid);

    event->motion.x = xievent->event_x;
    event->motion.y = xievent->event_y;
    event->motion.x_root = xievent->root_x;
    event->motion.y_root = xievent->root_y;
    event->motion.time = xievent->time;

    return (XfwmEvent *)event;
}
#endif

static XfwmEvent *
xfwm_device_translate_event_crossing_core (XEvent *xevent, XfwmEvent *event)
{
    xfwm_device_fill_meta (XFWM_EVENT_CROSSING, xevent->xany.window, None);

    event->crossing.root = xevent->xcrossing.root;
    event->crossing.enter = xevent->type == EnterNotify;
    event->crossing.mode = xevent->xcrossing.mode;
    event->crossing.detail = xevent->xcrossing.detail;
    event->crossing.x_root = xevent->xcrossing.x_root;
    event->crossing.y_root = xevent->xcrossing.y_root;
    event->crossing.time = xevent->xcrossing.time;

    return (XfwmEvent *)event;
}

#ifdef HAVE_XI2
static XfwmEvent *
xfwm_device_translate_event_crossing_xi2 (XEvent *xevent, XIEnterEvent *xievent, XfwmEvent *event)
{
    xfwm_device_fill_meta (XFWM_EVENT_CROSSING, xievent->event, xievent->deviceid);

    event->crossing.root = xievent->root;
    event->crossing.enter = xievent->evtype == XI_Enter;
    event->crossing.mode = xievent->mode;
    event->crossing.detail = xievent->detail;
    event->crossing.x_root = xievent->root_x;
    event->crossing.y_root = xievent->root_y;
    event->crossing.time = xievent->time;

    return (XfwmEvent *)event;
}
#endif

static XfwmEvent *
xfwm_device_translate_event_common (XEvent *xevent, XfwmEvent *event)
{
    xfwm_device_fill_meta (XFWM_EVENT_XEVENT, xevent->xany.window, None);

    return event;
}

XfwmEvent *
xfwm_device_translate_event (XfwmDevices *devices, XEvent *xevent, XfwmEvent *event)
{
    switch (xevent->type)
    {
        case KeyPress:
        case KeyRelease:
            return xfwm_device_translate_event_key_core (xevent, event);
        case ButtonPress:
        case ButtonRelease:
            return xfwm_device_translate_event_button_core (xevent, event);
        case MotionNotify:
            return xfwm_device_translate_event_motion_core (xevent, event);
        case EnterNotify:
        case LeaveNotify:
            return xfwm_device_translate_event_crossing_core (xevent, event);
#ifdef HAVE_XI2
        case GenericEvent:
            if (devices->xi2_available &&
                xevent->xgeneric.extension == devices->xi2_opcode &&
                xevent->xcookie.data != NULL)
            {
                XIEvent *xievent = xevent->xcookie.data;

                switch (xievent->evtype)
                {
                    case XI_KeyPress:
                    case XI_KeyRelease:
                        return xfwm_device_translate_event_key_xi2 (xevent, (XIDeviceEvent *)xievent, event);
                    case XI_ButtonPress:
                    case XI_ButtonRelease:
                        return xfwm_device_translate_event_button_xi2 (xevent, (XIDeviceEvent *)xievent, event);
                    case XI_Motion:
                        return xfwm_device_translate_event_motion_xi2 (xevent, (XIDeviceEvent *)xievent, event);
                    case XI_Enter:
                    case XI_Leave:
                        return xfwm_device_translate_event_crossing_xi2 (xevent, (XIEnterEvent *)xievent, event);
                }
            }
            break;
#endif
    }

    return xfwm_device_translate_event_common (xevent, event);
}

void
xfwm_device_free_event (XfwmEvent *event)
{
    g_free (event);
}

void
xfwm_device_button_update_window (XfwmEventButton *event, Window window)
{
	event->meta.window = window;
#ifdef HAVE_XI2
	if (event->meta.device != None)
	{
		((XIDeviceEvent *)event->meta.xevent->xcookie.data)->event = window;
	}
	else
#endif
	{
		event->meta.xevent->xany.window = window;
	}
}

#ifdef HAVE_XI2
static void
xfwm_device_fill_xi2_event_mask (XIEventMask *xievent_mask, gulong core_mask)
{
    gint len = XIMaskLen (XI_LASTEVENT);
    guchar *mask = g_new0 (guchar, len);
    guint i;

    xievent_mask->deviceid = XIAllMasterDevices;
    xievent_mask->mask_len = len;
    xievent_mask->mask = mask;

    for (i = 0; i < G_N_ELEMENTS (core_to_xi2); i++)
    {
        if ((core_mask & core_to_xi2[i].core_mask) == core_to_xi2[i].core_mask)
        {
            XISetMask (mask, core_to_xi2[i].xi2_event);
        }
    }

    #undef xi2_set_mask
}
#endif

#ifdef HAVE_XI2
void
xfwm_device_configure_xi2_event_mask (XfwmDevices *devices, Display *dpy,
                                      Window window, gulong core_mask)
{
    if (devices->xi2_available)
    {
        XIEventMask xievent_mask;
        xfwm_device_fill_xi2_event_mask (&xievent_mask, core_mask);
        XISelectEvents (dpy, window, &xievent_mask, 1);
        g_free (xievent_mask.mask);
    }
}
#endif

#ifdef HAVE_XI2
#define xi2_modifier_mask(core_mask) \
    ((((core_mask) & AnyModifier) == AnyModifier) \
    ? (((core_mask) & ~AnyModifier) | XIAnyModifier) \
    : (core_mask))
#endif

gboolean
xfwm_device_grab (XfwmDevices *devices, XfwmDevice *device, Display *display,
                  Window grab_window, gboolean owner_events, guint event_mask,
                  gint grab_mode, Window confine_to, Cursor cursor, Time time)
{
    gboolean result;
    Status status;
#ifdef HAVE_XI2
    XIEventMask xievent_mask;
#endif

#ifdef HAVE_XI2
    if (device->xi2_device != None)
    {
        xfwm_device_fill_xi2_event_mask (&xievent_mask, event_mask);
        status = XIGrabDevice (display, device->xi2_device, grab_window, time, cursor,
                               grab_mode, grab_mode, owner_events, &xievent_mask);
        g_free (xievent_mask.mask);
        result = (status == XIGrabSuccess);
    }
    else
#endif
    if (device->keyboard)
    {
        status = XGrabKeyboard (display, grab_window, owner_events,
                                grab_mode, grab_mode, time);
        result = (status == GrabSuccess);
    }
    else
    {
        status = XGrabPointer (display, grab_window, owner_events, event_mask,
                               grab_mode, grab_mode, confine_to, cursor, time);
        result = (status == GrabSuccess);
    }
    return result;
}

void
xfwm_device_ungrab (XfwmDevices *devices, XfwmDevice *device, Display *display, Time time)
{
#ifdef HAVE_XI2
    if (device->xi2_device != None)
    {
        XIUngrabDevice (display, device->xi2_device, time);
    }
    else
#endif
    if (device->keyboard)
    {
        XUngrabKeyboard (display, time);
    }
    else
    {
        XUngrabPointer (display, time);
    }
}

gboolean
xfwm_device_grab_button (XfwmDevices *devices, Display *display,
                         guint button, guint modifiers, Window grab_window,
                         gboolean owner_events, guint event_mask,
                         gint grab_mode, gint paired_device_mode,
                         Window confine_to, Cursor cursor)
{
    gboolean result;
    DisplayInfo *display_info;
#ifdef HAVE_XI2
    Status status;
    XIGrabModifiers xi2_modifiers;
    XIEventMask xievent_mask;
#endif

    display_info = myDisplayGetDefault ();
    myDisplayErrorTrapPush (display_info);
    result = XGrabButton (display, button, modifiers, grab_window,
                          owner_events, event_mask, grab_mode, paired_device_mode,
                          confine_to, cursor);
    if (myDisplayErrorTrapPop (display_info) || !result)
    {
        return FALSE;
    }

#ifdef HAVE_XI2
    if (devices->xi2_available)
    {
        xi2_modifiers.modifiers = xi2_modifier_mask (modifiers);
        xi2_modifiers.status = 0;

        xfwm_device_fill_xi2_event_mask (&xievent_mask, event_mask);
        myDisplayErrorTrapPush (display_info);
        status = XIGrabButton (display, devices->pointer.xi2_device, button, grab_window,
                               cursor, grab_mode, paired_device_mode, owner_events,
                               &xievent_mask, 1, &xi2_modifiers);
        g_free (xievent_mask.mask);
        if (myDisplayErrorTrapPop (display_info) || status != XIGrabSuccess)
        {
            return FALSE;
        }
    }
#endif

    return TRUE;
}

void
xfwm_device_ungrab_button (XfwmDevices *devices, Display *display,
                           guint button, guint modifiers, Window grab_window)
{
    DisplayInfo *display_info;
#ifdef HAVE_XI2
    XIGrabModifiers xi2_modifiers;
#endif

    display_info = myDisplayGetDefault ();
    myDisplayErrorTrapPush (display_info);

    XUngrabButton (display, button, modifiers, grab_window);

#ifdef HAVE_XI2
    if (devices->xi2_available)
    {
        xi2_modifiers.modifiers = xi2_modifier_mask (modifiers);
        xi2_modifiers.status = 0;

        XIUngrabButton (display, devices->pointer.xi2_device, button,
                        grab_window, 1, &xi2_modifiers);
    }
#endif

    myDisplayErrorTrapPopIgnored (display_info);
}

gboolean
xfwm_device_grab_keycode (XfwmDevices *devices, Display *display,
                          gint keycode, guint modifiers, Window grab_window,
                          gboolean owner_events, guint event_mask,
                          gint grab_mode, gint paired_device_mode)
{
    gboolean result;
    DisplayInfo *display_info;
#ifdef HAVE_XI2
    Status status;
    XIGrabModifiers xi2_modifiers;
    XIEventMask xievent_mask;
#endif

    display_info = myDisplayGetDefault ();
    myDisplayErrorTrapPush (display_info);
    result = XGrabKey (display, keycode, modifiers, grab_window,
                       owner_events, grab_mode, paired_device_mode);
    if (myDisplayErrorTrapPop (display_info) || !result)
    {
        return FALSE;
    }

#ifdef HAVE_XI2
    if (devices->xi2_available)
    {
        xi2_modifiers.modifiers = xi2_modifier_mask (modifiers);
        xi2_modifiers.status = 0;

        xfwm_device_fill_xi2_event_mask (&xievent_mask, event_mask);
        myDisplayErrorTrapPush (display_info);
        status = XIGrabKeycode (display, devices->keyboard.xi2_device, keycode, grab_window,
                                grab_mode, paired_device_mode, owner_events,
                                &xievent_mask, 1, &xi2_modifiers);
        g_free (xievent_mask.mask);
        if (myDisplayErrorTrapPop (display_info) || status != XIGrabSuccess)
        {
            return FALSE;
        }
    }
#endif

    return TRUE;
}

void
xfwm_device_ungrab_keycode (XfwmDevices *devices, Display *display,
                            gint keycode, guint modifiers, Window grab_window)
{
    DisplayInfo *display_info;
#ifdef HAVE_XI2
    XIGrabModifiers xi2_modifiers;
#endif

    display_info = myDisplayGetDefault ();
    myDisplayErrorTrapPush (display_info);

    XUngrabKey (display, keycode, modifiers, grab_window);

#ifdef HAVE_XI2
    if (devices->xi2_available)
    {
        xi2_modifiers.modifiers = xi2_modifier_mask (modifiers);
        xi2_modifiers.status = 0;

        XIUngrabKeycode (display, devices->keyboard.xi2_device, keycode,
                         grab_window, 1, &xi2_modifiers);
    }
#endif

    myDisplayErrorTrapPopIgnored (display_info);
}

#ifdef HAVE_XI2
typedef struct
{
    XfwmDevices *devices;
    XfwmEvent *event;
    XIEventMask xievent_mask;
} XI2CheckMaskContext;

static gboolean
xfwm_device_check_mask_event_xi2_predicate (Display *display, XEvent *xevent, XPointer user_data)
{
    XI2CheckMaskContext *context = (void *)user_data;

    if (xevent->type == GenericEvent &&
        xevent->xgeneric.extension == context->devices->xi2_opcode &&
        XIMaskIsSet (context->xievent_mask.mask, xevent->xgeneric.evtype))
    {
        /* GDK holds XI2 event data which we are replacing so it should be released here */
        XFreeEventData (display, &context->event->meta.xevent->xcookie);
        return TRUE;
    }

    return FALSE;
}
#endif

gboolean
xfwm_device_check_mask_event (XfwmDevices *devices, Display *display,
                              guint event_mask, XfwmEvent *event)
{
    gboolean result;
#ifdef HAVE_XI2
    XI2CheckMaskContext context;
#endif

#ifdef HAVE_XI2
    if (devices->xi2_available && event->meta.device != None)
    {
        context.devices = devices;
        context.event = event;
        xfwm_device_fill_xi2_event_mask (&context.xievent_mask, event_mask);
        result = XCheckIfEvent (display, event->meta.xevent,
                                xfwm_device_check_mask_event_xi2_predicate, (XPointer)&context);
        g_free (context.xievent_mask.mask);

        if (result)
        {
            /* Previos data was released in predicate, allocate a new data for the new event */
            XGetEventData (display, &event->meta.xevent->xcookie);
        }
    }
    else
#endif
    {
        result = XCheckMaskEvent (display, event_mask, event->meta.xevent);
    }

    if (result)
    {
        xfwm_device_translate_event (devices, event->meta.xevent, event);
    }

    return result;
}

XfwmDevices *
xfwm_devices_new (GdkDisplay *display)
{
    XfwmDevices *devices;
#ifdef HAVE_XI2
    GdkSeat *seat;
    GdkDevice *pointer_device;
    GdkDevice *keyboard_device;
    gint firstevent, firsterror;
#endif

    devices = g_new0 (XfwmDevices, 1);
    devices->xi2_available = FALSE;
    devices->xi2_opcode = 0;

    devices->pointer.keyboard = FALSE;
    devices->pointer.xi2_device = None;

    devices->keyboard.keyboard = TRUE;
    devices->keyboard.xi2_device = None;

#ifdef HAVE_XI2
    seat = gdk_display_get_default_seat (display);
    pointer_device = gdk_seat_get_pointer (seat);
    keyboard_device = gdk_seat_get_keyboard (seat);

    if (GDK_IS_X11_DEVICE_XI2 (pointer_device) || GDK_IS_X11_DEVICE_XI2 (keyboard_device))
    {
        /* GDK uses XI2, let's use it too */

        /* Obtain XI2 opcode */
        if (XQueryExtension (gdk_x11_display_get_xdisplay (display), "XInputExtension",
                             &devices->xi2_opcode, &firstevent, &firsterror))
        {
            devices->xi2_available = TRUE;
            devices->pointer.xi2_device = gdk_x11_device_get_id (pointer_device);
            devices->keyboard.xi2_device = gdk_x11_device_get_id (keyboard_device);
        }
    }
#endif

    return devices;
}
