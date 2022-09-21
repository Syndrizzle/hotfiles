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
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

#include "display.h"
#include "screen.h"
#include "events.h"
#include "event_filter.h"
#include "frame.h"
#include "settings.h"
#include "client.h"
#include "menu.h"
#include "focus.h"
#include "keyboard.h"
#include "workspaces.h"
#include "mywindow.h"
#include "session.h"
#include "startup_notification.h"
#include "compositor.h"
#include "spinning_cursor.h"

#define BASE_EVENT_MASK \
    SubstructureNotifyMask|\
    StructureNotifyMask|\
    SubstructureRedirectMask|\
    ButtonPressMask|\
    ButtonReleaseMask|\
    KeyPressMask|\
    KeyReleaseMask|\
    FocusChangeMask|\
    PropertyChangeMask|\
    ColormapChangeMask

#ifdef HAVE_COMPOSITOR
#define MAIN_EVENT_MASK BASE_EVENT_MASK|ExposureMask
#else /* HAVE_COMPOSITOR */
#define MAIN_EVENT_MASK BASE_EVENT_MASK
#endif /* HAVE_COMPOSITOR */

#ifdef HAVE_COMPOSITOR
static gboolean compositor = TRUE;
static vblankMode vblank_mode = VBLANK_AUTO;
#define XFWM4_ERROR      (xfwm4_error_quark ())

static GQuark
xfwm4_error_quark (void)
{
  return g_quark_from_static_string ("xfwm4-error-quark");
}
#endif /* HAVE_COMPOSITOR */

#ifdef DEBUG
static gboolean
setupLog (gboolean debug)
{
    const gchar *str;
    gchar *logfile;
    int fd;

    if (debug)
    {
        str = g_getenv ("XFWM4_LOG_FILE");
        if (str)
        {
            logfile = g_strdup (str);
        }
        else
        {
            logfile = g_strdup_printf ("xfwm4-debug-%d.log", (int) getpid ());
        }
    }
    else
    {
        logfile = "/dev/null";
    }

    fd = dup(fileno(stderr));
    if (fd == -1)
    {
        g_warning ("Fail to open %s: %s", logfile, g_strerror (errno));
        g_free (logfile);
        return FALSE;
    }

    if (!freopen (logfile, "w", stderr))
    {
        g_warning ("Fail to redirect stderr: %s", g_strerror (errno));
        g_free (logfile);
        close (fd);
        return FALSE;
    }

    if (debug)
    {
        g_print ("Logging to %s\n", logfile);
        g_free (logfile);
    }

    return TRUE;
}
#endif /* DEBUG */

static void
handleSignal (int sig)
{
    DisplayInfo *display_info;

    display_info = myDisplayGetDefault ();
    if (display_info)
    {
        switch (sig)
        {
            case SIGINT:
                /* Walk thru */
            case SIGTERM:
                gtk_main_quit ();
                display_info->quit = TRUE;
                break;
            case SIGHUP:
                /* Walk thru */
            case SIGUSR1:
                display_info->reload = TRUE;
                break;
            default:
                break;
        }
    }
}

static void
setupHandler (gboolean install)
{
    struct sigaction act;

    if (install)
        act.sa_handler = handleSignal;
    else
        act.sa_handler = SIG_DFL;

    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    sigaction (SIGINT,  &act, NULL);
    sigaction (SIGTERM, &act, NULL);
    sigaction (SIGHUP,  &act, NULL);
    sigaction (SIGUSR1, &act, NULL);
}

static void
cleanUp (void)
{
    GSList *screens;
    DisplayInfo *display_info;

    TRACE ("entering");

    setupHandler (FALSE);

    display_info = myDisplayGetDefault ();
    g_return_if_fail (display_info);

    eventFilterClose (display_info->xfilter);
    for (screens = display_info->screens; screens; screens = g_slist_next (screens))
    {
        ScreenInfo *screen_info_n = (ScreenInfo *) screens->data;
        myScreenClose (screen_info_n);
        g_free (screen_info_n);
    }
    sn_close_display ();
    sessionFreeWindowStates ();

    myDisplayClose (display_info);
    g_free (display_info);

    xfconf_shutdown();
}

