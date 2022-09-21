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

#include "range-debouncer.h"



struct _RangeDebouncerClass
{
  GObjectClass         __parent__;
};

struct _RangeDebouncer
{
  GObject              __parent__;

  GtkRange            *range;
  gboolean             pressed : 1;

  gulong               signal1;
  gulong               signal2;
  gulong               signal3;
};

enum
{
  PROP_0,
  PROP_VALUE,
  N_PROPERTIES,
};

static void              range_debouncer_finalize                        (GObject               *object);
static void              range_debouncer_get_property                    (GObject               *object,
                                                                          guint                  prop_id,
                                                                          GValue                *value,
                                                                          GParamSpec            *pspec);
static void              range_debouncer_set_property                    (GObject               *object,
                                                                          guint                  prop_id,
                                                                          const GValue          *value,
                                                                          GParamSpec            *pspec);

static void              range_debouncer_weak_notify                     (RangeDebouncer *range_debouncer,
                                                                          GObject        *object);

static void              range_debouncer_value_changed                   (RangeDebouncer *range_debouncer);
static gboolean          range_debouncer_button_pressed                  (RangeDebouncer *range_debouncer);
static gboolean          range_debouncer_button_released                 (RangeDebouncer *range_debouncer);

G_DEFINE_TYPE (RangeDebouncer, range_debouncer, G_TYPE_OBJECT)



static void
range_debouncer_class_init (RangeDebouncerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = range_debouncer_finalize;
  gobject_class->get_property = range_debouncer_get_property;
  gobject_class->set_property = range_debouncer_set_property;

  g_object_class_install_property (gobject_class, PROP_VALUE,
                                   g_param_spec_double ("value", NULL, NULL,
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}



static void
range_debouncer_init (RangeDebouncer *range_debouncer)
{
  range_debouncer->range = NULL;
  range_debouncer->pressed = FALSE;

  range_debouncer->signal1 = 0;
  range_debouncer->signal2 = 0;
  range_debouncer->signal3 = 0;
}



RangeDebouncer *
range_debouncer_bind (GtkRange *range)
{
  RangeDebouncer *range_debouncer;

  g_return_val_if_fail (GTK_IS_RANGE (range), NULL);

  range_debouncer = g_object_new (TYPE_RANGE_DEBOUNCER, NULL);
  range_debouncer->range = range;

  /* RangeDebouncer will be destroyed with GtkRange */
  g_object_weak_ref (G_OBJECT (range), (GWeakNotify)range_debouncer_weak_notify, range_debouncer);

  range_debouncer->signal1 =
    g_signal_connect_swapped (range, "value-changed",
                              G_CALLBACK (range_debouncer_value_changed), range_debouncer);
  range_debouncer->signal2 =
    g_signal_connect_swapped (range, "button-press-event",
                              G_CALLBACK (range_debouncer_button_pressed), range_debouncer);
  range_debouncer->signal3 =
    g_signal_connect_swapped (range, "button-release-event",
                              G_CALLBACK (range_debouncer_button_released), range_debouncer);

  return range_debouncer;
}



static void
range_debouncer_finalize (GObject *object)
{
  RangeDebouncer *range_debouncer = RANGE_DEBOUNCER (object);

  if (G_UNLIKELY (range_debouncer->range != NULL))
    {
      g_signal_handler_disconnect(range_debouncer->range, range_debouncer->signal1);
      g_signal_handler_disconnect(range_debouncer->range, range_debouncer->signal2);
      g_signal_handler_disconnect(range_debouncer->range, range_debouncer->signal3);
    }

  G_OBJECT_CLASS (range_debouncer_parent_class)->finalize (object);
}



static void
range_debouncer_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  RangeDebouncer *range_debouncer = RANGE_DEBOUNCER (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      if (G_LIKELY (range_debouncer->range != NULL))
        g_value_set_double (value, gtk_range_get_value (range_debouncer->range));
      else
        g_value_set_double (value, 0);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
range_debouncer_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  RangeDebouncer *range_debouncer = RANGE_DEBOUNCER (object);
  gdouble         val_double;

  switch (prop_id)
    {
    case PROP_VALUE:
      val_double = g_value_get_double (value);
      if (G_LIKELY (range_debouncer->range != NULL) &&
          gtk_range_get_value (range_debouncer->range) != val_double)
        {
          gtk_range_set_value (range_debouncer->range, val_double);
          g_object_notify (object, "value");
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
range_debouncer_weak_notify (RangeDebouncer *range_debouncer, GObject *object)
{
  range_debouncer->range = NULL;
  g_object_unref (range_debouncer);
}



static void
range_debouncer_value_changed (RangeDebouncer *range_debouncer)
{
  if (!range_debouncer->pressed)
    g_object_notify (G_OBJECT (range_debouncer), "value");
}



static gboolean
range_debouncer_button_pressed (RangeDebouncer *range_debouncer)
{
  range_debouncer->pressed = TRUE;
  return FALSE;
}



static gboolean
range_debouncer_button_released (RangeDebouncer *range_debouncer)
{
  range_debouncer->pressed = FALSE;
  g_object_notify (G_OBJECT (range_debouncer), "value");
  return FALSE;
}
