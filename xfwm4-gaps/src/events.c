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
        xfwm4    - (c) 2002-2020 Olivier Fourdan

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#ifdef HAVE_RANDR
#include <X11/extensions/Xrandr.h>
#endif
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include <common/xfwm-common.h>

#include "misc.h"
#include "workspaces.h"
#include "settings.h"
#include "mywindow.h"
#include "frame.h"
#include "client.h"
#include "moveresize.h"
#include "cycle.h"
#include "placement.h"
#include "stacking.h"
#include "transients.h"
#include "focus.h"
#include "netwm.h"
#include "menu.h"
#include "hints.h"
#include "startup_notification.h"
#include "compositor.h"
#include "events.h"
#include "event_filter.h"
#include "xsync.h"
#include "display.h"

#ifndef CHECK_BUTTON_TIME
#define CHECK_BUTTON_TIME 0
#endif

#define WIN_IS_BUTTON(win)      ((win == MYWINDOW_XWINDOW(c->buttons[HIDE_BUTTON])) || \
                                 (win == MYWINDOW_XWINDOW(c->buttons[CLOSE_BUTTON])) || \
                                 (win == MYWINDOW_XWINDOW(c->buttons[MAXIMIZE_BUTTON])) || \
                                 (win == MYWINDOW_XWINDOW(c->buttons[SHADE_BUTTON])) || \
                                 (win == MYWINDOW_XWINDOW(c->buttons[STICK_BUTTON])))

#define DOUBLE_CLICK_GRAB       (ButtonMotionMask | \
                                 PointerMotionMask | \
                                 ButtonPressMask | \
                                 ButtonReleaseMask)

static xfwmWindow menu_event_window;

/* Forward decl. */

static eventFilterStatus handleEvent (DisplayInfo *display_info,
                                      XfwmEvent *event);
static void menu_callback            (Menu * menu,
                                      MenuOp op,
                                      Window xid,
                                      gpointer menu_data,
                                      gpointer item_data);
static void show_window_menu         (Client *c,
                                      gint px,
                                      gint py,
                                      guint button,
                                      guint32 time,
                                      gboolean needscale);
static gboolean show_popup_cb        (GtkWidget * widget,
                                      GdkEventButton * ev,
                                      gpointer data);
static gboolean set_reload           (DisplayInfo *display_info);

typedef enum
{
    XFWM_BUTTON_UNDEFINED = 0,
    XFWM_BUTTON_DRAG = 1,
    XFWM_BUTTON_CLICK = 2,
    XFWM_BUTTON_CLICK_AND_DRAG = 3,
    XFWM_BUTTON_DOUBLE_CLICK = 4
}
XfwmButtonClickType;

typedef struct _XfwmButtonClickData XfwmButtonClickData;
struct _XfwmButtonClickData
{
    DisplayInfo *display_info;
    Window w;
    guint button;
    guint clicks;
    guint timeout;
    gint  x0;
    gint  y0;
    guint t0;
    gint  xcurrent;
    gint  ycurrent;
    guint tcurrent;
    gint  double_click_time;
    gint  double_click_distance;
    gboolean allow_double_click;
};

static gboolean
typeOfClick_end (gpointer data)
{
    XfwmButtonClickData *passdata;

    passdata = data;
    if (passdata->timeout)
    {
        g_source_remove (passdata->timeout);
        passdata->timeout = 0;
    }

    gtk_main_quit ();

    return FALSE;
}

static eventFilterStatus
typeOfClick_event_filter (XfwmEvent *event, gpointer data)
{
    XfwmButtonClickData *passdata;
    eventFilterStatus status;
    guint32 timestamp;
    gboolean keep_going;

    keep_going = TRUE;
    passdata = data;
    status = EVENT_FILTER_CONTINUE;

    /* Update the display time */
    timestamp = myDisplayUpdateCurrentTime (passdata->display_info, event);

    if (timestamp)
    {
        passdata->tcurrent = timestamp;
        if (((gint) passdata->tcurrent - (gint) passdata->t0) > passdata->double_click_time)
        {
            keep_going = FALSE;
        }
    }
    if (event->meta.type == XFWM_EVENT_BUTTON)
    {
        if (event->button.button == passdata->button)
        {
            passdata->clicks++;
        }
        if (((XfwmButtonClickType) passdata->clicks == XFWM_BUTTON_DOUBLE_CLICK)
            || (!(passdata->allow_double_click) &&
                 (XfwmButtonClickType) passdata->clicks == XFWM_BUTTON_CLICK))
        {
            keep_going = FALSE;
        }
        status = EVENT_FILTER_STOP;
    }
    else if (event->meta.type == XFWM_EVENT_MOTION)
    {
        passdata->xcurrent = event->motion.x_root;
        passdata->ycurrent = event->motion.y_root;

        if ((ABS (passdata->x0 - passdata->xcurrent) > passdata->double_click_distance) ||
            (ABS (passdata->y0 - passdata->ycurrent) > passdata->double_click_distance))
        {
            keep_going = FALSE;
        }
        status = EVENT_FILTER_STOP;
    }
    else if ((event->meta.xevent->type == DestroyNotify) || (event->meta.xevent->type == UnmapNotify))
    {
        if (event->meta.window == passdata->w)
        {
            /* Discard, mark the click as undefined */
            passdata->clicks = (guint) XFWM_BUTTON_UNDEFINED;
            keep_going = FALSE;
        }
    }

    if (!keep_going)
    {
        TRACE ("click type=%u", passdata->clicks);
        TRACE ("time=%ims (timeout=%ims)", (gint) passdata->tcurrent - (gint) passdata->t0, passdata->double_click_time);
        TRACE ("dist x=%i (max=%i)", ABS (passdata->x0 - passdata->xcurrent), passdata->double_click_distance);
        TRACE ("dist y=%i (max=%i)", ABS (passdata->y0 - passdata->ycurrent), passdata->double_click_distance);
        typeOfClick_end (data);
    }

    return status;
}

static XfwmButtonClickType
typeOfClick (ScreenInfo *screen_info, Window w, XfwmEventButton *event, gboolean allow_double_click)
{
    DisplayInfo *display_info;
    XfwmButtonClickData passdata;
    gboolean g;

    g_return_val_if_fail (screen_info != NULL, XFWM_BUTTON_UNDEFINED);
    g_return_val_if_fail (event != NULL, XFWM_BUTTON_UNDEFINED);
    g_return_val_if_fail (w != None, XFWM_BUTTON_UNDEFINED);

    display_info = screen_info->display_info;
    g = myScreenGrabPointer (screen_info, FALSE, DOUBLE_CLICK_GRAB, None, event->time);

    if (!g)
    {
        TRACE ("grab failed");
        gdk_display_beep (display_info->gdisplay);
        myScreenUngrabPointer (screen_info, event->time);
        return XFWM_BUTTON_UNDEFINED;
    }

    passdata.display_info = display_info;
    passdata.button = event->button;
    passdata.w = w;
    passdata.x0 = event->x_root;
    passdata.y0 = event->y_root;
    passdata.t0 = event->time;
    passdata.xcurrent = passdata.x0;
    passdata.ycurrent = passdata.y0;
    passdata.tcurrent = passdata.t0;
    passdata.clicks = 1;
    passdata.allow_double_click = allow_double_click;
    passdata.double_click_time = display_info->double_click_time;
    passdata.double_click_distance = display_info->double_click_distance;
    TRACE ("double click time= %i, distance=%i\n", display_info->double_click_time,
                                                   display_info->double_click_distance);
    passdata.timeout = g_timeout_add_full (G_PRIORITY_HIGH,
                                           display_info->double_click_time,
                                           typeOfClick_end,
                                           &passdata, NULL);

    TRACE ("entering loop");
    eventFilterPush (display_info->xfilter, typeOfClick_event_filter, &passdata);
    gtk_main ();
    eventFilterPop (display_info->xfilter);
    TRACE ("leaving loop");

    myScreenUngrabPointer (screen_info, myDisplayGetCurrentTime (display_info));

    return (XfwmButtonClickType) passdata.clicks;
}

static void
toggle_show_desktop (ScreenInfo *screen_info)
{
    screen_info->show_desktop = !screen_info->show_desktop;
    setHint (screen_info->display_info, screen_info->xroot, NET_SHOWING_DESKTOP,
             screen_info->show_desktop);
    sendRootMessage (screen_info, NET_SHOWING_DESKTOP, screen_info->show_desktop,
                     myDisplayGetCurrentTime (screen_info->display_info));
}

static eventFilterStatus
handleMotionNotify (DisplayInfo *display_info, XfwmEventMotion *event)
{
    TRACE ("entering");
    return EVENT_FILTER_REMOVE;
}

