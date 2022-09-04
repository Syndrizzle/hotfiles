"""A DBus eavesdropper for org.freedesktop.Notifications
interface.

This is created mainly to cache the raw image data that is
sent by stupid applications like Spotify, Discord, etc.
Now that I think about it all of the electron clients do this.

Usually any application, if they had to they'd send the
notifications as a path i.e. caching the image themselves
and then returning the path to it.

Also, <https://specifications.freedesktop.org/notification-spec/latest/>
is a really nice manual. Give it a read.
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
import os
import pathlib
import sys
import typing

import dbus
import utils

from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib


# TODO: Use actual enumerations by using the enum module from the standard library.
class Urgency:
    """Acts as an Enum for indicating the urgency levels as per
    the notifications specification.

    You may use these to match wheter a specific notification belongs
    to a specific urgency class or, not.

    Attributes:
        LOW: Ads, Login, etc.
        NORMAL: USB unplugged, Drive mounted, etc.
        CRITICAL: Your PC is on fire, Storage Full, etc.
    """
    LOW = b"\x00"
    NORMAL = b"\x01"
    CRITICAL = b"\x02"


class Eavesdropper:
    """A quick and naive way of saving the image-data.

    The main idea is to keep a notification server running that
    implements image-data and image-path as per the freedesktop
    notification specification.
    And, then you'd run the eavesdropper which will connect to that
    interface (org.freedesktop.Notifications) and will continuously
    monitor that interface.
    And, if any application sends a notification, that contains a raw
    icon then it will be saved into the cache directory.

    Attributes:
        callback:
            The arbitrary subroutine that will executed on getting a notification.
        cache_dir:
            The directory path that all of those image-data would be saved.
    """

    # TODO: Segregate more.
    def __init__(
        self,
        callback: typing.Callable = print,
        cache_dir: str = "/tmp"
    ):
        """Assigns the CTOR parameters to the field variables (duh..)

        Arguments:
            callback: The arbitrary subroutine that will executed on getting a notification.
            cache_dir: The directory path that all of those image-data would be saved.
        """
        self.callback = callback
        self.cache_dir = f"{os.path.expandvars(cache_dir)}/image-data"
        # translation: mkdir --parents cache_dir
        pathlib.PosixPath(self.cache_dir).mkdir(parents=True, exist_ok=True)

    def _message_callback(
        self, _,
        message: dbus.lowlevel.MethodReturnMessage
        or dbus.lowlevel.MethodCallMessage
    ):
        """A filter callback for parsing the specific messages that are received from the DBus interface.

        Arguments:
            proxy_bus:
                The bus that sent the message.
            message:
                In this case a message is sent when the
                Notify method is called AND when the Notify method returns something.

        If the message is of type dbus.lowlevel.MethodCallMessage then this will NOT call the passed callback.
        """

        # we will be filtering this out as we only need the value that the method call returns
        # i.e. dbus.lowlevel.MethodReturnMessage
        if type(message) != dbus.lowlevel.MethodCallMessage:
            return

        args_list = message.get_args_list()
        # convert dbus data types to pythonic ones
        # eg: dbus.String('Hello') -> 'Hello'
        args_list = [utils.unwrap(item) for item in args_list]

        # set fallbacks like a fine gentleman
        details = {
            "appname": args_list[0].strip() or "Unknown",
            "summary": args_list[3].strip() or "Summary Unavailable.",
            "body": args_list[4].strip() or "Body Unavailable.",
            "id": datetime.datetime.now().strftime("%s"),
            "urgency": "unknown",
        }

        if "urgency" in args_list[6]:
            details["urgency"] = args_list[6]["urgency"]

        if args_list[2].strip():
            # if the iconpath value is a path i.e. if it has slashes on them
            # then assign that as the iconpath
            if "/" in args_list[2] or "." in args_list[2]:
                details["iconpath"] = args_list[2]
            else:
                # and if the iconpath is just a string that has no extensions or,
                # a pathlike structure like: 'bell' or 'custom-folder-bookmark'
                # it might have a dash (-) sign but not all the time.
                # then fetch that actual path of that icon as that is a part of the
                # icon theme naming convention and the current icon theme should probably have it
                details["iconpath"] = utils.get_gtk_icon_path(args_list[2])
        else:
            # if there are no icon hints then use fallback (generic bell)
            details["iconpath"] = utils.get_gtk_icon_path(
                "custom-notification")

        if "image-data" in args_list[6]:
            # capture the raw image bytes and save them to the cache_dir/x.png path
            details["iconpath"] = f"{self.cache_dir}/{details['appname']}-{details['id']}.png"
            utils.save_img_byte(
                args_list[6]["image-data"], details["iconpath"])

        # BUG: add a print statement -> init logger.py and disown the process
        # BUG: then you'll notice the notifications with value (progress) hint does not get logged
        if "value" in args_list[6]:
            details["progress"] = args_list[6]["value"]

        # execute arbitrary callback and passing details about the current notification.
        self.callback(details)

    # TODO: Segregate more.
    def eavesdrop(
        self,
        timeout: int or bool = False,
        timeout_callback: typing.Callable = print
    ):
        """Primes the session bus instance and starts a GLib mainloop.

        Arguments:
            timeout:
                Intervals for executing the callback.
            timeout_callback:
                Callback that will be executed on intervals.
        """
        DBusGMainLoop(set_as_default=True)

        rules = {
            "interface": "org.freedesktop.Notifications",
            "member": "Notify",
            "eavesdrop": "true",  # https://bugs.freedesktop.org/show_bug.cgi?id=39450
        }

        bus = dbus.SessionBus()
        # discard all other interfaces except org.freedesktop.Notifications
        # setting eavesdrop to true enables DBus to send the messages that are
        # not meant for you.
        # removing the eavesdrop key from rules will not send the Notify method's
        # contents to you (you can try and see what happens)
        bus.add_match_string(",".join([f"{key}={value}" for key, value in rules.items()]))
        bus.add_message_filter(self._message_callback)

        try:
            loop = GLib.MainLoop()
            if timeout:
                # executes a callback in intervals
                GLib.set_timeout(timeout, timeout_callback)
            loop.run()
        except (KeyboardInterrupt, Exception) as excep:
            sys.stderr.write(str(excep) + "\n")
            bus.close()


# vim:filetype=python
