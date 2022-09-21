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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include "screen.h"
#include "focus.h"
#include "misc.h"
#include "client.h"
#include "frame.h"
#include "stacking.h"
#include "transients.h"
#include "workspaces.h"
#include "hints.h"
#include "netwm.h"

typedef struct _ClientPair ClientPair;
struct _ClientPair
{
    Client *prefered;
    Client *highest;
};

static Client *client_focus  = NULL;
static Client *pending_focus = NULL;
static Client *user_focus    = NULL;
static Client *delayed_focus = NULL;
static guint focus_timeout   = 0;

#if 0
static void
clientDumpList (ScreenInfo *screen_info)
{
    int i;
    Client *c;

    g_print ("Dumping client list\n");
    for (c = screen_info->clients, i = 0; (c) && (i < screen_info->client_count); c = c->next, i++)
    {
        g_print ("    [%i] 0x%lx - %s\n", i, c->window, c->name);
    }
}
#endif

static ClientPair
clientGetTopMostFocusable (ScreenInfo *screen_info, guint layer, GList * exclude_list)
{
    ClientPair top_client;
    Client *c;
    GList *list;

    TRACE ("entering");

    top_client.prefered = top_client.highest = NULL;
    for (list = screen_info->windows_stack; list; list = g_list_next (list))
    {
        c = (Client *) list->data;
        TRACE ("stack window \"%s\" (0x%lx), layer %i", c->name,
            c->window, (int) c->win_layer);

        if (!clientAcceptFocus (c) || (c->type & WINDOW_TYPE_DONT_FOCUS))
        {
            continue;
        }

        if (!g_list_find (exclude_list, (gconstpointer) c))
        {
            if (c->win_layer > layer)
            {
                break;
            }
            else if (FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE))
            {
                if (clientSelectMask (c, NULL, 0, WINDOW_REGULAR_FOCUSABLE))
                {
                    top_client.prefered = c;
                }
                top_client.highest = c;
            }
        }
    }

    return top_client;
}

void
clientFocusTop (ScreenInfo *screen_info, guint layer, guint32 timestamp)
{
    ClientPair top_client;

    top_client = clientGetTopMostFocusable (screen_info, layer, NULL);
    if (top_client.prefered)
    {
        clientSetFocus (screen_info, top_client.prefered,
                        timestamp,
                        NO_FOCUS_FLAG);
    }
    else
    {
        clientSetFocus (screen_info, top_client.highest,
                        timestamp,
                        NO_FOCUS_FLAG);
    }
}

gboolean
clientFocusNew(Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    gboolean give_focus;
    gboolean prevent_focus_stealing;
    gboolean prevented;

    g_return_val_if_fail (c != NULL, FALSE);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    give_focus = (c-> type & WINDOW_REGULAR_FOCUSABLE) && (screen_info->params->focus_new);
    prevent_focus_stealing = screen_info->params->prevent_focus_stealing;
    prevented = FALSE;

    /*  Try to avoid focus stealing */
    if (!clientAcceptFocus (c) || (c->type & WINDOW_TYPE_DONT_FOCUS))
    {
        give_focus = FALSE;
    }
    else if (FLAG_TEST (c->flags, CLIENT_FLAG_HAS_USER_TIME) && (c->user_time == (guint32) 0))
    {
        /*
         *  _NET_WM_USER_TIME definition from http://standards.freedesktop.org/wm-spec
         * [...] "The special value of zero on a newly mapped window can be used to
         * request that the window not be initially focused when it is mapped."
         */
        TRACE ("given startup time is nil, not focusing \"%s\"", c->name);
        give_focus = FALSE;
        prevented = FALSE;
    }
    else if ((client_focus) && (prevent_focus_stealing))
    {
        if (client_focus->win_layer > c->win_layer)
        {
            TRACE ("not focusing \"%s\" because the current focused window is on a upper layer", c->name);
            give_focus = FALSE;
            prevented = TRUE;
        }
        else if (client_focus->win_layer < c->win_layer)
        {
            /* We don't use focus stealing prevention against upper layers */
            TRACE ("ignoring startup prevention because the current focused window is on a lower layer");
            give_focus = TRUE;
            prevented = FALSE;
        }
        else if (FLAG_TEST (client_focus->xfwm_flags, XFWM_FLAG_MOVING_RESIZING))
        {
            give_focus = FALSE;
            prevented = TRUE;
        }
        else if (FLAG_TEST (c->flags, CLIENT_FLAG_HAS_STARTUP_TIME | CLIENT_FLAG_HAS_USER_TIME))
        {
            TRACE ("current time is %u, time for \"%s\" is %u",
                   (unsigned int) client_focus->user_time,
                   c->name, (unsigned int) c->user_time);
            if (TIMESTAMP_IS_BEFORE (c->user_time, client_focus->user_time))
            {
                give_focus = FALSE;
                prevented = TRUE;
            }
        }
    }
    if (FLAG_TEST(c->flags, CLIENT_FLAG_STATE_MODAL))
    {
        give_focus = TRUE;
    }
    if (give_focus)
    {
        if (client_focus)
        {
            clientAdjustFullscreenLayer (client_focus, FALSE);
        }
        clientRaise (c, None);
        clientShow (c, TRUE);
        clientSetFocus (screen_info, c,
                        myDisplayGetCurrentTime (display_info),
                        FOCUS_IGNORE_MODAL);
    }
    else
    {
        Client *c2 = clientGetFocus();

        clientSortRing(c);
        if ((c2 != NULL) && (c2->win_layer == c->win_layer) && prevented)
        {
            /*
             * Place windows under the currently focused only if focus
             * stealing prevention had prevented the focus transition,
             * otherwise, leave the unfocused window on top.
             */
            clientLower (c, c2->frame);
        }
        else
        {
            clientRaise (c, None);
        }
        clientSortRing(c2);

        if (prevented)
        {
            TRACE ("setting WM_STATE_DEMANDS_ATTENTION flag on \"%s\" (0x%lx)", c->name, c->window);
            FLAG_SET (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION);
        }

        clientShow (c, TRUE);
        clientSetNetState (c);
    }

    return (give_focus);
}