static eventFilterStatus
handleKeyPress (DisplayInfo *display_info, XfwmEventKey *event)
{
    eventFilterStatus status;
    ScreenInfo *screen_info;
    ScreenInfo *ev_screen_info;
    Client *c;
    int key;

    TRACE ("entering");

    ev_screen_info = myDisplayGetScreenFromRoot (display_info, event->root);
    if (!ev_screen_info)
    {
        return EVENT_FILTER_PASS;
    }

    status = EVENT_FILTER_PASS;
    c = clientGetFocus ();
    if (c)
    {
        screen_info = c->screen_info;
        key = myScreenGetKeyPressed (screen_info, event);
        status = EVENT_FILTER_REMOVE;

        switch (key)
        {
            case KEY_MOVE:
                clientMove (c, NULL);
                break;
            case KEY_RESIZE:
                clientResize (c, CORNER_BOTTOM_RIGHT, NULL);
                break;
            case KEY_CYCLE_WINDOWS:
            case KEY_CYCLE_REVERSE_WINDOWS:
                clientCycle (c, event);
                break;
            case KEY_CLOSE_WINDOW:
                clientClose (c);
                break;
            case KEY_HIDE_WINDOW:
                if (CLIENT_CAN_HIDE_WINDOW (c))
                {
                    clientWithdraw (c, c->win_workspace, TRUE);
                }
                break;
            case KEY_MAXIMIZE_WINDOW:
                clientToggleMaximized (c, CLIENT_FLAG_MAXIMIZED, TRUE);
                break;
            case KEY_MAXIMIZE_VERT:
                clientToggleMaximized (c, CLIENT_FLAG_MAXIMIZED_VERT, TRUE);
                break;
            case KEY_MAXIMIZE_HORIZ:
                clientToggleMaximized (c, CLIENT_FLAG_MAXIMIZED_HORIZ, TRUE);
                break;
            case KEY_SHADE_WINDOW:
                clientToggleShaded (c);
                break;
            case KEY_STICK_WINDOW:
                if (FLAG_TEST(c->xfwm_flags, XFWM_FLAG_HAS_STICK))
                {
                    clientToggleSticky (c, TRUE);
                    frameQueueDraw (c, FALSE);
                }
                break;
            case KEY_RAISE_WINDOW:
                clientRaise (c, None);
                break;
            case KEY_LOWER_WINDOW:
                clientLower (c, None);
                break;
            case KEY_RAISELOWER_WINDOW:
                if (clientIsTopMost (c))
                {
                    clientLower (c, None);
                }
                else
                {
                    clientRaise (c, None);
                }
                break;
            case KEY_TOGGLE_ABOVE:
                clientToggleLayerAbove (c);
                break;
            case KEY_TOGGLE_FULLSCREEN:
                clientToggleFullscreen (c);
                break;
            case KEY_MOVE_NEXT_WORKSPACE:
                workspaceSwitch (screen_info, screen_info->current_ws + 1, c, TRUE, event->time);
                break;
            case KEY_MOVE_PREV_WORKSPACE:
                workspaceSwitch (screen_info, screen_info->current_ws - 1, c, TRUE, event->time);
                break;
            case KEY_MOVE_UP_WORKSPACE:
                workspaceMove (screen_info, -1, 0, c, event->time);
                break;
            case KEY_MOVE_DOWN_WORKSPACE:
                workspaceMove (screen_info, 1, 0, c, event->time);
                break;
            case KEY_MOVE_LEFT_WORKSPACE:
                workspaceMove (screen_info, 0, -1, c, event->time);
                break;
            case KEY_MOVE_RIGHT_WORKSPACE:
                workspaceMove (screen_info, 0, 1, c, event->time);
                break;
            case KEY_MOVE_WORKSPACE_1:
            case KEY_MOVE_WORKSPACE_2:
            case KEY_MOVE_WORKSPACE_3:
            case KEY_MOVE_WORKSPACE_4:
            case KEY_MOVE_WORKSPACE_5:
            case KEY_MOVE_WORKSPACE_6:
            case KEY_MOVE_WORKSPACE_7:
            case KEY_MOVE_WORKSPACE_8:
            case KEY_MOVE_WORKSPACE_9:
            case KEY_MOVE_WORKSPACE_10:
            case KEY_MOVE_WORKSPACE_11:
            case KEY_MOVE_WORKSPACE_12:
                if ((guint) (key - KEY_MOVE_WORKSPACE_1) < screen_info->workspace_count)
                {
                    clientRaise (c, None);
                    clientSetWorkspace (c, key - KEY_MOVE_WORKSPACE_1, TRUE);
                }
                break;
            case KEY_POPUP_MENU:
                show_window_menu (c, frameExtentX (c) + frameLeft (c),
                                     frameExtentY (c) + frameTop (c),
                                     Button1, event->time, TRUE);
                break;
            case KEY_FILL_WINDOW:
                clientFill (c, CLIENT_FILL);
                break;
            case KEY_FILL_VERT:
                clientFill (c, CLIENT_FILL_VERT);
                break;
            case KEY_FILL_HORIZ:
                clientFill (c, CLIENT_FILL_HORIZ);
                break;
            case KEY_TILE_DOWN:
                clientToggleTile (c, TILE_DOWN);
                break;
            case KEY_TILE_LEFT:
                clientToggleTile (c, TILE_LEFT);
                break;
            case KEY_TILE_RIGHT:
                clientToggleTile (c, TILE_RIGHT);
                break;
            case KEY_TILE_UP:
                clientToggleTile (c, TILE_UP);
                break;
            case KEY_TILE_DOWN_LEFT:
                clientToggleTile (c, TILE_DOWN_LEFT);
                break;
            case KEY_TILE_DOWN_RIGHT:
                clientToggleTile (c, TILE_DOWN_RIGHT);
                break;
            case KEY_TILE_UP_LEFT:
                clientToggleTile (c, TILE_UP_LEFT);
                break;
            case KEY_TILE_UP_RIGHT:
                clientToggleTile (c, TILE_UP_RIGHT);
                break;
            default:
                break;
        }
    }
    else
    {
        key = myScreenGetKeyPressed (ev_screen_info, event);
        switch (key)
        {
            case KEY_CYCLE_WINDOWS:
                status = EVENT_FILTER_REMOVE;
                if (ev_screen_info->clients)
                {
                    clientCycle (ev_screen_info->clients->prev, event);
                }
                break;
            case KEY_CLOSE_WINDOW:
                status = EVENT_FILTER_REMOVE;
                if (display_info->session)
                {
                    xfce_sm_client_request_shutdown(display_info->session, XFCE_SM_CLIENT_SHUTDOWN_HINT_LOGOUT);
                }
                break;
            default:
                break;
        }
    }

    switch (key)
    {
        case KEY_SWITCH_WINDOW:
            clientSwitchWindow ();
            break;
        case KEY_SWITCH_APPLICATION:
            clientSwitchApp ();
            break;
        case KEY_NEXT_WORKSPACE:
            status = EVENT_FILTER_REMOVE;
            workspaceSwitch (ev_screen_info, ev_screen_info->current_ws + 1, NULL, TRUE, event->time);
            break;
        case KEY_PREV_WORKSPACE:
            status = EVENT_FILTER_REMOVE;
            workspaceSwitch (ev_screen_info, ev_screen_info->current_ws - 1, NULL, TRUE, event->time);
            break;
        case KEY_UP_WORKSPACE:
            status = EVENT_FILTER_REMOVE;
            workspaceMove(ev_screen_info, -1, 0, NULL, event->time);
            break;
        case KEY_DOWN_WORKSPACE:
            status = EVENT_FILTER_REMOVE;
            workspaceMove(ev_screen_info, 1, 0, NULL, event->time);
            break;
        case KEY_LEFT_WORKSPACE:
            status = EVENT_FILTER_REMOVE;
            workspaceMove(ev_screen_info, 0, -1, NULL, event->time);
            break;
        case KEY_RIGHT_WORKSPACE:
            status = EVENT_FILTER_REMOVE;
            workspaceMove(ev_screen_info, 0, 1, NULL, event->time);
            break;
        case KEY_ADD_WORKSPACE:
            status = EVENT_FILTER_REMOVE;
            workspaceSetCount (ev_screen_info, ev_screen_info->workspace_count + 1);
            break;
        case KEY_DEL_WORKSPACE:
            status = EVENT_FILTER_REMOVE;
            workspaceSetCount (ev_screen_info, ev_screen_info->workspace_count - 1);
            break;
        case KEY_ADD_ADJACENT_WORKSPACE:
            workspaceInsert (ev_screen_info, ev_screen_info->current_ws + 1);
            break;
        case KEY_DEL_ACTIVE_WORKSPACE:
            workspaceDelete (ev_screen_info, ev_screen_info->current_ws);
            break;
        case KEY_WORKSPACE_1:
        case KEY_WORKSPACE_2:
        case KEY_WORKSPACE_3:
        case KEY_WORKSPACE_4:
        case KEY_WORKSPACE_5:
        case KEY_WORKSPACE_6:
        case KEY_WORKSPACE_7:
        case KEY_WORKSPACE_8:
        case KEY_WORKSPACE_9:
        case KEY_WORKSPACE_10:
        case KEY_WORKSPACE_11:
        case KEY_WORKSPACE_12:
            status = EVENT_FILTER_REMOVE;
            if ((guint) (key - KEY_WORKSPACE_1) < ev_screen_info->workspace_count)
            {
                workspaceSwitch (ev_screen_info, key - KEY_WORKSPACE_1, NULL, TRUE, event->time);
            }
            break;
        case KEY_SHOW_DESKTOP:
            status = EVENT_FILTER_REMOVE;
            toggle_show_desktop (ev_screen_info);
            break;
        default:
            break;
    }

    /* Release pending events */
    XAllowEvents (display_info->dpy, SyncKeyboard, CurrentTime);

    return status;
}

static eventFilterStatus
handleKeyRelease (DisplayInfo *display_info, XfwmEventKey *event)
{
    TRACE ("entering");

    /* Release pending events */
    XAllowEvents (display_info->dpy, SyncKeyboard, CurrentTime);

    return EVENT_FILTER_PASS;
}

/* User has clicked on an edge or corner.
 * Button 1 : Raise and resize
 * Button 2 : Move
 * Button 3 : Resize
 */
static void
edgeButton (Client *c, int part, XfwmEventButton *event)
{
    ScreenInfo *screen_info;
    XfwmButtonClickType tclick;
    guint state;

    screen_info = c->screen_info;
    state = event->state & MODIFIER_MASK;

    if (event->button == Button2)
    {
        tclick = typeOfClick (screen_info, c->window, event, FALSE);
        if (tclick == XFWM_BUTTON_CLICK)
        {
            clientLower (c, None);
        }
        else if (tclick == XFWM_BUTTON_DRAG)
        {
            clientMove (c, event);
        }
    }
    else if ((event->button == Button1) || (event->button == Button3))
    {
        if ((event->button == Button1) ||
            ((screen_info->params->easy_click) && (state == screen_info->params->easy_click)))
        {
            if (!(c->type & WINDOW_TYPE_DONT_FOCUS))
            {
                clientSetFocus (screen_info, c, event->time, NO_FOCUS_FLAG);
            }
            clientRaise (c, None);
        }
        tclick = typeOfClick (screen_info, c->window, event, TRUE);
        if (tclick == XFWM_BUTTON_DOUBLE_CLICK)
        {
            switch (part)
            {
                case CORNER_COUNT + SIDE_LEFT:
                case CORNER_COUNT + SIDE_RIGHT:
                    clientFill(c, CLIENT_FILL_HORIZ);
                    break;
                case CORNER_COUNT + SIDE_TOP:
                case CORNER_COUNT + SIDE_BOTTOM:
                    clientFill(c, CLIENT_FILL_VERT);
                    break;
                default:
                    clientFill(c, CLIENT_FILL);
                    break;
            }
        }
        else if (tclick == XFWM_BUTTON_DRAG)
        {
            clientResize (c, part, event);
        }
    }
}

static int
edgeGetPart (Client *c, XfwmEventButton *event)
{
    int part, x_corner_pixels, y_corner_pixels, x_distance, y_distance;

    /* Corner is 1/3 of the side */
    x_corner_pixels = MAX(c->width / 3, 50);
    y_corner_pixels = MAX(c->height / 3, 50);

    /* Distance from event to edge of client window */
    x_distance = c->width / 2 - abs(c->width / 2 - event->x);
    y_distance = c->height / 2 - abs(c->height / 2 - event->y);

    if (x_distance < x_corner_pixels && y_distance < y_corner_pixels)
    {
        /* In a corner */
        if (event->x < c->width / 2)
        {
            if (event->y < c->height / 2)
            {
                part = CORNER_TOP_LEFT;
            }
            else
            {
                part = CORNER_BOTTOM_LEFT;
            }
        }
        else
        {
            if (event->y < c->height / 2)
            {
                part = CORNER_TOP_RIGHT;
            }
            else
            {
                part = CORNER_BOTTOM_RIGHT;
            }
        }
    }
    else
    {
        /* Not a corner - some side */
        if (x_distance / x_corner_pixels < y_distance / y_corner_pixels)
        {
            /* Left or right side */
            if (event->x < c->width / 2)
            {
                part = CORNER_COUNT + SIDE_LEFT;
            }
            else
            {
                part = CORNER_COUNT + SIDE_RIGHT;
            }
        }
        else
        {
            /* Top or bottom side */
            if (event->y < c->height / 2)
            {
                part = CORNER_COUNT + SIDE_TOP;
            }
            else
            {
                part = CORNER_COUNT + SIDE_BOTTOM;
            }
        }
    }

    return part;
}
static void
button1Action (Client *c, XfwmEventButton *event)
{
    ScreenInfo *screen_info;
    XfwmButtonClickType tclick;

    g_return_if_fail (c != NULL);
    g_return_if_fail (event != NULL);

    screen_info = c->screen_info;

    if (!(c->type & WINDOW_TYPE_DONT_FOCUS))
    {
        clientSetFocus (screen_info, c, event->time, NO_FOCUS_FLAG);
    }
    clientRaise (c, None);

    tclick = typeOfClick (screen_info, c->window, event, TRUE);
    if ((tclick == XFWM_BUTTON_DRAG) || (tclick == XFWM_BUTTON_CLICK_AND_DRAG))
    {
        clientMove (c, event);
    }
    else if (tclick == XFWM_BUTTON_DOUBLE_CLICK)
    {
        switch (screen_info->params->double_click_action)
        {
            case DOUBLE_CLICK_ACTION_MAXIMIZE:
                clientToggleMaximized (c, CLIENT_FLAG_MAXIMIZED, TRUE);
                break;
            case DOUBLE_CLICK_ACTION_SHADE:
                clientToggleShaded (c);
                break;
            case DOUBLE_CLICK_ACTION_FILL:
                clientFill(c, CLIENT_FILL);
                break;
            case DOUBLE_CLICK_ACTION_HIDE:
                if (CLIENT_CAN_HIDE_WINDOW (c))
                {
                    clientWithdraw (c, c->win_workspace, TRUE);
                }
                break;
            case DOUBLE_CLICK_ACTION_ABOVE:
                clientToggleLayerAbove (c);
                break;
            default:
                break;
        }
    }
}

