#!/usr/bin/env bash
amixer sget Capture | grep 'Left:' | awk -F'[][]' '{ print $2 }' | tr -d '%'