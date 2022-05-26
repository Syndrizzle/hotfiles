#!/bin/bash

# This script display an appropriate volume icon according to the volume level

# Authors: Piotr Miller, @natemaia
# e-mail: nwg.piotr@gmail.com
# Website: http://nwg.pl
# Project: https://github.com/nwg-piotr/t2ec
# License: GPL3

# Dependencies: `alsa-utils`
# arguments: [up] | [down] | [toggle] | [<level>] [-N]

if [[ $1 == up ]]; then
    amixer set Master 5%+ -q
elif [[ $1 == down ]]; then
    amixer set Master 5%- -q
elif [[ $1 == toggle ]]; then
    amixer sset Master toggle -q
else
    # If none of above, check if argument is a valid int, set volume if so
    if [[ $(($1)) == $1 ]] && [[ "$1" -ge 0 ]] && [[ "$1" -le 100 ]]; then
        amixer set Master "$1"% -q
    fi
fi

if [[ "$(amixer sget Master | grep -e 'Right:' -e 'Mono:')" == *"[on]"* ]]; then

    # search for the lines containing 'Right:' or 'Mono:', when more than 1 field exists
    # we strip the trailing '%' and round it up with printf "%0.0f" just in case
    vol=$(amixer sget Master | awk -F'[][]' '/Right:|Mono:/ && NF > 1 {sub(/%/, ""); printf "%0.0f\n", $2}')

    if [[ $1 == -N* ]]; then
        echo "Vol: ${vol}%"
    else
        if [[ ${vol} -ge 90 ]]; then
            echo /usr/share/t2ec/vol-full.svg
        elif [[ ${vol} -ge 40 ]]; then
            echo /usr/share/t2ec/vol-medium.svg
        elif [[ ${vol} -ge 10 ]]; then
            echo /usr/share/t2ec/vol-low.svg
        else
            echo /usr/share/t2ec/vol-lowest.svg
        fi
        echo ${vol}%
    fi
else
    vol=$(amixer sget Master | awk -F'[][]' '/Right:|Mono:/ && NF > 1 {sub(/%/, ""); printf "%0.0f\n", $2}')
    echo /usr/share/t2ec/vol-muted.svg
    echo ${vol}%
fi
