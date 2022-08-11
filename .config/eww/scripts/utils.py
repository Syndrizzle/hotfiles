#!/usr/bin/env python

"""Utility module. Shared across almost all of the python scripts / modules."""

# Authored By dharmx <dharmx.dev@gmail.com> under:
# GNU GENERAL PUBLIC LICENSE
# Version 3, 29 June 2007
#
# Copyright (C) 2007 Free Software Foundation, Inc. <https://fsf.org/>
# Everyone is permitted to copy and distribute verbatim copies
# of this license document, but changing it is not allowed.
#
# Permissions of this strong copyleft license are conditioned on
# making available complete source code of licensed works and
# modifications, which include larger works using a licensed work,
# under the same license. Copyright and license notices must be
# preserved. Contributors provide an express grant of patent rights.
#
# Read the complete license here:
# <https://github.com/dharmx/vile/blob/main/LICENSE.txt>

import json
import os
import pathlib
import random
import re
import sys
import time
import typing
import unicodedata

from wand.image import COLORSPACE_TYPES, Image
from html.parser import HTMLParser
from io import StringIO

import dbus
import gi

# supress GIO warnings
gi.require_version("Gtk", "3.0")
gi.require_version("GdkPixbuf", "2.0")

import requests
from gi.repository import GdkPixbuf, Gio, GLib, Gtk


class PangoStripper(HTMLParser):
    def __init__(self):
        super().__init__()
        self.reset()
        self.strict = False
        self.convert_charrefs = True
        self.text = StringIO()

    def handle_data(self, d):
        self.text.write(d)

    def get_data(self):
        return self.text.getvalue()


def contains_pango(string: str) -> bool:
    """Checks if a string contains HTML tags or not. Since a pango string has to have at least two tags

    Arguments:
        string: The Pango markup format string.

    Returns:
        A bool value i.e. whether a has pango markup or not.
    """
    return any(item in string for item in ["<span>", "</a>", "</span>"])


def strip_pango_tags(pango: str) -> str:
    """Removes HTML tags like <span>, <span align='left'>, etc.

    Arguments:
        pango: The Pango format string that needs to be stripped of its tags.

    Returns:
        A str without the tags.
    """
    stripper = PangoStripper()
    stripper.feed(pango)
    return stripper.get_data()


def create_parents_file(file_path: str):
    '''A replication of [ -f "$FILE_PATH" ] || mkdir --parents "$FILE_PATH"

    Arguments:
        file_path: The location of the file.
    '''
    pathlib.PosixPath(os.path.dirname(file_path)).mkdir(
        parents=True, exist_ok=True)
    pathlib.PosixPath(file_path).touch(exist_ok=True)


def watcher(file_path: str, callback: typing.Callable, interval: int, initial: bool = True):
    """A file watcher function that executes a callback function if any change is made to a file.

    Arguments:
        file_path: The file location that needs to be monitored.
        callback: A callback function that would be invoked when any change is noticed.
        interval: The time rate at which the file will be checked for changes.
        initial: Initially execute the callback once before entering the watch loop.
    """
    try:
        old = pathlib.PosixPath(file_path).read_text()
        if initial:
            callback(old)
        while not time.sleep(interval):
            new = pathlib.PosixPath(file_path).read_text()
            if new != old:
                callback(new)
                old = new
    except KeyboardInterrupt:
        sys.stdout.write("Closed.\n")
    except FileNotFoundError:
        sys.stderr.write("The path does not exist!\n")
    except Exception as excep:
        sys.stderr.write(f"{excep}\n")


def get_rand_quote(file_path: str, default_quote: str) -> str:
    """Get a random quote or, rather a random line.

    Arguments:
        file_path: The location of the file.
        default_quote: Fallback quote if the file doesn't exist r, is empty.
    """
    loaded_quotes: str = pathlib.PosixPath(file_path).read_text().strip()
    return random.choice(loaded_quotes.splitlines()) if loaded_quotes else default_quote


def file_matched_index_rm(file_path: str, pattern: str):
    """Removes line from a file if a pattern matches that line.

    Arguments:
        file_path: The location of the file.
        pattern: Pattern to match the line that needs to be removed.
    """
    posix_file_path = pathlib.PosixPath(file_path)
    lines = posix_file_path.read_text().splitlines()

    # skip the line if pattern matches
    rm_index_lines = [
        lines[index]
        for index in range(len(lines))
        if not re.search(pattern, lines[index])
    ]

    if len(lines) != len(rm_index_lines):
        posix_file_path.write_text("\n".join(rm_index_lines))