static void
titleButton (Client *c, guint state, XfwmEventButton *event)
{
    ScreenInfo *screen_info;

    g_return_if_fail (c != NULL);
    g_return_if_fail (event != NULL);

    /* Get Screen data from the client itself */
    screen_info = c->screen_info;

    if (event->button == Button1)
    {
        button1Action (c, event);
    }
    else if (event->button == Button2)
    {
        clientLower (c, None);
    }
    else if (event->button == Button3)
    {
        XfwmButtonClickType tclick;

        tclick = typeOfClick (screen_info, c->window, event, FALSE);
        if (tclick == XFWM_BUTTON_DRAG)
        {
            clientMove (c, event);
        }
        else if (tclick != XFWM_BUTTON_UNDEFINED)
        {
            if (!(c->type & WINDOW_TYPE_DONT_FOCUS))
            {
                clientSetFocus (screen_info, c, event->time, NO_FOCUS_FLAG);
            }
            if (screen_info->params->raise_on_click)
            {
                clientRaise (c, None);
            }
            xfwm_device_button_update_window (event, event->root);
            if (screen_info->button_handler_id)
            {
                g_signal_handler_disconnect (G_OBJECT (myScreenGetGtkWidget (screen_info)), screen_info->button_handler_id);
            }
            screen_info->button_handler_id = g_signal_connect (G_OBJECT (myScreenGetGtkWidget (screen_info)),
                                                      "button_press_event", G_CALLBACK (show_popup_cb), (gpointer) c);
            /* Let GTK handle this for us. */
        }
    }
    else if (event->button == Button4)
    {
        /* Mouse wheel scroll up */
#ifdef HAVE_COMPOSITOR
        if ((state) && (state == screen_info->params->easy_click) && compositorIsActive (screen_info))
        {
            clientIncOpacity(c);
        }
        else
#endif /* HAVE_COMPOSITOR */
        if (!FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
        {
            if (screen_info->params->mousewheel_rollup)
            {
                clientShade (c);
            }
        }
    }
    else if (event->button == Button5)
    {
        /* Mouse wheel scroll down */
#ifdef HAVE_COMPOSITOR
        if ((state) && (state == screen_info->params->easy_click) && compositorIsActive (screen_info))
        {
            clientDecOpacity(c);
        }
        else
#endif /* HAVE_COMPOSITOR */
        if (FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
        {
            if (screen_info->params->mousewheel_rollup)
            {
                clientUnshade (c);
            }
        }
    }
#ifdef HAVE_COMPOSITOR
    else if (screen_info->params->horiz_scroll_opacity)
    {
        if (event->button == Button6)
        {
            /* Mouse wheel scroll left, or left side button */
            clientDecOpacity(c);
        }
        else if (event->button == Button7)
        {
            /* Mouse wheel scroll right, or right side button */
            clientIncOpacity(c);
        }
    }
#endif /* HAVE_COMPOSITOR */
}

static void
rootScrollButton (DisplayInfo *display_info, XfwmEventButton *event)
{
    static guint32 lastscroll = CurrentTime;
    ScreenInfo *screen_info;

    if ((event->time - lastscroll) < 25)  /* ms */
    {
        /* Too many events in too little time, drop this event... */
        return;
    }
    lastscroll = event->time;

    /* Get the screen structure from the root of the event */
    screen_info = myDisplayGetScreenFromRoot (display_info, event->root);
    if (!screen_info)
    {
        return;
    }

    if (event->button == Button4)
    {
        workspaceSwitch (screen_info, screen_info->current_ws - 1, NULL, TRUE, event->time);
    }
    else if (event->button == Button5)
    {
        workspaceSwitch (screen_info, screen_info->current_ws + 1, NULL, TRUE, event->time);
    }
}


static eventFilterStatus
handleButtonPress (DisplayInfo *display_info, XfwmEventButton *event)
{
    ScreenInfo *screen_info;
    Client *c;
    Window win;
    guint state, part;
    gboolean replay;

    TRACE ("entering");

    replay = FALSE;
    c = myDisplayGetClientFromWindow (display_info, event->meta.window,
                                      SEARCH_FRAME | SEARCH_WINDOW);
    if (c)
    {
        state = event->state & MODIFIER_MASK;
        win = event->subwindow;
        screen_info = c->screen_info;

        if ((event->button == Button1) && (state) && (state == screen_info->params->easy_click))
        {
            button1Action (c, event);
        }
        else if ((event->button == Button2) && (state) && (state == screen_info->params->easy_click))
        {
            clientLower (c, None);
        }
        else if ((event->button == Button3) && (state) && (state == screen_info->params->easy_click))
        {
            part = edgeGetPart (c, event);
            edgeButton (c, part, event);
        }
#ifdef HAVE_COMPOSITOR
        else if ((event->button == Button4) && (screen_info->params->zoom_desktop) && (state) &&
                 (state == screen_info->params->easy_click) && compositorIsActive (screen_info))
        {
            compositorZoomIn(screen_info, event);
        }
        else if ((event->button == Button5) && (screen_info->params->zoom_desktop) && (state) &&
                 (state == screen_info->params->easy_click) && compositorIsActive (screen_info))
        {
            compositorZoomOut(screen_info, event);
        }
#endif /* HAVE_COMPOSITOR */
        else if ((event->button == Button8) && (state) && (state == screen_info->params->easy_click))
        {
            workspaceSwitch (screen_info, screen_info->current_ws - 1, NULL, TRUE, event->time);
        }
        else if ((event->button == Button9) && (state) && (state == screen_info->params->easy_click))
        {
            workspaceSwitch (screen_info, screen_info->current_ws + 1, NULL, TRUE, event->time);
        }

        else if (WIN_IS_BUTTON (win))
        {
            if (event->button <= Button3)
            {
                if (!(c->type & WINDOW_TYPE_DONT_FOCUS))
                {
                    clientSetFocus (screen_info, c, event->time, NO_FOCUS_FLAG);
                }
                if (screen_info->params->raise_on_click)
                {
                    clientClearDelayedRaise ();
                    clientRaise (c, None);
                }
                clientButtonPress (c, win, event);
            }
        }
        else if (win == MYWINDOW_XWINDOW (c->title))
        {
            titleButton (c, state, event);
        }
        else if (win == MYWINDOW_XWINDOW (c->buttons[MENU_BUTTON]))
        {
            if (event->button == Button1)
            {
                XfwmButtonClickType tclick;

                tclick = typeOfClick (screen_info, c->window, event, TRUE);

                if (tclick == XFWM_BUTTON_DOUBLE_CLICK)
                {
                    clientClose (c);
                }
                else if (tclick != XFWM_BUTTON_UNDEFINED)
                {
                    if (!(c->type & WINDOW_TYPE_DONT_FOCUS))
                    {
                        clientSetFocus (screen_info, c, event->time, NO_FOCUS_FLAG);
                    }
                    if (screen_info->params->raise_on_click)
                    {
                        clientClearDelayedRaise ();
                        clientRaise (c, None);
                    }
                    xfwm_device_button_update_window (event, event->root);
                    if (screen_info->button_handler_id)
                    {
                        g_signal_handler_disconnect (G_OBJECT (myScreenGetGtkWidget (screen_info)), screen_info->button_handler_id);
                    }
                    screen_info->button_handler_id = g_signal_connect (G_OBJECT (myScreenGetGtkWidget (screen_info)),
                                                              "button_press_event", G_CALLBACK (show_popup_cb), (gpointer) c);
                    /* Let GTK handle this for us. */
                }
            }
        }
        else if (win == MYWINDOW_XWINDOW (c->corners[CORNER_TOP_LEFT]))
        {
            edgeButton (c, CORNER_TOP_LEFT, event);
        }
        else if (win == MYWINDOW_XWINDOW (c->corners[CORNER_TOP_RIGHT]))
        {
            edgeButton (c, CORNER_TOP_RIGHT, event);
        }
        else if (win == MYWINDOW_XWINDOW (c->corners[CORNER_BOTTOM_LEFT]))
        {
            edgeButton (c, CORNER_BOTTOM_LEFT, event);
        }
        else if (win == MYWINDOW_XWINDOW (c->corners[CORNER_BOTTOM_RIGHT]))
        {
            edgeButton (c, CORNER_BOTTOM_RIGHT, event);
        }
        else if (win == MYWINDOW_XWINDOW (c->sides[SIDE_BOTTOM]))
        {
            edgeButton (c, CORNER_COUNT + SIDE_BOTTOM, event);
        }
        else if (win == MYWINDOW_XWINDOW (c->sides[SIDE_TOP]))
        {
            edgeButton (c, CORNER_COUNT + SIDE_TOP, event);
        }
        else if (win == MYWINDOW_XWINDOW (c->sides[SIDE_LEFT]))
        {
            edgeButton (c, CORNER_COUNT + SIDE_LEFT, event);
        }
        else if (win == MYWINDOW_XWINDOW (c->sides[SIDE_RIGHT]))
        {
            edgeButton (c, CORNER_COUNT + SIDE_RIGHT, event);
        }
        else if (event->meta.window == c->window)
        {
            replay = TRUE;
            if (((screen_info->params->raise_with_any_button) && (c->type & WINDOW_REGULAR_FOCUSABLE))
                    || (event->button == Button1))
            {
                if (!(c->type & WINDOW_TYPE_DONT_FOCUS))
                {
                    clientSetFocus (screen_info, c, event->time, NO_FOCUS_FLAG);
                }
                if ((screen_info->params->raise_on_click) ||
                    !FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_BORDER))
                {
                    clientClearDelayedRaise ();
                    clientRaise (c, None);
                }
            }
        }
    }
    else
    {
        /*
           The event did not occur in one of our known good clients...
           Get the screen structure from the root of the event.
         */
        screen_info = myDisplayGetScreenFromRoot (display_info, event->root);
        if (screen_info)
        {
            if ((event->meta.window == screen_info->xroot) && (screen_info->params->scroll_workspaces)
                    && ((event->button == Button4) || (event->button == Button5)))
            {
                rootScrollButton (display_info, event);
            }
            else
            {
                myDisplayErrorTrapPush (display_info);
                xfwm_device_ungrab (display_info->devices, &display_info->devices->pointer,
                                    display_info->dpy, event->time);
                XSendEvent (display_info->dpy, screen_info->xfwm4_win, FALSE, SubstructureNotifyMask, event->meta.xevent);
                myDisplayErrorTrapPopIgnored (display_info);
            }
        }
    }

    /* Release pending events */
    XAllowEvents (display_info->dpy, replay ? ReplayPointer : SyncPointer, CurrentTime);

    return EVENT_FILTER_REMOVE;
}

static eventFilterStatus
handleButtonRelease (DisplayInfo *display_info, XfwmEventButton *event)
{
    ScreenInfo *screen_info;

    TRACE ("entering");

    /* Get the screen structure from the root of the event */
    screen_info = myDisplayGetScreenFromRoot (display_info, event->root);
    myDisplayErrorTrapPush (display_info);
    if (screen_info)
    {
        XSendEvent (display_info->dpy, screen_info->xfwm4_win, FALSE, SubstructureNotifyMask,
                    (XEvent *) event->meta.xevent);
    }

    /* Release pending events */
    XAllowEvents (display_info->dpy, SyncPointer, CurrentTime);
    myDisplayErrorTrapPopIgnored (display_info);

    return EVENT_FILTER_REMOVE;
}

static eventFilterStatus
handleDestroyNotify (DisplayInfo *display_info, XDestroyWindowEvent * ev)
{
    eventFilterStatus status;
    GList *list_of_windows;
    Client *c;

#ifdef ENABLE_KDE_SYSTRAY_PROXY
    ScreenInfo *screen_info;
#endif

    TRACE ("window (0x%lx)", ev->window);

    status = EVENT_FILTER_PASS;
#ifdef ENABLE_KDE_SYSTRAY_PROXY
    screen_info = myDisplayGetScreenFromSystray (display_info, ev->window);
    if (screen_info)
    {
        /* systray window is gone */
        screen_info->systray = None;
        return EVENT_FILTER_REMOVE;
    }
#endif

    c = myDisplayGetClientFromWindow (display_info, ev->window, SEARCH_WINDOW);
    if (c)
    {
        TRACE ("client \"%s\" (0x%lx)", c->name, c->window);
        list_of_windows = clientListTransientOrModal (c);
        clientPassFocus (c->screen_info, c, list_of_windows);
        clientUnframe (c, FALSE);
        g_list_free (list_of_windows);
        status = EVENT_FILTER_REMOVE;
    }

    return status;
}

static eventFilterStatus
handleMapRequest (DisplayInfo *display_info, XMapRequestEvent * ev)
{
    eventFilterStatus status;
    Client *c;

    TRACE ("window (0x%lx)", ev->window);

    status = EVENT_FILTER_PASS;
    if (ev->window == None)
    {
        TRACE ("mapping None ???");
        return status;
    }

    c = myDisplayGetClientFromWindow (display_info, ev->window, SEARCH_WINDOW);
    if (c)
    {
        ScreenInfo *screen_info = c->screen_info;

        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MAP_PENDING))
        {
            TRACE ("ignoring MapRequest on window (0x%lx)", ev->window);
        }

        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_WAS_SHOWN))
        {
             clientClearAllShowDesktop (screen_info);
        }
        clientShow (c, TRUE);

        if (FLAG_TEST (c->flags, CLIENT_FLAG_STICKY) ||
            (c->win_workspace == screen_info->current_ws))
        {
            clientFocusNew(c);
        }
    }
    else
    {
        clientFrame (display_info, ev->window, FALSE);
    }

    status = EVENT_FILTER_REMOVE;
    return status;
}

