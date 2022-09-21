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

/* Initially inspired by xfwm, fvwm2, enlightment and twm implementations */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <signal.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <libxfce4ui/libxfce4ui.h>

#include "display.h"
#include "screen.h"
#include "hints.h"
#include "client.h"
#include "session.h"

typedef struct _match
{
    unsigned long win;
    unsigned long client_leader;
    char *client_id;
    char *res_name;
    char *res_class;
    char *window_role;
    char *wm_name;
    int wm_command_count;
    char **wm_command;
    int x;
    int y;
    int width;
    int height;
    int old_x;
    int old_y;
    int old_width;
    int old_height;
    int desktop;
    int screen;
    unsigned long flags;
    gboolean used;
}
Match;

static int num_match = 0;
static Match *matches = NULL;

static void
my_free_string_list (gchar ** list, gint n)
{
    gchar **s;
    gint i;

    if (!list || !n)
    {
        return;                 /* silently... :) */
    }

    i = 0;
    s = list;

    while ((i < n) && (s))
    {
        g_free (*s);
        *s = NULL;
        s++;
        i++;
    }
}

/*
   2-pass function to compute new string length,
   allocate memory and finally copy string
   - Returned value must be freed -
 */
static gchar *
escape_quote (gchar * s)
{
    gchar *ns;
    gchar *idx1, *idx2;
    gint nbquotes, lg;

    g_return_val_if_fail (s != NULL, NULL);

    nbquotes = 0;
    lg = strlen (s);
    /* First, count quotes in string */
    idx1 = s;
    while (*idx1)
    {
        if (*(idx1++) == '"')
        {
            nbquotes++;
        }
    }
    /* If there is no quote in the string, return it */
    if (!nbquotes)
    {
        return (g_strdup (s));
    }

    /* Or else, allocate memory for the new string */
    ns = g_new0 (gchar, lg + nbquotes + 1);
    /* And prepend a backslash before any quote found in string */
    idx1 = s;
    idx2 = ns;
    while (*idx1)
    {
        if (*idx1 == '"')
        {
            *(idx2++) = '\\';
            *(idx2++) = '"';
        }
        else
        {
            *(idx2++) = *idx1;
        }
        idx1++;
    }
    /* Add null char */
    *idx2 = '\0';
    return ns;
}

/*
   single-pass function to replace backslash+quotes
   by quotes.
   - Returned value must be freed -
 */
static gchar *
unescape_quote (gchar * s)
{
    gchar *ns;
    gboolean backslash;
    gchar *idx1, *idx2;
    gint lg;

    g_return_val_if_fail (s != NULL, NULL);

    lg = strlen (s);
    backslash = FALSE;
    ns = g_new0 (gchar, lg + 1);
    idx1 = s;
    idx2 = ns;
    while (*idx1)
    {
        if (*idx1 == '\\')
        {
            *(idx2++) = *idx1;
            backslash = TRUE;
        }
        else if ((*idx1 == '"') && backslash)
        {
            /* Move backward to override the "\" */
            *(--idx2) = *idx1;
            idx2++;
            backslash = FALSE;
        }
        else
        {
            *(idx2++) = *idx1;
            backslash = FALSE;
        }
        idx1++;
    }
    *idx2 = '\0';
    return ns;
}

static gchar *
getsubstring (gchar * s, gint * length)
{
    gchar pbrk;
    gchar *ns, *end, *idx1, *idx2, *skip;
    gint lg;
    gboolean finished, backslash;

    lg = *length = 0;
    finished = FALSE;
    backslash = FALSE;
    g_return_val_if_fail (s != NULL, NULL);

    end = skip = s;
    while ((*skip == ' ') || (*skip == '\t'))
    {
        end = ++skip;
        (*length)++;
    }
    if (*skip == '"')
    {
        pbrk = '"';
        end = ++skip;
        (*length)++;
    }
    else
    {
        pbrk = ' ';
    }

    while ((!finished) && (*end))
    {
        if (*end == '\\')
        {
            backslash = TRUE;
        }
        else if ((*end == pbrk) && backslash)
        {
            backslash = FALSE;
        }
        else if (*end == pbrk)
        {
            finished = TRUE;
        }
        end++;
        lg++;
        (*length)++;
    }
    ns = g_new0 (gchar, lg + 1);
    /* Skip pbrk character */
    end--;
    idx1 = skip;
    idx2 = ns;
    do
    {
        *(idx2++) = *idx1;
    }
    while (++idx1 < end);
    *idx2 = '\0';
    return ns;
}

