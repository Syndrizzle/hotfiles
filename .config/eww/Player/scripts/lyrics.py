#!/usr/bin/env python

from urllib.parse import quote
from datetime import datetime
from os import getlogin, path, remove
import subprocess
import requests
import sys
import pytz

api = "https://lyricsapi-seven.vercel.app/api/lyrics?"

# change source to %any ig
source = "spotify"
title = str(subprocess.run(["playerctl", "metadata", "title", "-p", source], capture_output=True).stdout)[2:-3]
artist = str(subprocess.run(["playerctl", "metadata", "artist", "-p", source], capture_output=True).stdout)[2:-3]
lrc_path = f"/home/{getlogin()}/.lyrics/{artist} - {title}.lrc"

if path.exists(lrc_path):
    exit()

file = open(lrc_path, "w")


resp = requests.get(f"{api}q={quote(title + ' ' + artist)}").json()
if not resp:
    print("Not found.")
    file.close()
    remove(lrc_path)
    exit()

lrc = f"[ti:{title}]\n[ar:{artist}]\n"
for line in resp:
    milli = int(line['time'])
    words = line['words']
    time = datetime.fromtimestamp(milli/1000.0)

    lrc += f"[{time.strftime('%M:%S:%f')[:8]}] {words}\n"






lrc = lrc[:-1]
file.write(lrc)
file.close()
