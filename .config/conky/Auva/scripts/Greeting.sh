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
# 00  Midnight
# 01  Morning
# 02  Morning
# 03  Morning
# 04  Morning
# 05  Morning
# 06  Morning
# 07  Morning
# 08  Morning
# 09  Morning
# 10  Morning
# 11  Noon
# 12  Noon
# 13  Afternoon
# 14  Afternoon
# 15  Afternoon
# 16  Afternoon
# 17  Afternoon
# 18  Evening
# 19  Evening
# 20  Evening
# 21  Evening
# 22  Evening
# 23  Evening