static gboolean
sessionSaveScreen (ScreenInfo *screen_info, FILE *f)
{
    DisplayInfo *display_info;
    Client *c;
    gchar *client_id, *window_role;
    gchar **wm_command;
    gint wm_command_count;
    guint client_idx;
    gboolean wrote_data = FALSE;

    display_info = screen_info->display_info;
    wm_command_count = 0;
    wm_command = NULL;
    window_role = NULL;
    client_id = NULL;

    for (c = screen_info->clients, client_idx = 0; client_idx < screen_info->client_count;
        c = c->next, client_idx++)
    {
        if (c->type & (WINDOW_TYPE_DONT_PLACE))
        {
            continue;
        }

        if (c->client_leader != None)
        {
            getWindowRole (display_info, c->window, &window_role);
        }
        else
        {
            window_role = NULL;
        }

        wrote_data = TRUE;

        fprintf (f, "[CLIENT] 0x%lx\n", c->window);

        getClientID (display_info, c->window, &client_id);
        if (client_id)
        {
            fprintf (f, "  [CLIENT_ID] %s\n", client_id);
            XFree (client_id);
            client_id = NULL;
        }

        if (c->client_leader)
        {
            fprintf (f, "  [CLIENT_LEADER] 0x%lx\n", c->client_leader);
        }

        if (window_role)
        {
            fprintf (f, "  [WINDOW_ROLE] %s\n", window_role);
            XFree (window_role);
            window_role = NULL;
        }

        if (c->class.res_class)
        {
            fprintf (f, "  [RES_NAME] %s\n", c->class.res_name);
        }

        if (c->class.res_name)
        {
            fprintf (f, "  [RES_CLASS] %s\n", c->class.res_class);
        }

        if (c->name)
        {
            fprintf (f, "  [WM_NAME] %s\n", c->name);
        }

        wm_command_count = 0;
        getWindowCommand (display_info, c->window, &wm_command, &wm_command_count);
        if ((wm_command_count > 0) && (wm_command))
        {
            gint j;
            fprintf (f, "  [WM_COMMAND] (%i)", wm_command_count);
            for (j = 0; j < wm_command_count; j++)
            {
                gchar *escaped_string;
                escaped_string = escape_quote (wm_command[j]);
                fprintf (f, " \"%s\"", escaped_string);
                g_free (escaped_string);
            }
            fprintf (f, "\n");
            XFreeStringList (wm_command);
            wm_command = NULL;
            wm_command_count = 0;
        }

        fprintf (f, "  [GEOMETRY] (%i,%i,%i,%i)\n", c->x, c->y, c->width,
            c->height);
        fprintf (f, "  [GEOMETRY-MAXIMIZED] (%i,%i,%i,%i)\n", c->saved_geometry.x,
            c->saved_geometry.y, c->saved_geometry.width, c->saved_geometry.height);
        fprintf (f, "  [SCREEN] %i\n", screen_info->screen);
        fprintf (f, "  [DESK] %i\n", c->win_workspace);
        fprintf (f, "  [FLAGS] 0x%lx\n", FLAG_TEST (c->flags,
                CLIENT_FLAG_STICKY | CLIENT_FLAG_ICONIFIED |
                CLIENT_FLAG_SHADED | CLIENT_FLAG_MAXIMIZED |
                CLIENT_FLAG_NAME_CHANGED));
    }

    return wrote_data;
}

gboolean
sessionSaveWindowStates (DisplayInfo *display_info, const gchar * filename)
{
    FILE *f;
    GSList *screens;
    gboolean wrote_data = FALSE;

    g_return_val_if_fail (filename != NULL, FALSE);
    g_return_val_if_fail (display_info != NULL, FALSE);

    if ((f = fopen (filename, "w")))
    {
        for (screens = display_info->screens; screens; screens = g_slist_next (screens))
        {
            ScreenInfo *screen_info_n = (ScreenInfo *) screens->data;
            if (sessionSaveScreen (screen_info_n, f))
              wrote_data = TRUE;
        }
        fclose (f);

        /* remove the file if nothing has been written */
        if (!wrote_data)
          g_unlink (filename);

        return TRUE;
    }
    return FALSE;
}