static void
ensure_basedir_spec (void)
{
    char *new, *old, path[PATH_MAX];
    GError *error;
    GDir *gdir;
    const char *name;

    /* test if new directory is there */

    new = xfce_resource_save_location (XFCE_RESOURCE_CONFIG,
                                       "xfce4" G_DIR_SEPARATOR_S "xfwm4",
                                       FALSE);

    if (g_file_test (new, G_FILE_TEST_IS_DIR))
    {
        g_free (new);
        return;
    }

    error = NULL;
    if (!xfce_mkdirhier(new, 0700, &error))
    {
        g_warning("Unable to create config dir %s: %s", new, error->message);
        g_error_free (error);
        g_free (new);
        return;
    }

    g_free (new);

    /* copy xfwm4rc */

    old = xfce_get_userfile ("xfwm4rc", NULL);

    if (g_file_test (old, G_FILE_TEST_EXISTS))
    {
        FILE *r, *w;

        g_strlcpy (path, "xfce4/xfwm4/xfwm4rc", PATH_MAX);
        new = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, path, FALSE);

        r = fopen (old, "r");
        w = fopen (new, "w");

        g_free (new);

        if (w && r)
        {
            int c;

            while ((c = getc (r)) != EOF)
            {
                putc (c, w);
            }
        }

        if (r)
        {
            fclose (r);
        }

        if (w)
        {
            fclose (w);
        }
    }

    g_free (old);

    /* copy saved session data */

    new = xfce_resource_save_location (XFCE_RESOURCE_CACHE, "sessions", FALSE);
    if (!xfce_mkdirhier(new, 0700, &error))
    {
        g_warning("Unable to create session dir %s: %s", new, error->message);
        g_error_free (error);
        g_free (new);
        return;
    }

    old = xfce_get_userfile ("sessions", NULL);
    gdir = g_dir_open (old, 0, NULL);

    if (gdir)
    {
        while ((name = g_dir_read_name (gdir)) != NULL)
        {
            FILE *r, *w;

            g_snprintf (path, PATH_MAX, "%s/%s", old, name);
            r = fopen (path, "r");

            g_snprintf (path, PATH_MAX, "%s/%s", new, name);
            w = fopen (path, "w");

            if (w && r)
            {
                int c;

                while ((c = getc (r)) != EOF)
                    putc (c, w);
            }

            if (r)
                fclose (r);
            if (w)
                fclose (w);
        }

        g_dir_close (gdir);
    }

    g_free (old);
    g_free (new);
}

