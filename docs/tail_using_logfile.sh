usage=$'\n\t'"Usage: $0 [<-c|-n> K] <tailserver_cat_pid> <socket_file> <compressed_file> <decompressor>"
usage="$usage"$'\n\t'"First of all, assumes you've launched your program like this: "
usage="$usage"$'\n\t\t'"program | cat -n | tailserver <socket_file> | <compressor> > <compressed_file>"
usage="$usage"$'\n\t'"Yes, this means you will have line numbers in your log files. If you didn't, how would you merge?"
usage="$usage"$'\n\t'"Output the last lines or bytes by decompressing <compressed_file>. "
usage="$usage""Add the lines from tailserver (which are not in the compressed file yet). Continue following tail."
usage="$usage"$'\n\t\n\t'"Specific example:"
usage="$usage"$'\n\t\t'"{ seq 1 1000000; seq 1000001 1e99 | while read line; do echo \"\$line\"; sleep 0.1; done; } "
usage="$usage"" | sed -u 's/^/My interesting line no./' | cat -n | tailserver -n 10000 test.sock | gzip > test.gz"
usage="$usage"$'\n\t\t'"PID=<pid of that "cat" of the previous line>"
usage="$usage"$'\n\t\t'"echo \"Outputting the first 10 and last 10 lines of a 19999 line span\""
usage="$usage"$'\n\t\t'"$0 -n 20000 \$PID test.sock test.gz zcat | sed -n '1,11p;19991,\$p'"
usage="$usage"$'\n\t\n\t'"Note that THIS IS A TOTAL HACK for example purposes only."

tail_args=""
if [ "$1" == "-c" -o "$1" == "-n" ]; then
	tail_args="$1 $2"; shift; shift;
fi
tailserver_cat_pid=${1?"$usage"}
socket_file=${2?"$usage"}
logfile=${3?"$usage"}
decompressor=${4?"$usage"}


# Stop for sync
kill -STOP $tailserver_cat_pid

# Get as much as possible from buffer
tmpfile=tail_using_logfile.tmp
tailclient $socket_file > $tmpfile

# Merge with file contents
# NOTE: One line less is output to get rid of the potentially truncated last line.
echo "zcat $logfile | tail $tail_args | head -n -1 | sort -nm - $tmpfile | uniq | tail $tail_args"
$decompressor $logfile | tail $tail_args | head -n -1 | sort -nm - $tmpfile | uniq | tail $tail_args

# Keep following
{ sleep 1; kill -CONT $tailserver_cat_pid; rm $tmpfile; } &
tailclient -n 0 -f $socket_file

