#!/usr/bin/env bash

# Prefer the `light` package, use `xbacklight` if `light` not found
if [[ $(which light) == *"/light"* ]]
then
    b=$(light -G)
else
    b=$(xbacklight -get)
fi

icon() {
    # Lets round the float result
    bri=$(echo "($b+0.5)/1" | bc)
    
    if [[ "$bri" -gt "90" ]]; then
        echo "󰃠"
        elif [[ "$bri" -gt "50" ]]; then
        echo "󰃝"
        elif [[ "$bri" -gt "30" ]]; then
        echo "󰃟"
    else
        echo "󰃞"
    fi
}

getbri() {
    bri=$(echo "($b+0.5)/1" | bc)
    echo $bri
}

if [[ $1 == "--icon" ]]; then
    icon
    elif [[ $1 == "--value" ]]; then
    getbri
fi