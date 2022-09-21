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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include <libxfce4util/libxfce4util.h>
#include "display.h"
#include "event_filter.h"

static eventFilterStatus
default_event_filter (XfwmEvent *event, gpointer data)
{
    switch (event->meta.type)
    {
        case XFWM_EVENT_KEY:
            TRACE ("unhandled XFWM_EVENT_KEY [%d] event", event->key.pressed);
            break;
        case XFWM_EVENT_BUTTON:
            TRACE ("unhandled XFWM_EVENT_BUTTON [%d] event", event->button.pressed);
            break;
        case XFWM_EVENT_MOTION:
            TRACE ("unhandled XFWM_EVENT_MOTION event");
            break;
        case XFWM_EVENT_CROSSING:
            TRACE ("unhandled XFWM_EVENT_CROSSING [%d] event", event->crossing.enter);
            break;
        case XFWM_EVENT_XEVENT:
            switch (event->meta.xevent->type)
            {
                case KeyPress:
                    TRACE ("unhandled KeyPress event");
                    break;
                case KeyRelease:
                    TRACE ("unhandled KeyRelease event");
                    break;
                case ButtonPress:
                    TRACE ("unhandled ButtonPress event");
                    break;
                case ButtonRelease:
                    TRACE ("unhandled ButtonRelease event");
                    break;
                case MotionNotify:
                    TRACE ("unhandled MotionNotify event");
                    break;
                case EnterNotify:
                    TRACE ("unhandled EnterNotify event");
                    break;
                case LeaveNotify:
                    TRACE ("unhandled LeaveNotify event");
                    break;
                case FocusIn:
                    TRACE ("unhandled FocusIn event");
                    break;
                case FocusOut:
                    TRACE ("unhandled FocusOut event");
                    break;
                case KeymapNotify:
                    TRACE ("unhandled KeymapNotify event");
                    break;
                case Expose:
                    TRACE ("unhandled Expose event");
                    break;
                case GraphicsExpose:
                    TRACE ("unhandled GraphicsExpose event");
                    break;
                case NoExpose:
                    TRACE ("unhandled NoExpose event");
                    break;
                case VisibilityNotify:
                    TRACE ("unhandled VisibilityNotify event");
                    break;
                case DestroyNotify:
                    TRACE ("unhandled DestroyNotify event");
                    break;
                case UnmapNotify:
                    TRACE ("unhandled UnmapNotify event");
                    break;
                case MapNotify:
                    TRACE ("unhandled MapNotify event");
                    break;
                case MapRequest:
                    TRACE ("unhandled MapRequest event");
                    break;
                case ReparentNotify:
                    TRACE ("unhandled ReparentNotify event");
                    break;
                case ConfigureNotify:
                    TRACE ("unhandled ConfigureNotify event");
                    break;
                case ConfigureRequest:
                    TRACE ("unhandled ConfigureRequest event");
                    break;
                case GravityNotify:
                    TRACE ("unhandled GravityNotify event");
                    break;
                case ResizeRequest:
                    TRACE ("unhandled ResizeRequest event");
                    break;
                case CirculateNotify:
                    TRACE ("unhandled CirculateNotify event");
                    break;
                case CirculateRequest:
                    TRACE ("unhandled CirculateRequest event");
                    break;
                case PropertyNotify:
                    TRACE ("unhandled PropertyNotify event");
                    break;
                case SelectionClear:
                    TRACE ("unhandled SelectionClear event");
                    break;
                case SelectionRequest:
                    TRACE ("unhandled SelectionRequest event");
                    break;
                case SelectionNotify:
                    TRACE ("unhandled SelectionNotify event");
                    break;
                case ColormapNotify:
                    TRACE ("unhandled ColormapNotify event");
                    break;
                default:
                    TRACE ("unhandled Unknown event");
                    break;
            }
    }
    /* This is supposed to be the default fallback event handler, so we return EVENT_FILTER_STOP since we have "treated" the event */
    return EVENT_FILTER_STOP;
}

