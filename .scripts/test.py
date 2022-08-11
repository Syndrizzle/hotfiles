#!/usr/bin/env python3

from gi.repository import Playerctl, GLib

player = Playerctl.Player()


def on_metadata(player, metadata):
    if 'xesam:artist' in metadata.keys() and 'xesam:title' in metadata.keys():
        print('{title}'.format(title=metadata['xesam:title']))

player.connect('metadata', on_metadata)
