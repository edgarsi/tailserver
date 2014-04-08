/* 
   Copyright (C) 1985-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 2+ of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* 
   Some code is borrowed from GNU coreutils tee.c and tail.c
   (I used the GLP3+ code but I've checked it hasn't changed significantly since the GPL2+ times)
   tee.c: Mike Parker, Richard M. Stallman, and David MacKenzie
   tail.c: Paul Rubin, David MacKenzie, Ian Lance Taylor, Giuseppe Scrivano
   Socket handling from mysqld.cc (GPL2=)
   Note that since MySQL does not include the "or any later version" clause,
   therefore the acting license of tailserver is forced to be GPL2 by the magic of law.
   
*/

#include "config.h"

#include "pipes.h"

#include "buffer.h"
#include "sockets.h"
#include "attribute.h"
#include "xfreopen.h"
#include "fadvise.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <error.h>
#include <stdlib.h>

#include "fcntl_wrap.h"

/* Eludes me why everyone (see mysql, postgre, ngircd, redis) tries to implement their own socket event handling */
#include <ev.h>

#define debug(...)
/*#define debug(x) fputs(x, stderr)*/


static ev_io stdin_watcher;


static void stdout_write_blocking (char* buf, ssize_t size)
{
    ssize_t n;

    do {
        n = write(STDOUT_FILENO, buf, size);
    } while (n < 0 && errno == EINTR); /*TODO: Where's my EAGAIN? */

    if (n < size) {
        perror(_("Writing to stdout encountered an error"));
    }
}

static void stdin_cb (EV_P_ ev_io* w ATTRIBUTE_UNUSED, int revents ATTRIBUTE_UNUSED)
{
    debug("stdin ready (probably)\n");

    /* O_NONBLOCK risks bugs and data races because all users of stdin get the effects, including writers. */
    /* fnctl and ioctl both modify the same (struct file*)->f_flags [linux/fs.h], therefore both unusable. */
    /* UPDATE: "struct file" is created each open, so freopen should solve our problems. If not, try aio_read() instead */

    /* "select, read" method has to read char by char (slow) */
    /* Is there data on stdin? */
    /*struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    if ( ! FD_ISSET(STDIN_FILENO, &fds)){
        debug("not ready\n");
        return;
    }
    debug("stdin ready for real\n");*/

    bool error = false;
    char* new_data_start = buffer_end();
    ssize_t res = read(STDIN_FILENO, new_data_start, buffer_available_for_append());
    if (res > 0) {

        debug("stdin data read!\n");
        buffer_set_appended(res);

        /* Write immediately */
        stdout_write_blocking(new_data_start, res);
        sockets_write_blocking(new_data_start, res);

    } else
    if (res == 0) {
        /* EOF */
        ev_break(EV_A_ EVBREAK_ALL);
    } else
    if (res < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
        perror(_("STDIN error during read"));
        ev_break(EV_A_ EVBREAK_ALL);
    }
}

void pipes_init ()
{
    /*if (O_BINARY && !isatty(STDIN_FILENO)) {  <- tail.c does not reopen tty but I can't figure out why... */
    xfreopen(NULL, "rb", stdin);
#ifdef HAVE_FCNTL_NONBLOCK
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
#endif
    fadvise(stdin, FADVISE_SEQUENTIAL);

    setvbuf(stdout, NULL, _IONBF, 0);

    /* Listen to STDIN */
    ev_io_init(&stdin_watcher, stdin_cb, STDIN_FILENO, EV_READ);
    ev_io_start(EV_DEFAULT_UC_ &stdin_watcher);
}

void pipes_final ()
{
    if (close(STDIN_FILENO) != 0) {
        error(EXIT_FAILURE, errno, _("standard input"));
    }
}

