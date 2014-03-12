/* 
   Copyright (C) 1985-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Some code is borrowed from GNU coreutils tee.c and tail.c (GLP3)
/* tee.c: Mike Parker, Richard M. Stallman, and David MacKenzie */
/* tail.c: Paul Rubin, David MacKenzie, Ian Lance Taylor, Giuseppe Scrivano */

/* Socket handling from mysqld.cc (GPL2) */ /* TODO: Does not include the "or any later version" clause. How does that affect licensing? */

/* Code keeps the coding styles of each of the borrowed functions. Might not be the best idea ever. */

#include <config.h>
#include <sys/types.h>
#include <signal.h>
#include <getopt.h>

#include "system.h"
#include "error.h"
#include "fadvise.h"
#include "stdio--.h"
#include "xfreopen.h"

//#include <sys/select.h>
#include <sys/socket.h> 

/* Eludes me why everyone (see mysql, ngircd, redis) tries to implement their own socket handling */
#include <ev.h>

#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#else
// As long as I've not implemented an IP server, sockets are a requirement.   
# error "Missing required sys/un.h definitions of UNIX domain sockets!" 
#endif

#define PROGRAM_NAME "tailserver"

/* Number of items to tail.  */
#define DEFAULT_N_LINES 1000

/* Test accept this many times. */
#define MAX_ACCEPT_RETRY 10

/* If true, interpret the numeric argument as the number of lines.
   Otherwise, interpret it as the number of bytes.  */
static bool count_lines;

/* If true, ignore interrupts. */
static bool ignore_interrupts;

typedef int     my_socket;      /* File descriptor for sockets */
#define INVALID_SOCKET -1

static my_socket unix_sock;
int socket_flags;
uint error_count;

static struct option const long_options[] =
{
  //{"append", no_argument, NULL, 'a'},
  {"bytes", required_argument, NULL, 'c'},
  {"lines", required_argument, NULL, 'n'},
  {"ignore-interrupts", no_argument, NULL, 'i'},
  {GETOPT_HELP_OPTION_DECL},
  {GETOPT_VERSION_OPTION_DECL},
  {NULL, 0, NULL, 0}
};

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("Usage: %s [OPTION]... <SOCKET_FILE>\n"), program_name);
      printf (_("\
Copy standard input to to standard output, and also serve it to UNIX domain socket connections.\n\
Give new connections to SOCKET_FILE the last %d lines. Output new lines as they arrive at standard input.\n\
\n\
"), DEFAULT_N_LINES);
      
      fputs (_("\
   -c, --bytes=K             output the last K bytes\n\
"), stdout);
      printf (_("\
   -n, --lines=K             output the last K lines, instead of the last %d;\n\
"), DEFAULT_N_LINES);
      fputs (_("\
   -i, --ignore-interrupts   ignore interrupt signals\n\
"), stdout);
      
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
K (the number of bytes or lines) may have a multiplier suffix:\n\
b 512, kB 1000, K 1024, MB 1000*1000, M 1024*1024,\n\
GB 1000*1000*1000, G 1024*1024*1024, and so on for T, P, E, Z, Y.\n\
\n\
"), stdout);
      emit_ancillary_info ();
    }
  exit (status);
}

static void
file_init ()
{
  if (O_BINARY && ! isatty (STDIN_FILENO))
    xfreopen (NULL, "rb", stdin);
  if (O_BINARY && ! isatty (STDOUT_FILENO))
    xfreopen (NULL, "wb", stdout);

  fadvise (stdin, FADVISE_SEQUENTIAL);

  setvbuf (stdout, NULL, _IONBF, 0);
}

static void
file_final ()
{
  /*  nothing to do */
}

