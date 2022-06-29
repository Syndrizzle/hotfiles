#!/bin/sh

if ! updates_arch=$(checkupdates 2> /dev/null | wc -l ); then
    updates_arch=0
fi

if ! updates_aur=$(paru -Qum 2> /dev/null | wc -l); then
    updates_aur=0
fi

updates=$((updates_arch + updates_aur))

if [ "$updates" -gt 0 ]; then
    dunstify --appname=Updates --icon=update-notifier "Updates Available" "You have $updates updates available. Please update your system."
    echo "$updates Updates"
else
    echo "Updated!"
fi
