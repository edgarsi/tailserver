tailserver
==========

"tee"-like app for serving "tail -f" on demand.

## Why
* **To `tail -f` an output which is being compressed**.
* or in any other case where writing raw output to file is undesired. *(Let me know if there are any)*

Compressing can save a lot of IO waste. `tailserver` allows to compress and avoid losing the ability to `tail -f`. There are alternatives, see below.

### Description (WARNING: pre-alpha...)
The utility reads standard input and immediately outputs it to standard output. 
Additionaly, it creates a unix domain socket server for output broadcasting. 
The last N lines will always be output to new clients, allowing for `tail -n <lines> -f` as long as you don't exceed N.
See MANPAGE.

### Example
Instead or running 
`myprogram | gzip > logfile.gz` 
you can run 
`myprogram | tailserver -s logfile.sock | gzip > logfile.gz`

Tailing:
`socat -u UNIX-CONNECT:logfile.sock - | tail -n 100 -f`
*TODO: I should deploy a `tailclient <file>` script as a cleaner rename for "socat -u UNIX-CONNECT:<file> -"*
*TODO: It could also handle the situations where to file does not exist - choice of wait or bail.*

### Advanced example 
This example combines all of these:
* Compress using LZO compression (because it's my favourite)
* Grep for "ERROR", store into another file
* Get the last 100 lines if works for more than an hour, and 10 lines each 10 minutes from then on 

`
tailclient -w logfile.sock | grep ERROR > errorfile.txt &
{	sleep 3600
	echo "[Works for more than an hour at $(date)]"
	tailclient logfile.sock | tail -n 100
	while [ 1 ]; do
		sleep 600
		echo -e "...\n...\n...\n[Last lines at $(date)]"
		tailclient logfile.sock | tail -n 10
	done
} > longrun_debug.txt & WAITPID=$!
myprogram | tailserver -w logfile.sock | lzop > logfile.lzo
kill $WAITPID
`
*TODO: Test this!*

Look at how pretty it is!

### Install
Dependencies: cmake, libev
Dependencies: tailclient uses `socat`. *TODO: Make a version which doesn't use socat.*
git clone git://github.com/edgarsi/tailserver.git
cd tailserver
sh ./configure
make
sudo make install

### Alternatives to tailserver
* *If you only need `tail -f`, not the `-n`*, 
	you can use [ftee](http://stackoverflow.com/questions/7360473/linux-non-blocking-fifo-on-demand-logging). Only works for one reader but it's rare to need more.
* *If you only need `tail -f`, not the `-n` and for some weird reason you may need (unpredictably) many readers*, 
	there's a kernel module for a block device which does this... its name forgotten.
* *If you need to start `tail -f` on a program which is already running*, you can use [capture_output.sh](capture_output.sh) which isn't perfect but fair enough
	for the simplest uses. 
* *If you always follow your output immediately with `tail -f`*, you can create a named pipe and `tee` to that, read from that.
* *If you need to tail large amount of lines*, 
	you will have to read the compressed file knowing that some of the last lines are probably not flushed to the output. 
	The only compressor I know which flushes regulary is the `xz-utils` compressor (--flush-timeout) and that will cost you compression ratio. 
	I don't know of any compressors which flush on request (such as unix signal). None will give you real-time `tail -f` either. 
	Anyway, I insist that merging the output of the compressed file with `tailserver` results is the easiest way, and most elegant.
	Try [tail_using_logfile.sh](tail_using_logfile.sh) for reference.




