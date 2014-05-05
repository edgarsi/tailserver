
socket=${1?$'\n\t'"Usage: $0 <socket_file_name>"$'\n\t'"Launches tailserver and feeds input to it."}
seq 1 1000000000 | tailserver $socket $2 $3 $4 $5 $6 $7 $8 $9
