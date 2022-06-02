#!/bin/bash

# USAGE:
# brightness up
# brightness down

if [ $1 = "up" ]
then
    sudo light -A 10 | cut -c 6-8 && tvolnoti-show -1 $HOME/.config/tvolnoti/themes/dark/bri-low.svg -2 $HOME/.config/tvolnoti/themes/dark/bri-medium.svg -3 $HOME/.config/tvolnoti/themes/dark/bri-high.svg -4 $HOME/.config/tvolnoti/themes/dark/bri-high.svg $(t2ec --lbrightness -N | cut -c 6-8)
elif [ $1 = "down" ]
then
    sudo light -U 10 | cut -c 6-8 && tvolnoti-show -1 $HOME/.config/tvolnoti/themes/dark/bri-low.svg -2 $HOME/.config/tvolnoti/themes/dark/bri-medium.svg -3 $HOME/.config/tvolnoti/themes/dark/bri-high.svg -4 $HOME/.config/tvolnoti/themes/dark/bri-high.svg $(t2ec --lbrightness -N | cut -c 6-8)
fi
