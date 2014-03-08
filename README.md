tailserver
==========

"tee"-like app for serving "tail -f" on demand.

## Why
* To `tail -f` a an output which is being compressed.
* or in any other case where writing raw output to file is undesired. *(Let me know if there are any)*

Compressing can save a lot of IO waste. `Tailserver` allows to compress and avoid losing the ability to `tail -f`. There are alternatives, see below.

### Description
This utility reads standard input and immediately outputs it to standard output. 
Additionaly, it creates a TCP or Unix domain socket server for output broadcasting. 
The last N lines will always be output to new clients, allowing for `tail -n <lines> -f` as long as you don't exceed N.
See MANPAGE.

### Example using sockets
Instead or running 
`myprogram | gzip > logfile.gz` 
you can run 
`myprogram | tailserver -s logfile.sock | gzip > logfile.gz`

Tailing:
`socat -u UNIX-CONNECT:logfile.sock - | tail -n 100 -f`

### Example using TCP *(if I do implement this useless feature)*
Instead or running 
`myprogram | gzip > logfile.gz` 
you can run 
`myprogram | tailserver -p 1234 | gzip > logfile.gz`

Tailing:
`connect localhost 1234 | tail -n 100 -f`

### Alternatives to tailserver
* *If you only need `tail -f`, not the `-n`*, 
	you can use [ftee](http://stackoverflow.com/questions/7360473/linux-non-blocking-fifo-on-demand-logging). Only works for one reader but it's rare to need more.
* *If you only need `tail -f`, not the `-n` and for some weird reason you may need (unpredictably) many readers*, 
	there's a kernel module for a block device which does this... its name forgotten.
* *If you need to start `tail -f` on a program which is already running*, you can use [capture_output.sh](capture_output.sh) which isn't perfect but fair enough. 
* *If you always follow your output immediately with `tail -f`*, 
	you can create a named pipe and `tee` to that, read from that.
* *If you need to tail large amount of lines*, 
	you will have to read the compressed file knowing that some of the last lines are probably not flushed to the output. 
	The only compressor I know which flushes regulary is the `xz-utils` compressor (--flush-timeout) and that will cost you compression ratio. 
	I don't know of any compressors which flush on request (such as unix signal). None will give you real-time `tail -f` either. 
	Anyway, I insist that merging the output of the compressed file with `tailserver` results is the easiest way, and most elegant.




