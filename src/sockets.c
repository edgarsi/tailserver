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
#include "pipes.h"
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
#include "safe_write.h"

/* Eludes me why everyone (see mysql, postgre, ngircd, redis) tries to implement their own event handling */
#include <ev.h>

#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#else
/* Unix domain sockets are needed - all communication between tailserver and tailclient happens through them. */   
# error "Missing required sys/un.h definitions of UNIX domain sockets!" 
#endif

#define debug(...)
/*#define debug(x) {fputs(x, stderr);}*/


/* Test accept this many times. */
#define MAX_ACCEPT_RETRY 10

#define INVALID_SOCKET -1


typedef unsigned int uint;
typedef SOCKET_SIZE_TYPE size_socket;


static const char* socket_file_path;
static int unix_sock = INVALID_SOCKET;
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

static bool tailer_block_forever;


static void tailer_final (tailer_t* tailer);

static ssize_t tailer_write_blocking (tailer_t* tailer, const char* buf, ssize_t size)
{
    ssize_t n;

    /* The socket is nonblocking (on normal platforms) but safe_write can handle that, partial blocking. */
    /* Full blocking: */
    /*  Make this function block as stdout_write_blocking does, i.e., forever. */
    /*  If this is eats too much CPU, try dynamically removing the O_NONBLOCK. It's still a weird solution though. */
    /* Partial blocking: */
    /*  Use that safe_write being safe will not block forever. Get rid of blocking tailers for stdout must not be blocked. */
    if (tailer_block_forever) {
        n = safe_write_forever(tailer->socket, buf, size);
    } else {
        n = safe_write(tailer->socket, buf, size);
    }

    if (n < size) {
        if (n < 0 && (errno == EPIPE /* Broken pipe */
                   || errno == ECONNRESET /* Connection reset by peer */
        )) {
            /* Normally broken connection. Not an error. */
        } else {
            perror(_("Writing to client encountered an error. Can't keep up the pace? Ditching the injured."));
        }
    }
    return n;
}

static void tailer_send_buffered_tail (tailer_t* tailer)
{
    /* Tail size */

    {
        char s[1024];
        size_t strlen_s;
        ssize_t size_written;
        
        sprintf(s, "%llu\n", (unsigned long long int)buffer_tail_size());
        strlen_s = strlen(s);

        size_written = tailer_write_blocking(tailer, s, (ssize_t)strlen_s);

        if (size_written < (ssize_t)strlen_s) {
            debug("written too little (writing size)\n");
            tailer_final(tailer);
            return;
        }
    }

    /* Contents of the tail */

    {
        const char* pos = buffer_get_tail_chunk();
        size_t offset = buffer_get_tail_offset();
        size_t chunk_size = buffer_chunk_size(pos);

        while (chunk_size > 0) {

            ssize_t size_written = tailer_write_blocking(tailer, pos + offset, (ssize_t)(chunk_size - offset));
    
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
}

void sockets_write_blocking (char* buf, ssize_t size)
{
    tailer_t* tailer = first_tailer;
    while (tailer) {
        
        ssize_t size_written = tailer_write_blocking(tailer, buf, size);
        
        if (size_written < size) {
            debug("misbehaving client\n")
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
        /* Client has sent something. End the connection. */
        /* This feature is not documented is used by tailclient to avoid unnecessary */
        /* transfers when client is in "tail but do not follow" mode. */
    } else
    if (res == 0) {
        /* EOF */
    } else
    if (res < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
        if (errno == ECONNRESET /* Connection reset by peer */
         || errno == ECONNABORTED /* Software caused connection abort */
        ) {
            /* Normally broken connection. Not an error. */
        } else {
            perror(_("Connection error during read"));
        }
        error = true;
    }

    if (res >= 0 || error) {
        debug("data, eof or error, ending client\n");
        tailer_final(tailer);
    }
}

tailer_t* tailer_init (int socket)
{
    tailer_t* tailer = malloc(sizeof(tailer_t));
    if (tailer == NULL) {
        fputs(_("Out of memory. Can't allocate for new client.\n"), stderr);
        return NULL;
    }
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

    debug("new connection request, apparently\n");

#ifdef HAVE_FCNTL_NONBLOCK
    fcntl(unix_sock, F_SETFL, socket_flags | O_NONBLOCK); /* mysqld.cc is really conservative... so am I. */
#endif
    for (retry=0; retry < MAX_ACCEPT_RETRY; retry++) {
        size_socket length = sizeof(struct sockaddr_storage);

        new_sock = accept(unix_sock, (struct sockaddr *)(&cAddr), &length);

        if (new_sock != INVALID_SOCKET || (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)) {
            break;
        }
        perror(_("Retrying accept")); /* This should never happen? */
#ifdef HAVE_FCNTL_NONBLOCK
        if (retry == MAX_ACCEPT_RETRY - 1) {
            fcntl(unix_sock, F_SETFL, socket_flags); /* Try without O_NONBLOCK */
        }
#endif
    }
#ifdef HAVE_FCNTL_NONBLOCK
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
        if (tailer == NULL) {
            shutdown(new_sock, SHUT_RDWR);
            close(new_sock);
            return;
        }

        tailer_send_buffered_tail(tailer);

    }

    pipes_on_new_client ();

    debug("new connection successful\n");
}

void sockets_config_file (const char* file_path)
{
    socket_file_path = file_path;
}

void sockets_config_tailer_block_forever (bool block)
{
    tailer_block_forever = block;
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
        fprintf(stderr, _("The socket file path is too long (> %u): %s\n"),
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

    /* Unlinking is useful if tailserver is killed abruptly. But what if some other tailserver has created 
     * and still uses that file? */
    /*unlink(socket_file_path);*/

    /* It is tempting to set at least u+rw regardless of umask because POSIX states that rw is needed
     * for the socket file to be connectable. Could be helpful. Could be restricting. Don't annoy the user. */
    /*mode_t omask = umask(0); umask(oumask & ~(S_IRUSR | S_IWUSR | S_IXUSR)) #include posixstat.h */

    if (bind(unix_sock, (struct sockaddr *)(&UNIXaddr), sizeof(UNIXaddr)) < 0) {
        perror(_("Can't start server : Bind on unix socket. "));
        fprintf(stderr, _("Do you already have another server running on socket: \"%s\" ? "
                "If not, try deleting the socket file and run again.\n"), socket_file_path);
        close(unix_sock);
        unix_sock = INVALID_SOCKET;
        return false;
    }

#if defined(S_IFSOCK) && defined(SECURE_SOCKETS)
    (void) chmod(socket_file_path,S_IFSOCK); /* Fix solaris 2.6 bug */
#endif

    if (listen(unix_sock, 65535) < 0) { /* backlog limiting is done with kernel parameters */
        fprintf(stderr, _("listen() on Unix socket failed with error %d\n"), errno);
        sockets_final();
        return false;
    }

#ifdef HAVE_FCNTL_NONBLOCK
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