gboolean
clientSelectMask (Client * c, Client *other, guint mask, guint type)
{
    g_return_val_if_fail (c != NULL, FALSE);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if ((mask & SEARCH_SAME_APPLICATION) && !clientSameApplication (c, other))
    {
        return FALSE;
    }
    if ((mask & SEARCH_DIFFERENT_APPLICATION) && clientSameApplication (c, other))
    {
        return FALSE;
    }
    if (!(mask & SEARCH_INCLUDE_SKIP_FOCUS) && !clientAcceptFocus (c))
    {
        return FALSE;
    }
    if (!(mask & SEARCH_INCLUDE_HIDDEN) && FLAG_TEST (c->flags, CLIENT_FLAG_ICONIFIED))
    {
        return FALSE;
    }
    if (!(mask & SEARCH_INCLUDE_ALL_WORKSPACES) && (c->win_workspace != c->screen_info->current_ws))
    {
        return FALSE;
    }
    if (!(mask & SEARCH_INCLUDE_SKIP_PAGER) && FLAG_TEST (c->flags, CLIENT_FLAG_SKIP_PAGER))
    {
        return FALSE;
    }
    if (!(mask & SEARCH_INCLUDE_SKIP_TASKBAR) && FLAG_TEST (c->flags, CLIENT_FLAG_SKIP_TASKBAR))
    {
        return FALSE;
    }
    if (c->type & type)
    {
        return TRUE;
    }

    return (c->type & type);
}

Client *
clientGetNext (Client * c, guint mask, guint type)
{
    Client *c2;
    unsigned int i;

    TRACE ("entering clientGetNext");

    if (c)
    {
        ScreenInfo *screen_info = c->screen_info;
        for (c2 = c->next, i = 0; (c2) && (i < screen_info->client_count - 1);
            c2 = c2->next, i++)
        {
            if (clientSelectMask (c2, c, mask, type))
            {
                return c2;
            }
        }
    }
    return NULL;
}

Client *
clientGetPrevious (Client * c, guint mask, guint type)
{
    Client *c2;
    unsigned int i;

    TRACE ("entering");

    if (c)
    {
        ScreenInfo *screen_info = c->screen_info;
        for (c2 = c->prev, i = 0; (c2) && (i < screen_info->client_count);
            c2 = c2->prev, i++)
        {
            if (clientSelectMask (c2, c, mask, type))
            {
                return c2;
            }
        }
    }
    return NULL;
}

