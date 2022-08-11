#!/usr/bin/env --split-string=python -u
"""Script for logging desktop notifications in the form of YUCK literal.

General idea is to have a file log the notifications in the form of YUCK
literal containing a widget structure which will then be concatenated into
a box widget to take a list-like structure.

The said structure needs to be re-rendered whenever the log file notices
a change. Like, deleting an entry or, adding an entry or, editing an entry.

Note, if you still have not guessed already, if you make any changes to the
log file then the list of notifications will be re-rendered again.
"""

# Authored By dharmx <dharmx@gmail.com> under:
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

# TODO: Utilize JSON instead of YUCK Literals.
# WARN: This script is under active development and is subject to frequent changes.
# WARN: Currently this script is quite inefficient. So, you have been warned.

import json
import os
import pathlib
import sys
from threading import main_thread

import cache
import handlers
import subprocess
import utils

# presetted notification format strings i.e. they are just utility items that help reducing the size of code
# another reason is convenience i.e. it would be easier to have all the formats in one place and defining them
# would be less cumbersome.
FORMATS = {
    "spotifyd": "(cardimage :identity ':::###::::XXXWWW%(id)s===::' :close_action 'scripts/logger.py rmid %(id)s' :limit_body '%(BODY_LIMITER)s' :limit_summary '%(SUMMARY_LIMITER)s' :summary '%(summary)s' :body '%(body)s' :close '' :image_height 50 :image_width 50 :image '%(iconpath)s' :appname '%(appname)s' :icon '%(iconpath)s' :icon_height 32 :icon_width 32 :timestamp '%(TIMESTAMP)s' :urgency '%(URGENCY)s')",
    "ncspot": "(cardimage :identity ':::###::::XXXWWW%(id)s===::' :close_action 'scripts/logger.py rmid %(id)s' :limit_body '%(BODY_LIMITER)s' :limit_summary '%(SUMMARY_LIMITER)s' :summary '%(summary)s' :body '%(body)s' :close '' :image_height 50 :image_width 50 :image '%(iconpath)s' :appname '%(appname)s' :icon '%(iconpath)s' :icon_height 32 :icon_width 32 :timestamp '%(TIMESTAMP)s' :urgency '%(URGENCY)s')",
    "Spotify": "(cardimage :class 'Spotify-rectangle' :identity ':::###::::XXXWWW%(id)s===::' :close_action 'scripts/logger.py rmid %(id)s' :limit_body '%(BODY_LIMITER)s' :limit_summary '%(SUMMARY_LIMITER)s' :summary '%(summary)s' :body '%(body)s' :close '' :image_height 50 :image_width 50 :image '%(iconpath)s' :appname '%(appname)s' :icon '%(iconpath)s' :icon_height 32 :icon_width 32 :timestamp '%(TIMESTAMP)s' :urgency '%(URGENCY)s')",
    "shot_icon": "(cardimage :class 'shot-image-bord-dashed' :identity ':::###::::XXXWWW%(id)s===::' :close_action 'scripts/logger.py rmid %(id)s' :limit_body '%(BODY_LIMITER)s' :limit_summary '%(SUMMARY_LIMITER)s' :summary '%(summary)s' :body '%(body)s' :close '' :image_height 50 :image_width 50 :image '%(iconpath)s' :appname '%(appname)s' :icon '%(iconpath)s' :icon_height 32 :icon_width 32 :timestamp '%(TIMESTAMP)s' :urgency '%(URGENCY)s')",
    "shot": "(cardscr :identity ':::###::::XXXWWW%(id)s===::' :close_action 'scripts/logger.py rmid %(id)s' :limit_body '%(BODY_LIMITER)s' :limit_summary '%(SUMMARY_LIMITER)s' :delete '%(DELETE)s' :open '%(OPEN)s' :summary '%(summary)s' :image '%(iconpath)s' :image_height 250 :image_width 100 :urgency '%(URGENCY)s' :close '' :timestamp '%(TIMESTAMP)s')",
    "default": "(cardimage :identity ':::###::::XXXWWW%(id)s===::' :close_action 'scripts/logger.py rmid %(id)s' :limit_body '%(BODY_LIMITER)s' :limit_summary '%(SUMMARY_LIMITER)s' :summary '%(summary)s' :body '%(body)s' :close '' :image_height 50 :image_width 50 :image '%(iconpath)s' :appname '%(appname)s' :icon '%(iconpath)s' :icon_height 32 :icon_width 32 :timestamp '%(TIMESTAMP)s' :urgency '%(URGENCY)s')",
    "notify-send": "(cardimage :identity ':::###::::XXXWWW%(id)s===::' :close_action 'scripts/logger.py rmid %(id)s' :limit_body '%(BODY_LIMITER)s' :limit_summary '%(SUMMARY_LIMITER)s' :summary '%(summary)s' :body '%(body)s' :close '' :image_height 50 :image_width 50 :image '%(iconpath)s' :appname '%(appname)s' :icon '%(iconpath)s' :icon_height 32 :icon_width 32 :timestamp '%(TIMESTAMP)s' :urgency '%(URGENCY)s')",
    "empty": "(box :class 'notifications-empty-box' :height 750 :orientation 'vertical' :space-evenly false (image :class 'notifications-empty-banner' :valign 'end' :vexpand true :path 'images/wedding-bells.png' :image-width 250 :image-height 250) (label :vexpand true :valign 'start' :wrap true :class 'notifications-empty-label' :text '%(QUOTE)s'))",
    "brightness": "(cardprog :identity ':::###::::XXXWWW%(id)s===::' :close_action 'scripts/logger.py rmid %(id)s' :limit_body '%(BODY_LIMITER)s' :limit_summary '%(SUMMARY_LIMITER)s' :summary '%(summary)s' :body '%(body)s' :close '' :image_height 50 :image_width 50 :image '%(iconpath)s' :appname '%(appname)s' :icon '%(iconpath)s' :icon_height 32 :icon_width 32 :timestamp '%(TIMESTAMP)s' :urgency '%(URGENCY)s' :progress '%(progress)s')",
    "todo": "(cardradial :identity ':::###::::XXXWWW%(id)s===::' :close_action 'scripts/logger.py rmid %(id)s' :limit_body '%(BODY_LIMITER)s' :limit_summary '%(SUMMARY_LIMITER)s' :summary '%(summary)s' :body '%(body)s' :close '' :appname '%(appname)s' :progress %(PERC)s :thickness 20.0 :total %(TOTAL)s :done %(DONE)s :timestamp '%(TIMESTAMP)s' :urgency '%(URGENCY)s')",
    "volume": "(cardprog :identity ':::###::::XXXWWW%(id)s===::' :close_action 'scripts/logger.py rmid %(id)s' :limit_body '%(BODY_LIMITER)s' :limit_summary '%(SUMMARY_LIMITER)s' :summary '%(summary)s' :body '%(body)s' :close '' :image_height 50 :image_width 50 :image '%(iconpath)s' :appname '%(appname)s' :icon '%(iconpath)s' :icon_height 32 :icon_width 32 :timestamp '%(TIMESTAMP)s' :urgency '%(URGENCY)s' :progress '%(progress)s')"
}

