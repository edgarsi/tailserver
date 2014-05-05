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

#include "config.h"

#include "clientloop.h"
#include "buffer.h"
#include "clientpipes.h"
#include "clientconn.h"
/*#include "xnanosleep.h"*/
#include "evsleep.h"

#include "error.h"
#include <stdio.h>
#include <errno.h>

/* Eludes me why everyone (see mysql, postgre, ngircd, redis) tries to implement their own socket event handling */
#include <ev.h>

#define debug(...)
/*#define debug(x) {fputs(x, stderr);}*/
#define fdebugf(...)
/*#define fdebugf fprintf*/

static bool follow;
static bool tail_output;

void clientloop_config_follow (bool set_follow)
{
    follow = set_follow;
}

void clientloop_init ()
{
    /* Nothing to do */
}

void clientloop_final ()
{
    clientconn_disconnect();
}

static bool output_buffered_tail ()
{
    const char* pos = buffer_get_tail_chunk();
    size_t offset = buffer_get_tail_offset();
    size_t chunk_size = buffer_chunk_size(pos);

    while (chunk_size > 0) {

        if ( ! clientpipes_write_blocking(pos + offset, (ssize_t)(chunk_size - offset))) {
            debug("write error\n");
            return false;
        }
        debug("looking for the next chunk\n");
        pos = buffer_advance_chunk(pos);
        offset = 0;
        chunk_size = buffer_chunk_size(pos);
    }

    return true;
}

/* Called when a tailserver possibly sends something. */
static void unix_sock_readable_cb (EV_P_ ev_io *w, int revents)
{
    bool error = false;
    ssize_t res;

    if ( ! clientconn_is_all_server_buffer_read()) {
        /* Read from server buffer into our tail buffer */
        res = clientconn_read_server_buffer(buffer_end(), (ssize_t)buffer_available_for_append());
        if (res > 0) {
            error |=! buffer_set_appended((size_t)res);
            if (error) { debug("error while appending\n") };
        }
    } else {
        /* Read the follow data from server */
        char buf[BUFSIZ];
        res = clientconn_read_server_follow(buf, sizeof(buf));
        if (res > 0) {
            error |=! clientpipes_write_blocking(buf, res);
            if (error) { debug("error while writing\n") };
        }
    }

    if (res == 0) {
        /* EOF or only protocol overhead read */
    } else
    if (res < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
        if (errno == ECONNRESET /* Connection reset by peer */
         || errno == ECONNABORTED /* Software caused connection abort */
        ) {
            /* Normally broken connection. Not an error from the user perspective. */
        } else {
            perror(_("Connection error during read"));
        }
        error = true;
    }

    /* When all server buffer is read, we want to output the tail. Then - follow or disconnect. */
    if (clientconn_is_all_server_buffer_read()) {
        if ( ! tail_output) {
            error |=! output_buffered_tail();
            tail_output = true;
        }
        if ( ! follow) {
            clientconn_disconnect();
        }
    }

    if (clientconn_eof() || error) {
        /* Quit the ev_run call */
        fdebugf(stderr,"breaking ev loop because %d || %d\n", clientconn_eof(), error);
        ev_io_stop(EV_DEFAULT_UC_ w);
        ev_break(EV_A_ EVBREAK_ALL);
    }
}

void clientloop_run (const char* file_path, bool wait_file_at_startup, bool reopen_file, double sleep_seconds)
{
    tail_output = false;

    while (true) {
        debug("clientloop_run iteration\n");

        /* Initialize */
        if ( ! clientconn_connect (file_path, (wait_file_at_startup | reopen_file), sleep_seconds)) {
            break;
        }
        if ( ! follow) {
            /* Keep connection throughput to a minimum by telling the server that we won't need more than its buffer. */
            const char c = '\n'; /* any data */
            if (clientconn_write (&c, 1) <= 0) {
                clientconn_disconnect();
                break;
            }
        } 

        /* Loop */
        {
            ev_io watcher;
            ev_io_init(&watcher, unix_sock_readable_cb, clientconn_socket(), EV_READ);
            ev_io_start (EV_DEFAULT_UC_ &watcher);
            ev_run (EV_DEFAULT_UC_ 0);
        }

        /* Finalize */
        clientconn_disconnect();

        /* Prepare for retry */
        fdebugf(stderr, "retry decision %d && %d && !%d\n", reopen_file, follow, clientpipes_broken());
        if (reopen_file && follow && !clientpipes_broken()) {
            /* Need to sleep here for cases where the creating the connection works but reading fails */
            evsleep(EV_DEFAULT_UC_ sleep_seconds);
            /*if (xnanosleep (sleep_seconds) != 0) {
                perror(_("Can't read realtime clock"));
                break; *//* because flooding with connections is not awesome */
            /*}*/
        } else {
            break;
        }
    }
}