gboolean
sessionLoadWindowStates (const gchar * filename)
{
    FILE *f;
    gchar s[4096], s1[4096];
    gint i, pos, pos1;
    unsigned long w;

    g_return_val_if_fail (filename != NULL, FALSE);
    if ((f = fopen (filename, "r")))
    {
        while (fgets (s, sizeof (s), f))
        {
            sscanf (s, "%4000s", s1);
            if (!strcmp (s1, "[CLIENT]"))
            {
                sscanf (s, "%*s 0x%lx", &w);
                num_match++;
                matches = g_realloc (matches, sizeof (Match) * num_match);
                matches[num_match - 1].win = w;
                matches[num_match - 1].client_id = NULL;
                matches[num_match - 1].res_name = NULL;
                matches[num_match - 1].res_class = NULL;
                matches[num_match - 1].window_role = NULL;
                matches[num_match - 1].wm_name = NULL;
                matches[num_match - 1].wm_command_count = 0;
                matches[num_match - 1].wm_command = NULL;
                matches[num_match - 1].x = 0;
                matches[num_match - 1].y = 0;
                matches[num_match - 1].width = 100;
                matches[num_match - 1].height = 100;
                matches[num_match - 1].old_x = matches[num_match - 1].x;
                matches[num_match - 1].old_y = matches[num_match - 1].y;
                matches[num_match - 1].old_width = matches[num_match - 1].width;
                matches[num_match - 1].old_height = matches[num_match - 1].height;
                matches[num_match - 1].desktop = 0;
                matches[num_match - 1].screen = 0;
                matches[num_match - 1].used = FALSE;
                matches[num_match - 1].flags = 0;
            }
            else if (!strcmp (s1, "[GEOMETRY]"))
            {
                sscanf (s, "%*s (%i,%i,%i,%i)", &matches[num_match - 1].x,
                    &matches[num_match - 1].y, &matches[num_match - 1].width,
                    &matches[num_match - 1].height);
            }
            else if (!strcmp (s1, "[GEOMETRY-MAXIMIZED]"))
            {
                sscanf (s, "%*s (%i,%i,%i,%i)", &matches[num_match - 1].old_x,
                    &matches[num_match - 1].old_y,
                    &matches[num_match - 1].old_width,
                    &matches[num_match - 1].old_height);
            }
            else if (!strcmp (s1, "[SCREEN]"))
            {
                sscanf (s, "%*s %i", &matches[num_match - 1].screen);
            }
            else if (!strcmp (s1, "[DESK]"))
            {
                sscanf (s, "%*s %i", &matches[num_match - 1].desktop);
            }
            else if (!strcmp (s1, "[CLIENT_LEADER]"))
            {
                sscanf (s, "%*s 0x%lx",
                    &matches[num_match - 1].client_leader);
            }
            else if (!strcmp (s1, "[FLAGS]"))
            {
                sscanf (s, "%*s 0x%lx", &matches[num_match - 1].flags);
            }
            else if (!strcmp (s1, "[CLIENT_ID]"))
            {
                sscanf (s, "%*s %[^\n]", s1);
                matches[num_match - 1].client_id = strdup (s1);
            }
            else if (!strcmp (s1, "[WINDOW_ROLE]"))
            {
                sscanf (s, "%*s %[^\n]", s1);
                matches[num_match - 1].window_role = strdup (s1);
            }
            else if (!strcmp (s1, "[RES_NAME]"))
            {
                sscanf (s, "%*s %[^\n]", s1);
                matches[num_match - 1].res_name = strdup (s1);
            }
            else if (!strcmp (s1, "[RES_CLASS]"))
            {
                sscanf (s, "%*s %[^\n]", s1);
                matches[num_match - 1].res_class = strdup (s1);
            }
            else if (!strcmp (s1, "[WM_NAME]"))
            {
                sscanf (s, "%*s %[^\n]", s1);
                matches[num_match - 1].wm_name = strdup (s1);
            }
            else if (!strcmp (s1, "[WM_COMMAND]"))
            {
                sscanf (s, "%*s (%i)%n", &matches[num_match - 1].wm_command_count, &pos);
                matches[num_match - 1].wm_command = g_new0 (gchar *, matches[num_match - 1].wm_command_count + 1);
                for (i = 0; i < matches[num_match - 1].wm_command_count; i++)
                {
                    gchar *substring;
                    substring = getsubstring (s + pos, &pos1);
                    pos += pos1;
                    matches[num_match - 1].wm_command[i] = unescape_quote (substring);
                    g_free (substring);
                }
                matches[num_match - 1].wm_command[matches[num_match - 1].wm_command_count] = NULL;
            }
        }
        fclose (f);
        return TRUE;
    }
    return FALSE;
}

