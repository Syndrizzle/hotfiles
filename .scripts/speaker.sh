#!/bin/sh

case $1 in
      "*") command="" ;;
      "up") command="--allow-boost -i 5" ;;
      "down") command="--allow-boost -d 5" ;;
      "toogle") command="-t" ;;
esac

[ -n "$command" ] && pamixer $command 
mute=$(pamixer --get-mute)
if [ "$mute" = "true" ]; then
      volume="muted"
      icon=""
else 
      volume="$(pamixer --get-volume)"
      if [ "$volume" -gt 66 ]; then
            icon=""
      elif [ "$volume" -gt 33 ]; then
            icon=""
      elif [ "$volume" -gt 0 ]; then 
            icon=""
      else 
            icon=""
      fi
      volume="$volume%"
fi

echo "{\"content\": \"$volume\", \"icon\": \"$icon\"}"

