#!/bin/bash

# A script to toggle between various desktop layouts for bspwm, intended to be used for my polybar module.

CURRENT=$(cat $HOME/.config/bspwm/scripts/current-layout)

# Switch layouts

# Tiled
if [[ $CURRENT == "[~]" ]]; then
        bsp-layout set tiled
        echo "[=]" > $HOME/.config/bspwm/scripts/current-layout
fi

# Monocle
if [[ $CURRENT == "[=]" ]]; then
        bsp-layout set monocle
        echo "[M]" > $HOME/.config/bspwm/scripts/current-layout
fi

# Grid
if [[ $CURRENT == "[M]" ]]; then
        bsp-layout set grid
        echo "[ | ]" > $HOME/.config/bspwm/scripts/current-layout
fi

# Wide
if [[ $CURRENT == "[ | ]" ]]; then
        bsp-layout set wide
        echo "[_]" > $HOME/.config/bspwm/scripts/current-layout
fi

# Even
if [[ $CURRENT == "[_]" ]]; then
        bsp-layout set even
        echo "[~]" > $HOME/.config/bspwm/scripts/current-layout
fi