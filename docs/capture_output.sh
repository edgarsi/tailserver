#!/bin/bash

pid=${1?$'\n\t'"Usage: $0 <pid>"$'\n\t'"Captures the output of any running program."}

strace -s1000000000 -e trace=write -q -p $pid 2>&1 | \
	grep --line-buffered "^write([12]," | \
	sed -u 's/^write([12], "\(.*\)", [0-9]*)\s*= [0-9]*$/\1/g' | \
	sed -u 's/%/%%/g' | \
	while IFS= read -r line; do
		printf "$line"
	done
