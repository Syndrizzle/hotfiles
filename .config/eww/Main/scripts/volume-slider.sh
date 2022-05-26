#!/usr/bin/env bash

if [[ "$(amixer sget Master | awk -F'[][]' '/Right:|Mono:/ && NF > 1 {print $4}')" = "on" ]]; then
    
    # search for the lines containing 'Right:' or 'Mono:', when more than 1 field exists
    # we strip the trailing '%' and round it up with printf "%0.0f" just in case
    vol=$(amixer sget Master | awk -F'[][]' '/Right:|Mono:/ && NF > 1 {sub(/%/, ""); printf "%0.0f\n", $2}')
    
    if [[ ${vol} -ge 80 ]]; then
        echo "󰕾"
        elif [[ ${vol} -ge 40 ]]; then
        echo "󰖀"
        elif [[ ${vol} -ge 10 ]]; then
        echo "󰕿"
    else
        echo "󰝟"
    fi
else
    echo "󰝟"
fi
