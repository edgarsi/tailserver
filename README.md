tailserver
==========

Achieve `tee <file>` `tail <file>` without the overhead of writing to `<file>`.

## Why
* **To `tail` an output which is being compressed**.  
Compressing can save a lot of IO waste. `tailserver` allows to compress and not lose the ability to `tail -f -n K`.
There are both nice and weird alternatives, see below.

Generally: If you have large amounts of data going through the pipe, and you may want to do 
`tail -f --lines=K` or `tail -f --bytes=K` on that pipe, use `tailserver`.

### Description
The `tailserver` command reads stdin and copies it to stdout.  
Additionaly, it serves the output to a UNIX domain socket. Clients (`tailclient`) receive the last lines
and lines which arrive since, if they want to.

### Example
Instead or running  
`myprogram | gzip > logfile.gz`  
you can run  
<pre style="display: inline">myprogram | <b>tailserver logfile.sock</b> | gzip > logfile.gz</pre>

Reading the tail:  
`tailclient -n 100 -f logfile.sock`  
`tailclient -n 5 logfile.sock`  
`...`  *TODO: When tailclient is not socat dependent, move arguments of the examples to the end.*

### Install
Dependencies: cmake, libev  
Dependencies: tailclient uses `socat`. *TODO: Make a version which doesn't use socat.* 
```
git clone git://github.com/edgarsi/tailserver.git
cd tailserver
sh configure
make
sudo make install
```

### Advanced example 
This example combines all of these:
* Compress using LZO compression (because it's my favourite)
* Grep for "ERROR", store into another file
* Get the last 100 lines if works for more than an hour, and 10 lines each 10 minutes from then on 

```
tailclient -w -f logfile.sock | grep ERROR > errorfile.txt &
{	sleep 3600
	echo "[Works for more than an hour at $(date)]"
	tailclient -n 100 logfile.sock
	while [ 1 ]; do
		sleep 600
		echo -e "...\n...\n...\n[Last lines at $(date)]"
		tailclient -n 10 logfile.sock
	done
} > longrun_debug.txt & WAITPID=$!
myprogram | tailserver -w logfile.sock | lzop > logfile.lzo 
kill $WAITPID
```
Look at how pretty it is!

### Alternatives to tailserver
* *If you only need `tail -f -n K` where K is small (less than a few screens)*,  
	you can use `screen` for your process.
	There are additional benefits for screen, so do consider this.

* *If you only need `tail -f`, without the need for last lines*,  
	you can use [ftee](http://stackoverflow.com/questions/7360473/linux-non-blocking-fifo-on-demand-logging). 
	Only works for one reader but it's rare to need more.
* *If you only need `tail -f`, without the need for last lines, and for some reason you may need
	(unpredictably)  many readers*,  
	there's a kernel module for a block device which does this... its name forgotten.

* *If you need to start `tail -f` on a program which is already running*,  
	you can use [capture_output.sh](docs/capture_output.sh) which isn't perfect but fair enough for the simplest uses.
	There are slightly riskier but less costly alternatives, such as redirecting the stdout using gdb - google knows of them.

* *If you always follow your output _immediately_ with `tail -f`*,  
	you can just create a named pipe and `tee` to that, read from that.

* *If you need to `tail -n` large amount of lines too costly to keep in memory*,  
	you will have to read the compressed file knowing that some of the last lines are probably not flushed to the output. 
	The only compressor I know which flushes regulary is the `xz-utils` compressor (--flush-timeout) and that will cost you compression ratio. 
	I don't know of any compressors which flush on request (such as unix signal). None will give you real-time `tail -f` either. 
	Anyway, I insist that merging the output of the compressed file with `tailserver` results is the easiest way, and most elegant.
	Try [tail_using_logfile.sh](docs/tail_using_logfile.sh) for reference.




