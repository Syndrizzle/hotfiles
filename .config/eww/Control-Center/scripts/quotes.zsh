#!/usr/bin/env zsh
source "$HOME/.config/eww/Control-Center/scripts/config.zsh"
_set_vars

mkdir "$DUNST_CACHE_DIR" 2>/dev/null
touch "$DUNST_QUOTES" 2>/dev/null

function add() { 
  [[ "$1" != "" ]] && print "$1" >> "$DUNST_QUOTES" 
}

function rm() { sed -i "$1d" "$DUNST_QUOTES" }

function rand() {
  local quote="$(cat $DUNST_QUOTES | shuf | head -n1)"
  [[ "$quote" == "" ]] && print "$DEFAULT_QUOTE" || print "$quote"
}

case "$1" in
  "add") add "$2";;
  "rm") rm "$2";;
  "rand") rand;;
  "all") cat "$DUNST_QUOTES";;
  *) print "$DEFAULT_QUOTE";;
esac

_unset_vars

# vim:ft=zsh