void
clientPassFocus (ScreenInfo *screen_info, Client *c, GList *exclude_list)
{
    DisplayInfo *display_info;
    ClientPair top_most;
    Client *new_focus;
    Client *current_focus;
    Window dr, window;
    unsigned int mask;
    int rx, ry, wx, wy;
    int look_in_layer;

    TRACE ("entering");

    look_in_layer = (c ? c->win_layer : WIN_LAYER_NORMAL);
    new_focus = NULL;
    current_focus = clientGetFocusOrPending ();

    if ((current_focus != NULL) && (c != current_focus))
    {
        return;
    }

    display_info = screen_info->display_info;
    top_most = clientGetTopMostFocusable (screen_info, look_in_layer, exclude_list);

    if (!(screen_info->params->click_to_focus) &&
        XQueryPointer (myScreenGetXDisplay (screen_info), screen_info->xroot, &dr, &window, &rx, &ry, &wx, &wy, &mask))
    {
        new_focus = clientAtPosition (screen_info, rx, ry, exclude_list);
    }
    if (!new_focus)
    {
        new_focus = top_most.prefered ? top_most.prefered : top_most.highest;
    }
    clientSetFocus (screen_info, new_focus,
                    myDisplayGetCurrentTime (display_info),
                    FOCUS_IGNORE_MODAL | FOCUS_FORCE | FOCUS_TRANSITION);
}

gboolean
clientAcceptFocus (Client * c)
{
    g_return_val_if_fail (c != NULL, FALSE);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    /* Modal dialogs *always* accept focus */
    if (FLAG_TEST(c->flags, CLIENT_FLAG_STATE_MODAL))
    {
        return TRUE;
    }
    if ((c->screen_info->params->focus_hint)
        && !FLAG_TEST (c->wm_flags, WM_FLAG_INPUT | WM_FLAG_TAKEFOCUS))
    {
        return FALSE;
    }

    return TRUE;
}

void
clientSortRing(Client *c)
{
    ScreenInfo *screen_info;

    TRACE ("entering");
    if (c == NULL)
    {
        return;
    }
    screen_info = c->screen_info;
    if ((screen_info->client_count > 2) && (c != screen_info->clients))
    {
        c->prev->next = c->next;
        c->next->prev = c->prev;

        c->prev = screen_info->clients->prev;
        c->next = screen_info->clients;

        screen_info->clients->prev->next = c;
        screen_info->clients->prev = c;
    }
    screen_info->clients = c;
}

void
clientSetLast(Client *c)
{
    ScreenInfo *screen_info;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    if (screen_info->client_count > 2)
    {
        if (screen_info->clients == c)
        {
            screen_info->clients = screen_info->clients->next;
        }
        else
        {
            c->prev->next = c->next;
            c->next->prev = c->prev;

            c->prev = screen_info->clients->prev;
            c->next = screen_info->clients;

            screen_info->clients->prev->next = c;
            screen_info->clients->prev = c;
        }
    }
}

static void
clientFocusNone (ScreenInfo *screen_info, Client *previous, guint32 timestamp)
{
    TRACE ("setting focus to none");

    pending_focus = NULL;

    if (previous)
    {
        clientSetNetState (previous);
        frameQueueDraw (previous, FALSE);
        if (previous->screen_info != screen_info)
        {
            clientSetNetActiveWindow (previous->screen_info, NULL, timestamp);
        }
    }
    clientSetNetActiveWindow (screen_info, NULL, timestamp);
    XSetInputFocus (myScreenGetXDisplay (screen_info), screen_info->xfwm4_win, RevertToPointerRoot, timestamp);
}

