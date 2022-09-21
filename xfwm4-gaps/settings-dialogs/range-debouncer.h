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

#ifndef __RANGE_DEBOUNCER_H__
#define __RANGE_DEBOUNCER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _RangeDebouncerClass      RangeDebouncerClass;
typedef struct _RangeDebouncer           RangeDebouncer;

#define TYPE_RANGE_DEBOUNCER             (range_debouncer_get_type ())
#define RANGE_DEBOUNCER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_RANGE_DEBOUNCER, RangeDebouncer))
#define RANGE_DEBOUNCER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_RANGE_DEBOUNCER, RangeDebouncerClass))
#define IS_RANGE_DEBOUNCER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_RANGE_DEBOUNCER))
#define IS_RANGE_DEBOUNCER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_RANGE_DEBOUNCER))
#define RANGE_DEBOUNCER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_RANGE_DEBOUNCER, RangeDebouncerClass))

GType                     range_debouncer_get_type                 (void)                           G_GNUC_CONST;

RangeDebouncer *          range_debouncer_bind                     (GtkRange          *range);

G_END_DECLS

#endif /* __RANGE_DEBOUNCER_H__ */