def file_rm_line(file_path: str, position: int | bool | range = True):
    """Ranged line removal, indexed line removal and stack-like (pop / reverse pop) line removal.

    Arguments:
        file_path: The location of the file.
        position: If the value is of type:
            range: then that the lines starting from that range will be removed.
            int: then that specific file line number will be removed.
            bool: True will pop and False will pop from the end.
    """
    file = pathlib.PosixPath(file_path)
    match str(type(position)):
        case "<class 'int'>":
            file_contents = file.read_text().splitlines()
            # no need to run the loop if all you need to do is remove the first line
            if position == 0:
                file_rm_line(file_path, position=True)
                return
            elif position == len(file_contents) - 1:
                file_rm_line(file_path, position=False)
                return
            line_removed_contents = []
            # skip the line that matches the position value.
            for index in range(len(file_contents)):
                if index != position:
                    line_removed_contents += [file_contents[index]]
            file.write_text("\n".join(line_removed_contents))
        # filter falsey and truthy and use the actual bool class signature
        case "<class 'bool'>":
            file_contents = file.read_text().splitlines()
            file_contents = file_contents[1:] if position else file_contents[:-1]
            file.write_text("\n".join(file_contents))
        case "<class 'range'>":
            if not position:
                file.write_text("")
                return
            file_contents = file.read_text().splitlines()
            write_contents = []
            # keep on skipping the lines if they are in the range.
            for index in range(len(file_contents)):
                if index not in position:
                    write_contents += [file_contents[index]]
            file.write_text("\n".join(write_contents))


def prettify_name(name: str) -> str:
    """Transforms 'Hello-World' -> 'Hello World', 'notify-send++lol' -> 'Notify Send Lol'

    Arguments:
        name: The string that needs to be prettified.

    Returns:
        A prettified str.
    """
    return " ".join(
        item.capitalize()
        for item in name.replace("-", " ").replace("_", " ").split(" ")
    )


def file_add_line(file_path: str, write_contents: str, limit: int, top: bool = True):
    """Functions like sliding window algorithm <https://towardsdev.com/sliding-window-algorithm-145f8e201a64>

    That is after the file hits a certain line limit, lines from the behind will be dropped (older entries)
    and THEN the newer ones will be entered.

    Arguments:
        file_path: The location of the file.
        write_contents: The contents that needs to be written to the file.
        limit: The line limit.
        top: Adds new entry at the beginning of the file if True, at the end otherwise.
    """
    file = pathlib.PosixPath(file_path)
    file_contents = file.read_text().splitlines()
    if len(file_contents) == limit:
        file_contents = file_contents[:-1]
    file_contents = (
        [write_contents] + file_contents if top else file_contents + [write_contents]
    )
    file.write_text("\n".join(file_contents))


def parse_and_print_stats(file_contents: str) -> dict:
    """Looks the words CRITICAL, LOW and NORMAL and calcuates its frequency.

    Arguments:
        file_contents: the string that needs to be calculated for frequency.

    Returns:
        Individual frequency of the words in dict format.
        {"CRITICAL": 10.00, "NORMAL": 85.00, "LOW": 5.00}
    """
    stats = {"critical": 0, "low": 0, "normal": 0, "total": 0}

    for line in file_contents.splitlines():
        if "CRITICAL" in line:
            stats["critical"] += 1
            stats["total"] += 1
        elif "LOW" in line:
            stats["low"] += 1
            stats["total"] += 1
        elif "NORMAL" in line:
            stats["normal"] += 1
            stats["total"] += 1

    # handle division / zero
    stats["critical"] = (
        stats["critical"] * 100 /
        stats["total"] if stats["critical"] > 0 else 0
    )
    stats["normal"] = (
        stats["normal"] * 100 / stats["total"] if stats["normal"] > 0 else 0
    )
    stats["low"] = stats["low"] * 100 / \
        stats["total"] if stats["low"] > 0 else 0
    return stats


def has_non_english_chars(string: str) -> dict:
    """Check if there is any CJK / Cyrillic characters in the given string.

    Arguments:
        string: the string that needs to be checked.

    Returns:
        A dict containing True / False for keys representing if such characters exist or not.
        {"CJK": True, "CYR": False} for string value: "おはようございます means Good Morning!"
    """
    return {
        "CJK": any(unicodedata.category(char) == "Lo" for char in string),
        "CYR": any(unicodedata.category(char) == "Lu" for char in string),
    }


def unwrap(value: dbus.Array
           | dbus.Boolean
           | dbus.Byte
           | dbus.Dictionary
           | dbus.Double
           | dbus.Int16
           | dbus.ByteArray
           | dbus.Int32
           | dbus.Int64
           | dbus.Signature
           | dbus.UInt16
           | dbus.UInt32
           | dbus.UInt64
           | dbus.String) -> str | int | list | tuple | float | dict | bool | bytes:
    """Try to trivially translate a dictionary's elements into nice string formatting.

    Arguments:
        value: A type out of:
            dbus.Boolean,
            dbus.Byte,
            dbus.Dictionary,
            dbus.Double,
            dbus.Int16,
            dbus.ByteArray,
            dbus.Int32,
            dbus.Int64,
            dbus.Signature,
            dbus.UInt16,
            dbus.UInt32,
            dbus.UInt64 and dbus.String

    Returns:
        A str or int or list or tuple or float or dict or bool or bytes depending on the value.
    """
    if isinstance(value, dbus.ByteArray):
        return "".join([str(byte) for byte in value])
    if isinstance(value, (dbus.Array, list, tuple)):
        return [unwrap(item) for item in value]
    if isinstance(value, (dbus.Dictionary, dict)):
        return dict([(unwrap(x), unwrap(y)) for x, y in value.items()])
    if isinstance(value, (dbus.Signature, dbus.String)):
        return str(value)
    if isinstance(value, dbus.Boolean):
        return bool(value)
    if isinstance(
        value,
        (dbus.Int16, dbus.UInt16, dbus.Int32,
         dbus.UInt32, dbus.Int64, dbus.UInt64),
    ):
        return int(value)
    if isinstance(value, dbus.Byte):
        return bytes([int(value)])
    return value


