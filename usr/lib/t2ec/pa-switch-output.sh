# Credits go to mreithub at https://unix.stackexchange.com/a/67398 (and comments below!)
#!/usr/bin/env bash

newSink="$1"

pactl list short sink-inputs|while read stream; do
    streamId=$(echo $stream|cut '-d ' -f1)
    pacmd set-default-sink "$newSink"
    pactl move-sink-input "$streamId" "$newSink"
done