void
sessionFreeWindowStates (void)
{
    gint i;

    for (i = 0; i < num_match; i++)
    {
        if (matches[i].client_id)
        {
            free (matches[i].client_id);
            matches[i].client_id = NULL;
        }
        if (matches[i].res_name)
        {
            free (matches[i].res_name);
            matches[i].res_name = NULL;
        }
        if (matches[i].res_class)
        {
            free (matches[i].res_class);
            matches[i].res_class = NULL;
        }
        if (matches[i].window_role)
        {
            free (matches[i].window_role);
            matches[i].window_role = NULL;
        }
        if (matches[i].wm_name)
        {
            free (matches[i].wm_name);
            matches[i].wm_name = NULL;
        }
        if ((matches[i].wm_command_count) && (matches[i].wm_command))
        {
            my_free_string_list (matches[i].wm_command,
                matches[i].wm_command_count);
            g_free (matches[i].wm_command);
            matches[i].wm_command_count = 0;
            matches[i].wm_command = NULL;
        }
    }
    if (matches)
    {
        g_free (matches);
        matches = NULL;
        num_match = 0;
    }
}

/* This complicated logic is from twm, where it is explained */

#define xstreq(a,b) ((!a && !b) || (a && b && (strcmp(a,b)==0)))

static gboolean
matchWin (Client * c, Match * m)
{
    ScreenInfo *screen_info;
    DisplayInfo *display_info;
    gchar *client_id;
    gchar *window_role;
    gchar **wm_command;
    gint wm_command_count;
    gint i;
    gboolean found;

    g_return_val_if_fail (c != NULL, FALSE);

    screen_info = c->screen_info;
    display_info = screen_info->display_info;
    wm_command = NULL;
    window_role = NULL;
    client_id = NULL;
    found = FALSE;

    getClientID (display_info, c->window, &client_id);
    if (xstreq (client_id, m->client_id))
    {
        /* client_id's match */
        if (c->client_leader != None)
        {
            getWindowRole (display_info, c->window, &window_role);
        }
        else
        {
            window_role = NULL;
        }
        if ((window_role) || (m->window_role))
        {
            /* We have or had a window role, base decision on it */
            found = xstreq (window_role, m->window_role);
        }
        else
        {
            /*
             * Compare res_class, res_name and WM_NAME, unless the
             * WM_NAME has changed
             */
            if (xstreq (c->class.res_name, m->res_name)
                && (FLAG_TEST (c->flags, CLIENT_FLAG_NAME_CHANGED)
                    || (m->flags & CLIENT_FLAG_NAME_CHANGED)
                    || xstreq (c->name, m->wm_name)))
            {
                if (client_id)
                {
                    /* If we have a client_id, we don't compare
                       WM_COMMAND, since it will be different. */
                    found = TRUE;
                }
                else
                {
                    /* for non-SM-aware clients we also compare WM_COMMAND */
                    wm_command_count = 0;
                    getWindowCommand (display_info, c->window, &wm_command, &wm_command_count);
                    if (wm_command_count == m->wm_command_count)
                    {
                        for (i = 0; i < wm_command_count; i++)
                        {
                            if (strcmp (wm_command[i], m->wm_command[i]) != 0)
                                break;
                        }

                        if ((i == wm_command_count) && (wm_command_count))
                        {
                            found = TRUE;
                        }
                    }
                    /*
                     * We have to deal with a now-SM-aware client, it means that it won't probably
                     * restore its state in a proper manner.
                     * Thus, we also mark all other instances of this application as used, to avoid
                     * dummy side effects in case we found a matching entry.
                     */
                    if (found)
                    {
                        for (i = 0; i < num_match; i++)
                        {
                            if (!(matches[i].used) && !(&matches[i] == m)
                                && (m->client_leader)
                                && (matches[i].client_leader == m->client_leader))
                            {
                                matches[i].used = TRUE;
                            }
                        }
                    }
                }
            }
        }
    }

    if (client_id)
    {
        XFree (client_id);
        client_id = NULL;
    }

    if (window_role)
    {
        XFree (window_role);
        window_role = NULL;
    }

    if ((wm_command_count > 0) && (wm_command))
    {
        XFreeStringList (wm_command);
        wm_command = NULL;
        wm_command_count = 0;
    }

    return found;
}