def save_img_byte(px_args: typing.Iterable, save_path: str):
    """Converts image data to an image file.

    See <https://docs.gtk.org/gdk-pixbuf/ctor.Pixbuf.new_from_bytes.html> for the whole description.

    Arguments:
        px_args: Should contain an iterable in the following format.
        [image_width, image_height, rowstride, has_alpha, bits_per_sample, _, image_bytes]
        save_path: the filepath where this pixbuf should be saved to.
    """
    # https://specifications.freedesktop.org/notification-spec/latest/ar01s08.html
    # https://specifications.freedesktop.org/notification-spec/latest/ar01s05.html
    GdkPixbuf.Pixbuf.new_from_bytes(
        width=px_args[0],
        height=px_args[1],
        has_alpha=px_args[3],
        data=GLib.Bytes(px_args[6]),
        colorspace=GdkPixbuf.Colorspace.RGB,
        rowstride=px_args[2],
        bits_per_sample=px_args[4],
    ).savev(save_path, "png")


def get_gtk_icon_path(icon_name: str, size: int = 128) -> str:
    """Returns the icon path by the specified name from your current icon theme.

    Recursively search for lower sizes until 32x32 and
    return a fallback if the requested path does not exists.

    Arguments:
        icon_name: Icon name as per the icon naming specification:
        <https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html>
        size: the pixel size (widthxheight -> heightxheight -> size) of the requested icon.
    """
    if size < 32:
        return os.path.expandvars("/usr/share/icons/Papirus-Dark/48x48/status/dialog-information.svg")
    if info := Gtk.IconTheme.get_default().lookup_icon(icon_name, size, 0):
        return info.get_filename()
    return get_gtk_icon_path(icon_name, size - 1)


def get_mime_icon_path(mimetype: str, size: int = 32) -> str:
    """Gets the default icon path from the current GTK icon theme for the specified mime type.

    Arguments:
        mimetype: The file type like png, json, etc.
        size: The of the returned icon

    Returns:
        A str that is the path to the requested icon.
    """
    icon = Gio.content_type_get_icon(mimetype)
    theme = Gtk.IconTheme.get_default()
    if info := theme.choose_icon(icon.get_names(), size, 0):
        return info.get_filename()


def get_location() -> dict | None:
    try:
        response = requests.get('https://api64.ipify.org?format=json').json()
        ip_address = response["ip"]
        response = requests.get(f'https://ipapi.co/{ip_address}/json/').json()
        return {
            "latitude": response.get("latitude"),
            "longitude": response.get("longitude"),
            "city": response.get("city"), 
            "country": response.get("country_name"),
            "lang": response.get("languages").split(",")[0],
        }
    except requests.exceptions.ConnectionError:
        return None


def auto_locate(cache_dir: str) -> dict | None:
    cache_posix_path = pathlib.PosixPath(f"{cache_dir}/location.json")
    if not cache_posix_path.is_file():  # assuming the directory exists
        fetched_location = get_location()
        if not fetched_location:
            return None

        cache_posix_path.write_text(json.dumps(fetched_location))
        return fetched_location
    return json.loads(cache_posix_path.read_text())


def fetch_save(link: str, save_path: str, callback: typing.Callable = None) -> bool:
    try:
        data = requests.get(link)
        if data.status_code == 200:
            metadata = data.json()
            if callback:
                metadata = callback(metadata)
            pathlib.PosixPath(save_path).write_text(json.dumps(metadata))
            return True
        return False
    except requests.exceptions.ConnectionError:
        return False


def img_dark_bright_col(filepath: str, colors: int = 10) -> tuple:
    with Image(filename=filepath) as image:
        image.quantize(
            number_colors=colors, 
            colorspace_type=COLORSPACE_TYPES[21], 
            dither=True, measure_error=False, treedepth=8)
        return tuple(
            "#%02X%02X%02X" % (item.red_int8, item.green_int8, item.blue_int8) 
            for item in image.histogram)


if __name__ == "__main__":
    match sys.argv[1]:
        case "histogram":
            image_path = sys.argv[2]
            histogram_colors = int(sys.argv[3])
            print(json.dumps(img_dark_bright_col(image_path, histogram_colors)))

# vim:filetype=python