static eventFilterStatus
handleMapNotify (DisplayInfo *display_info, XMapEvent * ev)
{
    Client *c;

    TRACE ("window (0x%lx)", ev->window);

    c = myDisplayGetClientFromWindow (display_info, ev->window, SEARCH_WINDOW);
    if (c)
    {
        TRACE ("client \"%s\" (0x%lx)", c->name, c->window);
        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MAP_PENDING))
        {
            FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_MAP_PENDING);
        }
        return EVENT_FILTER_REMOVE;
    }

    return EVENT_FILTER_PASS;
}

static eventFilterStatus
handleUnmapNotify (DisplayInfo *display_info, XUnmapEvent * ev)
{
    eventFilterStatus status;
    ScreenInfo *screen_info;
    GList *list_of_windows;
    Client *c;

    TRACE ("window (0x%lx)", ev->window);

    status = EVENT_FILTER_PASS;
    if (ev->from_configure)
    {
        TRACE ("ignoring UnmapNotify caused by parent's resize");
        return status;
    }

    c = myDisplayGetClientFromWindow (display_info, ev->window, SEARCH_WINDOW);
    if (c)
    {
        TRACE ("client \"%s\" (0x%lx) ignore_unmap %i", c->name, c->window, c->ignore_unmap);

        screen_info = c->screen_info;
        if ((ev->event != ev->window) && (ev->event != screen_info->xroot || !ev->send_event))
        {
            TRACE ("event ignored");
            return status;
        }

        status = EVENT_FILTER_REMOVE;
        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MAP_PENDING))
        {
            /*
             * This UnmapNotify event is caused by reparenting
             * so we just ignore it, so the window won't return
             * to withdrawn state by mistake.
             */
            TRACE ("client \"%s\" is not mapped, event ignored", c->name);
            return status;
        }

        /*
         * ICCCM spec states that a client wishing to switch
         * to WithdrawnState should send a synthetic UnmapNotify
         * with the event field set to root if the client window
         * is already unmapped.
         * Therefore, bypass the ignore_unmap counter and
         * unframe the client.
         */
        if ((ev->event == screen_info->xroot) && (ev->send_event))
        {
            if (!FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE))
            {
                TRACE ("ICCCM UnmapNotify for \"%s\"", c->name);
                list_of_windows = clientListTransientOrModal (c);
                clientPassFocus (screen_info, c, list_of_windows);
                clientUnframe (c, FALSE);
                g_list_free (list_of_windows);
            }
            return status;
        }

        if (c->ignore_unmap)
        {
            c->ignore_unmap--;
            TRACE ("ignore_unmap for \"%s\" is now %i", c->name, c->ignore_unmap);
        }
        else
        {
            TRACE ("unmapping \"%s\" as ignore_unmap is %i", c->name, c->ignore_unmap);
            list_of_windows = clientListTransientOrModal (c);
            clientPassFocus (screen_info, c, list_of_windows);
            clientUnframe (c, FALSE);
            g_list_free (list_of_windows);
        }
    }

    return status;
}

static eventFilterStatus
handleConfigureNotify (DisplayInfo *display_info, XConfigureEvent * ev)
{
    TRACE ("entering");
    return EVENT_FILTER_PASS;
}

static eventFilterStatus
handleConfigureRequest (DisplayInfo *display_info, XConfigureRequestEvent * ev)
{
    Client *c;
    XWindowChanges wc;

    TRACE ("window (0x%lx)", ev->window);

    wc.x = ev->x;
    wc.y = ev->y;
    wc.width = ev->width;
    wc.height = ev->height;
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;
    wc.border_width = ev->border_width;

    c = myDisplayGetClientFromWindow (display_info, ev->window, SEARCH_WINDOW);
    if (!c)
    {
        /* Some app tend or try to manipulate the wm frame to achieve fullscreen mode */
        c = myDisplayGetClientFromWindow (display_info, ev->window, SEARCH_FRAME);
        if (c)
        {
            TRACE ("client %s (0x%lx) is attempting to manipulate its frame!", c->name, c->window);
            if (ev->value_mask & CWX)
            {
                wc.x += frameLeft (c);
            }
            if (ev->value_mask & CWY)
            {
                wc.y += frameTop (c);
            }
            if (ev->value_mask & CWWidth)
            {
                wc.width -= frameLeft (c) + frameRight (c);
            }
            if (ev->value_mask & CWHeight)
            {
                wc.height -= frameTop (c) + frameBottom (c);
            }
            /* We don't allow changing stacking order by accessing the frame
               window because that would break the layer management in xfwm4
             */
            ev->value_mask &= ~(CWSibling | CWStackMode);
        }
    }
    if (c)
    {
        TRACE ("window \"%s\" (0x%lx)", c->name, c->window);
        if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_MOVING_RESIZING))
        {
            /* Sorry, but it's not the right time for configure request */
            return EVENT_FILTER_REMOVE;
        }
        clientAdjustCoordGravity (c, c->gravity, &wc, &ev->value_mask);
        clientMoveResizeWindow (c, &wc, ev->value_mask);
    }
    else
    {
        TRACE ("unmanaged configure request for window 0x%lx", ev->window);
        myDisplayErrorTrapPush (display_info);
        XConfigureWindow (display_info->dpy, ev->window, ev->value_mask, &wc);
        myDisplayErrorTrapPopIgnored (display_info);
    }

    return EVENT_FILTER_REMOVE;
}

static eventFilterStatus
handleEnterNotify (DisplayInfo *display_info, XfwmEventCrossing *event)
{
    ScreenInfo *screen_info;
    Client *c;
    int b;
    gboolean need_redraw;

    /* See http://rfc-ref.org/RFC-TEXTS/1013/chapter12.html for details */

    TRACE ("entering");

    if ((event->mode == NotifyGrab) || (event->mode == NotifyUngrab)
        || (event->detail > NotifyNonlinearVirtual))
    {
        /* We're not interested in such notifications */
        return EVENT_FILTER_PASS;
    }

    TRACE ("window (0x%lx)", event->meta.window);

    need_redraw = FALSE;
    c = myDisplayGetClientFromWindow (display_info, event->meta.window,
                                      SEARCH_FRAME | SEARCH_BUTTON);
    if (c)
    {
        screen_info = c->screen_info;

        TRACE ("client \"%s\"", c->name);
        if (!(screen_info->params->click_to_focus) && clientAcceptFocus (c))
        {
            if (!(c->type & (WINDOW_DOCK | WINDOW_DESKTOP)))
            {
                if(screen_info->params->focus_delay)
                {
                    clientClearDelayedFocus ();
                    clientAddDelayedFocus (c, event->time);
                }
                else
                {
                    clientSetFocus (c->screen_info, c, event->time, NO_FOCUS_FLAG);
                }
            }
            else
            {
                clientClearDelayedFocus ();
            }
        }
        if (c == clientGetFocus ())
        {
            for (b = 0; b < BUTTON_COUNT; b++)
            {
                if (MYWINDOW_XWINDOW(c->buttons[b]) == event->meta.window)
                {
                    if (!xfwmPixmapNone(clientGetButtonPixmap(c, b, PRELIGHT)))
                    {
                        c->button_status[b] = BUTTON_STATE_PRELIGHT;
                        need_redraw = TRUE;
                    }
                }
            }
            if (need_redraw)
            {
                frameQueueDraw (c, FALSE);
            }
        }

        /* No need to process the event any further */
        return EVENT_FILTER_REMOVE;
    }

    /* The event was not for a client window */

    if (display_info->nb_screens > 1)
    {
        /* Wrap workspace/wrap windows is disabled with multiscreen */
        return EVENT_FILTER_REMOVE;
    }

    /* Get the screen structure from the root of the event */
    screen_info = myDisplayGetScreenFromRoot (display_info, event->root);

    if (!screen_info)
    {
        return EVENT_FILTER_PASS;
    }

    if (screen_info->params->wrap_workspaces && screen_info->workspace_count > 1)
    {
        clientMoveWarp (NULL, screen_info, &event->x_root, &event->y_root, event->time);
    }

    return EVENT_FILTER_REMOVE;
}

static eventFilterStatus
handleLeaveNotify (DisplayInfo *display_info, XfwmEventCrossing *event)
{
    Client *c;
    int b;
    gboolean need_redraw;

    TRACE ("entering");

    need_redraw = FALSE;
    c = myDisplayGetClientFromWindow (display_info, event->meta.window,
                                      SEARCH_FRAME | SEARCH_BUTTON);
    if (c)
    {
        for (b = 0; b < BUTTON_COUNT; b++)
        {
            if ((c->button_status[b] == BUTTON_STATE_PRELIGHT) || (c->button_status[b] == BUTTON_STATE_PRESSED))
            {
                if (MYWINDOW_XWINDOW(c->buttons[b]) == event->meta.window)
                {
                    c->button_status[b] = BUTTON_STATE_NORMAL;
                    need_redraw = TRUE;
                }
            }
        }
        if (need_redraw)
        {
            frameQueueDraw (c, FALSE);
        }

        /* No need to process the event any further */
        return EVENT_FILTER_REMOVE;
    }


    return EVENT_FILTER_PASS;
}

