#!/bin/bash

theme=$1
weather_code=$2

cp -r ~/.config/conky/Izar/icons/${theme}/${weather_code}.png ~/.cache/weather-icon.png