static void
network_init ()
{
  /*
  ** Create the UNIX socket
  */
  struct sockaddr_un	UNIXaddr;
  
  if (strlen(socket_file_path) > (sizeof(UNIXaddr.sun_path) - 1))
  {
    printf(_("The socket file path is too long (> %u): %s"),
                    (uint) sizeof(UNIXaddr.sun_path) - 1, socket_file_path);
    file_final();
    exit(1);
  }
  if ((unix_sock= socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
  {
    printf(_("Can't start server : UNIX Socket ")); /* TODO: How do I decide between stderr and stdout? */
    file_final();
    exit(1);
  }
  bzero((char*) &UNIXaddr, sizeof(UNIXaddr));
  UNIXaddr.sun_family = AF_UNIX;
  strmov(UNIXaddr.sun_path, socket_file_path);
  /* TODO: Check if that path is a socket or unused... otherwise bail out and quit. */
  (void) unlink(socket_file_path);
  arg= 1;
  (void) setsockopt(unix_sock,SOL_SOCKET,SO_REUSEADDR,(char*)&arg, /* TODO: Why does mysqld set SO_REUSEADDR on a unix socket (specifically!)? */
                    sizeof(arg));
  umask(0); /* TODO: Both mysqld and postgre does this. Redis does not. Why? */ 
  if (bind(unix_sock, reinterpret_cast<struct sockaddr *>(&UNIXaddr),
           sizeof(UNIXaddr)) < 0)
  {
    printf(_("Can't start server : Bind on unix socket"));
    printf(_("Do you already have another server running on socket: %s ?", socket_file_path);
    /* TODO: postgre calls closesocket here. shouldn't I too? */
    closesocket();
    file_final();
    exit(1);
  }
  umask(((~my_umask) & 0666)); /* TODO: Why does postgre use 0777? Nginx uses 0666 too. Anyway, default to 0600 or so and allow to change using options. Group name param too. (User is useless because only root could change and root shouldn't launch tailable programs anyway) */
#if defined(S_IFSOCK) && defined(SECURE_SOCKETS)
  (void) chmod(socket_file_path,S_IFSOCK);	/* Fix solaris 2.6 bug */
#endif
  if (listen(unix_sock, 65535) < 0) /* backlog limiting is done with kernel parameters */
  {
    printf(_("listen() on Unix socket failed with error %d"),
                    socket_errno);
    network_final(); /* TODO: is shutdown() harmless in this position? mysqld does nothing, postgre calls closesocket only. */
    file_final();
    exit(1);
  }

#ifdef HAVE_FCNTL
  socket_flags=fcntl(unix_sock, F_GETFL, 0);
#endif
}

static void
network_final ()
{
  if (unix_sock != INVALID_SOCKET)
  {
    (void) shutdown(unix_sock, SHUT_RDWR);
    (void) closesocket(unix_sock);
    (void) unlink(socket_file_path);
    unix_sock= INVALID_SOCKET;
  }
}

static void
libev_error_handler (const char *msg)
{
  perror (msg); /* TODO: I smell the same error number output twice. Verify! */
  if (socket_errno != SOCKET_EINTR)
  {
    fprintf(stderr, _("Got error from select or poll: %d"),socket_errno);
  }
  network_final ();
  file_final ();
  exit (1);
}

static void
unix_sock_cb (EV_P_ ev_io *w, int revents)
{
  struct sockaddr_storage cAddr;
  
  /* Is this a new connection request ? */
#if defined(O_NONBLOCK)
  fcntl(unix_sock, F_SETFL, socket_flags | O_NONBLOCK); /* TODO: Why does redis use a blocking accept in anetGenericAccept? What does postgre do? */
#elif defined(O_NDELAY)
  fcntl(unix_sock, F_SETFL, socket_flags | O_NDELAY);
#endif
  for (uint retry=0; retry < MAX_ACCEPT_RETRY; retry++)
  {
    size_socket length= sizeof(struct sockaddr_storage);
    new_sock= accept(unix_sock, (struct sockaddr *)(&cAddr),
                     &length);
    if (new_sock != INVALID_SOCKET ||
        (socket_errno != SOCKET_EINTR && socket_errno != SOCKET_EAGAIN))
      break;
    if (retry == MAX_ACCEPT_RETRY - 1)
      fcntl(sock, F_SETFL, socket_flags);          /* Try without O_NONBLOCK */
  }
  fcntl(sock, F_SETFL, socket_flags);
  if (new_sock == INVALID_SOCKET)
  {
    if ((error_count++ & 255) == 0)           /* This can happen often */
      perror("Error in accept");
    if (socket_errno == SOCKET_ENFILE || socket_errno == SOCKET_EMFILE)
      sleep(1);                               /* Do not eat CPU like a madman */
    return;
  }

  {
    size_socket dummyLen;
    struct sockaddr_storage dummy;
    dummyLen = sizeof(dummy);
    if (  getsockname(new_sock,(struct sockaddr *)&dummy, 
                (SOCKET_SIZE_TYPE *)&dummyLen) < 0  )
    {
      perror("Error on new connection socket");
      (void) shutdown(new_sock, SHUT_RDWR); /* TODO: Why doesn't postgre do shutdown here? */
      (void) closesocket(new_sock);
      return;
    }
  }

  /*
  ** Add the new connection
  */
  
  /*
    We call fcntl() to set the flags and then immediately read them back
    to make sure that we and the system are in agreement on the state of
    things.

    An example of why we need to do this is FreeBSD (and apparently some
    other BSD-derived systems, like Mac OS X), where the system sometimes
    reports that the socket is set for non-blocking when it really will
    block.
  */
  fcntl(new_sock, F_SETFL, 0);
  tailer.fcntl_mode = fcntl(new_sock, F_GETFL);
  
  add to some linked list of trailer_t
  
  add to loop to check for eof
}

static void
tailer_cb (EV_P_ ev_io *w, int revents)
{
  /* In the presence of errors the socket is assumed to be connected. */

  /*
    The first step of detecting a EOF condition is veryfing
    whether there is data to read. Data in this case would
    be the EOF.
  */
  /* This step is skipped because not mandatory and libev makes sure it's quite likely we do have data. */
//#if defined(HAVE_POLL) /* TODO: Test if not having poll really works. */
//  struct pollfd fds;
//  int res;
//  DBUG_ENTER("socket_poll_read");
//  fds.fd=sd;
//  fds.events=POLLIN;
//  fds.revents=0;
//  if ((res=poll(&fds,1,0)) <= 0)
//  {
//    if (res >= 0)
//      return; /* Errors are interpreted as if there is data to read. */
//  }
//  if (!(fds.revents & (POLLIN | POLLERR | POLLHUP))
//    return;
//#endif
  /* TODO: If this doesn't work, maybe you need less passthrough check such as in net_data_is_ready() */
  
  fcntl(sock, F_SETFL, O_NONBLOCK); /* postgre does this. where mysqld does it and, if not, why? I mean - maybe useless to try if couldn't accept with O_NONBLOCK */
  
  /*
    The second step is read() or recv() from the socket returning
    0 (EOF). Unfortunelly, it's not possible to call read directly
    as we could inadvertently read meaningful connection data.
    Simulate a read by retrieving the number of bytes available to
    read -- 0 meaning EOF.
  */
  uint len; /* TODO undefined */
#if defined(FIONREAD_IN_SYS_IOCTL) || defined(FIONREAD_IN_SYS_FILIO)
  if (ioctl(vio->sd, FIONREAD, (int)len) < 0)
    return;
  
#else
  char buf[1];
  ssize_t res= recv(vio->sd, buf, 1, MSG_PEEK); /* TODO: nginx uses buffer size 1, but mysqld uses 1024. Why? */
  if (res < 0)
    return;
  len = res;
#endif
  if (len)
    return;
  
  /* TODO: why doesn't redis check after "read()" against EAGAIN or EWOULDBLOCK as if they are impossible? */
  if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) /* TODO: Can ioctl result in these? */
    return;
  
  will I really always get EOF when a connection is closed? even when data are pending I've never read and never will?
  
  /* EOF */
  todo remove this connection
}



static void
stdin_cb (EV_P_ ev_io *w, int revents)
{
  puts ("stdin ready", stdout);
  /* TODO: Try reading it and storing in buffer (see tail.c) */
  
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
      if  socket_errno == SOCKET_EINTR || 
              ((tailer->fcntl_mode & O_NONBLOCK) &&
                (socket_errno == SOCKET_EAGAIN || socket_errno == SOCKET_EWOULDBLOCK))
        continue /* TODO: All this seems too infinite to me... */
    } /* Shouldn't I prefer use of socket_errno and SOCKET_* macros instead of errno and * macros everywhere? Or the other way around? */
    break;
  }
}

