#!/bin/sh

# Author: Piotr Miller
# e-mail: nwg.piotr@gmail.com
# Website: http://nwg.pl
# Project: https://github.com/nwg-piotr/t2ec
# License: GPL3

# Credits: https://github.com/dsboger/gnome-shell-extension-bumblebee-status
# no-bumblebee icon by @edskeye

# Argument: [-N]

if [[ -f "/proc/acpi/bbswitch" ]]; then

    bb_status=$(cat /proc/acpi/bbswitch | awk -F ' ' '{print $2}')
    if [[ "$bb_status" = "ON" ]]; then

        t=$(nvidia-smi -q -d TEMPERATURE | grep "GPU Current Temp" | awk -F ' ' '{ print $5 }')

        if [[ $1 == -N* ]]; then
            echo "Nvidia: ${t}℃"
        else
            echo /usr/share/t2ec/nvidia.svg;
            echo ${t}"℃"
        fi

    elif [[ "$bb_status" = "OFF" ]]; then
        if [[ $1 == -N* ]]; then
            echo "Nvidia: off"
        else
            echo /usr/share/t2ec/nvidia-off.svg
        fi
    fi
else
    if [[ $1 == -N* ]]; then
            echo "No Bumblebee"
    else
        echo /usr/share/t2ec/no-bumblebee.svg
    fi
fi