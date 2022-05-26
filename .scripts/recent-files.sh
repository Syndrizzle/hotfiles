#!/usr/bin/env bash

## Copyright (C) 2020-2022 Aditya Shakya <adi1090x@gmail.com>
## Everyone is permitted to copy and distribute copies of this file under GNU-GPL3

## Openbox Pipemenu to parse .recently-used.xbel and generate openbox pipe menu

maximum_entries=15

#######################################################################

# look for recently-used.xbel
if [ "$XDG_DATA_HOME" ] && [ -r "${XDG_DATA_HOME}/recently-used.xbel" ]; then
    file_path="${XDG_DATA_HOME}/recently-used.xbel"
elif [ -r "${HOME}/.local/share/recently-used.xbel" ]; then
    file_path="${HOME}/.local/share/recently-used.xbel"
elif [ -r "${HOME}/.recently-used.xbel" ]; then
    file_path="${HOME}/.recently-used.xbel"
else
    echo "$0: cannot find a readable recently-used.xbel file" >&2
    echo '<openbox_pipe_menu>
<separator label="No recently-used.xbel file found." />
</openbox_pipe_menu>'
    exit 1
fi

# if argument is --clear, empty .recently-used.xbel
[[ "$1" == '--clear' ]] && {
    cat <<':EOF' > "${file_path}"
<?xml version="1.0" encoding="UTF-8"?>
<xbel version="1.0"
      xmlns:bookmark="http://www.freedesktop.org/standards/desktop-bookmarks"
      xmlns:mime="http://www.freedesktop.org/standards/shared-mime-info"
>
</xbel>
:EOF
    exit
}

maximum_entries=$((maximum_entries+2))

pre='    <item label="'
mid='">
    <action name="Execute"><command>'
post='</command></action>
    </item>'

files=$(tac "${file_path}" | awk -v MAX="$maximum_entries" -v PR="$pre" -v MI="$mid" -v PO="$post" 'BEGIN {
    RS="</bookmark>";
    FS="<info>";
}
(NR == MAX) {exit}
!/<bookmark/ {next}
!/href=[\"'\'']file:\/\// {next}
# $1 is the command, $2 the file path
{
    sub(/^.*exec=\"\&apos\;/,"",$1)
    sub(/\&apos\;.*$/,"",$1)
    sub(/ *%./,"",$1)
    sub(/^.*file:\/\//,"",$2)
    sub(/[\"'\''].*$/,"",$2)
    gsub(/%22/,"\\&quot;",$2)
    gsub(/%3C/,"\\&lt;",$2)
    gsub(/%3E/,"\\&gt;",$2)
    name=$2
    sub(/^.*\//,"",name)
    gsub(/_/,"__",name)
    gsub(/\&apos;/,"\\&apos;\\&quot;\\&apos;\\&quot;\\&apos;",$2)
    print (PR name MI $1 " '"'"'" $2 "'"'"'" PO)
}')

# use perl to decode urlencoded characters
files=$(perl -MURI::Escape -e 'print uri_unescape($ARGV[0]);' "$files")

output='<openbox_pipe_menu>
'"$files"'
<separator/>
  <item label="Clear Recent Files">
    <action name="Execute">
      <command>&apos;'"$0"'&apos; --clear</command>
    </action>
  </item>
</openbox_pipe_menu>
'
printf '%s' "$output"  # printf because echo sometimes eats backslashes
