"""This file is tightly integrated with logger.py and won't work without it.
Unlike the files cache.py and utils.py.

This module just redirects specific presets of messages based on the source
(the application) that sent that message.

The redirector will call the specific function based off the appname, then
the called handler function will evaluate the YUCK literal and replace all
of the items on the format string with the passed attributes and then return it.
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

import datetime

import cache
import utils

# WARN: Subject to heavy change.
# TODO: Use OOP instead of passing around variables
# TODO: Add fallback / canned lambdas for common handler operations eg: formats["appname"] % attributes


# TODO: Handle '\n' in body / summary.
def redir_to_handlers(formats, attributes: dict) -> str:
    r"""Function for evaluating which handler function will be called.

    Before calling the handler function it will do some filtering and
    then actually call the handler which should return a fully evaluated
    YUCK literal string.

    Arguments:
        formats:
            All of the YUCK literal format strings.
        attributes:
            Details about the currently sent notification like summary, body, appname, etc.

    Returns:
        A str that is a primed YUCK literal with passed attributes.

        Example:
            Format:
                (_cardimage :identity ':::###::::XXXWWW%(id)s===::'
                            :close_action 'scripts/logger.py rmid %(id)s'
                            :limit_body '%(BODY_LIMITER)s'
                            :limit_summary '%(SUMMARY_LIMITER)s'
                            :summary '%(summary)s'
                            :body '%(body)s'
                            :close '繁'
                            :image_height 100
                            :image_width 100
                            :image '%(iconpath)s'
                            :appname '%(appname)s'
                            :icon '%(iconpath)s'
                            :icon_height 32
                            :icon_width 32
                            :timestamp '%(TIMESTAMP)s'
                            :urgency '%(URGENCY)s')
            Primed:
                (_cardimage :identity ':::###::::XXXWWW1658665761===::'
                            :close_action 'scripts/logger.py rmid 1658665761'
                            :limit_body '110'
                            :limit_summary '30'
                            :summary 'Picom'
                            :body 'The compositer is now disabled.'
                            :close '繁'
                            :image_height 100
                            :image_width 100
                            :image '/home/maker/.icons/custom/stock/128/custom-crying.png'
                            :appname 'Picom'
                            :icon '/home/maker/.icons/custom/stock/128/custom-crying.png'
                            :icon_height 32
                            :icon_width 32
                            :timestamp '17:59'
                            :urgency 'CRITICAL')
    """

    # assign the timestamp of the notification.
    attributes["TIMESTAMP"] = datetime.datetime.now().strftime(
        attributes["TIMESTAMP_FORMAT"])

    # turn the urgency values (which are in bytes) to a more readable format (string).
    match attributes["urgency"]:
        case cache.Urgency.LOW:
            attributes["URGENCY"] = "LOW"
        case cache.Urgency.NORMAL:
            attributes["URGENCY"] = "NORMAL"
        case cache.Urgency.CRITICAL:
            attributes["URGENCY"] = "CRITICAL"
        case _:
            attributes["URGENCY"] = "NORMAL"

    # handle next lines (especially discord code blocks)
    # NOTE: may make this only discord / firefox specific
    attributes["body"] = attributes["body"].replace("\n", " ")
    attributes["summary"] = attributes["summary"].replace("\n", " ")

    # check if there are any pango tags on the body and summary and if so
    # it will then remove it.
    if utils.contains_pango(attributes["body"]):
        attributes["body"] = utils.strip_pango_tags(attributes["body"])
    if utils.contains_pango(attributes["summary"]):
        attributes["summary"] = utils.strip_pango_tags(
            attributes["summary"]
        )

    # it will replace all of the apostrophes with \' i.e. escape them.
    if "'" in attributes["body"]:
        attributes["body"] = attributes["body"].replace("'", "\\'")
    if "'" in attributes["summary"]:
        attributes["summary"] = attributes["summary"].replace("'", "\\'")

    # only 15 chars would be taken if the summary has non-english charecters.
    attributes["SUMMARY_LIMITER"] = ""
    summary_lang_char_check = utils.has_non_english_chars(
        attributes["summary"][:15]
    )
    # if summary has asian characters like Japanese, Hindi or, Chinese
    if summary_lang_char_check["CJK"]:
        attributes["SUMMARY_LIMITER"] = 14
    # if summary has Cyrillic characters
    elif summary_lang_char_check["CYR"]:
        attributes["SUMMARY_LIMITER"] = 30

    attributes["BODY_LIMITER"] = ""
    body_lang_char_check = utils.has_non_english_chars(attributes["body"][:70])
    if body_lang_char_check["CJK"]:
        attributes["BODY_LIMITER"] = 80
    elif body_lang_char_check["CYR"]:
        attributes["BODY_LIMITER"] = 110
    else:
        attributes["BODY_LIMITER"] = 100

    # redirect to handlers
    match attributes["appname"]:
        case "notify-send":
            return notify_send_handler(formats, attributes)
        case "volume":
            return volume_handler(formats, attributes)
        case "backlight":
            return brightness_handler(formats, attributes)
        case "shot":
            return shot_handler(formats, attributes)
        case "shot_icon":
            return shot_icon_handler(formats, attributes)
        case "todo":
            return todo_handler(formats, attributes)
        case "Spotify":
            return Spotify_handler(formats, attributes)
        case _:
            return default_handler(formats, attributes)


# TODO: Segregate redir_to_handlers into more utility / helper functions.
# NOTE: Currently it is a bit hard to utilize config values without making the code dirtier.
# NOTE: For instance shot_handler will eventually load the command strings from the config JSON for attributes["DELETE"]. See logger.py
def shot_handler(formats, attributes: dict) -> str:
    """Handler for screenshot related notifications.

    Note that this handler will only handle the screenshots itself.
    That is, it won't handle it if say.. the screenshot is copied to the clipboard, etc.
    All of those are handled by shot_icon_handler.

    Arguments:
        formats: See redir_to_handlers.
        attributes: See redir_to_handlers.

    Returns:
        See redir_to_handlers
    """
    # TODO: Make this better
    attributes["DELETE"] = f"rm --force \\'{attributes['iconpath']}\\' && scripts/logger.py rmid {attributes['id']}"
    attributes["OPEN"] = f"xdg-open \\'{attributes['iconpath']}\\'"

    # capitalize words i.e. notify_send -> Notify Send
    attributes["appname"] = utils.prettify_name(attributes["appname"])
    return formats["shot"] % attributes


def Spotify_handler(formats, attributes: dict) -> str:
    """Handler for notifications related to the official electron client for Spotify.

    Arguments:
        formats: See redir_to_handlers.
        attributes: See redir_to_handlers.

    Returns:
        See redir_to_handlers
    """
    return formats["Spotify"] % attributes


def default_handler(formats, attributes: dict) -> str:
    r"""Handler for basic notifications. The notifications that are ordinary.
    Or, rather the notifications that do not match any of the match-cases in
    the redir_to_handlers function.

    Arguments:
        formats: See redir_to_handlers.
        attributes: See redir_to_handlers.

    Example:
        notify-send Hello
        notify-send Greetings
        notify-send -u low -i bell Greetings Ding\!
        notify-send -a appname-does-not-exist -i bell Yo

    Returns:
        See redir_to_handlers
    """
    attributes["appname"] = utils.prettify_name(attributes["appname"])
    return formats["default"] % attributes


def notify_send_handler(formats, attributes: dict) -> str:
    """Handler for notifications related to the notify-send command.

    See:
        default_handler

    Arguments:
        formats: See redir_to_handlers.
        attributes: See redir_to_handlers.

    Returns:
        See redir_to_handlers
    """
    attributes["appname"] = utils.prettify_name(attributes["appname"])
    return formats["notify-send"] % attributes


def brightness_handler(formats, attributes: dict) -> str:
    """Handler for notifications related to brightness control.

    Arguments:
        formats: See redir_to_handlers.
        attributes: See redir_to_handlers.

    Returns:
        See redir_to_handlers
    """
    attributes["appname"] = utils.prettify_name(attributes["appname"])
    return formats["brightness"] % attributes


def volume_handler(formats, attributes: dict) -> str:
    """Handler for notifications related to volume control.

    Arguments:
        formats: See redir_to_handlers.
        attributes: See redir_to_handlers.

    Returns:
        See redir_to_handlers
    """
    attributes["appname"] = utils.prettify_name(attributes["appname"])
    return formats["volume"] % attributes


# TODO: Make this general purpose and not just todo specific.
def todo_handler(formats, attributes: dict) -> str:
    """Handler for notifications related to todo-bin CLI application by Siddomy.

    The notification body needs to be in a particular format in order for it to register.
    That is: <completed_tasks> tasks done and <total_tasks> are remaining.

    The fragments <completed_tasks> and <total_tasks> will be picked up by this handler.

    Arguments:
        formats: See redir_to_handlers.
        attributes: See redir_to_handlers.

    Returns:
        See redir_to_handlers
    """
    splitted = attributes["body"].split(" ")
    attributes["TOTAL"] = int(splitted[4])
    attributes["DONE"] = int(splitted[0])

    # handle division by zero
    attributes["PERC"] = (attributes["DONE"] / attributes["TOTAL"]
                          ) * 100 if attributes["DONE"] > 0 else 0

    attributes["appname"] = utils.prettify_name(attributes["appname"])
    return formats["todo"] % attributes


def shot_icon_handler(formats, attributes: dict) -> str:
    """Almost same as default_handler only just it uses a different icon.

    Redundant, but still nice to if you want to add additional
    functionalities on this particular appname.

    See:
        default_handler
        shot_handler

    Arguments:
        formats: See redir_to_handlers.
        attributes: See redir_to_handlers.

    Returns:
        See redir_to_handlers
    """
    attributes["appname"] = utils.prettify_name(attributes["appname"])
    return formats["shot_icon"] % attributes


# vim:filetype=python