static eventFilterStatus
handleFocusIn (DisplayInfo *display_info, XFocusChangeEvent * ev)
{
    ScreenInfo *screen_info;
    Client *c, *user_focus, *current_focus;

    /* See http://rfc-ref.org/RFC-TEXTS/1013/chapter12.html for details */

    TRACE ("window (0x%lx) mode = %s",
                ev->window,
                (ev->mode == NotifyNormal) ?
                "NotifyNormal" :
                (ev->mode == NotifyWhileGrabbed) ?
                "NotifyWhileGrabbed" :
                (ev->mode == NotifyGrab) ?
                "NotifyGrab" :
                (ev->mode == NotifyUngrab) ?
                "NotifyUngrab" :
                "(unknown)");
    TRACE ("window (0x%lx) detail = %s",
                ev->window,
                (ev->detail == NotifyAncestor) ?
                "NotifyAncestor" :
                (ev->detail == NotifyVirtual) ?
                "NotifyVirtual" :
                (ev->detail == NotifyInferior) ?
                "NotifyInferior" :
                (ev->detail == NotifyNonlinear) ?
                "NotifyNonlinear" :
                (ev->detail == NotifyNonlinearVirtual) ?
                "NotifyNonlinearVirtual" :
                (ev->detail == NotifyPointer) ?
                "NotifyPointer" :
                (ev->detail == NotifyPointerRoot) ?
                "NotifyPointerRoot" :
                (ev->detail == NotifyDetailNone) ?
                "NotifyDetailNone" :
                "(unknown)");

    if ((ev->mode == NotifyGrab) || (ev->mode == NotifyUngrab))
    {
        /* We're not interested in such notifications */
        return EVENT_FILTER_PASS;
    }

    screen_info = myDisplayGetScreenFromWindow (display_info, ev->window);
    if (screen_info &&
        ((ev->detail == NotifyDetailNone) ||
         ((ev->mode == NotifyNormal) && (ev->detail == NotifyInferior))))
    {
        /*
           Handle unexpected focus transition to root (means that an unknown
           window has vanished and the focus is returned to the root).
         */
        c = clientGetFocusOrPending ();
        clientSetFocus (screen_info, c, getXServerTime (display_info), FOCUS_FORCE);
        return EVENT_FILTER_PASS;
    }

    c = myDisplayGetClientFromWindow (display_info, ev->window, SEARCH_FRAME | SEARCH_WINDOW);
    user_focus = clientGetUserFocus ();
    current_focus = clientGetFocus ();

    TRACE ("window (0x%lx)", ev->window);
    if ((c) && (c != current_focus) && (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE)))
    {
        TRACE ("focus transfered to \"%s\" (0x%lx)", c->name, c->window);

        screen_info = c->screen_info;

        clientUpdateFocus (screen_info, c, FOCUS_SORT);
        if ((user_focus != c) && (user_focus != NULL))
        {
            /*
               Focus stealing prevention:
               Some apps tend to focus the window directly. If focus stealing prevention is enabled,
               we revert the user set focus to the window that we think has focus and then set the
               demand attention flag.

               Note that focus stealing prevention is ignored between windows of the same group or
               between windows that have a transient relationship, as some apps tend to play with
               focus with their "own" windows.
             */

            if (screen_info->params->prevent_focus_stealing &&
                !clientSameGroup (c, user_focus) &&
                !clientIsTransientOrModalFor (c, user_focus))
            {
                TRACE ("setting focus back to \"%s\" (0x%lx)", user_focus->name, user_focus->window);
                clientSetFocus (user_focus->screen_info, user_focus, getXServerTime (display_info), NO_FOCUS_FLAG);

                if (current_focus)
                {
                    TRACE ("setting WM_STATE_DEMANDS_ATTENTION flag on \"%s\" (0x%lx)", c->name, c->window);
                    FLAG_SET (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION);
                    clientSetNetState (c);
                }
            }
        }
    }


    return EVENT_FILTER_REMOVE;
}

static eventFilterStatus
handleFocusOut (DisplayInfo *display_info, XFocusChangeEvent * ev)
{
    Client *c;

    /* See http://rfc-ref.org/RFC-TEXTS/1013/chapter12.html for details */

    TRACE ("window (0x%lx) mode = %s",
                ev->window,
                (ev->mode == NotifyNormal) ?
                "NotifyNormal" :
                (ev->mode == NotifyWhileGrabbed) ?
                "NotifyWhileGrabbed" :
                "(unknown)");
    TRACE ("window (0x%lx) detail = %s",
                ev->window,
                (ev->detail == NotifyAncestor) ?
                "NotifyAncestor" :
                (ev->detail == NotifyVirtual) ?
                "NotifyVirtual" :
                (ev->detail == NotifyInferior) ?
                "NotifyInferior" :
                (ev->detail == NotifyNonlinear) ?
                "NotifyNonlinear" :
                (ev->detail == NotifyNonlinearVirtual) ?
                "NotifyNonlinearVirtual" :
                (ev->detail == NotifyPointer) ?
                "NotifyPointer" :
                (ev->detail == NotifyPointerRoot) ?
                "NotifyPointerRoot" :
                (ev->detail == NotifyDetailNone) ?
                "NotifyDetailNone" :
                "(unknown)");

    if ((ev->mode == NotifyGrab) || (ev->mode == NotifyUngrab) ||
        (ev->detail == NotifyInferior) || (ev->detail > NotifyNonlinearVirtual))
    {
        /* We're not interested in such notifications */
        return EVENT_FILTER_PASS;
    }

    if ((ev->mode == NotifyNormal)
        && ((ev->detail == NotifyNonlinear)
            || (ev->detail == NotifyNonlinearVirtual)))
    {
        c = myDisplayGetClientFromWindow (display_info, ev->window, SEARCH_FRAME | SEARCH_WINDOW);
        TRACE ("window (0x%lx)", ev->window);
        if ((c) && (c == clientGetFocus ()))
        {
            TRACE ("focus lost from \"%s\" (0x%lx)", c->name, c->window);
            clientUpdateFocus (c->screen_info, NULL, NO_FOCUS_FLAG);
            clientClearDelayedRaise ();
        }
    }

    return EVENT_FILTER_REMOVE;
}

static eventFilterStatus
handlePropertyNotify (DisplayInfo *display_info, XPropertyEvent * ev)
{
    eventFilterStatus status;
    ScreenInfo *screen_info;
    Client *c;

    TRACE ("entering");

    status = EVENT_FILTER_PASS;
    c = myDisplayGetClientFromWindow (display_info, ev->window, SEARCH_WINDOW | SEARCH_WIN_USER_TIME);
    if (c)
    {
        status = EVENT_FILTER_REMOVE;
        screen_info = c->screen_info;
        if (ev->atom == XA_WM_NORMAL_HINTS)
        {
            TRACE ("client \"%s\" (0x%lx) has received a XA_WM_NORMAL_HINTS notify", c->name, c->window);
            clientGetWMNormalHints (c, TRUE);
        }
        else if ((ev->atom == XA_WM_NAME) ||
                 (ev->atom == display_info->atoms[NET_WM_NAME]) ||
                 (ev->atom == display_info->atoms[WM_CLIENT_MACHINE]))
        {
            TRACE ("client \"%s\" (0x%lx) has received a XA_WM_NAME/NET_WM_NAME/WM_CLIENT_MACHINE notify", c->name, c->window);
            clientUpdateName (c);
        }
        else if (ev->atom == display_info->atoms[MOTIF_WM_HINTS])
        {
            TRACE ("client \"%s\" (0x%lx) has received a MOTIF_WM_HINTS notify", c->name, c->window);
            clientGetMWMHints (c);
            clientApplyMWMHints (c, TRUE);
        }
        else if (ev->atom == XA_WM_HINTS)
        {
            TRACE ("client \"%s\" (0x%lx) has received a XA_WM_HINTS notify", c->name, c->window);

            /* Free previous wmhints if any */
            if (c->wmhints)
            {
                XFree (c->wmhints);
            }

            myDisplayErrorTrapPush (display_info);
            c->wmhints = XGetWMHints (display_info->dpy, c->window);
            myDisplayErrorTrapPopIgnored (display_info);

            if (c->wmhints)
            {
                if (c->wmhints->flags & WindowGroupHint)
                {
                    c->group_leader = c->wmhints->window_group;
                }
                if ((c->wmhints->flags & IconPixmapHint) && (screen_info->params->show_app_icon))
                {
                    clientUpdateIcon (c);
                }
                if (HINTS_ACCEPT_INPUT (c->wmhints))
                {
                    FLAG_SET (c->wm_flags, WM_FLAG_INPUT);
                }
                else
                {
                    FLAG_UNSET (c->wm_flags, WM_FLAG_INPUT);
                }
            }
            clientUpdateUrgency (c);
        }
        else if (ev->atom == display_info->atoms[WM_PROTOCOLS])
        {
            TRACE ("client \"%s\" (0x%lx) has received a WM_PROTOCOLS notify", c->name, c->window);
            clientGetWMProtocols (c);
        }
        else if (ev->atom == display_info->atoms[WM_TRANSIENT_FOR])
        {
            Window w;

            TRACE ("client \"%s\" (0x%lx) has received a WM_TRANSIENT_FOR notify", c->name, c->window);
            c->transient_for = None;
            getTransientFor (display_info, c->screen_info->xroot, c->window, &w);
            if (clientCheckTransientWindow (c, w))
            {
                c->transient_for = w;
            }
            /* Recompute window type as it may have changed */
            clientWindowType (c);
        }
        else if (ev->atom == display_info->atoms[NET_WM_WINDOW_TYPE])
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_WM_WINDOW_TYPE notify", c->name, c->window);
            clientGetNetWmType (c);
            frameQueueDraw (c, TRUE);
        }
        else if ((ev->atom == display_info->atoms[NET_WM_STRUT]) ||
                 (ev->atom == display_info->atoms[NET_WM_STRUT_PARTIAL]))
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_WM_STRUT notify", c->name, c->window);
            if (clientGetNetStruts (c) && FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE))
            {
                workspaceUpdateArea (c->screen_info);
            }
        }
        else if (ev->atom == display_info->atoms[WM_COLORMAP_WINDOWS])
        {
            TRACE ("client \"%s\" (0x%lx) has received a WM_COLORMAP_WINDOWS notify", c->name, c->window);
            clientUpdateColormaps (c);
            if (c == clientGetFocus ())
            {
                clientInstallColormaps (c);
            }
        }
        else if (ev->atom == display_info->atoms[NET_WM_USER_TIME])
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_WM_USER_TIME notify", c->name, c->window);
            clientGetUserTime (c);
        }
        else if (ev->atom == display_info->atoms[NET_WM_USER_TIME_WINDOW])
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_WM_USER_TIME_WINDOW notify", c->name, c->window);
            clientRemoveUserTimeWin (c);
            c->user_time_win = getNetWMUserTimeWindow(display_info, c->window);
            clientAddUserTimeWin (c);
        }
        else if (ev->atom == display_info->atoms[NET_WM_PID])
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_WM_PID notify", c->name, c->window);
            if (c->pid == 0)
            {
                c->pid = getWindowPID (display_info, c->window);
                TRACE ("client \"%s\" (0x%lx) updated PID = %i", c->name, c->window, c->pid);
            }
        }
        else if (ev->atom == display_info->atoms[NET_WM_WINDOW_OPACITY])
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_WM_WINDOW_OPACITY notify", c->name, c->window);
            if (!getOpacity (display_info, c->window, &c->opacity))
            {
                c->opacity =  NET_WM_OPAQUE;
            }
            clientSetOpacity (c, c->opacity, 0, 0);
        }
        else if (ev->atom == display_info->atoms[NET_WM_WINDOW_OPACITY_LOCKED])
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_WM_WINDOW_OPACITY_LOCKED notify", c->name, c->window);
            if (getOpacityLock (display_info, c->window))
            {
                FLAG_SET (c->xfwm_flags, XFWM_FLAG_OPACITY_LOCKED);
            }
            else
            {
                FLAG_UNSET (c->xfwm_flags, XFWM_FLAG_OPACITY_LOCKED);
            }
        }
        else if ((screen_info->params->show_app_icon) &&
                 ((ev->atom == display_info->atoms[NET_WM_ICON]) ||
                  (ev->atom == display_info->atoms[KWM_WIN_ICON])))
        {
            clientUpdateIcon (c);
        }
        else if (ev->atom == display_info->atoms[GTK_FRAME_EXTENTS])
        {
            TRACE ("client \"%s\" (0x%lx) has received a GTK_FRAME_EXTENTS notify", c->name, c->window);
            if (clientGetGtkFrameExtents (c))
            {
                if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED))
                {
                    clientUpdateMaximizeSize (c);
                }
                else if (c->tile_mode != TILE_NONE)
                {
                    clientUpdateTileSize (c);
                }
            }
        }
        else if (ev->atom == display_info->atoms[GTK_HIDE_TITLEBAR_WHEN_MAXIMIZED])
        {
            TRACE ("client \"%s\" (0x%lx) has received a GTK_HIDE_TITLEBAR_WHEN_MAXIMIZED notify", c->name, c->window);
            if (clientGetGtkHideTitlebar (c))
            {
                if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED))
                {
                    clientUpdateMaximizeSize (c);
                    clientReconfigure (c, CFG_FORCE_REDRAW);
                }
            }
        }
