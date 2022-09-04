#!/bin/bash

killall conky
sleep 2s
		
conky -c $HOME/.config/conky/Betelgeuse/Betelgeuse.conf &> /dev/null &
