#!/bin/bash

# Set the source audio player here.
# Players supporting the MPRIS spec are supported.
# Examples: spotify, vlc, chrome, mpv and others.
# Use `playerctld` to always detect the latest player.
# See more here: https://github.com/altdesktop/playerctl/#selecting-players-to-control
PLAYER="spotify"

# Format of the information displayed
# Eg. {{ artist }} - {{ album }} - {{ title }}
# See more attributes here: https://github.com/altdesktop/playerctl/#printing-properties-and-metadata
FORMAT="{{ title }}"


PLAYERCTL_STATUS=$(playerctl --player=$PLAYER status 2>/dev/null)
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    STATUS=$PLAYERCTL_STATUS
else
    STATUS="No Music Detected"
fi

if [ "$STATUS" = "Stopped" ]; then
    echo "No Music Detected"
    elif [ "$STATUS" = "Paused"  ]; then
    playerctl --player=$PLAYER metadata --format "$FORMAT"
    elif [ "$STATUS" = "No Music Detected"  ]; then
    echo "$STATUS"
else
    playerctl --player=$PLAYER metadata --format "$FORMAT"
fi
