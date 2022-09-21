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


        xfwm4    - (c) 2002-2015 Olivier Fourdan

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xsync.h"

#ifdef HAVE_XSYNC

/* See http://fishsoup.net/misc/wm-spec-synchronization.html */
#ifndef XSYNC_VALUE_INCREMENT
#define XSYNC_VALUE_INCREMENT 240
#endif

static void
addToXSyncValue (XSyncValue *value, gint i)
{
    XSyncValue add;
    int overflow;

    XSyncIntToValue (&add, i);
    XSyncValueAdd (value, *value, add, &overflow);
}

gboolean
clientCreateXSyncAlarm (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    XSyncAlarmAttributes attrs;

    g_return_val_if_fail (c != NULL, FALSE);
    g_return_val_if_fail (c->xsync_counter != None, FALSE);

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    clientDestroyXSyncAlarm (c);
    if (FLAG_TEST (c->flags, CLIENT_FLAG_XSYNC_EXT_COUNTER))
    {
        /* Get the counter value from the client, if not, bail out... */
        if (!XSyncQueryCounter(display_info->dpy, c->xsync_counter, &c->xsync_value))
        {
            FLAG_UNSET (c->flags, CLIENT_FLAG_XSYNC_EXT_COUNTER);
            return FALSE;
        }
    }
    else
    {
        XSyncIntToValue (&c->xsync_value, 0);
        XSyncSetCounter (display_info->dpy, c->xsync_counter, c->xsync_value);
    }

    c->next_xsync_value = c->xsync_value;
    if (!FLAG_TEST (c->flags, CLIENT_FLAG_XSYNC_EXT_COUNTER) ||
        (XSyncValueLow32(c->next_xsync_value) % 2 == 0))
    {
        addToXSyncValue (&c->next_xsync_value, 1);
    }

    attrs.trigger.counter = c->xsync_counter;
    XSyncIntToValue (&attrs.delta, 1);
    XSyncIntToValue (&attrs.trigger.wait_value, 1);
    attrs.trigger.value_type = XSyncRelative;
    attrs.trigger.test_type = XSyncPositiveComparison;
    attrs.events = TRUE;
    c->xsync_alarm = XSyncCreateAlarm (display_info->dpy,
                                       XSyncCACounter |
                                       XSyncCADelta |
                                       XSyncCAEvents |
                                       XSyncCATestType |
                                       XSyncCAValue |
                                       XSyncCAValueType,
                                       &attrs);
    return (c->xsync_alarm != None);
}

void
clientDestroyXSyncAlarm (Client *c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    clientXSyncClearTimeout (c);
    if (c->xsync_alarm != None)
    {
        screen_info = c->screen_info;
        display_info = screen_info->display_info;

        myDisplayErrorTrapPush (display_info);
        XSyncDestroyAlarm (display_info->dpy, c->xsync_alarm);
        myDisplayErrorTrapPopIgnored (display_info);
        c->xsync_alarm = None;
    }
}

gboolean
clientGetXSyncCounter (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    gulong *data;
    int nitems;

    g_return_val_if_fail (c != NULL, FALSE);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    data = NULL;
    if (getCardinalList (display_info, c->window, NET_WM_SYNC_REQUEST_COUNTER, &data, &nitems))
    {
        switch (nitems)
        {
            case 0:
                FLAG_UNSET (c->flags, CLIENT_FLAG_XSYNC_EXT_COUNTER);
                FLAG_UNSET (c->flags, CLIENT_FLAG_XSYNC_ENABLED);
                break;
            case 1:
                c->xsync_counter = (XSyncCounter) data[0];
                FLAG_UNSET (c->flags, CLIENT_FLAG_XSYNC_EXT_COUNTER);
                FLAG_SET (c->flags, CLIENT_FLAG_XSYNC_ENABLED);
                break;
            default:
                c->xsync_counter = (XSyncCounter) data[1];
                FLAG_SET (c->flags, CLIENT_FLAG_XSYNC_EXT_COUNTER);
                FLAG_SET (c->flags, CLIENT_FLAG_XSYNC_ENABLED);
                break;
        }
    }

    if (data)
    {
        XFree (data);
    }

    return FLAG_TEST (c->flags, CLIENT_FLAG_XSYNC_ENABLED);
}

void
clientXSyncClearTimeout (Client * c)
{
    g_return_if_fail (c != NULL);

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    FLAG_UNSET (c->flags, CLIENT_FLAG_XSYNC_WAITING);
    if (c->xsync_timeout_id)
    {
        g_source_remove (c->xsync_timeout_id);
        c->xsync_timeout_id = 0;
    }
}

static gboolean
clientXSyncTimeout (gpointer data)
{
    Client *c;

    TRACE ("entering");

    c = (Client *) data;
    if (c)
    {
        g_warning ("XSync timeout for client \"%s\" (0x%lx)", c->name, c->window);
        clientXSyncClearTimeout (c);

        /* Disable XSync for this client */
        FLAG_UNSET (c->flags, CLIENT_FLAG_XSYNC_ENABLED);
    }
    return FALSE;
}

static void
clientXSyncResetTimeout (Client * c)
{
    g_return_if_fail (c != NULL);

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    clientXSyncClearTimeout (c);
    c->xsync_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT,
                                              CLIENT_XSYNC_TIMEOUT,
                                              clientXSyncTimeout, c, NULL);
}

void
clientXSyncRequest (Client * c)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    XSyncValue next_value;
    XClientMessageEvent xev;

    g_return_if_fail (c != NULL);
    g_return_if_fail (c->window != None);

    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    if (FLAG_TEST (c->flags, CLIENT_FLAG_XSYNC_WAITING))
    {
        return;
    }

    screen_info = c->screen_info;
    display_info = screen_info->display_info;

    next_value = c->next_xsync_value;
    addToXSyncValue (&next_value, XSYNC_VALUE_INCREMENT);
    c->next_xsync_value = next_value;

    xev.type = ClientMessage;
    xev.window = c->window;
    xev.message_type = display_info->atoms[WM_PROTOCOLS];
    xev.format = 32;
    xev.data.l[0] = (long) display_info->atoms[NET_WM_SYNC_REQUEST];
    xev.data.l[1] = (long) myDisplayGetCurrentTime (display_info);
    xev.data.l[2] = (long) XSyncValueLow32 (next_value);
    xev.data.l[3] = (long) XSyncValueHigh32 (next_value);
    xev.data.l[4] = (long) (FLAG_TEST (c->flags, CLIENT_FLAG_XSYNC_EXT_COUNTER) ? 1 : 0);
    XSendEvent (display_info->dpy, c->window, FALSE, NoEventMask, (XEvent *) &xev);

    clientXSyncResetTimeout (c);
    FLAG_SET (c->flags, CLIENT_FLAG_XSYNC_WAITING);
}

void
clientXSyncUpdateValue (Client *c, XSyncValue value)
{
    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    c->xsync_value = value;
    if (FLAG_TEST (c->flags, CLIENT_FLAG_XSYNC_EXT_COUNTER))
    {
        if (XSyncValueLow32(c->xsync_value) % 2 == 0)
        {
            addToXSyncValue (&value, 1);
        }
    }
    c->next_xsync_value = value;
    clientXSyncClearTimeout (c);
}

#endif /* HAVE_XSYNC */
