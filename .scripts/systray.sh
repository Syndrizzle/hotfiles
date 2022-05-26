#!/bin/bash
if pgrep tint2; then
	pkill tint2
	exit 1
else
	exec tint2 &
fi
exit
