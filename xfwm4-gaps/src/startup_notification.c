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


        Metacity - (c) 2003 Havoc Pennington
        xfwm4    - (c) 2002-2011 Olivier Fourdan

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <libsn/sn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include "display.h"
#include "screen.h"
#include "client.h"
#include "startup_notification.h"

#define STARTUP_TIMEOUT (10 /* seconds */ * 1000)

static SnDisplay *sn_display = NULL;

typedef struct
{
    GSList *list;
    gint64 now;
}
CollectTimedOutData;

static gboolean sn_startup_sequence_timeout (void *data);

static void
sn_error_trap_push (SnDisplay * d, Display * dpy)
{
    myDisplayErrorTrapPush (myDisplayGetDefault ());
}

static void
sn_error_trap_pop (SnDisplay * d, Display * dpy)
{
    myDisplayErrorTrapPopIgnored (myDisplayGetDefault ());
}

static void
sn_update_feedback (ScreenInfo *screen_info)
{
    g_return_if_fail (screen_info != NULL);

    if (screen_info->startup_sequences != NULL)
    {
        XDefineCursor (myScreenGetXDisplay (screen_info), screen_info->xroot,
                       myDisplayGetCursorBusy(screen_info->display_info));
    }
    else
    {
        XDefineCursor (myScreenGetXDisplay (screen_info), screen_info->xroot,
                       myDisplayGetCursorRoot(screen_info->display_info));
    }
}

static void
sn_add_sequence (ScreenInfo *screen_info, SnStartupSequence * sequence)
{
    g_return_if_fail (screen_info != NULL);
    g_return_if_fail (sequence != NULL);

    sn_startup_sequence_ref (sequence);
    screen_info->startup_sequences = g_slist_prepend (screen_info->startup_sequences, sequence);

    if (screen_info->startup_sequence_timeout == 0)
    {
        screen_info->startup_sequence_timeout =
            g_timeout_add (1000, sn_startup_sequence_timeout, screen_info);
    }
    sn_update_feedback (screen_info);
}

static void
sn_remove_sequence (ScreenInfo *screen_info, SnStartupSequence * sequence)
{
    g_return_if_fail (screen_info != NULL);
    g_return_if_fail (sequence != NULL);

    screen_info->startup_sequences = g_slist_remove (screen_info->startup_sequences, sequence);
    sn_startup_sequence_unref (sequence);

    if ((screen_info->startup_sequences == NULL) && (screen_info->startup_sequence_timeout != 0))
    {
        g_source_remove (screen_info->startup_sequence_timeout);
        screen_info->startup_sequence_timeout = 0;
    }
    sn_update_feedback (screen_info);
}

static void
sn_collect_timed_out_foreach (void *element, void *data)
{
    CollectTimedOutData *ctod;
    SnStartupSequence *sequence;
    time_t tv_sec;
    suseconds_t tv_usec;
    long l_sec, l_usec;
    double elapsed;

    g_return_if_fail (data != NULL);
    g_return_if_fail (element != NULL);

    sequence = element;
    ctod = (CollectTimedOutData *) data;
    sn_startup_sequence_get_last_active_time (sequence, &l_sec, &l_usec);
    tv_sec = l_sec; tv_usec = l_sec;

    elapsed = ((double) ctod->now - (tv_sec * G_USEC_PER_SEC) - tv_usec)
        / 1000.0;

    if (elapsed > STARTUP_TIMEOUT)
    {
        ctod->list = g_slist_prepend (ctod->list, sequence);
    }
}

static gboolean
sn_startup_sequence_timeout (void *data)
{
    ScreenInfo * screen_info;
    CollectTimedOutData ctod;
    GSList *tmp;

    screen_info = (ScreenInfo *) data;
    g_return_val_if_fail (screen_info != NULL, FALSE);

    ctod.list = NULL;
    ctod.now = g_get_real_time ();
    g_slist_foreach (screen_info->startup_sequences, sn_collect_timed_out_foreach, &ctod);

    tmp = ctod.list;
    while (tmp != NULL)
    {
        SnStartupSequence *sequence = tmp->data;

        sn_startup_sequence_complete (sequence);

        tmp = tmp->next;
    }

    g_slist_free (ctod.list);

    if (screen_info->startup_sequences != NULL)
    {
        return TRUE;
    }
    else
    {
        /* remove */
        screen_info->startup_sequence_timeout = 0;
        return FALSE;
    }
}

