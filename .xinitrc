#!/bin/sh
[[ -f ~/.Xresources ]] && xrdb -merge -I$HOME ~/.Xresources
# start some nice programs

if [ -d /etc/X11/xinit/xinitrc.d ] ; then
 for f in /etc/X11/xinit/xinitrc.d/?*.sh ; do
  [ -x "$f" ] && . "$f"
 done
 unset f
fi
export XDG_CONFIG_HOME="$HOME/.config" &
export XDG_DATA_HOME="$HOME/.local/share" &
export XDG_CACHE_HOME="$HOME/.cache" &
exec fvwm2
