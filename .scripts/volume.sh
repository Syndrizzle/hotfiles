#!/bin/bash
ctl=/usr/bin/pamixer
lockfile=~/.scripts/volume-lockfile
iconDir="$HOME/.icons/tmp"

while [ -f "$lockfile" ]; do
    sleep 0.1;
done
touch "$lockfile"


getIcon() {
    vol=$("$ctl" --get-volume)
    mute=$("$ctl" --get-mute)
    #echo $vol

    if [ "$mute" == "true" ]; then
        echo "$iconDir/volume_muted.svg"
    else
        if [ "$vol" -eq 0 ]; then
            echo "$iconDir/volume_off.svg"
        elif [ "$vol" -lt 33 ]; then
            echo "$iconDir/volume_low.svg"
        elif [ "$vol" -lt 66 ]; then
            echo "$iconDir/volume_medium.svg"
        else
            echo "$iconDir/volume_high.svg"
        fi
    fi
}
 


val="5"
orig=$("$ctl" --get-volume)
subinc=5

if [ "$1" == "mute" ]; then
    opt="--mute"
    "$ctl" "$opt"
else
    if [ "$1" == "inc" ]; then
        opt="-i"
        if [ "$2" != '' ]; then val="$2"; fi

    elif [ "$1" == "dec" ]; then
        opt="-d"
        if [ "$2" != '' ]; then val="$2"; fi

    fi
    "$ctl" --unmute
    "$ctl" "$opt" "$val" &
    
    # Fake the animated volume
    for i in $(seq "$val"); do
        operation="+"
        inverse="-"
        if [ "$1" == "dec" ]; then
            operation="-"
            inverse="+"
        fi

        current=$(( ($orig "$operation" $i) "$inverse" 1 ))
        if [ "$current" -lt 0 ]; then
            current=0
            rm "$lockfile"
            exit 1
        fi
        
        dunstify -i "$(getIcon)" -u normal -h string:x-dunst-stack-tag:volume -a "Volume" "Volume at ${current}%" -h "int:value:${current}"
    done

fi

current=$("$ctl" --get-volume)
mute=$("$ctl" --get-mute)
ntext="Volume at $current%"
stacktag="volume"

if [ "$mute" == "true" ]; then
    ntext="Volume muted"
    stacktag="volume-muted"
fi

dunstify -i "$(getIcon)" -u normal -h string:x-dunst-stack-tag:"$stacktag" -a "Volume" "$ntext" -h "int:value:${current}"


rm "$lockfile"