static GdkFilterReturn
eventXfwmFilter (GdkXEvent *gdk_xevent, GdkEvent *gevent, gpointer data)
{
    XfwmEvent *event;
    eventFilterStatus loop;
    eventFilterSetup *setup;
    eventFilterStack *filterelt;

    setup = (eventFilterSetup *) data;
    g_return_val_if_fail (setup != NULL, GDK_FILTER_CONTINUE);

    filterelt = setup->filterstack;
    g_return_val_if_fail (filterelt != NULL, GDK_FILTER_CONTINUE);

    event = xfwm_device_translate_event (setup->devices, (XEvent *)gdk_xevent, NULL);
    loop = EVENT_FILTER_CONTINUE;

    while ((filterelt) && (loop == EVENT_FILTER_CONTINUE))
    {
        eventFilterStack *filterelt_next = filterelt->next;
        loop = (*filterelt->filter) (event, filterelt->data);
        filterelt = filterelt_next;
    }

    xfwm_device_free_event (event);
    return (loop & EVENT_FILTER_REMOVE) ? GDK_FILTER_REMOVE : GDK_FILTER_CONTINUE;
}

eventFilterStack *
eventFilterPush (eventFilterSetup *setup, XfwmFilter filter, gpointer data)
{
    g_assert (filter != NULL);
    if (setup->filterstack)
    {
        eventFilterStack *newfilterstack =
            (eventFilterStack *) g_new0 (eventFilterStack, 1);
        newfilterstack->filter = filter;
        newfilterstack->data = data;
        newfilterstack->next = setup->filterstack;
        setup->filterstack = newfilterstack;
    }
    else
    {
        setup->filterstack =
            (eventFilterStack *) g_new0 (eventFilterStack, 1);
        setup->filterstack->filter = filter;
        setup->filterstack->data = data;
        setup->filterstack->next = NULL;
    }
    return (setup->filterstack);
}

eventFilterStack *
eventFilterPop (eventFilterSetup *setup)
{
    eventFilterStack *oldfilterstack;

    g_return_val_if_fail (setup->filterstack != NULL, NULL);

    oldfilterstack = setup->filterstack;
    setup->filterstack = oldfilterstack->next;
    g_free (oldfilterstack);

    return (setup->filterstack);
}

GdkWindow *
eventFilterAddWin (GdkScreen *gscr, XfwmDevices *devices, long event_mask)
{
    XWindowAttributes attribs;
    Display *dpy;
    Window xroot;
    GdkWindow *event_win;
    guint error;
    GdkDisplay *gdisplay;

    g_return_val_if_fail (gscr, NULL);
    g_return_val_if_fail (GDK_IS_SCREEN (gscr), NULL);

    event_win = gdk_screen_get_root_window (gscr);
    xroot = gdk_x11_window_get_xid (event_win);
    gdisplay = gdk_window_get_display (event_win);
    dpy = gdk_x11_display_get_xdisplay (gdisplay);

    myDisplayErrorTrapPush (myDisplayGetDefault ());
    gdk_x11_grab_server ();

    XGetWindowAttributes (dpy, xroot, &attribs);
    XSelectInput (dpy, xroot, attribs.your_event_mask | event_mask);
#ifdef HAVE_XI2
    xfwm_device_configure_xi2_event_mask (devices, dpy, xroot, attribs.your_event_mask | event_mask);
#endif

    gdk_x11_ungrab_server ();
    error = myDisplayErrorTrapPop (myDisplayGetDefault ());

    if (error)
    {
        TRACE ("error code: %i", error);
        return (NULL);
    }

    return event_win;
}

eventFilterSetup *
eventFilterInit (XfwmDevices *devices, gpointer data)
{
    eventFilterSetup *setup;

    setup = g_new0 (eventFilterSetup, 1);
    setup->filterstack = NULL;
    setup->devices = devices;
    eventFilterPush (setup, default_event_filter, data);
    gdk_window_add_filter (NULL, eventXfwmFilter, (gpointer) setup);

    return (setup);
}

void
eventFilterClose (eventFilterSetup *setup)
{
    eventFilterStack *filterelt;

    filterelt = setup->filterstack;
    while ((filterelt = eventFilterPop (setup)));
    gdk_window_remove_filter (NULL, eventXfwmFilter, NULL);
    setup->filterstack = NULL;
}
