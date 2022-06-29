#!/usr/bin/env bash
path=$HOME/.config/rofi/window-switcher/box.rasi
result="$(xprop -root _NET_CLIENT_LIST)"
options="\
       -kb-cancel "Alt+Escape,Escape" \
       -kb-accept-entry "!Alt-Tab,Return,!Alt-l,!Alt-h"\
       -kb-row-down "Alt+Tab,Alt+l,Alt+ISO_Left_Tab" \
       -kb-row-up "Alt+Shift+Tab,Alt+h,Alt+n""
no_of_window=0
for x in $result
do
   if [[ "$x" == *'x'* ]];
    then
        ((no_of_window+=1))
    fi
done
if [[ $no_of_window = 1 ]]; then
    rofi -no-lazy-grab -show window -icon-theme "Papirus" -theme $path -width 11 $options &
elif [[ $no_of_window = 2 ]]; then
    rofi -no-lazy-grab -show window -icon-theme "Papirus" -theme $path -width 20 $options &
elif [[ $no_of_window = 3 ]]; then
    rofi -no-lazy-grab -show window -icon-theme "Papirus" -theme $path -width 29 $options &
elif [[ $no_of_window = 4 ]]; then
    rofi -no-lazy-grab -show window -icon-theme "Papirus" -theme $path -width 37 $options &
elif [[ $no_of_window = 5 ]]; then
    rofi -no-lazy-grab -show window -icon-theme "Papirus" -theme $path -width 45 $options &
elif [[ $no_of_window = 6 ]]; then
    rofi -no-lazy-grab -show window -icon-theme "Papirus" -theme $path -width 55 $options &
elif [[ $no_of_window = 7 ]]; then
    rofi -no-lazy-grab -show window -icon-theme "Papirus" -theme $path -width 64 $options &
else
 rofi -show window -theme $path -width 72 $options
fi
# as soon as alt+tab is pressed it will show you a rofi window menu and it wont go to next window by defalut so follownig line helps to press a fake Tab key
xdotool key l

