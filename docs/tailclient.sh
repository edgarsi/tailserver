#!/bin/bash

if [ "$1" == "" -o "$1" == "-h" -o "$1" == "--help" ]; then
	echo "NOTE: This script is deprecated in favour of 'tailclient' executable."
	echo "Usage: tailcient.sh [-w] [<-n|-c> K] [-f] <SOCKET_FILE>"
	echo -e "\t-w\tIf SOCKET_FILE does not exist, wait until it appears"
	echo -e "\tThe other arguements act as they do for 'tail'."
	echo -e "\tNote that this script requires the fixed order of arguments."
	exit
fi

wait=0
if [ "$1" == "-w" ]; then
	wait=1
	shift
fi

tail_arguments=""
if [ "$1" == "-n" -o "$1" == "-c" ]; then
	tail_arguments="$1 $2"
	shift
	shift
fi

follow=0
if [ "$1" == "-f" ]; then
	follow=1
	shift
fi

file=$1

if [ $wait -eq 1 ]; then
	while [ ! -e "$file" ]; do
		sleep 0.1
	done
fi


{
	if [ $follow -eq 1 ]; then
		socat -u UNIX-CONNECT:"$file" -
	else
		echo | socat UNIX-CONNECT:"$file" -
	fi
} | {
	read tail_size
	if [ "$tail_size" == "" ]; then
		echo "Could not make a connection." 1>&2
		exit
	fi
	head -c $tail_size | tail $tail_arguments
	if [ $follow -eq 1 ]; then
		cat
	fi
}

