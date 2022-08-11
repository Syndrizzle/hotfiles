#!/bin/bash

killall conky
sleep 2s
		
conky -c $HOME/.config/conky/Izar/Izar.conf &> /dev/null &
