#!/usr/bin/env zsh

DUNST_LOG="$HOME/.cache/dunst.log.json"
[ ! -f "$DUNST_LOG" ] && touch "$DUNST_LOG"

function get_glyph() {
  case "$1" in
    "LOW") print "";;
    "NORMAL") print "";;
    "CRITICAL") print "";;
    *) print "";;
  esac
}

function append_json() {
  local json_object="$(jq -n                  \
    --arg an "$DUNST_APP_NAME"                \
    --arg su "$DUNST_SUMMARY"                 \
    --arg bd "$DUNST_BODY"                    \
    --arg ic "$DUNST_ICON_PATH"               \
    --arg ug "$DUNST_URGENCY"                 \
    --arg it "$DUNST_ID"                      \
    --arg pg "$DUNST_PROGRESS"                \
    --arg ct "$DUNST_CATEGORY"                \
    --arg st "$DUNST_STACK_TAG"               \
    --arg ul "$DUNST_URLS"                    \
    --arg to "$DUNST_TIMEOUT"                 \
    --arg ts "$DUNST_TIMESTAMP"               \
    --arg et "$DUNST_DESKTOP_ENTRY"           \
    --arg gl "$(get_glyph "$DUNST_URGENCY")"  \
    '{appname:$an,summary:$su,body:$bd,icon:$ic,urgency:$ug,glyph:$gl,id:$it,progress:$pg,category:$ct,stack:$st,urls:$ul,timeout:$to,timestamp:$ts,entry:$et}')"

  local data="$(cat "$DUNST_LOG")"
  local final
  if [[ "$data" == "" ]]; then 
    final="$(jq --compact-output --monochrome-output --null-input "{items:["$json_object"]}")"
  else
    final="$(print "$data" | jq --compact-output --monochrome-output ".items[.items|length] = "$json_object"")"
  fi
  print "$final" > "$DUNST_LOG"
}

function rmv_idx() {
  local data="$(cat "$DUNST_LOG" | jq --compact-output --monochrome-output "del(.items[]|select(.id==\"$1\"))")"
  print "$data" > "$DUNST_LOG"
}

function pop() {
  local data="$(cat "$DUNST_LOG" | jq --compact-output --monochrome-output "del(.items[0])")"
  print "$data" > "$DUNST_LOG"
}

function drop() {
  local data="$(cat "$DUNST_LOG" | jq --compact-output --monochrome-output "del(.items[.items|length-1])")"
  print "$data" > "$DUNST_LOG"
}

function clear_logs() { print > "$DUNST_LOG" }

function get_time_id() {
  local data="$(dunstctl history | jq --compact-output --monochrome-output ".data[0][]|select(.id.data==$1).timestamp.data")"
  date --date="@$data" +%H:%M
}

function get_crit_percent() {
  local format='(([(.items[]|select(.urgency=="CRITICAL"))]|length)/(.items|length))*100'
  local percent="$(cat ~/.cache/dunst.log.json | jq "$format")"
  print "$percent"
}

case "$1" in
  "rm_id") rmv_idx $2;;
  "time_st_id") get_time_id $2;;
  "pop") pop;;
  "drop") drop;;
  "glyph") get_glyph $2;;
  "clear") clear_logs;;
  "crits") get_crit_percent;;
  *) append_json;;
esac

unset DUNST_LOG

# vim:ft=zsh