static void
handle_connections_and_stdin ()
{
  struct ev_loop *loop = ev_default_loop (EVFLAG_NOENV | EVFLAG_NOINOTIFY | EVFLAG_NOSIGMASK 
      | EVBACKEND_SELECT | EVBACKEND_POLL);
  ev_io stdin_watcher;
  
  ev_set_syserr_cb (libev_error_handler);
  
  ev_io_init (&stdin_watcher, stdin_cb, STDIN_FILENO, EV_READ);
  ev_io_start (EV_DEFAULT_ loop, &stdin_watcher);
  
  ev_io_init (&unix_sock_watcher, unix_sock_cb, unix_sock, EV_READ);
  ev_io_start (EV_DEFAULT_ loop, &unix_sock_watcher);
  
  ev_run (EV_DEFAULT_ loop, 0);
}

static int
tailserver ()
{
  file_init ();
  network_init ();
  
  handle_connections_and_stdin ();
  
  network_final ();
  file_final ();
  
  return 1; /* success */
}

int
main (int argc, char **argv)
{
  bool ok;
  int optc;
  uintmax_t n_units = DEFAULT_N_LINES;

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  atexit (close_stdout);

  count_lines = true;
  ignore_interrupts = false;

  while ((optc = getopt_long (argc, argv, "c:n:i", long_options, NULL)) != -1)
    {
      switch (optc)
        {
        case 'c':
        case 'n':
          count_lines = (c == 'n');
          
          {
            strtol_error s_err;
            s_err = xstrtoumax (optarg, NULL, 10, n_units, "bkKmMGTPEZY0");
            if (s_err != LONGINT_OK)
              {
                error (EXIT_FAILURE, 0, "%s: %s", optarg,
                       (c == 'n'
                        ? _("invalid number of lines")
                        : _("invalid number of bytes")));
              }
          }
          break;

        case 'i':
          ignore_interrupts = true;
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, "WWW");

        default:
          usage (EXIT_FAILURE);
        }
    }

  /* TODO: If not ignoring, should ev_signal_init be invoked to attempt to close handlers nicely? */ 
  if (ignore_interrupts)
    signal (SIGINT, SIG_IGN);

  ok = tailserver ();
  if (close (STDIN_FILENO) != 0)
    error (EXIT_FAILURE, errno, _("standard input"));

  exit (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}

