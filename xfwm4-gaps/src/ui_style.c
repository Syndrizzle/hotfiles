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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <pango/pango-font.h>

#include <libxfce4util/libxfce4util.h>
#include "ui_style.h"

char *states[] = {
    "normal", "active", "prelight", "selected", "insensitive", NULL
};

char *names[] = {
    "fg", "bg", "light", "dark", "mid", NULL
};

#define GTKSTYLE_FG    0
#define GTKSTYLE_BG    1
#define GTKSTYLE_LIGHT 2
#define GTKSTYLE_DARK  3
#define GTKSTYLE_MID   4

#define GTKSTATE_NORMAL      0
#define GTKSTATE_ACTIVE      1
#define GTKSTATE_PRELIGHT    2
#define GTKSTATE_SELECTED    3
#define GTKSTATE_INSENSITIVE 4

#define LIGHTNESS_MULT 1.3
#define DARKNESS_MULT  0.7

static gint
state_value (const gchar * s)
{
    gint u = 0;

    while ((states[u]) && (strcmp (states[u], s)))
    {
        u++;
    }
    if (states[u])
    {
        return (u);
    }
    return (-1);
}

static gint
name_value (const gchar * s)
{
    gint u = 0;

    while ((names[u]) && (strcmp (names[u], s)))
    {
        u++;
    }
    if (names[u])
    {
        return (u);
    }
    return (-1);
}

static void
rgb_to_hls (gdouble * r, gdouble * g, gdouble * b)
{
    /* from gtkstyle.c in gtk2 branch */

    gdouble min;
    gdouble max;
    gdouble red;
    gdouble green;
    gdouble blue;
    gdouble h, l, s;
    gdouble delta;

    red = *r;
    green = *g;
    blue = *b;

    max = MAX (red, MAX (green, blue));
    min = MIN (red, MIN (green, blue));

    l = (max + min) / 2;
    s = 0;
    h = 0;

    if (max != min)
    {
        if (l <= 0.5)
            s = (max - min) / (max + min);
        else
            s = (max - min) / (2 - max - min);

        delta = max - min;
        if (red == max)
            h = (green - blue) / delta;
        else if (green == max)
            h = 2 + (blue - red) / delta;
        else if (blue == max)
            h = 4 + (red - green) / delta;

        h *= 60;
        if (h < 0.0)
            h += 360;
    }

    *r = h;
    *g = l;
    *b = s;
}

static void
hls_to_rgb (gdouble * h, gdouble * l, gdouble * s)
{
    /* from gtkstyle.c in gtk2 branch */

    gdouble hue;
    gdouble lightness;
    gdouble saturation;
    gdouble m1, m2;
    gdouble r, g, b;

    lightness = *l;
    saturation = *s;

    if (lightness <= 0.5)
        m2 = lightness * (1 + saturation);
    else
        m2 = lightness + saturation - lightness * saturation;
    m1 = 2 * lightness - m2;

    if (saturation == 0)
    {
        *h = lightness;
        *l = lightness;
        *s = lightness;
    }
    else
    {
        hue = *h + 120;
        while (hue > 360)
            hue -= 360;
        while (hue < 0)
            hue += 360;

        if (hue < 60)
            r = m1 + (m2 - m1) * hue / 60;
        else if (hue < 180)
            r = m2;
        else if (hue < 240)
            r = m1 + (m2 - m1) * (240 - hue) / 60;
        else
            r = m1;

        hue = *h;
        while (hue > 360)
            hue -= 360;
        while (hue < 0)
            hue += 360;

        if (hue < 60)
            g = m1 + (m2 - m1) * hue / 60;
        else if (hue < 180)
            g = m2;
        else if (hue < 240)
            g = m1 + (m2 - m1) * (240 - hue) / 60;
        else
            g = m1;

        hue = *h - 120;
        while (hue > 360)
            hue -= 360;
        while (hue < 0)
            hue += 360;

        if (hue < 60)
            b = m1 + (m2 - m1) * hue / 60;
        else if (hue < 180)
            b = m2;
        else if (hue < 240)
            b = m1 + (m2 - m1) * (240 - hue) / 60;
        else
            b = m1;

        *h = r;
        *l = g;
        *s = b;
    }
}

static void
rgba_shade (GdkRGBA * color, gdouble value)
{
  rgb_to_hls (&color->red, &color->green, &color->blue);
  color->green = MAX (MIN (color->green * value, 1), 0);
  color->blue = MAX (MIN (color->blue * value, 1), 0);
  hls_to_rgb (&color->red, &color->green, &color->blue);
}