void
clientUpdateFocus (ScreenInfo *screen_info, Client * c, unsigned short flags)
{
    Client *c2;
    gboolean restacked;

    TRACE ("entering");

    c2 = ((client_focus != c) ? client_focus : NULL);
    if ((c) && !clientAcceptFocus (c))
    {
        TRACE ("SKIP_FOCUS set for client \"%s\" (0x%lx)", c->name, c->window);
        pending_focus = NULL;
        return;
    }

    if ((c) && (c == client_focus) && !(flags & FOCUS_FORCE))
    {
        TRACE ("client \"%s\" (0x%lx) is already focused, ignoring request", c->name, c->window);
        pending_focus = NULL;
        return;
    }

    client_focus = c;
    if (c2)
    {
        clientSetNetState (c2);
        clientAdjustFullscreenLayer (c2, FALSE);
        frameQueueDraw (c2, FALSE);
        clientUpdateOpacity (c2);
    }
    if (c)
    {
        user_focus = c;
        clientInstallColormaps (c);
        if (flags & FOCUS_SORT)
        {
            clientSortRing(c);
        }
        if (FLAG_TEST(c->flags, CLIENT_FLAG_DEMANDS_ATTENTION))
        {
            TRACE ("un-setting WM_STATE_DEMANDS_ATTENTION flag on \"%s\" (0x%lx)", c->name, c->window);
            FLAG_UNSET (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION);
        }
        clientSetNetState (c);
        restacked = clientAdjustFullscreenLayer (c, TRUE);

        if (!restacked && screen_info->params->raise_on_focus)
        {
            if (screen_info->params->click_to_focus)
            {
                clientRaise (c, None);
                clientSetLastRaise (c);
            }
            else
            {
                clientResetDelayedRaise (screen_info);
            }
        }
        frameQueueDraw (c, FALSE);
        clientUpdateOpacity (c);
    }
    clientSetNetActiveWindow (screen_info, c, 0);
    pending_focus = NULL;
}

void
clientSetFocus (ScreenInfo *screen_info, Client *c, guint32 timestamp, unsigned short flags)
{
    Client *c2;

    TRACE ("entering");

    c2 = NULL;
    if ((c) && !(flags & FOCUS_IGNORE_MODAL))
    {
        c2 = clientGetModalFor (c);

        if (c2)
        {
            c = c2;
        }
    }
    c2 = ((client_focus != c) ? client_focus : NULL);
    if ((c) && FLAG_TEST (c->xfwm_flags, XFWM_FLAG_VISIBLE))
    {
        TRACE ("setting focus to client \"%s\" (0x%lx) with timestamp %u", c->name, c->window, (unsigned int) timestamp);
        user_focus = c;
        if (FLAG_TEST(c->flags, CLIENT_FLAG_DEMANDS_ATTENTION))
        {
            TRACE ("un-setting WM_STATE_DEMANDS_ATTENTION flag on \"%s\" (0x%lx)", c->name, c->window);
            FLAG_UNSET (c->flags, CLIENT_FLAG_DEMANDS_ATTENTION);
            clientSetNetState (c);
        }
        if ((c == client_focus) && !(flags & FOCUS_FORCE))
        {
            TRACE ("client \"%s\" (0x%lx) is already focused, ignoring request", c->name, c->window);
            return;
        }
        if (!clientAcceptFocus (c))
        {
            TRACE ("SKIP_FOCUS set for client \"%s\" (0x%lx)", c->name, c->window);
            return;
        }
        if (FLAG_TEST (c->wm_flags, WM_FLAG_INPUT) || !(screen_info->params->focus_hint))
        {
            pending_focus = c;
            /*
             * When shaded, the client window is unmapped, so it can not be focused.
             * Instead, we focus the frame that is still mapped.
             */
            myDisplayErrorTrapPush (screen_info->display_info);
            if (FLAG_TEST (c->flags, CLIENT_FLAG_SHADED))
            {
                XSetInputFocus (myScreenGetXDisplay (screen_info), c->frame, RevertToPointerRoot, timestamp);
            }
            else
            {
                XSetInputFocus (myScreenGetXDisplay (screen_info), c->window, RevertToPointerRoot, timestamp);
            }
            if (myDisplayErrorTrapPop (screen_info->display_info) != Success)
            {
                client_focus = NULL;
                clientFocusNone (screen_info, c2, timestamp);
                clientClearDelayedFocus ();
            }
        }
        else if (flags & FOCUS_TRANSITION)
        {
            /*
             * If we are relying only on the client application to take focus, we need to set the focus
             * explicitely on our own fallback window otherwise there is a race condition between the
             * application and the window manager. If the application does not take focus before the
             * the previously focused window is unmapped (when iconifying or closing for example), the focus
             * will be reverted to the root window and focus transition will fail.
             */
            clientFocusNone (screen_info, c2, timestamp);
        }

        if (FLAG_TEST(c->wm_flags, WM_FLAG_TAKEFOCUS))
        {
            pending_focus = c;
            sendClientMessage (screen_info, c->window, WM_TAKE_FOCUS, timestamp);
        }
    }
    else
    {
        TRACE ("setting focus to none");

        client_focus = NULL;
        clientFocusNone (screen_info, c2, timestamp);
        clientClearDelayedFocus ();
    }
}

