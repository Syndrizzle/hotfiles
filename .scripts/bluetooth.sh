#!/bin/bash

# not connected
if [ $(bluetoothctl show | grep "Powered: yes" | wc -c) -eq 0 ]
then
#     echo $HOME/.local/icons/bluetooth-off.svg
      :
else
 # connected, but no device
 if [ $(echo info | bluetoothctl | grep 'Device' | wc -c) -eq 0 ]
 then
#     echo $HOME/.local/icons/bluetooth-on.svg
      :
 else
   # get device alias
   DEVICE=`echo info | bluetoothctl | grep 'Alias:' | awk -F:  '{ print $2 }'`
   BATTERY=`upower -i /org/freedesktop/UPower/devices/headset_dev_33_33_55_33_90_D0 | grep percentage | cut -b 26-28`
   if [[ -z "${DEVICE// }" ]];
   then
     echo $HOME/.local/icons/bluetooth-connected.svg
     echo "Connected"
   else
     echo $HOME/.local/icons/bluetooth-connected.svg
     echo "$DEVICE ($BATTERY)"
   fi
 fi
fi
