#!/bin/bash

killall conky
sleep 2s
		
conky -c $HOME/.config/conky/Thabit/Thabit.conf &> /dev/null &
