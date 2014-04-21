
# The advanced example from readme 

myprogram()
{
	seq 1 100 | while read line; do echo "ERROR $line"; sleep 0.2; done
}

tailclient -w -f logfile.sock | grep ERROR > errorfile.txt &
{	sleep 10
	echo "[Works for more than a minute at $(date)]"
	tailclient -n 100 logfile.sock
	while [ -S logfile.sock ]; do
		sleep 1
		echo -e "...\n...\n...\n[Last lines at $(date)]"
		tailclient -n 10 logfile.sock
	done
} > longrun_debug.txt & WAITPID=$!
myprogram | tailserver -w logfile.sock | lzop > logfile.lzo
kill $WAITPID

sleep 1
rm errorfile.txt
rm longrun_debug.txt
rm logfile.lzo
