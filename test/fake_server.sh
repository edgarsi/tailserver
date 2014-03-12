
socket=${1?$'\n\t'"Usage: $0 <socket_file_name>"$'\n\t'"Creates a socket server which sends test lines to clients. Multiple clients can compete for lines."}
seq 1 1000000 | while read line; do sleep 0.1; echo $line; done | socat -d -d UNIX-LISTEN:test.sock,fork,unlink-early -
