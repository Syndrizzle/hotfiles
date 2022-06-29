
#First host a vercel instance of: https://github.com/raitonoberu/lyricsapi
#The above program will run the backened stuff, I just made a wrapper
from urllib.parse import quote
from datetime import datetime
from os import getlogin, path, remove
import subprocess
import requests
import sys
import pytz

#add the suffix /api/lyrics? the end of your vercel app url
api = "https://api.textyl.co/api/lyrics?"

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
    secs = int(line['seconds'])
    words = line['lyrics']
    time = datetime.fromtimestamp(secs, pytz.timezone("UTC"))
    lrc += f"[{time.strftime('%M:%S')}] {words}\n"

lrc = lrc[:-1]
file.write(lrc)
file.close()
print(lrc)
