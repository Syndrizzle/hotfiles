#!/bin/sh
b=$(light -G)
killed=false
for pid in $(pidof -x brightness.sh); do
    if [ $pid != $$ ]; then
        kill -9 $pid
        killed=true
    fi
done >/dev/null

if ! $killed; then
    eww -c $HOME/.config/eww/Indicators/ update brightness-hidden=true
    eww -c $HOME/.config/eww/Indicators/ open brightness-indicator
fi

eww -c $HOME/.config/eww/Indicators/ update brightness-level=$(echo "($b+0.5)/1" | bc)
eww -c $HOME/.config/eww/Indicators/ update brightness-hidden=false
sleep 2
eww -c $HOME/.config/eww/Indicators/ update brightness-hidden=true
sleep 1
eww -c $HOME/.config/eww/Indicators/ close brightness-indicator
unset killed