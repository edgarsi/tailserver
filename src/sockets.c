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

#include "config.h"

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
#include <sys/socket.h>*/ 

/* Eludes me why everyone (see mysql, postgre, ngircd, redis) tries to implement their own event handling */
#include <ev.h>

#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#else
/* As long as I've not implemented an IP server, sockets are a requirement. */   
# error "Missing required sys/un.h definitions of UNIX domain sockets!" 
#endif

/* Test accept this many times. */
#define MAX_ACCEPT_RETRY 10

#define INVALID_SOCKET -1

static int unix_sock;
static int socket_flags;
static uint accept_error_count;

static ev_io unix_sock_watcher;


/* Called when a tailer socket is active. False positives possible. */
static void tailer_cb (EV_P_ ev_io *w, int revents)
{
    fcntl(sock, F_SETFL, O_NONBLOCK);

    bool error = false;
    char buf[1024];
    ssize_t res = read(vio->sd, buf, sizeof(buf));
    if (res > 0) {
        /* Client has sent something. Ignoring seems nicer than killing the connection. */
    } else
    if (res == 0) {
        /* EOF */
    } else
    if (res < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
        perror(_("Connection error during read"));
        error = true;
    }

    if (!error) {
        fcntl(sock, F_SETFL, 0);
    }

    if (res == 0 || error) {
        todo remove this connection
    }
}

/* Called when listener socket is active. False positives possible. */
static void unix_sock_cb (EV_P_ ev_io *w, int revents)
{
    struct sockaddr_storage cAddr;
    int new_sock = INVALID_SOCKET;

    /* Is this a new connection request? */
    /* TODO: Look first, then try to accept! */

    fcntl(unix_sock, F_SETFL, socket_flags | O_NONBLOCK); /* mysqld.cc is really conservative... so am I. */
    for (uint retry=0; retry < MAX_ACCEPT_RETRY; retry++) {
        size_socket length = sizeof(struct sockaddr_storage);
        
        new_sock = accept(unix_sock, (struct sockaddr *)(&cAddr), &length);
        
        if (new_sock != INVALID_SOCKET || (errno != EINTR && errno != EAGAIN)) {
            break;
        }
        perror(_("Retrying accept")); /* This should never happen? */
        if (retry == MAX_ACCEPT_RETRY - 1) {
            fcntl(sock, F_SETFL, socket_flags); /* Try without O_NONBLOCK */
        }
    }
    fcntl(unix_sock, F_SETFL, socket_flags);
    if (new_sock == INVALID_SOCKET) {
        if ((accept_error_count++ & 255) == 0) { /* This can happen often */
            perror(_("Error in accept"));
        }
        if (errno == ENFILE || errno == EMFILE) {
            sleep(1); /* Do not eat CPU like a madman */
        }
        return;
    }

    {
        size_socket dummyLen;
        struct sockaddr_storage dummy;
        dummyLen = sizeof(dummy);
        if ( getsockname(new_sock,(struct sockaddr *)&dummy, (SOCKET_SIZE_TYPE *)&dummyLen) < 0 )
        {
            perror("Error on new connection socket");
            shutdown(new_sock, SHUT_RDWR);
            closesocket(new_sock);
            return;
        }
    }

    fcntl(new_sock, F_SETFL, 0); /* vio.c (mysql) thinks this may be useful... */

    /* Add the new connection */

    add to some linked list of tailer_t

    add to loop to check for eof and write-ready
}

static bool sockets_init ()
{
    /* Create the UNIX socket */

    struct sockaddr_un UNIXaddr;

    if (strlen(socket_file_path) > (sizeof(UNIXaddr.sun_path) - 1)) {
        fprintf(stderr, _("The socket file path is too long (> %u): %s"),
                (uint) sizeof(UNIXaddr.sun_path) - 1, socket_file_path);
        return false;
    }
    if ((unix_sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror(_("Can't start server : UNIX Socket "));
        return false;
    }
    bzero((char*) &UNIXaddr, sizeof(UNIXaddr));
    UNIXaddr.sun_family = AF_UNIX;
    strmov(UNIXaddr.sun_path, socket_file_path);

    /* TODO: Check if that path is a socket or unused... otherwise bail out and quit. */
    unlink(socket_file_path);

    arg = 1;
    setsockopt(unix_sock, SOL_SOCKET, SO_REUSEADDR, (char*) &arg, sizeof(arg)); /* Future OSs are unknown and this doesn't hurt. */

    umask(0); /* TODO: Both mysqld and postgre does this. Redis does not. Why? */

    if (bind(unix_sock, reinterpret_cast<struct sockaddr *>(&UNIXaddr), sizeof(UNIXaddr)) < 0) {
        printf(_("Can't start server : Bind on unix socket"));
        printf(_("Do you already have another server running on socket: %s ?"), socket_file_path);
        close();
        unix_sock = INVALID_SOCKET;
        return false;
    }

    umask(((~my_umask) & 0666)); /* TODO: Why does postgre use 0777? Nginx uses 0666 too. Anyway, default to 0600 or so and allow to change using options. Group name param too. (User is useless because only root could change and root shouldn't launch tailable programs anyway) */
#if defined(S_IFSOCK) && defined(SECURE_SOCKETS)
    (void) chmod(socket_file_path,S_IFSOCK); /* Fix solaris 2.6 bug */
#endif

    if (listen(unix_sock, 65535) < 0) { /* backlog limiting is done with kernel parameters */
        printf(_("listen() on Unix socket failed with error %d"), socket_errno);
        sockets_final();
        return false;
    }

    socket_flags = fcntl(unix_sock, F_GETFL, 0);

    ev_io_init(&unix_sock_watcher, unix_sock_cb, unix_sock, EV_READ);
    ev_io_start (EV_DEFAULT_UC_ &unix_sock_watcher);

    return true;
}

static void sockets_final ()
{
    if (unix_sock != INVALID_SOCKET) {
        shutdown(unix_sock, SHUT_RDWR);
        close(unix_sock);
        unlink(socket_file_path);
        unix_sock = INVALID_SOCKET;
    }
}


