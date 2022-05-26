#!/bin/sh

# This script displays battery icon according to the charge level and charging state

# Author: Piotr Miller
# e-mail: nwg.piotr@gmail.com
# Website: http://nwg.pl
# Project: https://github.com/nwg-piotr/t2ec
# License: GPL3

# Dependencies: `acpi`
# argument: [-l] - append textual brightness level

bat=$(acpi -b)
if [[ $bat ]]; then
    state=$(echo ${bat} | awk '{print $3}')

    if [[ "$state" = "Not" ]]; then
        level=$(echo ${bat} | awk '{print $5}')
        level=${level::-1}

    else
        level=$(echo ${bat} | awk '{print $4}')
        if [[ "$state" == *"Unknown"* ]]; then
            level=${level::-1}
        else
            if [[ "$level" == "100%" ]]; then
              level=${level::-1}
            else
              level=${level::-2}
            fi
        fi
    fi

    if [[ $1 == -N* ]]; then
            echo "Bat: ${level}%"
    else
        if [[ "$bat" == *"until"* ]]; then

            if [[ "$level" -ge "95" ]]; then
                echo /usr/share/t2ec/bat-full-charging.svg
            elif [[ "$level" -ge "75" ]]; then
                echo /usr/share/t2ec/bat-threefourth-charging.svg
            elif [[ "$level" -ge "35" ]]; then
                echo /usr/share/t2ec/bat-half-charging.svg
            elif [[ "$level" -ge "15" ]]; then
                echo /usr/share/t2ec/bat-quarter-charging.svg
            else
                echo /usr/share/t2ec/bat-empty-charging.svg
            fi
        else
            if [[ "$level" -ge "95" ]]; then
                echo /usr/share/t2ec/bat-full.svg
            elif [[ "$level" -ge "75" ]]; then
                echo /usr/share/t2ec/bat-threefourth.svg
            elif [[ "$level" -ge "35" ]]; then
                echo /usr/share/t2ec/bat-half.svg
            elif [[ "$level" -ge "15" ]]; then
                echo /usr/share/t2ec/bat-quarter.svg
            else
                echo /usr/share/t2ec/bat-empty.svg
            fi
        fi
        if  [[ $1 = "-l" ]]; then
            echo ${level}%
        fi
    fi
fi