#ifdef HAVE_STARTUP_NOTIFICATION
        else if (ev->atom == display_info->atoms[NET_STARTUP_ID])
        {
            if (c->startup_id)
            {
                g_free (c->startup_id);
                c->startup_id = NULL;
            }
            getWindowStartupId (display_info, c->window, &c->startup_id);
        }
#endif /* HAVE_STARTUP_NOTIFICATION */
#ifdef HAVE_XSYNC
        else if (ev->atom == display_info->atoms[NET_WM_SYNC_REQUEST_COUNTER])
        {
            TRACE ("window 0x%lx has received NET_WM_SYNC_REQUEST_COUNTER", c->window);
            clientGetXSyncCounter (c);
        }
#endif /* HAVE_XSYNC */

        return status;
    }

    screen_info = myDisplayGetScreenFromWindow (display_info, ev->window);
    if (!screen_info)
    {
        return status;
    }

    if (ev->atom == display_info->atoms[NET_DESKTOP_NAMES])
    {
        gchar **names;
        guint items;

        TRACE ("root has received a NET_DESKTOP_NAMES notify");
        if (getUTF8StringList (display_info, screen_info->xroot, NET_DESKTOP_NAMES, &names, &items))
        {
            workspaceSetNames (screen_info, names, items);
        }
    }
    else if (ev->atom == display_info->atoms[NET_DESKTOP_LAYOUT])
    {
        TRACE ("root has received a NET_DESKTOP_LAYOUT notify");
        getDesktopLayout(display_info, screen_info->xroot, screen_info->workspace_count, &screen_info->desktop_layout);
        placeSidewalks(screen_info, screen_info->params->wrap_workspaces);
    }

    return status;
}

static eventFilterStatus
handleClientMessage (DisplayInfo *display_info, XClientMessageEvent * ev)
{
    eventFilterStatus status;
    ScreenInfo *screen_info;
    Client *c;

    TRACE ("window (0x%lx)", ev->window);

    if (ev->window == None)
    {
        /* Some do not set the window member, not much we can do without */
        return EVENT_FILTER_PASS;
    }
    status = EVENT_FILTER_PASS;
    c = myDisplayGetClientFromWindow (display_info, ev->window, SEARCH_WINDOW);
    if (c)
    {
        status = EVENT_FILTER_REMOVE;

        if ((ev->message_type == display_info->atoms[WM_CHANGE_STATE]) && (ev->format == 32) && (ev->data.l[0] == IconicState))
        {
            TRACE ("client \"%s\" (0x%lx) has received a WM_CHANGE_STATE event", c->name, c->window);
            if (!FLAG_TEST (c->flags, CLIENT_FLAG_ICONIFIED))
            {
                clientWithdraw (c, c->win_workspace, TRUE);
            }
        }
        else if ((ev->message_type == display_info->atoms[NET_WM_DESKTOP]) && (ev->format == 32))
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_WM_DESKTOP event", c->name, c->window);
            clientUpdateNetWmDesktop (c, ev);
        }
        else if ((ev->message_type == display_info->atoms[NET_CLOSE_WINDOW]) && (ev->format == 32))
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_CLOSE_WINDOW event", c->name, c->window);
            clientClose (c);
        }
        else if ((ev->message_type == display_info->atoms[NET_WM_STATE]) && (ev->format == 32))
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_WM_STATE event", c->name, c->window);
            clientUpdateNetState (c, ev);
        }
        else if ((ev->message_type == display_info->atoms[NET_WM_MOVERESIZE]) && (ev->format == 32))
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_WM_MOVERESIZE event", c->name, c->window);
            clientNetMoveResize (c, ev);
        }
        else if ((ev->message_type == display_info->atoms[NET_MOVERESIZE_WINDOW]) && (ev->format == 32))
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_MOVERESIZE_WINDOW event", c->name, c->window);
            clientNetMoveResizeWindow (c, ev);
        }
        else if ((ev->message_type == display_info->atoms[NET_ACTIVE_WINDOW]) && (ev->format == 32))
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_ACTIVE_WINDOW event", c->name, c->window);
            clientHandleNetActiveWindow (c, (guint32) ev->data.l[1], (gboolean) (ev->data.l[0] == 1));
        }
        else if (ev->message_type == display_info->atoms[NET_REQUEST_FRAME_EXTENTS])
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_REQUEST_FRAME_EXTENTS event", c->name, c->window);
            setNetFrameExtents (display_info, c->window, frameTop (c), frameLeft (c),
                                                         frameRight (c), frameBottom (c));
        }
        else if (ev->message_type == display_info->atoms[NET_WM_FULLSCREEN_MONITORS])
        {
            TRACE ("client \"%s\" (0x%lx) has received a NET_WM_FULLSCREEN_MONITORS event", c->name, c->window);
            clientSetFullscreenMonitor (c, (gint) ev->data.l[0], (gint) ev->data.l[1],
                                           (gint) ev->data.l[2], (gint) ev->data.l[3]);
        }
        else if ((ev->message_type == display_info->atoms[GTK_SHOW_WINDOW_MENU]) && (ev->format == 32))
        {
            TRACE ("client \"%s\" (0x%lx) has received a GTK_SHOW_WINDOW_MENU event", c->name, c->window);
            show_window_menu (c, (gint) ev->data.l[1], (gint) ev->data.l[2], Button3, (Time) myDisplayGetCurrentTime (display_info), TRUE);
        }
    }
    else
    {
        screen_info = myDisplayGetScreenFromWindow (display_info, ev->window);
        if (!screen_info)
        {
            return status;
        }

        status = EVENT_FILTER_REMOVE;
        if ((ev->message_type == display_info->atoms[NET_CURRENT_DESKTOP]) && (ev->format == 32))
        {
            TRACE ("root has received a win_workspace or a NET_CURRENT_DESKTOP event %li", ev->data.l[0]);
            if ((ev->data.l[0] >= 0) && (ev->data.l[0] < (long) screen_info->workspace_count) &&
                (ev->data.l[0] != (long) screen_info->current_ws))
            {
                workspaceSwitch (screen_info, ev->data.l[0], NULL, TRUE,
                                 myDisplayGetTime (display_info, (guint32) ev->data.l[1]));
            }
        }
        else if ((ev->message_type == display_info->atoms[NET_NUMBER_OF_DESKTOPS]) && (ev->format == 32))
        {
            TRACE ("root has received a win_workspace_count event");
            if (ev->data.l[0] != (long) screen_info->workspace_count)
            {
                workspaceSetCount (screen_info, ev->data.l[0]);
                getDesktopLayout(display_info, screen_info->xroot, screen_info->workspace_count, &screen_info->desktop_layout);
            }
        }
        else if ((ev->message_type == display_info->atoms[NET_SHOWING_DESKTOP]) && (ev->format == 32))
        {
            TRACE ("root has received a NET_SHOWING_DESKTOP event");
            screen_info->show_desktop = (ev->data.l[0] != 0);
            clientToggleShowDesktop (screen_info);
            setHint (display_info, screen_info->xroot, NET_SHOWING_DESKTOP, ev->data.l[0]);
        }
        else if (ev->message_type == display_info->atoms[NET_REQUEST_FRAME_EXTENTS])
        {
            TRACE ("window (0x%lx) has received a NET_REQUEST_FRAME_EXTENTS event", ev->window);
            /* Size estimate from the decoration extents */
            setNetFrameExtents (display_info, ev->window,
                                frameDecorationTop (screen_info),
                                frameDecorationLeft (screen_info),
                                frameDecorationRight (screen_info),
                                frameDecorationBottom (screen_info));
        }
        else if ((ev->message_type == display_info->atoms[MANAGER]) && (ev->format == 32))
        {
            Atom selection;

            TRACE ("window (0x%lx) has received a MANAGER event", ev->window);
            selection = (Atom) ev->data.l[1];
#ifdef ENABLE_KDE_SYSTRAY_PROXY
            if (selection == screen_info->net_system_tray_selection)
            {
                TRACE ("root has received a NET_SYSTEM_TRAY_MANAGER selection event");
                screen_info->systray = getSystrayWindow (display_info, screen_info->net_system_tray_selection);
            }
            else
#endif
            if (myScreenCheckWMAtom (screen_info, selection))
            {
                TRACE ("root has received a WM_Sn selection event");
                display_info->quit = TRUE;
            }
        }
        else if (ev->message_type == display_info->atoms[WM_PROTOCOLS])
        {
            if ((Atom) ev->data.l[0] == display_info->atoms[NET_WM_PING])
            {
                TRACE ("root has received a NET_WM_PING (pong) event\n");
                clientReceiveNetWMPong (screen_info, (guint32) ev->data.l[1]);
            }
        }
        else if (ev->message_type == display_info->atoms[GTK_READ_RCFILES])
        {
            TRACE ("window (0x%lx) has received a GTK_READ_RCFILES event", ev->window);
            set_reload (display_info);
        }
        else
        {
            TRACE ("unidentified client message for window 0x%lx", ev->window);
        }
    }

    return status;
}

static eventFilterStatus
handleSelectionClear (DisplayInfo *display_info, XSelectionClearEvent * ev)
{
    eventFilterStatus status;
    ScreenInfo *screen_info, *pscreen;
    GSList *list;

    DBG ("entering handleSelectionClear 0x%lx", ev->window);

    status = EVENT_FILTER_PASS;
    screen_info = NULL;

    for (list = display_info->screens; list; list = g_slist_next (list))
    {
        pscreen = (ScreenInfo *) list->data;
        if (ev->window == pscreen->xfwm4_win)
        {
            screen_info = pscreen;
            break;
        }
    }

    if (screen_info)
    {
        if (myScreenCheckWMAtom (screen_info, ev->selection))
        {
            TRACE ("root has received a WM_Sn selection event");
            display_info->quit = TRUE;
            status = EVENT_FILTER_REMOVE;
        }
    }

    return status;
}

static eventFilterStatus
handleShape (DisplayInfo *display_info, XShapeEvent * ev)
{
    Client *c;
    gboolean update;

    TRACE ("entering");

    c = myDisplayGetClientFromWindow (display_info, ev->window, SEARCH_WINDOW);
    if (c)
    {
        update = FALSE;
        if (ev->kind == ShapeInput)
        {
            frameSetShapeInput (c);
            update = TRUE;
        }
        else if (ev->kind == ShapeBounding)
        {
            if ((ev->shaped) && !FLAG_TEST (c->flags, CLIENT_FLAG_HAS_SHAPE))
            {
                update = TRUE;
                FLAG_SET (c->flags, CLIENT_FLAG_HAS_SHAPE);
                clientGetMWMHints (c);
                clientApplyMWMHints (c, TRUE);
            }
            else if (!(ev->shaped) && FLAG_TEST (c->flags, CLIENT_FLAG_HAS_SHAPE))
            {
                update = TRUE;
                FLAG_UNSET (c->flags, CLIENT_FLAG_HAS_SHAPE);
                clientGetMWMHints (c);
                clientApplyMWMHints (c, TRUE);
            }
        }
        if (!update)
        {
            frameQueueDraw (c, FALSE);
        }
    }

    return EVENT_FILTER_REMOVE;
}