spotify_song_name = str(subprocess.run(["playerctl", "metadata", "title", "-p", "spotify"], capture_output=True).stdout)[2:-3]

if __name__ == "__main__":
    # load only notification related options from the config JSON
    config = json.loads(
        pathlib.PosixPath(
            os.path.expandvars("$XDG_CONFIG_HOME/eww/.config.json")
        ).read_text()
    )["notify"]

    # kind of like the sliding window algorithm i.e. will pop the notifications if this number is reached
    HISTORY_LIMIT = config["limit"]
    # file path where the notifications will be saved
    CACHE_PATH = os.path.expandvars(config["cache_path"])
    # directory path where the notifications will be saved
    # WARN: Do not edit this; Only edit this if you know what you are doing!
    CACHE_DIR = os.path.dirname(CACHE_PATH)
    # file path where the quotes are stored
    QUOTE_PATH = os.path.expandvars(config["quote_path"])
    # fallback if the quote DB has no quotes in them
    DEFAULT_QUOTE = config["default_quote"]
    # Watcher interval; Reflects how fast notifications will be rendered.
    INTERVAL = config["interval"]

    # handle IndexError
    if len(sys.argv) < 2:
        sys.argv = ("dummy", "dummy")
    match sys.argv[1]:
        case "subscribe":
            utils.create_parents_file(CACHE_PATH)  # mkdir --parents
            utils.create_parents_file(QUOTE_PATH)  # mkdir --parents
            utils.watcher(
                CACHE_PATH,
                lambda contents: sys.stdout.write(
                    "(box :spacing 20 :orientation 'vertical' :space-evenly false " +
                    # handle empty and display fallback
                    contents.replace("\n", " ") + ")\n"
                    if contents.strip()
                    else (
                        (FORMATS["empty"] + "\n")
                        % {"QUOTE": utils.get_rand_quote(QUOTE_PATH, DEFAULT_QUOTE)}
                    )
                ),
                INTERVAL,
            )
        case "rmid":
            # grep based off the notification id and then remove that YUCK literal entry from the log file
            utils.file_matched_index_rm(
                CACHE_PATH, f":identity ':::###::::XXXWWW{sys.argv[2]}===::'"
            )
        case "stats":
            sys.stdout.write(
                json.dumps(
                    utils.parse_and_print_stats(
                        pathlib.PosixPath(CACHE_PATH).read_text()
                    )
                )
                + "\n"
            )
        case "rm":
            utils.file_rm_line(CACHE_PATH, int(sys.argv[2]))
        case "quote":
            sys.stdout.write(utils.get_rand_quote(QUOTE_PATH, DEFAULT_QUOTE))
        case "cls":
            pathlib.PosixPath(CACHE_PATH).write_text("")
        case "init":
            def master_callback(details: dict):
                r"""Callback function that handles fetching and logging the notification details.

                Arguments:
                    details: the JSON that should have a similar structure to the following:
                    {
                       "summary": "Hello",
                       "body": "Who's there?",
                       "id": 16444442,
                       "urgency": "LOW"
                    }
                """
                details["TIMESTAMP_FORMAT"] = config["timestamp"]
                if not config["excluded_appnames"] or details["appname"] not in config["excluded_appnames"]:
                    saved_path = handlers.redir_to_handlers(FORMATS, details)
                    # actual point where the notification is being logged.
                    utils.file_add_line(CACHE_PATH, saved_path, HISTORY_LIMIT)

            # start eavesdropping on the org.freedesktop.Notifications interface and log the notification info
            cache.Eavesdropper(master_callback, CACHE_DIR).eavesdrop()

# vim:filetype=python
