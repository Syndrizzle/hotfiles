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

        metacity - (c) 2001, 2002 Havoc Pennington
        xfwm4    - (c) 2002-2011 Olivier Fourdan

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <common/xfwm-common.h>

#include "client.h"
#include "terminate.h"

void
terminateCloseDialog (Client *c)
{
    g_return_if_fail (c != NULL);

    if (c->dialog_pid)
    {
        kill (c->dialog_pid, SIGKILL);
        c->dialog_pid = 0;
    }
    if (c->dialog_fd >= 0)
    {
        close (c->dialog_fd);
        c->dialog_fd = -1;
    }
}

static gboolean
terminateProcessIO (GIOChannel   *channel,
                    GIOCondition  condition,
                    gpointer      data)
{
    Client *c;
    char *str;
    gsize len;
    GError *err;

    c = (Client *) data;
    g_return_val_if_fail (c != NULL, FALSE);

    str = NULL;
    len = 0;
    err = NULL;

    if (condition & G_IO_IN)
    {
        g_io_channel_read_to_end (channel, &str, &len, &err);

        if (err)
        {
            g_warning (_("Error reading data from child process: %s\n"), err->message);
            g_error_free (err);
        }
        if (len > 0)
        {
            if (!g_ascii_strncasecmp (str, "yes", 3))
            {
                clientTerminate (c);
            }
        }

        g_free (str);
    }

    terminateCloseDialog (c);

    return FALSE;
}

static void
terminateShowDialogSetEnvironment (gpointer user_data)
{
    GdkScreen *screen = user_data;

    g_setenv ("DISPLAY", xfwm_make_display_name (screen), TRUE);
}

gboolean
terminateShowDialog (Client *c)
{
    ScreenInfo *screen_info;
    char *argv[4];
    GError *err;
    int child_pid;
    int outpipe;
    GIOChannel *channel;
    gchar *xid;

    if (c->dialog_pid > 0)
    {
        return FALSE;
    }

    screen_info = c->screen_info;
    xid = g_strdup_printf ("0x%lx", c->window);

    argv[0] = HELPERDIR "/xfce4/xfwm4/helper-dialog";
    argv[1] = xid;
    argv[2] = c->name;
    argv[3] = NULL;

    err = NULL;
    if (!g_spawn_async_with_pipes (NULL, argv, NULL, 0,
                                   terminateShowDialogSetEnvironment,
                                   screen_info->gscr,
                                   &child_pid, NULL, &outpipe,
                                   NULL, &err))
    {
        g_warning (_("Cannot spawn helper-dialog: %s\n"), err->message);
        g_error_free (err);
        g_free (xid);
        return FALSE;
    }
    g_free (xid);

    c->dialog_pid = child_pid;
    c->dialog_fd = outpipe;

    channel = g_io_channel_unix_new (c->dialog_fd);
    g_io_add_watch_full (channel, G_PRIORITY_DEFAULT,
                         G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
                         terminateProcessIO,
                         (gpointer) c, NULL);
    g_io_channel_unref (channel);

    return TRUE;
}