static eventFilterStatus
handleColormapNotify (DisplayInfo *display_info, XColormapEvent * ev)
{
    Client *c;

    TRACE ("entering");

    c = myDisplayGetClientFromWindow (display_info, ev->window, SEARCH_WINDOW);
    if ((c) && (ev->window == c->window) && (ev->new))
    {
        if (c == clientGetFocus ())
        {
            clientInstallColormaps (c);
        }
        return EVENT_FILTER_REMOVE;
    }

    return EVENT_FILTER_PASS;
}

static eventFilterStatus
handleReparentNotify (DisplayInfo *display_info, XReparentEvent * ev)
{
    TRACE ("window 0x%lx reparented in 0x%lx", ev->window, ev->parent);
    return EVENT_FILTER_PASS;
}

#ifdef HAVE_XSYNC
static eventFilterStatus
handleXSyncAlarmNotify (DisplayInfo *display_info, XSyncAlarmNotifyEvent * ev)
{
    Client *c;

    TRACE ("entering");
    if (display_info->have_xsync)
    {
        c = myDisplayGetClientFromXSyncAlarm (display_info, ev->alarm);
        if (c)
        {
            clientXSyncUpdateValue (c, ev->counter_value);
        }
    }
    return EVENT_FILTER_REMOVE;
}
#endif /* HAVE_XSYNC */

static eventFilterStatus
handleEvent (DisplayInfo *display_info, XfwmEvent *event)
{
    eventFilterStatus status;
    status = EVENT_FILTER_PASS;

    TRACE ("entering");

    /* Update the display time */
    myDisplayUpdateCurrentTime (display_info, event);
    sn_process_event (event->meta.xevent);

    switch (event->meta.type)
    {
        case XFWM_EVENT_KEY:
            if (event->key.pressed)
            {
                status = handleKeyPress (display_info, &event->key);
            }
            else
            {
                status = handleKeyRelease (display_info, &event->key);
            }
            break;
        case XFWM_EVENT_BUTTON:
            if (event->button.pressed)
            {
                status = handleButtonPress (display_info, &event->button);
            }
            else
            {
                status = handleButtonRelease (display_info, &event->button);
            }
            break;
        case XFWM_EVENT_MOTION:
            status = handleMotionNotify (display_info, &event->motion);
            break;
        case XFWM_EVENT_CROSSING:
            if (event->crossing.enter)
            {
                status = handleEnterNotify (display_info, &event->crossing);
            }
            else
            {
                status = handleLeaveNotify (display_info, &event->crossing);
            }
            break;
        case XFWM_EVENT_XEVENT:
            switch (event->meta.xevent->type)
            {
                case DestroyNotify:
                    status = handleDestroyNotify (display_info, (XDestroyWindowEvent *) event->meta.xevent);
                    break;
                case UnmapNotify:
                    status = handleUnmapNotify (display_info, (XUnmapEvent *) event->meta.xevent);
                    break;
                case MapRequest:
                    status = handleMapRequest (display_info, (XMapRequestEvent *) event->meta.xevent);
                    break;
                case MapNotify:
                    status = handleMapNotify (display_info, (XMapEvent *) event->meta.xevent);
                    break;
                case ConfigureNotify:
                    status = handleConfigureNotify (display_info, (XConfigureEvent *) event->meta.xevent);
                    break;
                case ConfigureRequest:
                    status = handleConfigureRequest (display_info, (XConfigureRequestEvent *) event->meta.xevent);
                    break;
                case FocusIn:
                    status = handleFocusIn (display_info, (XFocusChangeEvent *) event->meta.xevent);
                    break;
                case FocusOut:
                    status = handleFocusOut (display_info, (XFocusChangeEvent *) event->meta.xevent);
                    break;
                case PropertyNotify:
                    status = handlePropertyNotify (display_info, (XPropertyEvent *) event->meta.xevent);
                    break;
                case ClientMessage:
                    status = handleClientMessage (display_info, (XClientMessageEvent *) event->meta.xevent);
                    break;
                case SelectionClear:
                    status = handleSelectionClear (display_info, (XSelectionClearEvent *) event->meta.xevent);
                    break;
                case ColormapNotify:
                    handleColormapNotify (display_info, (XColormapEvent *) event->meta.xevent);
                    break;
                case ReparentNotify:
                    status = handleReparentNotify (display_info, (XReparentEvent *) event->meta.xevent);
                    break;
                default:
                    if ((display_info->have_shape) &&
                            (event->meta.xevent->type == display_info->shape_event_base))
                    {
                        status = handleShape (display_info, (XShapeEvent *) event->meta.xevent);
                    }
#ifdef HAVE_XSYNC
                    if ((display_info->have_xsync) &&
                            (event->meta.xevent->type == (display_info->xsync_event_base + XSyncAlarmNotify)))
                    {
                        status = handleXSyncAlarmNotify (display_info, (XSyncAlarmNotifyEvent *) event->meta.xevent);
                    }
#endif /* HAVE_XSYNC */
                    break;
            }
            break;
    }
    if (!gdk_events_pending () && !XPending (display_info->dpy))
    {
        if (display_info->reload)
        {
            reloadSettings (display_info, UPDATE_ALL);
            display_info->reload = FALSE;
        }
        else if (display_info->quit)
        {
            /*
             * Qutting on purpose, update session manager so
             * it does not restart the program immediately
             */
            xfce_sm_client_set_restart_style(display_info->session, XFCE_SM_CLIENT_RESTART_NORMAL);
            gtk_main_quit ();
        }
    }

    compositorHandleEvent (display_info, event->meta.xevent);

    return status;
}

eventFilterStatus
xfwm4_event_filter (XfwmEvent *event, gpointer data)
{
    eventFilterStatus status;
    DisplayInfo *display_info;

    display_info = (DisplayInfo *) data;

    TRACE ("entering");
    status = handleEvent (display_info, event);
    TRACE ("leaving");
    return EVENT_FILTER_STOP | status;
}

/* GTK specific stuff */

static void
menu_callback (Menu * menu, MenuOp op, Window xid, gpointer menu_data, gpointer item_data)
{
    Client *c;

    TRACE ("entering");

    if (!xfwmWindowDeleted(&menu_event_window))
    {
        xfwmWindowDelete (&menu_event_window);
    }

    c = NULL;
    if ((menu_data != NULL) && (xid != None))
    {
        ScreenInfo *screen_info = (ScreenInfo *) menu_data;
        c = myScreenGetClientFromWindow (screen_info, xid, SEARCH_WINDOW);
    }

    if (c)
    {
        c->button_status[MENU_BUTTON] = BUTTON_STATE_NORMAL;

        switch (op)
        {
            case MENU_OP_QUIT:
                gtk_main_quit ();
                break;
            case MENU_OP_MAXIMIZE:
            case MENU_OP_UNMAXIMIZE:
                if (CLIENT_CAN_MAXIMIZE_WINDOW (c))
                {
                    clientToggleMaximized (c, CLIENT_FLAG_MAXIMIZED, TRUE);
                }
                break;
            case MENU_OP_MINIMIZE:
                if (CLIENT_CAN_HIDE_WINDOW (c))
                {
                    clientWithdraw (c, c->win_workspace, TRUE);
                }
                break;
            case MENU_OP_MOVE:
                clientMove (c, NULL);
                break;
            case MENU_OP_RESIZE:
                clientResize (c, CORNER_BOTTOM_RIGHT, NULL);
                break;
            case MENU_OP_MINIMIZE_ALL:
                clientWithdrawAll (c, c->win_workspace);
                break;
            case MENU_OP_UNMINIMIZE:
                if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_WAS_SHOWN))
                {
                    clientClearAllShowDesktop (c->screen_info);
                }
                clientShow (c, TRUE);
                break;
            case MENU_OP_SHADE:
            case MENU_OP_UNSHADE:
                clientToggleShaded (c);
                break;
            case MENU_OP_STICK:
                clientToggleSticky (c, TRUE);
                frameQueueDraw (c, FALSE);
                break;
            case MENU_OP_WORKSPACES:
                clientSetWorkspace (c, GPOINTER_TO_INT (item_data), TRUE);
                break;
            case MENU_OP_DELETE:
                clientClose (c);
                break;
            case MENU_OP_CONTEXT_HELP:
                clientEnterContextMenuState (c);
                break;
            case MENU_OP_ABOVE:
                clientToggleLayerAbove (c);
                break;
            case MENU_OP_NORMAL:
                clientSetLayerNormal (c);
                break;
            case MENU_OP_BELOW:
                clientToggleLayerBelow (c);
                break;
            case MENU_OP_FULLSCREEN:
            case MENU_OP_UNFULLSCREEN:
                clientToggleFullscreen (c);
                break;
            default:
                frameQueueDraw (c, FALSE);
                break;
        }
    }
    else
    {
        gdk_display_beep (gdk_display_get_default ());
    }
    menu_free (menu);
}

void
initMenuEventWin (void)
{
    xfwmWindowInit (&menu_event_window);
}

