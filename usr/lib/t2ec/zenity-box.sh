#!/bin/sh

# Set desired level with a zenity dialog box

# Dependencies: `alsa-utils`, `xbacklight` or `light-git`, `zenity`
# Arguments: [bri] | [vol]

if [[ $1 == bri* ]]; then

    # Prefer the `light` package, use `xbacklight` if `light` not found
    if [[ $(which light) == *"/light"* ]]; then
        lvl=$(light -G)
    else
        lvl=$(xbacklight -get)
    fi

    lvl=$(echo ${lvl} | awk '{ printf"%0.0f\n", $1 }')

    lvl=$(rof -P zenity zenity --scale --value ${lvl} --title "Brightness" --text "Set brightness level")

    if [[ ${lvl} ]]; then
        if [[ $(which light) == *"/light"* ]]; then
            exec light -S ${lvl}
        else
            exec xbacklight -set ${lvl}
        fi
    fi

elif [[ $1 == vol* ]]; then
    lvl=$(amixer sget Master | grep 'Right:' | awk -F'[][]' '{ print $2 }')
    lvl=${lvl::-1}
    lvl=$(rof -P zenity zenity --scale --value ${lvl} --title "Volume" --text "Set master volume level")
    if [[ ${lvl} ]]; then
        exec amixer sset 'Master' ${lvl}% -q
    fi
else
    echo "Allowed arguments: vol | bri"

fi