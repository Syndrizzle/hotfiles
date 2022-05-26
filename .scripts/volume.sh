#!/bin/bash

# USAGE:
# vol up
# vol down
# vol mute
# vol 10+
# vol 5-
# vol 60%

if [ $1 = "mute" ]
then
    amixer set Master toggle && if amixer get Master | grep -Fq '[off]'; then volnoti-show -m; else volnoti-show $(amixer get Master | grep -Po '[0-9]+(?=%)' | head -1); fi
    notify-send "Volume muted"
elif [ $1 = "up" ]
then
    str=`amixer set Master 5%+ && tvolnoti-show $(amixer get Master | grep -Po '[0-9]+(?=%)' | head -1)`
    vol=`echo $str| awk '{print $22}'`
    canberra-gtk-play -f /usr/share/sounds/freedesktop/stereo/audio-volume-change.oga
elif [ $1 = "down" ]
then
    str=`amixer set Master 5%- && tvolnoti-show $(amixer get Master | grep -Po '[0-9]+(?=%)' | head -1)`
    vol=`echo $str| awk '{print $22}'`
    canberra-gtk-play -f /usr/share/sounds/freedesktop/stereo/audio-volume-change.oga
else
    str=`amixer set Master $1 && tvolnoti-show $(amixer get Master | grep -Po '[0-9]+(?=%)' | head -1)`
    vol=`echo $str| awk '{print $22}'`
    canberra-gtk-play -f /usr/share/sounds/freedesktop/stereo/audio-volume-change.oga
fi