void
clientInitFocusFlag (Client * c)
{
    ScreenInfo *screen_info;
    Client *c2;
    GList *list;
    guint workspace;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (!clientAcceptFocus (c) || (c->type & WINDOW_TYPE_DONT_FOCUS))
    {
       return;
    }

    screen_info = c->screen_info;
    workspace = c->win_workspace;
    for (list = screen_info->windows_stack; list; list = g_list_next (list))
    {
        c2 = (Client *) list->data;
        if ((c2->win_workspace == workspace) && FLAG_TEST (c2->xfwm_flags, XFWM_FLAG_FOCUS))
        {
            FLAG_UNSET (c2->xfwm_flags, XFWM_FLAG_FOCUS);
        }
    }
    FLAG_SET (c->xfwm_flags, XFWM_FLAG_FOCUS);
}

Client *
clientGetFocus (void)
{
    return (client_focus);
}

Client *
clientGetFocusPending (void)
{
    return (pending_focus);
}

Client *
clientGetFocusOrPending (void)
{
    if (client_focus)
    {
        return (client_focus);
    }
    return (pending_focus);
}

Client *
clientGetUserFocus (void)
{
    return (user_focus);
}

void
clientClearFocus (Client *c)
{
    if ((c == NULL) || (c == client_focus))
    {
        client_focus = NULL;
    }
    if ((c == NULL) || (c == pending_focus))
    {
        pending_focus = NULL;
    }
    if ((c == NULL) || (c == user_focus))
    {
        user_focus = NULL;
    }
}

void
clientGrabMouseButton (Client * c)
{
    ScreenInfo *screen_info;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    if (screen_info->params->raise_with_any_button)
    {
        grabButton (screen_info->display_info->devices, clientGetXDisplay (c),
                    AnyButton, AnyModifier, c->window);
    }
    else
    {
        grabButton (screen_info->display_info->devices, clientGetXDisplay (c),
                    Button1, AnyModifier, c->window);
    }
}

void
clientUngrabMouseButton (Client * c)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    ungrabButton (c->screen_info->display_info->devices, clientGetXDisplay (c),
                  AnyButton, AnyModifier, c->window);
    /* We've ungrabbed way too much, so regrab the regular buttons/modifiers */
    clientGrabButtons (c);
}

void
clientGrabMouseButtonForAll (ScreenInfo *screen_info)
{
    Client *c;
    guint i;

    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    for (c = screen_info->clients, i = 0; (c) && (i < screen_info->client_count); c = c->next, i++)
    {
        clientGrabMouseButton (c);
    }
}

void
clientUngrabMouseButtonForAll (ScreenInfo *screen_info)
{
    Client *c;
    guint i;

    g_return_if_fail (screen_info != NULL);
    TRACE ("entering");

    for (c = screen_info->clients, i = 0; (c) && (i < screen_info->client_count); c = c->next, i++)
    {
        clientUngrabMouseButton (c);
    }
}

static gboolean
delayed_focus_cb (gpointer data)
{
    ScreenInfo *screen_info;
    guint32 timestamp = (guint32) GPOINTER_TO_INT (data);

    g_return_val_if_fail (delayed_focus != NULL, FALSE);
    TRACE ("client \"%s\" (0x%lx)", delayed_focus->name, delayed_focus->window);

    screen_info = delayed_focus->screen_info;
    clientSetFocus (screen_info, delayed_focus, timestamp, NO_FOCUS_FLAG);
    focus_timeout = 0;
    delayed_focus = NULL;

    return FALSE;
}

void
clientClearDelayedFocus (void)
{
    if (focus_timeout)
    {
        g_source_remove (focus_timeout);
        focus_timeout = 0;
    }
    delayed_focus = NULL;
}

void
clientAddDelayedFocus (Client *c, guint32 timestamp)
{
    ScreenInfo *screen_info;

    screen_info = c->screen_info;
    delayed_focus = c;
    focus_timeout = g_timeout_add_full (G_PRIORITY_DEFAULT,
                                        screen_info->params->focus_delay,
                                        delayed_focus_cb,
                                        GINT_TO_POINTER (timestamp), NULL);
}

Client *
clientGetDelayedFocus (void)
{
    return delayed_focus;
}
