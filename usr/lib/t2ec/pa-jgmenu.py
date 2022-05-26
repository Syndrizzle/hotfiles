#!/usr/bin/python
# _*_ coding: utf-8 _*_

"""
Author: Piotr Miller
e-mail: nwg.piotr@gmail.com
Website: http://nwg.pl
Project: https://github.com/nwg-piotr/t2ec
License: GPL3

Credits: mreithub at https://unix.stackexchange.com/a/67398 (and comments below!)

Dependencies: 'jgmenu' 'pulseaudio' 'pulseaudio-alsa'

This script creates jgmenu to switch audio output with the pa-switch-output.sh helper script
"""

import os
import subprocess


def main():
    # get sink numbers
    sinks = subprocess.check_output("pactl list short sinks", shell=True).decode("utf-8").splitlines()

    # get output descriptions
    names = subprocess.check_output("pacmd list-sinks | grep device.description | awk -F '=' '{print $2}'",
                                    shell=True).decode("utf-8").splitlines()
    outputs = []

    # Tuples ("sink_number", "output_description")
    for output in range(len(sinks)):
        output = sinks[output].split()[0], names[output].strip()[1:-1]
        outputs.append(output)

    # Create jgmenu script
    jgmenu = []
    jgmenu.append("#!/bin/sh\n")
    jgmenu.append("config_file=$(mktemp)")
    jgmenu.append("menu_file=$(mktemp)")
    jgmenu.append("trap \"rm -f ${config_file} ${menu_file}\" EXIT\n")
    jgmenu.append("cat << 'EOF' >${config_file}")
    jgmenu.append("stay_alive           = 0")
    jgmenu.append("tint2_look           = 1")
    jgmenu.append("menu_width           = 40")
    jgmenu.append("menu_border          = 1")
    jgmenu.append("item_height          = 20")
    jgmenu.append("font                 = Sans 10")
    jgmenu.append("icon_size            = 0")
    jgmenu.append("color_norm_fg        = #eeeeee 100")
    jgmenu.append("color_sel_fg         = #eeeeee 100")
    jgmenu.append("EOF\n")
    jgmenu.append("cat <<'EOF' >${menu_file}")

    # Use previously saved tuples (sink_number, output_description) to create jgmenu entries
    for output in outputs:
        jgmenu.append(output[1] + ", " + "/usr/lib/t2ec/pa-switch-output.sh " + output[0])

    jgmenu.append("EOF\n")
    jgmenu.append("jgmenu --config-file=${config_file} --csv-file=${menu_file}")

    # Exit if jgmenu not installed
    try:
        subprocess.check_output("which jgmenu", shell=True)
    except subprocess.CalledProcessError:
        os.system("notify-send 'Install jgmenu package, run jgmenu init'")
        exit(0)

    # Save jgmenu script
    t2ec_dir = os.getenv("HOME") + "/.t2ecol"
    if not os.path.isdir(t2ec_dir):
        os.makedirs(t2ec_dir)

    with open(t2ec_dir + '/pulseaudio-menu.sh', 'w') as f:
        for item in jgmenu:
            f.write("%s\n" % item)

    # Make executable
    os.system("chmod u+x " + t2ec_dir + "/pulseaudio-menu.sh")

    # Execute jgmenu script
    os.system(t2ec_dir + "/pulseaudio-menu.sh &")


if __name__ == "__main__":
    main()
