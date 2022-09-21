/* vi:set sw=2 sts=2 ts=2 et ai: */
/*-
 * Copyright (c) 2008 Jannis Pohlmann <jannis@xfce.org>.
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

#ifndef __XFWM_SETTINGS_H__
#define __XFWM_SETTINGS_H__

#include <glib-object.h>

G_BEGIN_DECLS;

typedef struct _XfwmSettingsPrivate XfwmSettingsPrivate;
typedef struct _XfwmSettingsClass   XfwmSettingsClass;
typedef struct _XfwmSettings        XfwmSettings;

#define XFWM_TYPE_SETTINGS            (xfwm_settings_get_type ())
#define XFWM_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFWM_TYPE_SETTINGS, XfwmSettings))
#define XFWM_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFWM_TYPE_SETTINGS, XfwmSettingsClass))
#define XFWM_IS_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFWM_TYPE_SETTINGS))
#define XFWM_IS_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFWM_TYPE_SETTINGS)
#define XFWM_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFWM_TYPE_SETTINGS, XfwmSettingsClass))

GType         xfwm_settings_get_type (void) G_GNUC_CONST;

XfwmSettings *xfwm_settings_new      (void) G_GNUC_MALLOC;



struct _XfwmSettingsClass
{
  GObjectClass __parent__;
};

struct _XfwmSettings
{
  GObject __parent__;

  XfwmSettingsPrivate *priv;
};

G_END_DECLS;

#endif /* !__XFWM_SETTINGS_H__ */