gboolean
sessionMatchWinToSM (Client * c)
{
    gint i;

    g_return_val_if_fail (c != NULL, FALSE);
    for (i = 0; i < num_match; i++)
    {
        if (!matches[i].used && (c->screen_info->screen == matches[i].screen) && matchWin (c, &matches[i]))
        {
            matches[i].used = TRUE;
            c->x = matches[i].x;
            c->y = matches[i].y;
            c->width = matches[i].width;
            c->height = matches[i].height;
            c->saved_geometry.x = matches[i].old_x;
            c->saved_geometry.y = matches[i].old_y;
            c->saved_geometry.width = matches[i].old_width;
            c->saved_geometry.height = matches[i].old_height;
            c->win_workspace = matches[i].desktop;
            FLAG_SET (c->flags,
                matches[i].
                flags & (CLIENT_FLAG_STICKY | CLIENT_FLAG_SHADED |
                    CLIENT_FLAG_MAXIMIZED | CLIENT_FLAG_ICONIFIED |
                    CLIENT_FLAG_RESTORE_SIZE_POS));
            FLAG_SET (c->xfwm_flags, XFWM_FLAG_WORKSPACE_SET);
            return TRUE;
        }
    }
    return FALSE;
}

static void
sessionLoad (DisplayInfo *display_info)
{
    const gchar *filename;

    filename = xfce_sm_client_get_state_file (display_info->session);
    DBG ("Restoring session from \"%s\"", filename);
    if (filename)
    {
        sessionLoadWindowStates (filename);
    }
}

static void
sessionSavePhase2 (XfceSMClient *session,
                   DisplayInfo *display_info)
{
    const gchar *filename;

    g_return_if_fail (XFCE_IS_SM_CLIENT (session));
    g_return_if_fail (session == display_info->session);

    filename = xfce_sm_client_get_state_file (display_info->session);
    DBG ("Saving session to \"%s\"", filename);
    if (filename)
    {
        sessionSaveWindowStates (display_info, filename);
    }
}

static void
sessionDie (XfceSMClient *session,
            DisplayInfo *display_info)
{
    g_return_if_fail (XFCE_IS_SM_CLIENT (session));
    g_return_if_fail (session == display_info->session);

    /*
     * Do not change the session restart style to NORMAL here, else
     * xfwm4 will never be restarted the next time we login. just
     * gracefully quit the application.
     */
    DBG ("Session clients asked to quit");
    gtk_main_quit ();
}

int
sessionStart (DisplayInfo *display_info)
{
    XfceSMClient *session;
    GError *error = NULL;

    DBG ("Starting session client");

    session = xfce_sm_client_get ();
    xfce_sm_client_set_restart_style (session, XFCE_SM_CLIENT_RESTART_IMMEDIATELY);
    xfce_sm_client_set_priority (session, XFCE_SM_CLIENT_PRIORITY_WM);
    display_info->session = session;

    if (xfce_sm_client_connect(session, &error))
    {
        if (xfce_sm_client_is_resumed (session))
            sessionLoad (display_info);

        /* save-state-extended is special for window managers to store
         * the window positions of all the clients */
        g_signal_connect (G_OBJECT (session), "save-state-extended",
                          G_CALLBACK (sessionSavePhase2), display_info);
        g_signal_connect (G_OBJECT (session), "quit",
                          G_CALLBACK (sessionDie), display_info);

        return 1;
    }
    else
    {
        g_warning ("Failed to connect to session manager: %s", error->message);
        g_error_free (error);
    }

    return 0;
}