static void
sn_screen_event (SnMonitorEvent * event, void *user_data)
{
    ScreenInfo *screen_info;
    SnStartupSequence *sequence;

    g_return_if_fail (event != NULL);
    sequence = sn_monitor_event_get_startup_sequence (event);

    screen_info = (ScreenInfo *) user_data;
    g_return_if_fail (screen_info != NULL);

    switch (sn_monitor_event_get_type (event))
    {
        case SN_MONITOR_EVENT_INITIATED:
            sn_add_sequence (screen_info, sequence);
            break;

        case SN_MONITOR_EVENT_COMPLETED:
            sn_remove_sequence (screen_info, sn_monitor_event_get_startup_sequence (event));
            break;

        case SN_MONITOR_EVENT_CHANGED:
            break;

        case SN_MONITOR_EVENT_CANCELED:
        default:
            break;
    }
}

void
sn_client_startup_properties (Client * c)
{
    ScreenInfo *screen_info;
    GSList *tmp;
    SnStartupSequence *sequence;
    char *startup_id;

    g_return_if_fail (c != NULL);
    TRACE ("client \"%s\" (0x%lx)", c->name, c->window);

    startup_id = clientGetStartupId (c);
    screen_info = c->screen_info;
    sequence = NULL;
    if (startup_id == NULL)
    {
        tmp = screen_info->startup_sequences;
        while (tmp != NULL)
        {
            const char *wmclass;

            wmclass = sn_startup_sequence_get_wmclass (tmp->data);

            if ((wmclass != NULL) && ((c->class.res_class && !strcmp (wmclass, c->class.res_class))
                || (c->class.res_name && !strcmp (wmclass, c->class.res_name))))
            {
                sequence = tmp->data;

                free (c->startup_id);
                c->startup_id = strdup (sn_startup_sequence_get_id (sequence));
                startup_id = c->startup_id;

                sn_startup_sequence_complete (sequence);
                break;
            }

            tmp = tmp->next;
        }
    }

    if (startup_id == NULL)
    {
        return;
    }

    if (sequence == NULL)
    {
        tmp = screen_info->startup_sequences;
        while (tmp != NULL)
        {
            const char *id;

            id = sn_startup_sequence_get_id (tmp->data);

            if (!strcmp (id, startup_id))
            {
                sequence = tmp->data;
                break;
            }

            tmp = tmp->next;
        }
    }

    if (sequence != NULL)
    {
        int workspace;
        guint32 timestamp;

        /* Set initial time */
        timestamp = sn_startup_sequence_get_timestamp (sequence);
        TRACE ("startup time: %u", (unsigned int) timestamp);
        if ((c->user_time == (guint32) 0) || TIMESTAMP_IS_BEFORE(c->user_time, timestamp))
        {
            c->user_time = timestamp;
            if (c->user_time != (guint32) 0)
            {
                myDisplayUpdateLastUserTime (screen_info->display_info, c->user_time);
            }
        }
        FLAG_SET (c->flags, CLIENT_FLAG_HAS_STARTUP_TIME);

        /* Set initial workspace */
        if (!FLAG_TEST (c->xfwm_flags, XFWM_FLAG_WORKSPACE_SET))
        {
            workspace = sn_startup_sequence_get_workspace (sequence);
            if (workspace >= 0)
            {
                FLAG_SET (c->xfwm_flags, XFWM_FLAG_WORKSPACE_SET);
                c->win_workspace = workspace;
            }
        }
    }
}

void
sn_init_display (ScreenInfo *screen_info)
{
    g_return_if_fail (screen_info != NULL);
    g_return_if_fail (myScreenGetXDisplay (screen_info) != NULL);

    if (!sn_display)
    {
        sn_display = sn_display_new (myScreenGetXDisplay (screen_info), sn_error_trap_push, sn_error_trap_pop);
    }
    screen_info->sn_context = NULL;
    if (sn_display != NULL)
    {
        screen_info->sn_context =
            sn_monitor_context_new (sn_display, screen_info->screen, sn_screen_event, screen_info, NULL);
    }
    screen_info->startup_sequences = NULL;
    screen_info->startup_sequence_timeout = 0;
}

void
sn_close_display (void)
{
    if (sn_display)
    {
        sn_display_unref (sn_display);
    }
    sn_display = NULL;
}

void
sn_process_event (XEvent * event)
{
    g_return_if_fail (sn_display != NULL);
    sn_display_process_event (sn_display, event);
}

#endif
