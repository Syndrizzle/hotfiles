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

#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <libxfce4util/libxfce4util.h>

#include "settings.h"
#include "parserc.h"

#define TOINT(x)                (x ? atoi(x) : 0)

gboolean
parseRc (const gchar * file, const gchar * dir, Settings *rc)
{
    gchar buf[255];
    gchar *filename, *lvalue, *rvalue;
    FILE *fp;

    TRACE ("file \"%s\" dir \"%s\"", file, dir);

    g_return_val_if_fail (file != NULL, FALSE);

    if (dir)
    {
        filename = g_build_filename (dir, file, NULL);
    }
    else
    {
        filename = g_strdup (file);
    }

    fp = fopen (filename, "r");
    g_free (filename);

    if (!fp)
    {
        return FALSE;
    }
    while (fgets (buf, sizeof (buf), fp))
    {
        lvalue = strtok (buf, "=");
        rvalue = strtok (NULL, "\n");
        if ((lvalue) && (rvalue))
        {
            setValue (lvalue, rvalue, rc);
        }
    }
    fclose (fp);
    return TRUE;
}

gboolean
checkRc (Settings *rc)
{
    gint i;
    gboolean rval;

    TRACE ("entering");

    rval = TRUE;
    for (i = 0; rc[i].option; i++)
    {
        if (rc[i].required && !rc[i].value)
        {
            fprintf (stderr, "missing value for option %s\n", rc[i].option);
            rval = FALSE;
        }
    }
    return rval;
}

GValue *
getGValue (const gchar * option, Settings *rc)
{
    gint i;

    g_return_val_if_fail (option != NULL, NULL);

    TRACE ("option \"%s\"", option);

    for (i = 0; rc[i].option; i++)
    {
        if (!g_ascii_strcasecmp (option, rc[i].option))
        {
            return rc[i].value;
        }
    }
    return NULL;
}

