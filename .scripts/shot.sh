#!/usr/bin/env bash

rofi_command="rofi"

time=$(date +%s)
geometry=$(xrandr | head -n1 | cut -d',' -f2 | tr -d '[:blank:],current')
dir="$HOME/Pictures"
file="screenshot-$time-$geometry.png"

# Buttons
screen=""
area=""
window=""
infive=""
inten=""

# notify and view screenshot
function notify_view() {
  notify-send -i xclipboard -a shot_clipboard -u low Screenshot "Copied to clipboard."
  viewnior "$dir/$file"
  if [[ -e "$dir/$file" ]]; then
    notify-send -i "$dir/$file" -a shot -u low Screenshot "Screenshot Saved."
  else
    notify-send -i user-trash-full -a shot_deleted -u low Screenshot "Screenshot Deleted."
  fi
}

# countdown
function countdown() {
  for sec in $(seq "$1" -1 1); do
    notify-send -i custom-clock-blue -a shot_icon -t 1000 Screenshot "Taking shot in $sec..."
    sleep 1
  done
}

# take shots
function shotnow() {
  cd "$dir" && sleep 0.5 && maim -u -f png | tee "$file" | xclip -selection clipboard -t image/png
  notify_view
}

function shot5() {
  countdown '5'
  sleep 1 && cd "$dir" && maim -u -f png | tee "$file" | xclip -selection clipboard -t image/png
  notify_view
}

function shot10() {
  countdown '10'
  sleep 1 && cd "$dir" && maim -u -f png | tee "$file" | xclip -selection clipboard -t image/png
  notify_view
}

function shotwin() {
  cd "$dir" && maim -u -f png -i "$(xdotool getactivewindow)" | tee "$file" | xclip -selection clipboard -t image/png
  notify_view
}

function shotarea() {
  cd "$dir" || return
  local bytes
    if [ "$(pgrep -x picom)" ]; then
      bytes="$(maim -u -f png -s -b 2 -c 0.35,0.55,0.85,0.25 -l | base64)"
  else
      bytes="$(maim -u -f png -s -b 2 | base64)"
  fi
  if [ "$bytes" ]; then
    echo "$bytes" | base64 --decode | tee "$file" | xclip -selection clipboard -t image/png
    notify_view
  else
    notify-send -i custom-cancel -a shot_icon -u low Screenshot "Operation Cancelled."
  fi
}

# Variable passed to rofi
options="$screen\n$area\n$window\n$infive\n$inten"

chosen="$(echo -e "$options" | "$rofi_command" -theme ~/.config/rofi/screenshot/config.rasi -p 'Take Screenshot' -dmenu -hover-select -me-select-entry '' -me-accept-entry MousePrimary -location 4 -selected-row 1)"
case $chosen in
  "$screen")
    shotnow
    ;;
  "$area")
    shotarea
    ;;
  "$window")
    shotwin
    ;;
  "$infive")
    shot5
    ;;
  "$inten")
    shot10
    ;;
esac

# vim:ft=sh
