#!/bin/sh
for file in /sys/devices/virtual/net/*/bridge/multicast_snooping
do
	if test -f $file 
		then 
		echo $1 > $file 
	fi
done