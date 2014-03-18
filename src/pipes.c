/* 
   Copyright (C) 1985-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* 
   Some code is borrowed from GNU coreutils tee.c and tail.c
   (I used the GLP3+ code but I'm pretty sure it hasn't changed since the GPL2 times)
   tee.c: Mike Parker, Richard M. Stallman, and David MacKenzie
   tail.c: Paul Rubin, David MacKenzie, Ian Lance Taylor, Giuseppe Scrivano
   Socket handling from mysqld.cc (GPL2=)
   Note that since MySQL does not include the "or any later version" clause, the license of tailserver 
   is forced to be GPL2 by the magic of law.
   
*/

/*#include <config.h>
#include <sys/types.h>
#include <signal.h>
#include <getopt.h>

#include "system.h"
#include "error.h"
#include "fadvise.h"
#include "stdio--.h"
#include "xfreopen.h"

//#include <sys/select.h>
#include <sys/socket.h> */

/* Eludes me why everyone (see mysql, postgre, ngircd, redis) tries to implement their own socket event handling */
#include <ev.h>


static ev_io stdin_watcher;


static void stdin_cb (EV_P_ ev_io *w, int revents)
{
    puts("stdin ready (probably)", stderr);

    /* Is there data on stdin? */
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    if ( ! FD_ISSET(STDIN_FILENO, &fds)){
        puts("not ready");
        return;
    }
    puts("stdin ready for real", stderr);

    read into buffer provided by buffer.h

    /* TODO: if stdin is closed, remove it from event queue */

    tailer_t* tailer = w - offsetof(tailer_t, tailer_t.watcher);

    /* I don't create buffers, and this costs a blocking write. DoS attack is easy. TODO: Fix! */
    for (;;) {
        r = write(tailer->sd, buf, size);
        if (r == (size_t) -1) {
            /*
             man 2 read write
             EAGAIN or EWOULDBLOCK when a socket is a non-blocking mode means
             that the read/write would block.
             man 7 socket
             EAGAIN or EWOULDBLOCK when a socket is in a blocking mode means
             that the corresponding receiving or sending timeout was reached.
             */
            if (errno == EINTR ||
                ((tailer->fcntl_mode & O_NONBLOCK) &&
                (errno == EAGAIN || errno == EWOULDBLOCK))
            ) {
                continue /* TODO: All this seems too infinite to me... */
            }
        }
        break;
    }
}

void pipes_init ()
{
    if (O_BINARY && !isatty(STDIN_FILENO)) {
        xfreopen(NULL, "rb", stdin);
    }
    if (O_BINARY && !isatty(STDOUT_FILENO)) {
        xfreopen(NULL, "wb", stdout);
    }

    fadvise(stdin, FADVISE_SEQUENTIAL);

    setvbuf(stdout, NULL, _IONBF, 0);

    /* Listen to STDIN */
    ev_io_init(&stdin_watcher, stdin_cb, STDIN_FILENO, EV_READ);
    ev_io_start(EV_DEFAULT_UC_ & stdin_watcher);
}

void pipes_final ()
{
    if (close(STDIN_FILENO) != 0) {
        error(EXIT_FAILURE, errno, _("standard input"));
    }
}

