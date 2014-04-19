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
   (I used the GLP3+ code but I've checked it hasn't changed significantly since the GPL2+ times)
   tee.c: Mike Parker, Richard M. Stallman, and David MacKenzie
   tail.c: Paul Rubin, David MacKenzie, Ian Lance Taylor, Giuseppe Scrivano
   Socket handling from mysqld.cc (GPL2=)
   Note that since MySQL does not include the "or any later version" clause,
   therefore the acting license of tailserver is forced to be GPL2 by the magic of law.
   
*/

#include "config.h"

#include "sockets.h"
#include "buffer.h"

#include <sys/stat.h>
/* Some Posix-wannabe systems define _S_IF* macros instead of S_IF*, but
   do not provide the S_IS* macros that Posix requires. */
#if defined (_S_IFSOCK) && !defined (S_IFSOCK)
#define S_IFSOCK _S_IFSOCK
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "fcntl_wrap.h"
#include "bzero.h"
#include "offsetof.h"
#include "strmov.h"

/* Eludes me why everyone (see mysql, postgre, ngircd, redis) tries to implement their own event handling */
#include <ev.h>

#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#else
/* Unix domain sockets are needed - all communication between tailserver and tailclient happens through them. */   
# error "Missing required sys/un.h definitions of UNIX domain sockets!" 
#endif

#define debug(...)
/*#define debug(x) fputs(x, stderr)*/


/* Test accept this many times. */
#define MAX_ACCEPT_RETRY 10

#define INVALID_SOCKET -1


typedef unsigned int uint;
typedef SOCKET_SIZE_TYPE size_socket;


static const char* socket_file_path;
static int unix_sock;
static int socket_flags;
static uint accept_error_count;

static ev_io unix_sock_watcher;

typedef struct tailer_t {
    int socket;
    ev_io readable_watcher;
    struct tailer_t* prev;
    struct tailer_t* next;
} tailer_t;

static tailer_t* first_tailer;


static void tailer_final (tailer_t* tailer);

static ssize_t tailer_write_blocking (tailer_t* tailer, const char* buf, ssize_t size)
{
    ssize_t n;

    n = safe_write(tailer->socket, buf, size);

    if (n < size) {
        perror(_("Writing to client encountered an error"));
    }
    return n;
}

static void tailer_send_buffered_tail (tailer_t* tailer)
{
    const char* pos = buffer_get_tail_chunk();
    size_t offset = buffer_get_tail_offset();
    size_t chunk_size = buffer_chunk_size(pos);

    while (chunk_size > 0) {
        
        ssize_t size_written = tailer_write_blocking(tailer, pos + offset, chunk_size - offset);
        
        if (size_written < (ssize_t)(chunk_size - offset)) {
            debug("written too little\n");
            tailer_final(tailer);
            break;
        }
        debug("looking for the next chunk\n");
        pos = buffer_advance_chunk(pos);
        offset = 0;
        chunk_size = buffer_chunk_size(pos);
    }
}

void sockets_write_blocking (char* buf, ssize_t size)
{
    tailer_t* tailer = first_tailer;
    while (tailer) {
        
        ssize_t size_written = tailer_write_blocking(tailer, buf, size);
        
        if (size_written < size) {
            debug("misbehaving client\n");
            tailer_t* next = tailer->next;
            tailer_final(tailer); /* Destroy misbehaving clients */
            tailer = next;
        } else {
            tailer = tailer->next;
        }
    }
}

/* Called when a tailer socket possibly sends something. */
static void tailer_readable_cb (EV_P_ ev_io* w, int revents)
{
    tailer_t* tailer = (tailer_t*)((char*)w - offsetof(tailer_t, readable_watcher));
    bool error = false;
    char buf[1024];

    ssize_t res = read(tailer->socket, buf, sizeof(buf));

    if (res > 0) {
        /* Client has sent something. Ignoring seems nicer than killing the connection. Undocumented. */
    } else
    if (res == 0) {
        /* EOF */
    } else
    if (res < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
        perror(_("Connection error during read"));
        error = true;
    }

    if (res == 0 || error) {
        debug("eof or error, ending client\n");
        tailer_final(tailer);
    }
}

tailer_t* tailer_init (int socket)
{
    /* TODO: malloc error handling! */
    tailer_t* tailer = malloc(sizeof(tailer_t));
    tailer->prev = NULL;
    tailer->next = first_tailer;
    if (first_tailer) {
        first_tailer->prev = tailer;
    }
    first_tailer = tailer;

    tailer->socket = socket;
    ev_io_init(&tailer->readable_watcher, tailer_readable_cb, socket, EV_READ);
    ev_io_start(EV_DEFAULT_UC_ &tailer->readable_watcher);
    
    return tailer;
}

void tailer_final (tailer_t* tailer)
{
    ev_io_stop(EV_DEFAULT_UC_ &tailer->readable_watcher);
    shutdown(tailer->socket, SHUT_RDWR);
    close(tailer->socket);

    if (tailer->prev) {
        tailer->prev->next = tailer->next;
    } else {
        first_tailer = tailer->next;
    }
    if (tailer->next) {
        tailer->next->prev = tailer->prev;
    }
    free(tailer);
}