static void
show_window_menu (Client *c, gint px, gint py, guint button, guint32 timestamp, gboolean needscale)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    Menu *menu;
    MenuOp ops;
    MenuOp insensitive;
    gboolean is_transient;
    gint x, y;
    gint scale = 1;

    TRACE ("coords (%d,%d)", px, py);

    if ((button != Button1) && (button != Button3))
    {
        return;
    }

    if (!c || !FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE))
    {
        return;
    }

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    is_transient = clientIsValidTransientOrModal (c);
    scale = gdk_window_get_scale_factor (gdk_screen_get_root_window (screen_info->gscr));

    x = px;
    y = py;

    if (needscale) {
        x /= scale;
        y /= scale;
    }

    c->button_status[MENU_BUTTON] = BUTTON_STATE_PRESSED;
    frameQueueDraw (c, FALSE);
    if (CLIENT_HAS_FRAME (c))
    {
        x = px;
        y = c->y / scale;
        if (needscale) {
            x /= scale;
        }
    }
    ops = MENU_OP_DELETE | MENU_OP_MINIMIZE_ALL | MENU_OP_WORKSPACES | MENU_OP_MOVE | MENU_OP_RESIZE;
    insensitive = 0;

    if (FLAG_TEST (c->flags, CLIENT_FLAG_MAXIMIZED))
    {
        ops |= MENU_OP_UNMAXIMIZE;
    }
    else
    {
        ops |= MENU_OP_MAXIMIZE;
    }

    if (!FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_MOVE))
    {
        insensitive |= MENU_OP_MOVE;
    }

    if (FLAG_TEST (c->flags, CLIENT_FLAG_ICONIFIED))
    {
        ops |= MENU_OP_UNMINIMIZE;
    }
    else
    {
        ops |= MENU_OP_MINIMIZE;
    }

    if (FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
    {
        ops |= MENU_OP_UNSHADE;
    }
    else
    {
        ops |= MENU_OP_SHADE;
    }

    if (!FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
    {
        ops |= MENU_OP_STICK;
    }

    if (!FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_BORDER))
    {
        insensitive |= MENU_OP_SHADE | MENU_OP_UNSHADE;
    }


    if (!FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_CLOSE))
    {
        insensitive |= MENU_OP_DELETE;
    }

    if (is_transient || !FLAG_TEST(c->xfwm_flags, XFWM_FLAG_HAS_STICK))
    {
        insensitive |= MENU_OP_STICK;
    }

    if (!CLIENT_CAN_HIDE_WINDOW (c))
    {
        insensitive |= MENU_OP_MINIMIZE;
    }

    if (!CLIENT_CAN_MAXIMIZE_WINDOW (c))
    {
        insensitive |= MENU_OP_MAXIMIZE;
    }

    if (!FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_MOVE))
    {
        insensitive |= MENU_OP_MOVE;
    }

    if (!FLAG_TEST_ALL (c->xfwm_flags, XFWM_FLAG_HAS_RESIZE | XFWM_FLAG_IS_RESIZABLE) ||
        FLAG_TEST_ALL (c->flags, CLIENT_FLAG_MAXIMIZED))
    {
        insensitive |= MENU_OP_RESIZE;
    }

    if (FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        insensitive |= MENU_OP_SHADE | MENU_OP_MOVE | MENU_OP_RESIZE | MENU_OP_MAXIMIZE | MENU_OP_UNMAXIMIZE;
    }

    if (FLAG_TEST(c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        ops |= MENU_OP_UNFULLSCREEN;
    }
    else
    {
        ops |= MENU_OP_FULLSCREEN;
    }

    if (is_transient || (c->type != WINDOW_NORMAL))
    {
        insensitive |= MENU_OP_FULLSCREEN | MENU_OP_UNFULLSCREEN;
    }

    if (FLAG_TEST(c->flags, CLIENT_FLAG_ABOVE))
    {
        ops |= MENU_OP_NORMAL | MENU_OP_BELOW;
    }
    else if (FLAG_TEST(c->flags, CLIENT_FLAG_BELOW))
    {
        ops |= MENU_OP_NORMAL | MENU_OP_ABOVE;
    }
    else
    {
        ops |= MENU_OP_ABOVE | MENU_OP_BELOW;
    }

    if (is_transient ||
        !(c->type & WINDOW_REGULAR_FOCUSABLE) ||
        FLAG_TEST (c->flags, CLIENT_FLAG_FULLSCREEN))
    {
        insensitive |= MENU_OP_NORMAL | MENU_OP_ABOVE | MENU_OP_BELOW;
    }

    /* KDE extension */
    clientGetWMProtocols(c);
    if (FLAG_TEST (c->wm_flags, WM_FLAG_CONTEXT_HELP))
    {
        ops |= MENU_OP_CONTEXT_HELP;
    }

    if (is_transient
        || !FLAG_TEST (c->xfwm_flags, XFWM_FLAG_HAS_STICK)
        || FLAG_TEST (c->flags, CLIENT_FLAG_STICKY))
    {
        insensitive |= MENU_OP_WORKSPACES;
    }

    if (screen_info->button_handler_id)
    {
        g_signal_handler_disconnect (G_OBJECT (myScreenGetGtkWidget (screen_info)), screen_info->button_handler_id);
    }
    screen_info->button_handler_id = g_signal_connect (G_OBJECT (myScreenGetGtkWidget (screen_info)),
                                              "button_press_event", G_CALLBACK (show_popup_cb), (gpointer) NULL);

    if (!xfwmWindowDeleted(&menu_event_window))
    {
        xfwmWindowDelete (&menu_event_window);
    }
    /*
       Since all button press/release events are catched by the windows frames, there is some
       side effect with GTK menu. When a menu is opened, any click on the window frame is not
       detected as a click outside the menu, and the menu doesn't close.
       To avoid this (painless but annoying) behavior, we just setup a no event window that
       "hides" the events to regular windows.
       That might look tricky, but it's very efficient and save plenty of lines of complicated
       code.
       Don't forget to delete that window once the menu is closed, though, or we'll get in
       trouble.
     */
    xfwmWindowTemp (screen_info,
                    NULL, 0,
                    screen_info->xroot,
                    &menu_event_window, 0, 0,
                    screen_info->width,
                    screen_info->height,
                    NoEventMask,
                    FALSE);
    menu = menu_default (screen_info->gscr, c->window, ops, insensitive, menu_callback,
                         c->win_workspace, screen_info->workspace_count,
                         screen_info->workspace_names, screen_info->workspace_names_items,
                         display_info->xfilter, screen_info);

    if (!menu_popup (menu, x, y, button, timestamp))
    {
        TRACE ("cannot open menu");
        gdk_display_beep (display_info->gdisplay);
        c->button_status[MENU_BUTTON] = BUTTON_STATE_NORMAL;
        frameQueueDraw (c, FALSE);
        xfwmWindowDelete (&menu_event_window);
        menu_free (menu);
    }
}

static gboolean
show_popup_cb (GtkWidget * widget, GdkEventButton * ev, gpointer data)
{
    TRACE ("entering");

    show_window_menu ((Client *) data, (gint) ev->x_root, (gint) ev->y_root, ev->button, ev->time, FALSE);

    return TRUE;
}

static gboolean
set_reload (DisplayInfo *display_info)
{
    TRACE ("setting reload flag so all prefs will be reread at next event loop");

    display_info->reload = TRUE;
    return TRUE;
}

static gboolean
double_click_time_cb (GObject * obj, GdkEvent * ev, gpointer data)
{
    DisplayInfo *display_info;
    GValue tmp_val = { 0, };

    display_info = (DisplayInfo *) data;
    g_return_val_if_fail (display_info, TRUE);

    g_value_init (&tmp_val, G_TYPE_INT);
    if (gdk_setting_get ("gtk-double-click-time", &tmp_val))
    {
        display_info->double_click_time = abs (g_value_get_int (&tmp_val));
    }

    return TRUE;
}

static gboolean
double_click_distance_cb (GObject * obj, GdkEvent * ev, gpointer data)
{
    DisplayInfo *display_info;
    GValue tmp_val = { 0, };

    display_info = (DisplayInfo *) data;
    g_return_val_if_fail (display_info, TRUE);

    g_value_init (&tmp_val, G_TYPE_INT);
    if (gdk_setting_get ("gtk-double-click-distance", &tmp_val))
    {
        display_info->double_click_distance = abs (g_value_get_int (&tmp_val));
    }

    return TRUE;
}

static void
cursor_theme_cb (GObject * obj, GParamSpec * pspec, gpointer data)
{
    DisplayInfo * display_info;
    GSList *list;

    display_info = (DisplayInfo *) data;
    g_return_if_fail (display_info);

    myDisplayFreeCursor (display_info);
    myDisplayCreateCursor (display_info);

    for (list = display_info->screens; list; list = g_slist_next (list))
    {
        ScreenInfo *screen_info = (ScreenInfo *) list->data;
        clientUpdateAllCursor (screen_info);
        XDefineCursor (display_info->dpy, screen_info->xroot, display_info->root_cursor);
   }
}

static void
update_screen_font (ScreenInfo *screen_info)
{
    myScreenUpdateFontAttr (screen_info);
    clientUpdateAllFrames (screen_info, UPDATE_FRAME);
}

static gboolean
refresh_font_cb (GObject * obj, GdkEvent * ev, gpointer data)
{
    DisplayInfo * display_info;
    GSList *list;

    display_info = (DisplayInfo *) data;
    g_return_val_if_fail (display_info, TRUE);

    for (list = display_info->screens; list; list = g_slist_next (list))
    {
        update_screen_font (list->data);
    }

    return TRUE;
}

/*
 * The size-changed signal is emitted when the pixel width or height
 * of a screen changes.
 */
static void
size_changed_cb(GdkScreen *gscreen, gpointer data)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    gboolean size_changed;

    TRACE ("entering");

    screen_info = (ScreenInfo *) data;
    g_return_if_fail (screen_info);
    display_info = screen_info->display_info;

    if (xfwm_get_n_monitors (screen_info->gscr) == 0)
    {
        /*
         * Recent Xorg drivers disable the output when the lid
         * is closed, leaving no active monitor, in that case simply
         * ignore the event to avoid messing with windows' positions.
         */
        return;
    }

    size_changed = myScreenComputeSize (screen_info);
    if (size_changed)
    {
        myScreenInvalidateMonitorCache (screen_info);

        setNetWorkarea (display_info, screen_info->xroot, screen_info->workspace_count,
                        screen_info->width, screen_info->height, screen_info->margins);
        setNetDesktopInfo (display_info, screen_info->xroot, screen_info->current_ws,
                           screen_info->width, screen_info->height);

        placeSidewalks (screen_info, screen_info->params->wrap_workspaces);

        compositorUpdateScreenSize (screen_info);
    }

    clientScreenResize (screen_info, FALSE);
}

/*
 * The monitors-changed signal is emitted when the number, size or
 * position of the monitors attached to the screen change.
 */
static void
monitors_changed_cb(GdkScreen *gscreen, gpointer data)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    gint previous_num_monitors;
    gboolean size_changed;

    TRACE ("entering");

    screen_info = (ScreenInfo *) data;
    g_return_if_fail (screen_info);
    display_info = screen_info->display_info;

    if (xfwm_get_n_monitors (screen_info->gscr) == 0)
    {
        /*
         * Recent Xorg drivers disable the output when the lid
         * is closed, leaving no active monitor, in that case simply
         * ignore the event to avoid messing with windows' positions.
         */
        return;
    }

    /*
     * We have added/removed a monitor or even changed the layout,
     * the cache for monitor position we use in our screen structure
     * is not valid anymore and potentially refers to a monitor that
     * was just removed, so invalidate it.
     */
    previous_num_monitors = screen_info->num_monitors;
    myScreenInvalidateMonitorCache (screen_info);
    myScreenRebuildMonitorIndex (screen_info);
    size_changed = myScreenComputeSize (screen_info);

    if (size_changed || (screen_info->num_monitors != previous_num_monitors))
    {

        setNetWorkarea (display_info, screen_info->xroot, screen_info->workspace_count,
                        screen_info->width, screen_info->height, screen_info->margins);
        setNetDesktopInfo (display_info, screen_info->xroot, screen_info->current_ws,
                           screen_info->width, screen_info->height);

        placeSidewalks (screen_info, screen_info->params->wrap_workspaces);
    }

    if (size_changed)
    {
        compositorUpdateScreenSize (screen_info);
    }

    clientScreenResize (screen_info, (screen_info->num_monitors < previous_num_monitors));
}

void
initPerScreenCallbacks (ScreenInfo *screen_info)
{
    g_return_if_fail (screen_info);

    screen_info->button_handler_id =
        g_signal_connect (G_OBJECT (myScreenGetGtkWidget (screen_info)),
                          "button_press_event", G_CALLBACK (show_popup_cb), (gpointer) NULL);
    g_object_connect (G_OBJECT(screen_info->gscr),
                      "signal::size-changed",
                      G_CALLBACK(size_changed_cb), (gpointer) (screen_info),
                      "signal::monitors-changed",
                      G_CALLBACK(monitors_changed_cb), (gpointer) (screen_info),
                      NULL);

    g_signal_connect_swapped (G_OBJECT (myScreenGetGtkWidget (screen_info)),
                              "notify::scale-factor",
                              G_CALLBACK (update_screen_font),
                              screen_info);
}

void
initPerDisplayCallbacks (DisplayInfo *display_info)
{
    GtkSettings *settings;

    g_return_if_fail (display_info);

    settings = gtk_settings_get_default ();
    g_object_connect (settings,
                      "swapped-signal::notify::gtk-theme-name",
                      G_CALLBACK (set_reload), (gpointer) (display_info),
                      "swapped-signal::notify::gtk-font-name",
                      G_CALLBACK (set_reload), (gpointer) (display_info),
                      "signal::notify::gtk-double-click-time",
                      G_CALLBACK (double_click_time_cb), (gpointer) (display_info),
                      "signal::notify::gtk-double-click-distance",
                      G_CALLBACK (double_click_distance_cb), (gpointer) (display_info),
                      "signal::notify::gtk-cursor-theme-name",
                      G_CALLBACK (cursor_theme_cb), (gpointer) (display_info),
                      "signal::notify::gtk-cursor-theme-size",
                      G_CALLBACK (cursor_theme_cb), (gpointer) (display_info),
                      "signal::notify::gtk-xft-antialias",
                      G_CALLBACK (refresh_font_cb), (gpointer) (display_info),
                      "signal::notify::gtk-xft-dpi",
                      G_CALLBACK (refresh_font_cb), (gpointer) (display_info),
                      "signal::notify::gtk-xft-hinting",
                      G_CALLBACK (refresh_font_cb), (gpointer) (display_info),
                      "signal::notify::gtk-xft-hintstyle",
                      G_CALLBACK (refresh_font_cb), (gpointer) (display_info),
                      "signal::notify::gtk-xft-rgba",
                      G_CALLBACK (refresh_font_cb), (gpointer) (display_info),
                      NULL);
}

