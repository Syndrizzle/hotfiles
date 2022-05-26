STATUS=$(gamemoded -s | grep -o "inactive")
if [ "$STATUS" == "inactive" ]; then
	:
else
	echo $HOME/.local/icons/gamemode.svg
	echo Gamemode
#	notify-send "Gamemode is active." "Your system is now optimized for performance!" --icon=applications-games-symbolic --app-name=GameMode
fi