/* Called when listener socket is active. False positives possible. */
static void unix_sock_cb (EV_P_ ev_io *w, int revents)
{
    struct sockaddr_storage cAddr;
    int new_sock = INVALID_SOCKET;
    uint retry;

    /* Is this a new connection request? */
    /* TODO: Look first, then try to accept! (I think I've seen a flag somewhere which indicates if connections are pending to be accepted) */

    debug("new connection request, apparently\n");

#ifdef HAVE_FCNTL_NONBLOCK
    fcntl(unix_sock, F_SETFL, socket_flags | O_NONBLOCK); /* mysqld.cc is really conservative... so am I. */
#endif
    for (retry=0; retry < MAX_ACCEPT_RETRY; retry++) {
        size_socket length = sizeof(struct sockaddr_storage);
        
        new_sock = accept(unix_sock, (struct sockaddr *)(&cAddr), &length);
        
        if (new_sock != INVALID_SOCKET || (errno != EINTR && errno != EAGAIN)) {
            break;
        }
        perror(_("Retrying accept")); /* This should never happen? */
#ifdef HAVE_FCNTL_NONBLOCK
        if (retry == MAX_ACCEPT_RETRY - 1) {
            fcntl(unix_sock, F_SETFL, socket_flags); /* Try without O_NONBLOCK */
        }
#endif
    }
#ifdef HAVE_FCNTL
    /* TODO: Is there a good reason why listener's flags are set to accepted sockets or just performance I don't need? */
    fcntl(unix_sock, F_SETFL, socket_flags);
#endif
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
            close(new_sock);
            return;
        }
    }

#ifdef HAVE_FCNTL_NONBLOCK
    fcntl(new_sock, F_SETFL, O_NONBLOCK);
#endif

    debug("creating client\n");

    {
        /* Add the new connection, send it the tail */

        tailer_t* tailer = tailer_init(new_sock);

        tailer_send_buffered_tail(tailer);

    }

    debug("new connection successful\n");
}

void sockets_config_file (const char* file_path)
{
    socket_file_path = file_path;
}

bool sockets_init ()
{
    struct sockaddr_un UNIXaddr;

    /* Allow tailserver be run without parameters */

    if (socket_file_path == NULL) {
        unix_sock = INVALID_SOCKET;
        return true;
    }

    /* Create the UNIX socket */

    if (strlen(socket_file_path) > (sizeof(UNIXaddr.sun_path) - 1)) {
        fprintf(stderr, _("The socket file path is too long (> %u): %s"),
                (uint) sizeof(UNIXaddr.sun_path) - 1, socket_file_path);
        return false;
    }
    if ((unix_sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror(_("Can't start server : UNIX Socket ")); /* TODO: Doesn't this need to go to stderr? */
        return false;
    }
    bzero((char*) &UNIXaddr, sizeof(UNIXaddr));
    UNIXaddr.sun_family = AF_UNIX;
    strmov(UNIXaddr.sun_path, socket_file_path);

    /* TODO: Check if that path is a socket or unused... otherwise bail out and quit. */
    /* UPDATE: Don't bail out! Instead of letting set params and group by options, allow creating file beforehand. Unlink at exit only if created. */
    unlink(socket_file_path);

    {
        int arg = 1;
        setsockopt(unix_sock, SOL_SOCKET, SO_REUSEADDR, (char*) &arg, sizeof(arg));
        /* Future OSs are unknown and this doesn't hurt. */
    }

    umask(0); /* TODO: Both mysqld and postgre does this. Redis does not. Why? */

    if (bind(unix_sock, (struct sockaddr *)(&UNIXaddr), sizeof(UNIXaddr)) < 0) { /* TODO: Why casting magic? */
        printf(_("Can't start server : Bind on unix socket"));
        printf(_("Do you already have another server running on socket: %s ?"), socket_file_path);
        close(unix_sock);
        unix_sock = INVALID_SOCKET;
        return false;
    }

    /* TODO: Isn't (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) nicer? 
     * UPDATE: Maybe, but then you need to copy posixstat.h because of the problems described in it. */ 
    umask(0660); /* TODO: Why does postgre use 0777? Nginx uses 0666 too. Anyway, allow to change using options. Group name param too. (User is useless because only root could change and root shouldn't launch tailable programs anyway) */
#if defined(S_IFSOCK) && defined(SECURE_SOCKETS)
    (void) chmod(socket_file_path,S_IFSOCK); /* Fix solaris 2.6 bug */
#endif

    if (listen(unix_sock, 65535) < 0) { /* backlog limiting is done with kernel parameters */
        printf(_("listen() on Unix socket failed with error %d"), errno);
        sockets_final();
        return false;
    }

#ifdef HAVE_FCNTL
    socket_flags = fcntl(unix_sock, F_GETFL, 0);
#endif

    ev_io_init(&unix_sock_watcher, unix_sock_cb, unix_sock, EV_READ);
    ev_io_start (EV_DEFAULT_UC_ &unix_sock_watcher);

    return true;
}

void sockets_final ()
{
    debug("sockets finalizing\n");

    if (unix_sock != INVALID_SOCKET) {
        shutdown(unix_sock, SHUT_RDWR);
        close(unix_sock);
        unlink(socket_file_path);
        unix_sock = INVALID_SOCKET;
    }

    while (first_tailer) {
        tailer_final(first_tailer);
    }
}