static void
print_version (void)
{
    g_print ("\tThis is %s version %s (revision %s) for Xfce %s\n",
                    PACKAGE, VERSION, REVISION, xfce_version_string());
    g_print ("\tReleased under the terms of the GNU General Public License.\n");
    g_print ("\tCompiled against GTK+-%d.%d.%d, ",
                    GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
    g_print ("using GTK+-%d.%d.%d.\n",
                    gtk_major_version, gtk_minor_version, gtk_micro_version);
    g_print ("\n");
    g_print ("\tBuild configuration and supported features:\n");

    g_print ("\t- Startup notification support:                 ");
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    g_print ("Yes\n");
#else
    g_print ("No\n");
#endif

    g_print ("\t- XSync support:                                ");
#ifdef HAVE_XSYNC
    g_print ("Yes\n");
#else
    g_print ("No\n");
#endif

    g_print ("\t- Render support:                               ");
#ifdef HAVE_RENDER
    g_print ("Yes\n");
#else
    g_print ("No\n");
#endif

    g_print ("\t- Xrandr support:                               ");
#ifdef HAVE_RANDR
    g_print ("Yes\n");
#else
    g_print ("No\n");
#endif

    g_print ("\t- Xpresent support:                             ");
#ifdef HAVE_PRESENT_EXTENSION
    g_print ("Yes\n");
#else
    g_print ("No\n");
#endif

    g_print ("\t- X Input 2 support:                            ");
#ifdef HAVE_XI2
    g_print ("Yes\n");
#else
    g_print ("No\n");
#endif

    g_print ("\t- Embedded compositor:                          ");
#ifdef HAVE_COMPOSITOR
    g_print ("Yes\n");
#else
    g_print ("No\n");
#endif

#ifdef HAVE_COMPOSITOR
    g_print ("\t- Epoxy support:                                ");
#ifdef HAVE_EPOXY
    g_print ("Yes\n");
#else
    g_print ("No\n");
#endif /* HAVE_EPOXY */
#endif /* HAVE_COMPOSITOR */

    g_print ("\t- KDE systray proxy (deprecated):               ");
#ifdef ENABLE_KDE_SYSTRAY_PROXY
    g_print ("Yes\n");
#else
    g_print ("No\n");
#endif
}

#ifdef HAVE_COMPOSITOR
static gboolean
compositor_callback (const gchar  *name,
                     const gchar  *value,
                     gpointer      user_data,
                     GError      **error)
{
    gboolean succeed = TRUE;

    g_return_val_if_fail (value != NULL, FALSE);

    if (strcmp (value, "off") == 0)
    {
        compositor = FALSE;
    }
    else if (strcmp (value, "on") == 0)
    {
        compositor = TRUE;
    }
    else
    {
        g_set_error (error, XFWM4_ERROR, 0, "Unrecognized compositor option \"%s\"", value);
        succeed = FALSE;
    }

    return succeed;
}

static gboolean
vblank_callback (const gchar  *name,
                 const gchar  *value,
                 gpointer      user_data,
                 GError      **error)
{
    gboolean succeed = TRUE;

    g_return_val_if_fail (value != NULL, FALSE);

#ifdef HAVE_PRESENT_EXTENSION
    if (strcmp (value, "xpresent") == 0)
    {
        vblank_mode = VBLANK_XPRESENT;
    }
    else
#endif /* HAVE_PRESENT_EXTENSION */
#ifdef HAVE_EPOXY
    if (strcmp (value, "glx") == 0)
    {
        vblank_mode = VBLANK_GLX;
    }
    else
#endif /* HAVE_EPOXY */
    if (strcmp (value, "off") == 0)
    {
        vblank_mode = VBLANK_OFF;
    }
    else
    {
        g_set_error (error, XFWM4_ERROR, 0, "Unrecognized compositor option \"%s\"", value);
        succeed = FALSE;
    }

    return succeed;
}

static void
init_compositor_screen (ScreenInfo *screen_info)
{
    DisplayInfo *display_info;

    display_info = screen_info->display_info;
    if (vblank_mode != VBLANK_AUTO)
    {
        compositorSetVblankMode (screen_info, vblank_mode);
    }

    if (display_info->enable_compositor)
    {
        gboolean xfwm4_compositor;

        xfwm4_compositor = TRUE;
        if (screen_info->params->use_compositing)
        {
            /* Enable compositor if "use compositing" is enabled */
            xfwm4_compositor = compositorManageScreen (screen_info);
        }
        /*
           The user may want to use the manual compositing, but the installed
           system may not support it, so we need to double check, to see if
           initialization of the compositor was successful.
          */
        if (xfwm4_compositor)
        {
            /*
               Acquire selection on XFWM4_COMPOSITING_MANAGER to advertise our own
               compositing manager (used by WM tweaks to determine whether or not
               show the "compositor" tab.
             */
            setAtomIdManagerOwner (display_info, XFWM4_COMPOSITING_MANAGER,
                                   screen_info->xroot, screen_info->xfwm4_win);
        }
    }
}
#endif /* HAVE_COMPOSITOR */

static int
initialize (gboolean replace_wm)
{
    DisplayInfo *display_info;
    gint i, nscreens, default_screen;

    DBG ("xfwm4 starting, using GTK+-%d.%d.%d", gtk_major_version,
         gtk_minor_version, gtk_micro_version);

    ensure_basedir_spec ();

    initMenuEventWin ();
    clientClearFocus (NULL);
    display_info = myDisplayInit (gdk_display_get_default ());

#ifdef HAVE_COMPOSITOR
    display_info->enable_compositor = compositor;
#else
    display_info->enable_compositor = FALSE;
#endif /* HAVE_COMPOSITOR */

    initModifiers (display_info->dpy);

    setupHandler (TRUE);

    nscreens = ScreenCount (display_info->dpy);
    default_screen = DefaultScreen (display_info->dpy);
    for(i = 0; i < nscreens; i++)
    {
        ScreenInfo *screen_info;
        GdkScreen *gscr;
        Window temp_xwindow;
        GdkWindow *screen_window;

        if (i == default_screen)
        {
            gscr = gdk_display_get_default_screen (display_info->gdisplay);
        }
        else
        {
            /* create temp 1x1 child window on this screen */
            temp_xwindow = XCreateSimpleWindow (display_info->dpy,
                                                RootWindow (display_info->dpy, i),
                                                0, 0, 1, 1, 0, 0, 0);
            /* allocate new GdkWindow with GdkScreen for this window */
            screen_window =
                gdk_x11_window_foreign_new_for_display (display_info->gdisplay,
                                                        temp_xwindow);
            XDestroyWindow (display_info->dpy, temp_xwindow);

            if (screen_window == NULL)
            {
                g_warning ("Cannot create GdkScreen for screen %i", i);
                continue;
            }

            gscr = gdk_window_get_screen (screen_window);

            /* foreign windows have 2 references */
            g_object_unref (screen_window);
            g_object_unref (screen_window);
        }
        screen_info = myScreenInit (display_info, gscr, MAIN_EVENT_MASK, replace_wm);

        if (!screen_info)
        {
            continue;
        }

        if (!initSettings (screen_info))
        {
            return -2;
        }
#ifdef HAVE_COMPOSITOR
        if (display_info->enable_compositor)
        {
            init_compositor_screen (screen_info);
        }
#endif /* HAVE_COMPOSITOR */
        sn_init_display (screen_info);
        myDisplayAddScreen (display_info, screen_info);
        screen_info->current_ws = getNetCurrentDesktop (display_info, screen_info->xroot);
        setUTF8StringHint (display_info, screen_info->xfwm4_win, NET_WM_NAME, "Xfwm4");
        setNetSupportedHint (display_info, screen_info->xroot, screen_info->xfwm4_win);
        setNetDesktopInfo (display_info, screen_info->xroot, screen_info->current_ws,
                                   screen_info->width,
                                   screen_info->height);
        workspaceUpdateArea (screen_info);
        XSetInputFocus (display_info->dpy, screen_info->xfwm4_win, RevertToPointerRoot, CurrentTime);

        clientFrameAll (screen_info);

        initPerScreenCallbacks (screen_info);

        XDefineCursor (display_info->dpy, screen_info->xroot, myDisplayGetCursorRoot(display_info));
    }

    /* No screen to manage, give up */
    if (!display_info->nb_screens)
    {
        return -1;
    }
    display_info->xfilter = eventFilterInit (display_info->devices, (gpointer) display_info);
    eventFilterPush (display_info->xfilter, xfwm4_event_filter, (gpointer) display_info);
    initPerDisplayCallbacks (display_info);

    return sessionStart (display_info);
}

static void
init_pango_cache (void)
{
    GtkWidget *tmp_win;
    PangoLayout *layout;

    /*
     * The first time the first Gtk application on a display uses pango,
     * pango grabs the XServer while it creates the font cache window.
     * Therefore, force the cache window to be created now instead of
     * trying to do it while we have another grab and deadlocking the server.
     */
    tmp_win = gtk_window_new (GTK_WINDOW_POPUP);
    layout = gtk_widget_create_pango_layout (tmp_win, "-");
    pango_layout_get_pixel_extents (layout, NULL, NULL);
    g_object_unref (G_OBJECT (layout));
    gtk_widget_destroy (GTK_WIDGET (tmp_win));
}

int
main (int argc, char **argv)
{
    gboolean version = FALSE;
    gboolean replace_wm = FALSE;
    int status;
    GOptionContext *context;
    GError *error = NULL;
#ifdef DEBUG
    gboolean debug = FALSE;
#endif /* DEBUG */
    GOptionEntry option_entries[] =
    {
#ifdef HAVE_COMPOSITOR
        { "compositor", 'c', 0, G_OPTION_ARG_CALLBACK,
          compositor_callback, N_("Set the compositor mode"), "on|off" },
        { "vblank", 'b', 0, G_OPTION_ARG_CALLBACK,
          vblank_callback, N_("Set the vblank mode"), "off"
#ifdef HAVE_PRESENT_EXTENSION
          "|xpresent"
#endif /* HAVE_PRESENT_EXTENSION */
#ifdef HAVE_EPOXY
          "|glx"
#endif /* HAVE_EPOXY */
        },
#endif /* HAVE_COMPOSITOR */
        { "replace", 'r', 0, G_OPTION_ARG_NONE,
          &replace_wm, N_("Replace the existing window manager"), NULL },
        { "version", 'V', 0, G_OPTION_ARG_NONE,
          &version, N_("Print version information and exit"), NULL },
#ifdef DEBUG
        { "debug", 'd', 0, G_OPTION_ARG_NONE,
          &debug, N_("Enable debug logging"), NULL },
#endif /* DEBUG */
        { NULL }
    };

#ifdef HAVE_EPOXY
    /* NVIDIA proprietary/closed source driver queues up to 2 frames by
     * default before blocking in glXSwapBuffers(), whereas our compositor
     * expects `glXSwapBuffers()` to block until the next vblank.
     *
     * To avoid that, our compositor was issuing a `glXWaitGL()` immediately
     * after the call to `glXSwapBuffers()` but that translates as a busy
     * wait, hence dramatically increasing CPU usage of xfwm4 with the
     * NVIDIA proprietary/closed source driver.
     *
     * Instruct the NVIDIA proprietary/closed source driver to allow only
     * 1 frame using the environment variable “__GL_MaxFramesAllowed” so
     * that it matches our expectations.
     *
     * This must be set before libGL is loaded, hence before gtk_init().
     *
     * Taken from similar patch posted by NVIDIA developer for kwin:
     * https://phabricator.kde.org/D19867
     */
    g_setenv("__GL_MaxFramesAllowed", "1", TRUE);
#endif /* HAVE_EPOXY */

    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    /* xfwm4 is an X11 window manager, no point in trying to connect to
     * any other display server (like when running nested within a
     * Wayland compositor).
     */
    gdk_set_allowed_backends ("x11");

#ifndef HAVE_XI2
    /* Disable XI2 in GDK */
    gdk_disable_multidevice ();
#endif

    context = g_option_context_new (_("[ARGUMENTS...]"));
    g_option_context_add_main_entries (context, option_entries, GETTEXT_PACKAGE);
    g_option_context_add_group (context, gtk_get_option_group (FALSE));
    g_option_context_add_group (context, xfce_sm_client_get_option_group (argc, argv));
    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
          g_print ("%s: %s.\n", PACKAGE_NAME, error->message);
          g_print (_("Type \"%s --help\" for usage."), G_LOG_DOMAIN);
          g_print ("\n");
          g_error_free (error);

          return EXIT_FAILURE;
    }
    g_option_context_free (context);

#ifdef DEBUG
    setupLog (debug);
#endif /* DEBUG */
    DBG ("xfwm4 starting");

    gtk_init (&argc, &argv);

    if (G_UNLIKELY (version))
    {
         print_version ();
         return EXIT_SUCCESS;
    }
    init_pango_cache ();

    status = initialize (replace_wm);
    /*
       status  < 0   =>   Error, cancel execution
       status == 0   =>   Run w/out session manager
       status == 1   =>   Connected to session manager
     */
    switch (status)
    {
        case -1:
            g_warning ("Could not find a screen to manage, exiting");
            exit (1);
            break;
        case -2:
            g_warning ("Missing data from default files");
            exit (1);
            break;
        case 0:
        case 1:
            /* enter GTK main loop */
            gtk_main ();
            break;
        default:
            g_warning ("Unknown error occurred");
            exit (1);
            break;
    }
    cleanUp ();
    DBG ("xfwm4 terminated");
    return 0;
}