gboolean
setValue (const gchar * lvalue, const gchar *rvalue, Settings *rc)
{
    gint i;

    TRACE ("entering setValue");

    g_return_val_if_fail (lvalue != NULL, FALSE);
    g_return_val_if_fail (rvalue != NULL, FALSE);

    TRACE ("lvalue \"%s\" rvalue \"%s\"", lvalue, rvalue);

    for (i = 0; rc[i].option; i++)
    {
        if (!g_ascii_strcasecmp (lvalue, rc[i].option))
        {
            if (rvalue)
            {
                switch (rc[i].type)
                {
                    case G_TYPE_STRING:
                        setStringValue (lvalue, rvalue, rc);
                        break;
                    case G_TYPE_INT:
                        setIntValue (lvalue, TOINT(rvalue), rc);
                        break;
                    case G_TYPE_BOOLEAN:
                        setBooleanValue (lvalue, !g_ascii_strcasecmp ("true", rvalue), rc);
                        break;
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

static gboolean
setGValue (const gchar * lvalue, const GValue *rvalue, Settings *rc)
{
    gint i;

    g_return_val_if_fail (lvalue != NULL, FALSE);
    g_return_val_if_fail (rvalue != NULL, FALSE);

    TRACE ("lvalue \"%s\"", lvalue);

    for (i = 0; rc[i].option; i++)
    {
        if (!g_ascii_strcasecmp (lvalue, rc[i].option))
        {
            if (rvalue)
            {
                if (rc[i].value)
                {
                    g_value_unset (rc[i].value);
                    g_value_init (rc[i].value, G_VALUE_TYPE(rvalue));
                }
                else
                {
                    rc[i].value = g_new0(GValue, 1);
                    g_value_init (rc[i].value, G_VALUE_TYPE(rvalue));
                }

                g_value_copy (rvalue, rc[i].value);
                return TRUE;
            }
        }
    }
    return FALSE;
}

gboolean
setBooleanValue (const gchar * lvalue, gboolean value, Settings *rc)
{
    GValue tmp_val = {0, };
    g_value_init(&tmp_val, G_TYPE_BOOLEAN);
    g_value_set_boolean(&tmp_val, value);
    return setGValue (lvalue, &tmp_val, rc);
}

gboolean
setIntValue (const gchar * lvalue, gint value, Settings *rc)
{
    GValue tmp_val = {0, };
    g_value_init(&tmp_val, G_TYPE_INT);
    g_value_set_int(&tmp_val, value);
    return setGValue (lvalue, &tmp_val, rc);
}

gboolean
setStringValue (const gchar * lvalue, const gchar *value, Settings *rc)
{
    GValue tmp_val = {0, };
    g_value_init(&tmp_val, G_TYPE_STRING);
    g_value_set_string(&tmp_val, value);
    return setGValue (lvalue, &tmp_val, rc);
}

gchar *
getSystemThemeDir (void)
{
    return g_build_filename (DATADIR, "themes", DEFAULT_THEME, "xfwm4", NULL);
}

gchar *
getThemeDir (const gchar * theme, const gchar * file)
{
    if (!theme)
    {
        return g_build_filename (DATADIR, "themes", DEFAULT_THEME, "xfwm4",
                                 NULL);
    }
    else if (g_path_is_absolute (theme))
    {
        if (g_file_test (theme, G_FILE_TEST_IS_DIR))
        {
            return g_strdup (theme);
        }
        else
        {
            return getSystemThemeDir ();
        }
    }
    else
    {
        gchar *test_file, *path;

        path = g_build_filename (theme, "xfwm4", file, NULL);

        xfce_resource_push_path (XFCE_RESOURCE_THEMES,
                                 DATADIR G_DIR_SEPARATOR_S "themes");
        test_file = xfce_resource_lookup (XFCE_RESOURCE_THEMES, path);
        xfce_resource_pop_path (XFCE_RESOURCE_THEMES);

        g_free (path);

        if (test_file)
        {
            path = g_path_get_dirname (test_file);
            g_free (test_file);
            return path;
        }
    }

    /* Pfew, really can't find that theme nowhere! */
    return getSystemThemeDir ();
}

void
freeRc (Settings *rc)
{
    gint i;

    TRACE ("entering");

    for (i = 0; rc[i].option; i++)
    {
        if (rc[i].value)
        {
            g_value_unset(rc[i].value);
            g_free (rc[i].value);
            rc[i].value = NULL;
        }
    }
}

const gchar *
getStringValue (const gchar *option, Settings *rc)
{
    gint i;

    g_return_val_if_fail (option != NULL, NULL);

    TRACE ("option \"%s\"", option);

    for (i = 0; rc[i].option; i++)
    {
        if (!g_ascii_strcasecmp (option, rc[i].option))
        {
            if (rc[i].value == NULL)
                return NULL;
            if (G_VALUE_TYPE(rc[i].value) != G_TYPE_STRING)
            {
                g_warning ("Option \"%s\" in not of expected type string", option);
                return NULL;
            }
            return g_value_get_string(rc[i].value);
        }
    }
    return NULL;
}

gint
getIntValue (const gchar *option, Settings *rc)
{
    gint i;

    g_return_val_if_fail (option != NULL, 0);

    TRACE ("option \"%s\"", option);

    for (i = 0; rc[i].option; i++)
    {
        if (!g_ascii_strcasecmp (option, rc[i].option))
        {
            if (rc[i].value == NULL)
                return 0;
            if (G_VALUE_TYPE(rc[i].value) != G_TYPE_INT)
            {
                g_warning ("Option \"%s\" in not of expected type int", option);
                return 0;
            }
            return g_value_get_int(rc[i].value);
        }
    }
    return 0;
}

gboolean
getBoolValue (const gchar *option, Settings *rc)
{
    gint i;

    g_return_val_if_fail (option != NULL, FALSE);

    TRACE ("option \"%s\"", option);

    for (i = 0; rc[i].option; i++)
    {
        if (!g_ascii_strcasecmp (option, rc[i].option))
        {
            if (rc[i].value == NULL)
                return FALSE;
            if (G_VALUE_TYPE(rc[i].value) != G_TYPE_BOOLEAN)
            {
                g_warning ("Option \"%s\" in not of expected type boolean", option);
                return FALSE;
            }
            return g_value_get_boolean(rc[i].value);
        }
    }
    return FALSE;
}
