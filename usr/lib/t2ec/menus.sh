#!/bin/sh

if [[ ! -d ~/.t2ecol ]]; then
    mkdir -p ~/.t2ecol
fi

if [[ $1 == --bbswitch ]]; then
    if [[ ! -f ~/.t2ecol/menu-bbswitch.sh ]]; then
        cp /usr/lib/t2ec/menu-template.sh ~/.t2ecol/menu-bbswitch.sh
    fi
    exec ~/.t2ecol/menu-bbswitch.sh
fi

if [[ $1 == --brightness ]]; then
    if [[ ! -f ~/.t2ecol/menu-brightness.sh ]]; then
        cp /usr/lib/t2ec/menu-template.sh ~/.t2ecol/menu-brightness.sh
    fi
    exec ~/.t2ecol/menu-brightness.sh
fi

if [[ $1 == --brightness ]]; then
    if [[ ! -f ~/.t2ecol/menu-brightness.sh ]]; then
        cp /usr/lib/t2ec/menu-template.sh ~/.t2ecol/menu-brightness.sh
    fi
    exec ~/.t2ecol/menu-brightness.sh
fi

if [[ $1 == --lbrightness ]]; then
    if [[ ! -f ~/.t2ecol/menu-lbrightness.sh ]]; then
        cp /usr/lib/t2ec/menu-template.sh ~/.t2ecol/menu-lbrightness.sh
    fi
    exec ~/.t2ecol/menu-lbrightness.sh
fi

if [[ $1 == --volume ]]; then
    if [[ ! -f ~/.t2ecol/menu-volume.sh ]]; then
        cp /usr/lib/t2ec/menu-template.sh ~/.t2ecol/menu-volume.sh
    fi
    exec ~/.t2ecol/menu-volume.sh
fi

if [[ $1 == --wifi ]]; then
    if [[ ! -f ~/.t2ecol/menu-wifi.sh ]]; then
        cp /usr/lib/t2ec/menu-template.sh ~/.t2ecol/menu-wifi.sh
    fi
    exec ~/.t2ecol/menu-wifi.sh
fi
