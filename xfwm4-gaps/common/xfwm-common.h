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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <gtk/gtk.h>

void              xfwm_widget_reparent                  (GtkWidget    *widget,
                                                         GtkWidget    *new_parent);

void              xfwm_get_screen_dimensions            (gint         *width,
                                                         gint         *height);

void              xfwm_get_monitor_geometry             (GdkScreen    *screen,
                                                         gint          monitor_num,
                                                         GdkRectangle *geometry,
                                                         gboolean      scaled);

gboolean          xfwm_get_primary_monitor_geometry     (GdkScreen    *screen,
                                                         GdkRectangle *geometry,
                                                         gboolean      scaled);

gint              xfwm_get_primary_refresh_rate         (GdkScreen    *screen);

gboolean          xfwm_monitor_is_primary               (GdkScreen *screen,
                                                         gint      monitor_num);

gint              xfwm_get_n_monitors                   (GdkScreen    *screen);

gchar            *xfwm_make_display_name                (GdkScreen    *screen);

gboolean          xfwm_is_default_screen                (GdkScreen    *screen);

#endif /* !__COMMON_H__ */
