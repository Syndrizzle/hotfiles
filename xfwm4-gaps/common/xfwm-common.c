/* vi:set sw=2 sts=2 ts=2 et ai tw=100: */
/*-
 * Copyright (c) 2017 Viktor Odintsev <zakhams@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <string.h>
#include <gdk/gdkx.h>

#include "xfwm-common.h"
#define DEFAULT_REFRESH_RATE 30


void
xfwm_widget_reparent (GtkWidget *widget,
                      GtkWidget *new_parent)
{
  GtkWidget *old_parent;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (GTK_IS_CONTAINER (new_parent));

  old_parent = gtk_widget_get_parent (widget);

  g_return_if_fail (old_parent != NULL);

  if (old_parent != new_parent)
    {
      g_object_ref (widget);
      gtk_container_remove (GTK_CONTAINER (old_parent), widget);
      gtk_container_add (GTK_CONTAINER (new_parent), widget);
      g_object_unref (widget);
    }
}



void
xfwm_get_screen_dimensions (gint *width, gint *height)
{
  GdkDisplay   *display;
  GdkMonitor   *monitor;
  GdkRectangle  geometry;

  display = gdk_display_get_default ();
  monitor = gdk_display_get_primary_monitor (display);
  gdk_monitor_get_geometry (monitor, &geometry);

  if (width != NULL)
    *width = geometry.width;
  if (height != NULL)
    *height = geometry.height;
}



static void
xfwm_geometry_convert_to_device_pixels (GdkRectangle *geometry,
                                        gint          scale)
{
  if (geometry != NULL)
    {
      geometry->x *= scale;
      geometry->y *= scale;
      geometry->width *= scale;
      geometry->height *= scale;
    }
}



void
xfwm_get_monitor_geometry (GdkScreen    *screen,
                           gint          monitor_num,
                           GdkRectangle *geometry,
                           gboolean      scaled)
{
  gint        scale;
  GdkDisplay *display;
  GdkMonitor *monitor;

  display = gdk_screen_get_display (screen);
  monitor = gdk_display_get_monitor (display, monitor_num);
  scale = gdk_monitor_get_scale_factor (monitor);
  gdk_monitor_get_geometry (monitor, geometry);

  if (scaled && scale != 1)
    xfwm_geometry_convert_to_device_pixels (geometry, scale);
}



gboolean
xfwm_get_primary_monitor_geometry (GdkScreen    *screen,
                                   GdkRectangle *geometry,
                                   gboolean      scaled)
{
  gint        scale;
  GdkDisplay *display;
  GdkMonitor *monitor;

  display = gdk_screen_get_display (screen);
  monitor = gdk_display_get_primary_monitor (display);

  if (!monitor)
  {
      geometry->width = 0;
      geometry->height = 0;

      return FALSE;
  }

  scale = gdk_monitor_get_scale_factor (monitor);
  gdk_monitor_get_geometry (monitor, geometry);

  if (scaled && scale != 1)
    xfwm_geometry_convert_to_device_pixels (geometry, scale);

  return TRUE;
}



gint
xfwm_get_primary_refresh_rate (GdkScreen *screen)
{
  GdkDisplay *display;
  GdkMonitor *monitor;
  int         refresh_rate;

  display = gdk_screen_get_display (screen);
  monitor = gdk_display_get_primary_monitor (display);
  refresh_rate = 0;

  if (monitor)
    refresh_rate = gdk_monitor_get_refresh_rate (monitor) / 1000;

  if (refresh_rate)
    return refresh_rate;

  return DEFAULT_REFRESH_RATE;
}



gboolean
xfwm_monitor_is_primary (GdkScreen *screen,
                         gint      monitor_num)
{
  GdkDisplay *display;
  GdkMonitor *monitor;

  display = gdk_screen_get_display (screen);
  monitor = gdk_display_get_monitor (display, monitor_num);

  return gdk_monitor_is_primary (monitor);
}



gint
xfwm_get_n_monitors (GdkScreen *screen)
{
  return gdk_display_get_n_monitors (gdk_screen_get_display (screen));
}



static gchar *
substitute_screen_number (const gchar *display_name,
                          gint         screen_number)
{
  GString *str;
  gchar   *p;

  str = g_string_new (display_name);

  p = strrchr (str->str, '.');
  if (p != NULL && p > strchr (str->str, ':'))
    {
      /* remove screen number from string */
      g_string_truncate (str, p - str->str);
    }

  g_string_append_printf (str, ".%d", screen_number);

  return g_string_free (str, FALSE);
}



gchar *
xfwm_make_display_name (GdkScreen *screen)
{
  const gchar *name;
  gint         number;

  name = gdk_display_get_name (gdk_screen_get_display (screen));
  number = gdk_x11_screen_get_screen_number (screen);

  return substitute_screen_number (name, number);
}



gboolean
xfwm_is_default_screen (GdkScreen *screen)
{
  return gdk_x11_screen_get_screen_number (screen) == gdk_x11_get_default_screen();
}
