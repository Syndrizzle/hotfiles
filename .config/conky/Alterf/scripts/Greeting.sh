#!/bin/bash
# ----------------------------------------------------------------------------------
# name    : Sapaan bahasa inggris
# version : 0.1
#
# TENTANG
# ----------------------------------------------------------------------------------
# Menampilkan Sapaan dalam bahasa inggris
# 
#

Greeting=$(date +%H)
cat $0 | grep $Greeting | sed 's/# '$Greeting' //'
# 

# terinspirasi dari imsakiyah.sh
# 
# --------------------------------------------------------------------------------
# 00 Good Midnight
# 01 Good Morning
# 02 Good Morning
# 03 Good Morning
# 04 Good Morning
# 05 Good Morning
# 06 Good Morning
# 07 Good Morning
# 08 Good Morning
# 09 Good Morning
# 10 Good Morning
# 11 Good Day
# 12 Good Day
# 13 Good Afternoon
# 14 Good Afternoon
# 15 Good Afternoon
# 16 Good Afternoon
# 17 Good Afternoon
# 18 Good Evening
# 19 Good Evening
# 20 Good Evening
# 21 Good Evening
# 22 Good Evening
# 23 Good Evening
