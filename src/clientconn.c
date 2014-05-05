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

#include "clientconn.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>

#include "bzero.h"
#include "strmov.h"
#include "safe_write.h"
/*#include "xnanosleep.h"*/
#include "evsleep.h"

#include "common-macros.h"

#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#else
/* Unix domain sockets are needed - all communication between tailserver and tailclient happens through them. */   
# error "Missing required sys/un.h definitions of UNIX domain sockets!" 
#endif

#define debug(...)
/*#define debug(x) {fputs(x, stderr);}*/
#define fdebugf(...)
/*#define fdebugf fprintf*/


typedef unsigned int uint;

#define INVALID_SOCKET -1
static int client_unix_sock = INVALID_SOCKET;

/* Server has a buffered tail reading which will not block outside kernel. */ 
static bool server_buffer_size_known;
static ssize_t server_buffer_size;
static ssize_t server_buffer_size_remain;


static int try_open_unix_socket (const char* file_path)
{
    int unix_sock;
    struct sockaddr_un UNIXaddr;
    int res;

    if (strlen(file_path) > (sizeof(UNIXaddr.sun_path) - 1)) {
        fprintf(stderr, _("The socket file path is too long (> %u): %s\n"),
                (uint) sizeof(UNIXaddr.sun_path) - 1, file_path);
        return false;
    }
    if ((unix_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }
    bzero((char*) &UNIXaddr, sizeof(UNIXaddr));
    UNIXaddr.sun_family = AF_UNIX;
    strmov(UNIXaddr.sun_path, file_path);

    res = connect(unix_sock, (struct sockaddr*) &UNIXaddr, sizeof(UNIXaddr));

    fdebugf(stderr, "connect try ends with res %d and errno %d\n", res, errno);
    if (res == 0) {
        return unix_sock;
    } else {
        close(unix_sock);
        return INVALID_SOCKET;
    }
}

static int open_unix_socket (const char* file_path, bool wait_for_file, double sleep_seconds)
{
    do {
        debug("trying to connect\n")
        int unix_sock = try_open_unix_socket(file_path);
        if (unix_sock != INVALID_SOCKET) {
            /* Finally.. some success! */
            return unix_sock;
        }

        /* Don't retry if */
        if (/* not asked to wait */
            !wait_for_file || 
            /* or we know the error can't be resolved by waiting */
            errno == EAFNOSUPPORT || errno == EALREADY || errno == EBADF || errno == EISCONN ||
            errno == ENOTSOCK || errno == EPROTOTYPE || errno == ENAMETOOLONG ||
            errno == EINVAL || errno == EOPNOTSUPP ||
            /* or user requests abort */
            errno == EINTR
        ) {
            break;
        }

        evsleep(EV_DEFAULT_UC_ sleep_seconds);
        /*if (xnanosleep (sleep_seconds) != 0) {
            perror(_("Can't read realtime clock"));
            return INVALID_SOCKET; *//* because flooding with connection attempts might be damaging */
        /*}*/

    } while (true);
    
    if (errno != EINTR) {
        perror(_("Can't access tailserver socket file"));
    }
    return INVALID_SOCKET;
}

bool clientconn_connect (const char* file_path, bool wait_for_file, double sleep_seconds)
{
    if (client_unix_sock != INVALID_SOCKET) {
        fputs(_("Error in tailclient - connection already open\n"), stderr);
        return true;
    }

    server_buffer_size_known = false;
    server_buffer_size = 0;
    server_buffer_size_remain = 0;

    client_unix_sock = open_unix_socket(file_path, wait_for_file, sleep_seconds);

    return client_unix_sock != INVALID_SOCKET;
}

void clientconn_disconnect ()
{
    if (client_unix_sock != INVALID_SOCKET) {
        shutdown(client_unix_sock, SHUT_RDWR);
        close(client_unix_sock);
        client_unix_sock = INVALID_SOCKET;
    }
}

int clientconn_socket ()
{
    return client_unix_sock;
}

bool clientconn_is_all_server_buffer_read ()
{
    return (server_buffer_size_known && server_buffer_size_remain == 0);
}

static const char* on_parse_error(const char* buf, ssize_t size)
{
    fputs(_("Tailserver violates communication protocol!\n"), stderr);
    clientconn_disconnect(); /* stop all further communication */
    return buf + size; /* treat all data as junk */
}

/* Returns pointer to where tail data starts */
static const char* parse_server_buffer_size (const char* buf, ssize_t size)
{
    const char* p = buf;
    bool error = false;
    while (!server_buffer_size_known && p < buf + size) {
        fdebugf(stderr, "tail size parsing '%c'\n", *p);
        if (ISDIGIT(*p)) {
            /* I'm not validating here. Should I be bothered? */
            server_buffer_size = (server_buffer_size * 10) + (*p - '0');
            if (server_buffer_size < 0) {
                return on_parse_error(buf, size);
            }
        } else
        if (*p == '\n') {
            server_buffer_size_known = true;
            server_buffer_size_remain = server_buffer_size;
        } else {
            return on_parse_error(buf, size);
        }
        p++;
    }
    fdebugf(stderr, "tail buffer size %d completed %d\n", server_buffer_size, server_buffer_size_known);
    
    return p;
}

ssize_t clientconn_read_server_buffer (char* buf, ssize_t size)
{
    ssize_t res, tail_data_res;
    if (size == 0 || clientconn_is_all_server_buffer_read()) {
        return 0;
    }
    if ( ! server_buffer_size_known) {
        /* Careful not to read past the buffer here */
        /* Reading byte by byte is simpler than buffering data between reads... */
        ssize_t tmpsize = MIN((server_buffer_size+1), MIN((ssize_t)BUFSIZ, size)); /* ...and we even have a lower bound. */
        char tmpbuf[BUFSIZ];
        fdebugf(stderr, "reading %d bytes (requested %d), buffer size at least %d\n", tmpsize, size, server_buffer_size);

        res = read(client_unix_sock, tmpbuf, (size_t)tmpsize);

        fdebugf(stderr, "read result %d\n", res);
        if (res > 0) {
            /* The first line received during a connection is the size of the tail */
            const char* p = parse_server_buffer_size(tmpbuf, res);
            /* The rest is contents of the tail */
            tail_data_res = tmpbuf + res - p;
            if (tail_data_res > 0) {
                memcpy(buf, p, (size_t)tail_data_res);
            }
        }
    } else {
        /* Read directly into output buffer - no protocol data remain */

        res = read(client_unix_sock, buf, (size_t)(MIN(size,server_buffer_size_remain)));

        tail_data_res = MAX(0, res);
    }

    if (res == 0) {
        clientconn_disconnect();
    }

    /* Track the server buffer size */
    if (server_buffer_size_remain >= tail_data_res) {
        server_buffer_size_remain -= tail_data_res;
    } else {
        server_buffer_size_remain = 0;
    }
    if (res > 0) {
        return tail_data_res; /* may be 0, so do check clientconn_eof() */
    } else {
        return res;
    }
}

ssize_t clientconn_read_server_follow (char* buf, ssize_t size)
{
    ssize_t res;
    if (size == 0 || ! clientconn_is_all_server_buffer_read()) {
        return 0;
    }

    res = read(client_unix_sock, buf, (size_t)size);

    if (res == 0) {
        clientconn_disconnect();
    }
    return res;
}

bool clientconn_eof ()
{
    return client_unix_sock == INVALID_SOCKET;
}

ssize_t clientconn_write (const char* buf, ssize_t size)
{
    ssize_t n = safe_write(client_unix_sock, buf, size);

    if (n <= 0) {
        if (n < 0 && errno == EPIPE /* Broken pipe */
        ) {
            /* Normally broken connection. Not an error. */
        } else {
            perror(_("Communicating to server encountered an error."));
        }
    }
    return n;
}


