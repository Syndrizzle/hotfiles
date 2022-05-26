#!/bin/bash
# Default Values
LOCATION=0
QRCODE_LOCATION=$LOCATION
Y_AXIS=0
X_AXIS=0
NOTIFICATIONS_INIT="off"
QRCODE_DIR="/tmp/"
WIDTH_FIX_MAIN=0
WIDTH_FIX_STATUS=0
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PASSWORD_ENTER="if connection is stored, hit enter/esc"
WIRELESS_INTERFACES=($(nmcli device | awk '$2=="wifi" {print $1}'))
WIRELESS_INTERFACES_PRODUCT=()
WLAN_INT=0

function initialization() {
	if [[ -f "$DIR/rofi-network-manager.conf" ]]; then
		source "$DIR/rofi-network-manager.conf"
	elif [[ -f "${XDG_CONFIG_HOME:-$HOME/.config}/rofi/rofi-network-manager.conf" ]]; then
		source "${XDG_CONFIG_HOME:-$HOME/.config}/rofi/rofi-network-manager.conf"
	fi
	if [[ -f "$DIR/rofi-network-manager.rasi" ]]; then
		RASI_DIR="$DIR/rofi-network-manager.rasi"
	elif [[ -f "${XDG_CONFIG_HOME:-$HOME/.config}/rofi/rofi-network-manager.rasi" ]]; then
		RASI_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/rofi/rofi-network-manager.rasi"
	fi
	for i in "${WIRELESS_INTERFACES[@]}"
	do
		WIRELESS_INTERFACES_PRODUCT+=($(nmcli -f general.product device show $i | awk '{print $2}'))
	done
	wireless_interface_state
	ethernet_interface_state
}
function notification() {
	if [[ "$NOTIFICATIONS_INIT" == "on" ]]; then
		dunstify -r $1 -u $2 $3 "$4"
	fi
}
function wireless_interface_state() {
	ACTIVE_SSID=$(nmcli device status | grep "^${WIRELESS_INTERFACES[WLAN_INT]}." |  awk '{print $4}')
	WIFI_CON_STATE=$(nmcli device status | grep "^${WIRELESS_INTERFACES[WLAN_INT]}." |  awk '{print $3}')
	if [[ "$WIFI_CON_STATE" =~ "unavailable" ]]; then
		WIFI_LIST="   ***Wi-Fi Disabled***"
		WIFI_SWITCH="~Wi-Fi On"
		LINES=6
	elif [[ "$WIFI_CON_STATE" =~ "connected" ]]; then
		WIFI_LIST=$(nmcli --fields IN-USE,SSID,SECURITY,BARS device wifi list ifname ${WIRELESS_INTERFACES[WLAN_INT]} | sed "s/^IN-USE\s//g" | sed "/--/d" | sed "/*/d" | sed "s/^ *//")
		LINES=$(echo -e "$WIFI_LIST" | wc -l)
		if [[ "$ACTIVE_SSID" == "--" ]]; then
			WIFI_SWITCH="~Manual/Hidden\n~Wi-Fi Off"
			((LINES+=6))
		else
			WIFI_SWITCH="~Disconnect\n~Manual/Hidden\n~Share Wifi Password\n~Wi-Fi Off"
			((LINES+=8))
		fi
	fi
	WIDTH=$(echo "$WIFI_LIST" | head -n 1 | awk '{print length($0); }')
}
function ethernet_interface_state() {
	WIRE_CON_STATE=$(nmcli device status | grep "ethernet"  |  awk '{print $3}')
	if [[ "$WIRE_CON_STATE" == "disconnected" ]]; then
		WIRE_SWITCH="~Eth On"
	elif [[ "$WIRE_CON_STATE" == "connected" ]]; then
		WIRE_SWITCH="~Eth Off"
	elif [[ "$WIRE_CON_STATE" == "unavailable" ]]; then
		WIRE_SWITCH=" ***Wired Unavailable***"
	fi
}
function rofi_menu() {	
	((WIDTH+=$WIDTH_FIX_MAIN))
	if [[ $LINES -eq  0 ]]; then
	notification "5" "normal" "Initialization" "Some connections are being initializing.Please try again in a moment."
	exit
	fi
	PROMPT=${WIRELESS_INTERFACES_PRODUCT[WLAN_INT]}[${WIRELESS_INTERFACES[WLAN_INT]}]
	if [[ $(nmcli device | awk '$2=="wifi" {print $1}' | wc -l) -ne "1" ]]; then
		((LINES+=1))
		SELECTION=$(echo -e "$WIFI_LIST\n~Scan\n$WIFI_SWITCH\n$WIRE_SWITCH\n~Change Wifi Interface\n~Status\n~Restart Network" | uniq -u | \
		rofi -dmenu -location "$LOCATION" -yoffset "$Y_AXIS" -xoffset "$X_AXIS" \
		-a "0" -theme "$RASI_DIR" -theme-str '
		window{width: '"$(($WIDTH/2))"'em;}
		listview{lines: '"$LINES"';}
		textbox-prompt-colon{str:"'$PROMPT':";}
		')
	else
		SELECTION=$(echo -e "$WIFI_LIST\n~Scan\n$WIFI_SWITCH\n$WIRE_SWITCH\n~Status\n~Restart Network" | uniq -u | \
		rofi -dmenu -location "$LOCATION" -yoffset "$Y_AXIS" -xoffset "$X_AXIS" \
		-a "0" -theme "$RASI_DIR" -theme-str '
		window{width: '"$(($WIDTH/2))"'em;}
		listview{lines: '"$LINES"';}
		textbox-prompt-colon{str:"'$PROMPT':";}
		')
	fi
	SSID_SELECTION=$(echo "$SELECTION" | sed  "s/\s\{2,\}/\|/g" | awk -F "|" '{print $1}')
	selection_action
}
function change_wireless_interface() {
	PROMPT=">_"
	if [[ $(nmcli device | awk '$2=="wifi" {print $1}' | wc -l) -eq "2" ]]; then
		if [[ $WLAN_INT -eq "0" ]]; then
			WLAN_INT=1
		else
			WLAN_INT=0
		fi
	else
		for i in "${!WIRELESS_INTERFACES[@]}"
		do
			LIST_WLAN_INT=(${LIST_WLAN_INT[@]}"${WIRELESS_INTERFACES_PRODUCT[$i]}[${WIRELESS_INTERFACES[$i]}]\n")
		done
		LINES=$(nmcli device | awk '$2=="wifi" {print $1}' | wc -l)
		CHANGE_WLAN_INT=$(echo -e  ${LIST_WLAN_INT[@]}| \
		rofi -dmenu -location "$LOCATION" -yoffset "$Y_AXIS" -xoffset "$X_AXIS" \
		-theme "$RASI_DIR" -theme-str '
		window{width: '"$(($WIDTH/2))"'em;}
		listview{lines: '"$LINES"';}
		textbox-prompt-colon{str:"'$PROMPT':";}
		')
		for i in "${!WIRELESS_INTERFACES[@]}"
		do
			if [[ $CHANGE_WLAN_INT == "${WIRELESS_INTERFACES_PRODUCT[$i]}[${WIRELESS_INTERFACES[$i]}]" ]];then
				WLAN_INT=$i
				break
			fi
		done
	fi
	wireless_interface_state
	rofi_menu
}
function scan() {
	if [[ "$WIFI_CON_STATE" =~ "unavailable" ]]; then
		change_wifi_state "5" "normal" "Wi-Fi" "Enabling Wi-Fi connection" "on"
		sleep 2
	fi
	notification "5" "normal" "-t 0 Wifi" "Please Wait Scanning"
	WIFI_LIST=$(nmcli --fields IN-USE,SSID,SECURITY,BARS device wifi list ifname ${WIRELESS_INTERFACES[WLAN_INT]} --rescan yes | sed "s/^IN-USE\s//g" | sed "/--/d" | sed "/*/d" | sed "s/^ *//")
	wireless_interface_state
	notification "5" "normal" "-t 1 Wifi" "Please Wait Scanning"
	rofi_menu
}
function change_wifi_state() {
	notification $1 $2 $3 "$4"
	nmcli radio wifi $5
}
function change_wire_state() {
	notification $1 $2 $3 "$4"
	nmcli con $5 "$(nmcli -t -f NAME,TYPE con | grep "ethernet" | cut -d":" -f1)"
}
function net_restart() {
	notification $1 $2 $3 "$4"
	nmcli networking off
	sleep 3
	nmcli networking on
}
function disconnect() {
	TRUE_ACTIVE_SSID=$(nmcli -t -f GENERAL.CONNECTION dev show ${WIRELESS_INTERFACES[WLAN_INT]} |  cut -d ':' -f2)
	notification $1 $2 $3 "You're now disconnected from Wi-Fi network '$TRUE_ACTIVE_SSID'"
	nmcli con down id  "$TRUE_ACTIVE_SSID"
}
function check_wifi_connected() {
	if [[ "$(nmcli device status | grep "^${WIRELESS_INTERFACES[WLAN_INT]}." | awk '{print $3}')" == "connected" ]]; then
		disconnect "5" "normal" "Connection_Terminated"
	fi
}
function connect() {
	check_wifi_connected
	notification "5" "normal" "Wi-Fi" "Connecting to $1"
	if [ $(nmcli dev wifi con "$1" password "$2" ifname ${WIRELESS_INTERFACES[WLAN_INT]}| grep -c "successfully activated" ) == "1" ]; then
		notification "5" "normal" "Connection_Established" "You're now connected to Wi-Fi network '$1' "
	else
		notification "5" "normal" "Connection_Error" "Connection can not be established"
	fi
}
function stored_connection() {
	check_wifi_connected
	notification "5" "normal" "Wi-Fi" "Connecting to $1"
	if [ $(nmcli dev wifi con "$1" ifname ${WIRELESS_INTERFACES[WLAN_INT]}| grep -c "successfully activated" ) = "1" ]; then
		notification "5" "normal" "Connection_Established" "You're now connected to Wi-Fi network '$1' "
	else
		notification "5" "normal" "Connection_Error" "Connection can not be established"
	fi
}
function ssid_manual() {
	LINES=0
	PROMPT="Enter_SSID"
	WIDTH=35
	SSID=$(	rofi -dmenu -location "$LOCATION" -yoffset "$Y_AXIS" -xoffset "$X_AXIS" \
	-theme "$RASI_DIR" -theme-str '
	window{width: '"$(($WIDTH/2))"'em;}
	listview{lines: '"$LINES"';}
	textbox-prompt-colon{str:"'$PROMPT':";}
	')
	echo $SSID
	if [[ ! -z $SSID ]]; then
		LINES=1
		WIDTH=$(echo "$PASSWORD_ENTER" | awk '{print length($0); }')
		((WIDTH+=$WIDTH_FIX_MAIN))
		PROMPT="Enter_Password"
		PASS=$(echo "$PASSWORD_ENTER" | \
		rofi -dmenu -location "$LOCATION" -yoffset "$Y_AXIS" -xoffset "$X_AXIS" \
		-password -theme "$RASI_DIR" -theme-str '
		window{width: '"$(($WIDTH/2))"'em;}
		listview{lines: '"$LINES"';}
		textbox-prompt-colon{str:"'$PROMPT':";}
		')
		if [[ ! -z "$PASS" ]] ; then
			if [[ "$PASS" =~ "$PASSWORD_ENTER" ]]; then
				stored_connection "$SSID"
			else
				connect "$SSID" $PASS
			fi
		else
			stored_connection "$SSID"
		fi
	fi
}
function ssid_hidden() {
	LINES=0
	PROMPT="Enter_SSID"
	WIDTH=35
	SSID=$(	rofi -dmenu -location "$LOCATION" -yoffset "$Y_AXIS" -xoffset "$X_AXIS" \
	-theme "$RASI_DIR" -theme-str '
	window{width: '"$(($WIDTH/2))"'em;}
	listview{lines: '"$LINES"';}
	textbox-prompt-colon{str:"'$PROMPT':";}
	')
	echo $SSID
	if [[ ! -z $SSID ]]; then
		LINES=1
		WIDTH=$(echo "$PASSWORD_ENTER" | awk '{print length($0); }')
		((WIDTH+=$WIDTH_FIX_MAIN))
		PROMPT="Enter_Password"
		PASS=$(echo "$PASSWORD_ENTER" | \
		rofi -dmenu -location "$LOCATION" -yoffset "$Y_AXIS" -xoffset "$X_AXIS" \
		-password -theme "$RASI_DIR" -theme-str '
		window{width: '"$(($WIDTH/2))"'em;}
		listview{lines: '"$LINES"';}
		textbox-prompt-colon{str:"'$PROMPT':";}
		')
		if [[ ! -z "$PASS" ]] ; then
			if [[ "$PASS" =~ "$PASSWORD_ENTER" ]]; then
				check_wifi_connected
				notification "5" "normal" "Wi-Fi" "Connecting to $SSID"
				if [ $(nmcli dev wifi con "$SSID" ifname ${WIRELESS_INTERFACES[WLAN_INT]} hidden yes| grep -c "successfully activated" ) = "1" ]; then
					notification "5" "normal" "Connection_Established" "You're now connected to Wi-Fi network '$SSID' "
				else
					notification "5" "normal" "Connection_Error" "Connection can not be established"
				fi
			else
				check_wifi_connected
				notification "5" "normal" "Wi-Fi" "Connecting to $SSID"
				if [ $(nmcli dev wifi con "$SSID" password "$PASS" ifname ${WIRELESS_INTERFACES[WLAN_INT]}| grep -c "successfully activated" ) == "1" ]; then
					notification "5" "normal" "Connection_Established" "You're now connected to Wi-Fi network '$SSID' "
				else
					notification "5" "normal" "Connection_Error" "Connection can not be established"
				fi
			fi
		else
			check_wifi_connected
			notification "5" "normal" "Wi-Fi" "Connecting to $1"
			if [ $(nmcli dev wifi con "'$SSID'" ifname ${WIRELESS_INTERFACES[WLAN_INT]} hidden yes| grep -c "successfully activated" ) = "1" ]; then
				notification "5" "normal" "Connection_Established" "You're now connected to Wi-Fi network '$1' "
			else
				notification "5" "normal" "Connection_Error" "Connection can not be established"
			fi
		fi
	fi
}
function status() {
	LINES=2
	WIDTH=0
	for i in "${!WIRELESS_INTERFACES[@]}"
	do
		WIFI_CON_STATE=$(nmcli device status | grep "^${WIRELESS_INTERFACES[i]}." |  awk '{print $3}')
		if [[ "$WIFI_CON_STATE" == "connected" ]]; then
			WLAN_STATUS=(${WLAN_STATUS[@]}"${WIRELESS_INTERFACES_PRODUCT[$i]}[${WIRELESS_INTERFACES[$i]}]:\n\t$(nmcli -t -f GENERAL.CONNECTION dev show ${WIRELESS_INTERFACES[$i]} | awk -F '[:]' '{print $2}') ~ $(nmcli -t -f IP4.ADDRESS dev show ${WIRELESS_INTERFACES[$i]} | awk -F '[:/]' '{print $2}')\n")
		else
			WLAN_STATUS=(${WLAN_STATUS[@]}"${WIRELESS_INTERFACES_PRODUCT[$i]}[${WIRELESS_INTERFACES[$i]}]:\n\t${WIFI_CON_STATE^}\n")
		fi
		((LINES+=2))
	done
	WIDTH_TEMP=$(echo -e $WLAN_STATUS | tail -n 2 | head -n 1 | awk '{print length($0); }' )
	if [[ $WIDTH_TEMP -gt $WIDTH ]];then
		WIDTH=$WIDTH_TEMP
	fi
	WIRE_CON_STATE=$(nmcli device status | grep "ethernet"  |  awk '{print $3}')
	if [[ "$WIRE_CON_STATE" == "connected" ]]; then
		ETH_STATUS="$(nmcli device | awk '$2=="ethernet" {print $1}'):\n\t"$(nmcli -t -f GENERAL.CONNECTION dev show "$(nmcli device | awk '$2=="ethernet" {print $1}')" | cut -d":" -f2)" ~ "$(nmcli -t -f IP4.ADDRESS dev show "$(nmcli device | awk '$2=="ethernet" {print $1}')" | awk -F '[:/]' '{print $2}')
	else
		ETH_STATUS="$(nmcli device | awk '$2=="ethernet" {print $1}'):\n\t"$(nmcli -t -f GENERAL.CONNECTION dev show "$(nmcli device | awk '$2=="ethernet" {print $1}')" | cut -d":" -f2)"${WIRE_CON_STATE^}"
	fi
	WIDTH_TEMP=$(echo -e $ETH_STATUS | tail -n 1 | awk '{print length($0); }' )
	if [[ $WIDTH_TEMP -gt $WIDTH ]];then
		WIDTH=$WIDTH_TEMP
	fi
	((WIDTH+=WIDTH_FIX_STATUS))
	if [[ $WIDTH -le 20 ]];then
		WIDTH=35
	fi
	echo -e "$ETH_STATUS\n${WLAN_STATUS[@]}"| \
	rofi -dmenu -location "$LOCATION" -yoffset "$Y_AXIS" -xoffset "$X_AXIS" \
	-theme "$RASI_DIR" -theme-str '
		window{width: '"$(($WIDTH/2))"'em;
		children: [listview];}
		listview{lines: '"$LINES"';}
	'
}
function share_pass() {
	LINES=$(nmcli dev wifi show-password | grep -e SSID:  -e Password: | wc -l)
	((LINES+=1))
	WIDTH=35
	SELECTION=$(echo -e "$(nmcli dev wifi show-password | grep -e SSID:  -e Password:)\n~QrCode" | \
	rofi -dmenu -location "$LOCATION" -yoffset "$Y_AXIS" -xoffset "$X_AXIS" \
	-a "2" -theme "$RASI_DIR" -theme-str '
		window{width: '"$(($WIDTH/2))"'em;
		children: [listview];}
		listview{lines: '"$LINES"';}
	')
	selection_action
}
function gen_qrcode() {
	qrencode -t png -o /tmp/wifi_qr.png -s 10 -m 2 "WIFI:S:""$( nmcli dev wifi show-password | grep -oP '(?<=SSID: ).*' | head -1)"";T:""$(nmcli dev wifi show-password | grep -oP '(?<=Security: ).*' | head -1)"";P:""$(nmcli dev wifi show-password | grep -oP '(?<=Password: ).*' | head -1)"";;"
	rofi -dmenu -location "$QRCODE_LOCATION" -yoffset "$Y_AXIS" -xoffset "$X_AXIS" \
	-theme-str '* {
	background-color: transparent;
	text-color:       transparent;
	}
  	window {
		border-radius: 6px;
		padding: 1em;
		background-color: transparent;
		background-image: url("'$QRCODE_DIR'wifi_qr.png",width);
		width: 20em;
	}
	textbox-prompt-colon {
		expand: false;
		margin: 0;
		str:"";
	}
  	listview {
		lines: 15;
	}
  	entry {
		enabled: false;
	}'
}
function manual_hidden() {
	LINES=2
	WIDTH=35
	SELECTION=$(echo -e "~Manual\n~Hidden" |\
	rofi -dmenu -location "$LOCATION" -yoffset "$Y_AXIS" -xoffset "$X_AXIS" \
	-theme "$RASI_DIR" -theme-str '
	window{width: '"$(($WIDTH/2))"'em;
	children: [listview];}
	listview{lines: '"$LINES"';}
	')
	selection_action
	echo $SELECTION
}
function selection_action () {
	case "$SELECTION" in
		"~Disconnect")
			disconnect "5" "normal" "Connection_Terminated"
			;;
		"~Scan")
			scan
			;;
		"~Status")
			status
			;;
		"~Share Wifi Password")
			share_pass
			;;
		"~Manual/Hidden")
			manual_hidden
				;;
		"~Manual")
			ssid_manual
				;;
		"~Hidden")
			ssid_hidden
				;;
		"~Wi-Fi On")
			change_wifi_state "5" "normal" "Wi-Fi" "Enabling Wi-Fi connection" "on"
			;;
		"~Wi-Fi Off")
			change_wifi_state "5" "normal" "Wi-Fi" "Disabling Wi-Fi connection" "off"
			;;
		"~Eth Off")
			change_wire_state "5" "normal" "Ethernet" "Disabling Wired connection" "down"
			;;
		"~Eth On")
			change_wire_state "5" "normal" "Ethernet" "Enabling Wired connection" "up"
			;;
		"   ***Wi-Fi Disabled***   ")
			;;
		" ***Wired Unavailable***")
			;;
		"~Change Wifi Interface")
			change_wireless_interface
			;;
		"~Restart Network")
			net_restart "5" "normal" "Network" "Restarting Network"
			;;
		"~QrCode")
			gen_qrcode
			;;
		*)
			LINES=1
			WIDTH=$(echo "$PASSWORD_ENTER" | awk '{print length($0); }')
			((WIDTH+=$WIDTH_FIX_MAIN))
			PROMPT="Enter_Password"
			if [[ ! -z "$SELECTION" ]] && [[ "$WIFI_LIST" =~ .*"$SELECTION".*  ]]; then
				if [ "$SSID_SELECTION" = "*" ]; then
					SSID_SELECTION=$(echo "$SELECTION" | sed  "s/\s\{2,\}/\|/g "| awk -F "|" '{print $3}')
				fi
				if [[ "$ACTIVE_SSID" == "$SSID_SELECTION" ]]; then
					nmcli con up "$SSID_SELECTION" ifname ${WIRELESS_INTERFACES[WLAN_INT]}
				else
					if [[ "$SELECTION" =~ "WPA2" ]] || [[ "$SELECTION" =~ "WEP" ]]; then
						PASS=$(echo "$PASSWORD_ENTER" | \
						rofi -dmenu -location "$LOCATION" -yoffset "$Y_AXIS" -xoffset "$X_AXIS" \
						-password -theme "$RASI_DIR" -theme-str '
						window{width: '"$(($WIDTH/2))"'em;}
						listview{lines: '"$LINES"';}
						textbox-prompt-colon{str:"'$PROMPT':";}
						')
					fi
					if [[ ! -z "$PASS" ]] ; then
						if [[ "$PASS" =~ "$PASSWORD_ENTER" ]]; then
							stored_connection "$SSID_SELECTION"
						else
							connect "$SSID_SELECTION" $PASS
						fi
					else
						stored_connection "$SSID_SELECTION"
					fi
				fi
			fi
		;;
	esac
}
function main() {
	initialization
	rofi_menu
}
main
