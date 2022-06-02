#!/bin/sh

killed=false
for pid in $(pidof -x volume.sh); do
    if [ $pid != $$ ]; then
        kill -9 $pid
        killed=true
    fi
done >/dev/null

if ! $killed; then
    eww -c $HOME/.config/eww/Indicators/ update volume-hidden=true
    eww -c $HOME/.config/eww/Indicators/ open volume-indicator
fi

eww -c $HOME/.config/eww/Indicators/ update volume-level=$(pamixer --get-volume)
eww -c $HOME/.config/eww/Indicators/ update volume-muted=$(pamixer --get-mute)
eww -c $HOME/.config/eww/Indicators/ update volume-hidden=false
sleep 2
eww -c $HOME/.config/eww/Indicators/ update volume-hidden=true
sleep 1
eww -c $HOME/.config/eww/Indicators/ close volume-indicator
unset killed