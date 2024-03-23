#!/bin/bash

start_port=6000

# Creating 12 pair of MTP sockets in the following form
# <port_no> <----communicates with----> <port_no>
# 6000 <--> 6001
# 6002 <--> 6003
# 6004 <--> 6005
# 6006 <--> 6007
# 6008 <--> 6009
# 6010 <--> 6011
# 6012 <--> 6013
# 6014 <--> 6015
# 6016 <--> 6017
# 6018 <--> 6019
# 6020 <--> 6021
# 6022 <--> 6023

for ((i=0; i<2; i++))
do
    myport=$((start_port + i + i))
    dest_port=$((myport + 1))
    ./tempuser2 "$dest_port" "$myport" &
    ./tempuser1 "$myport" "$dest_port" &
    # echo "##########"
done