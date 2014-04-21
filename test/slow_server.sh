
socket=${1?$'\n\t'"Usage: $0 <socket_file_name>"$'\n\t'"Launches tailserver and slowly feeds input to it."}
seq 1 1000000000 | while read line; do sleep 0.1; echo $line; done | tailserver $socket