gboolean
getUIStyleColor (GtkWidget * win, const gchar * name, const gchar * state, GdkRGBA * rgba)
{
    GtkStyleContext *ctx;
    GdkRGBA         *result;
    GtkStateFlags    flags;
    gint             gtkstyle;
    gchar           *property;

    TRACE ("name \"%s\", state \"%s\"", name, state);

    g_return_val_if_fail (win != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_WIDGET (win), FALSE);
    g_return_val_if_fail (gtk_widget_get_realized (win), FALSE);

    gtkstyle = name_value (name);

    switch (gtkstyle)
    {
        case GTKSTYLE_FG:
            property = GTK_STYLE_PROPERTY_COLOR;
            break;
        case GTKSTYLE_BG:
        case GTKSTYLE_LIGHT:
        case GTKSTYLE_DARK:
        case GTKSTYLE_MID:
            property = GTK_STYLE_PROPERTY_BACKGROUND_COLOR;
            break;
        default:
            return FALSE;
    }

    switch (state_value (state))
    {
        case GTKSTATE_NORMAL:
            flags = GTK_STATE_FLAG_NORMAL;
            break;
        case GTKSTATE_ACTIVE:
            flags = GTK_STATE_FLAG_ACTIVE;
            break;
        case GTKSTATE_PRELIGHT:
            flags = GTK_STATE_FLAG_PRELIGHT;
            break;
        case GTKSTATE_SELECTED:
            flags = GTK_STATE_FLAG_SELECTED;
            break;
        case GTKSTATE_INSENSITIVE:
            flags = GTK_STATE_FLAG_INSENSITIVE;
            break;
        default:
            return FALSE;
    }

    ctx = gtk_widget_get_style_context (win);

    gtk_style_context_save (ctx);
    gtk_style_context_add_class (ctx, "gtkstyle-fallback");
    gtk_style_context_get (ctx, flags, property, &result, NULL);
    gtk_style_context_restore (ctx);

    *rgba = *result;

    switch (gtkstyle)
    {
        case GTKSTYLE_LIGHT:
            rgba_shade (rgba, LIGHTNESS_MULT);
            break;
        case GTKSTYLE_DARK:
            rgba_shade (rgba, DARKNESS_MULT);
            break;
        case GTKSTYLE_MID:
            rgba_shade (rgba, LIGHTNESS_MULT);
            rgba_shade (result, DARKNESS_MULT);
            rgba->red = (rgba->red + result->red) / 2;
            rgba->green = (rgba->green + result->green) / 2;
            rgba->blue = (rgba->blue + result->blue) / 2;
            break;
    }

    gdk_rgba_free (result);

    return TRUE;
}

gchar *
getUIStyleString (GtkWidget * win, const gchar * name, const gchar * state)
{
    GdkRGBA color = {0, };
    GdkRGBA bg = {0, };
    gint red;
    gint green;
    gint blue;

    TRACE ("name \"%s\", state \"%s\"", name, state);

    if (getUIStyleColor (win, name, state, &color))
    {
        if (color.alpha < 1 && g_strcmp0 (name, "bg") && getUIStyleColor (win, "bg", state, &bg))
        {
            /* compose bg and fg colors to get opaque color */
            color.red = color.red * color.alpha + bg.red * (1 - color.alpha);
            color.green = color.green * color.alpha + bg.green * (1 - color.alpha);
            color.blue = color.blue * color.alpha + bg.blue * (1 - color.alpha);
        }
    }

    red = color.red * 0xff;
    green = color.green * 0xff;
    blue = color.blue * 0xff;

    return g_strdup_printf ("#%02x%02x%02x%02x%02x%02x", red, red, green, green, blue, blue);
}

PangoFontDescription *
getUIPangoFontDesc (GtkWidget * win)
{
    GtkStyleContext      *ctx;
    PangoFontDescription *font_desc;

    TRACE ("entering");

    g_return_val_if_fail (win != NULL, NULL);
    g_return_val_if_fail (GTK_IS_WIDGET (win), NULL);
    g_return_val_if_fail (gtk_widget_get_realized (win), NULL);

    ctx = gtk_widget_get_style_context (win);
    gtk_style_context_get (ctx, GTK_STATE_FLAG_NORMAL,
                           GTK_STYLE_PROPERTY_FONT, &font_desc,
                           NULL);

    return font_desc;
}

PangoContext *
getUIPangoContext (GtkWidget * win)
{
    TRACE ("entering");

    g_return_val_if_fail (win != NULL, NULL);
    g_return_val_if_fail (GTK_IS_WIDGET (win), NULL);
    g_return_val_if_fail (gtk_widget_get_realized (win), NULL);

    return (gtk_widget_get_pango_context (win));
}
