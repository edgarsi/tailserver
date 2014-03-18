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
#include <sys/socket.h>*/ 

/* Eludes me why everyone (see mysql, postgre, ngircd, redis) tries to implement their own socket event handling */
#include <ev.h>

#include "provider.h"
#include "pipes.h"

#define PROGRAM_NAME "tailserver"

static bool ignore_interrupts;

/* TODO: Option to stall reading input until either a single connection or timeout occurs 
   -w, --wait[=T] postpone reading input until a connection occurs or timeout (in seconds), if specified
   TODO: Option to choose either block or not block when some socket can't keep up. */
*/
static struct option const long_options[] =
{
    {"bytes", required_argument, NULL, 'c'},
    {"lines", required_argument, NULL, 'n'},
    {"ignore-interrupts", no_argument, NULL, 'i'},
    {GETOPT_HELP_OPTION_DECL},
    {GETOPT_VERSION_OPTION_DECL},
    {NULL, 0, NULL, 0}
};

static void usage (int status)
{
    if (status != EXIT_SUCCESS) {
        emit_try_help();
    }
    else {
        printf(_("Usage: %s [OPTION]... <SOCKET_FILE>\n"), program_name);
        printf(_("\
Copy standard input to to standard output, and also serve it to UNIX domain socket connections.\n\
Give new connections to SOCKET_FILE the last %d lines. Output new lines as they arrive at standard input.\n\
\n"), DEFAULT_N_LINES);
        fputs(_("\
   -c, --bytes=K             output the last K bytes\n\
"), stdout);
        printf(_("\
   -n, --lines=K             output the last K lines, instead of the last %d;\n\
"), DEFAULT_N_LINES);
        fputs(_("\
   -i, --ignore-interrupts   ignore interrupt signals\n\
"), stdout);
        fputs(HELP_OPTION_DESCRIPTION, stdout);
        fputs(VERSION_OPTION_DESCRIPTION, stdout);
        fputs(_("\
\n\
K (the number of bytes or lines) may have a multiplier suffix:\n\
b 512, kB 1000, K 1024, MB 1000*1000, M 1024*1024,\n\
GB 1000*1000*1000, G 1024*1024*1024, and so on for T, P, E, Z, Y.\n\
\n\
"), stdout);
        emit_ancillary_info();
    }
    exit(status);
}

static void init_all ()
{
    server_init();
    buffer_init();
    pipes_init();
    if ( ! sockets_init()) {
        pipes_final();
        buffer_final();
        server_final();
        exit(EXIT_FAILURE);
    }
}

static void final_all ()
{
    sockets_final();
    pipes_final();
    buffer_final();
    server_final();
}

static void libev_error_handler (const char *msg)
{
    perror(msg);
    final_all();
    exit(EXIT_FAILURE);
}

static void sigint_cb (EV_P_ ev_signal *w, int revents)
{
    ev_break(EV_A_ EVBREAK_ALL);
}

static void server_init ()
{
    ev_default_loop(
            EVFLAG_NOENV | EVFLAG_NOINOTIFY | EVFLAG_NOSIGMASK
            | EVBACKEND_SELECT | EVBACKEND_POLL);

    ev_set_syserr_cb(libev_error_handler);

    /* Either ignore SIGINT or exit gracefully, depending on user's choice */
    ev_signal signal_watcher;
    if (ignore_interrupts) {
        signal(SIGINT, SIG_IGN);
    } else {
        ev_signal_init(&signal_watcher, sigint_cb, SIGINT);
        ev_io_start(EV_DEFAULT_UC_ &signal_watcher);
    }
}

static void server_run ()
{
    ev_run (EV_DEFAULT_UC_ 0);
}

static void server_final ()
{
    /* Nothing to do */
}

int main (int argc, char **argv)
{
    bool ok;
    int optc;

    initialize_main(&argc, &argv);
    set_program_name(argv[0]);
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    atexit(close_stdout);

    bool count_lines = true;
    uintmax_t n_units = DEFAULT_N_LINES;
    ignore_interrupts = false;

    while ((optc = getopt_long(argc, argv, "c:n:i", long_options, NULL)) != -1) {
        switch (optc)
        {
        case 'c':
        case 'n':
            count_lines = (c == 'n');

            {
                strtol_error s_err;
                s_err = xstrtoumax(optarg, NULL, DEFAULT_N_LINES, n_units, "bkKmMGTPEZY0");
                if (s_err != LONGINT_OK) {
                    error(EXIT_FAILURE, 0, "%s: %s", optarg,
                            (c == 'n' ?
                                    _("invalid number of lines") :
                                    _("invalid number of bytes")));
                }
            }
            break;

        case 'i':
            ignore_interrupts = true;
            break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR(PROGRAM_NAME, "WWW");

        default:
            usage(EXIT_FAILURE);
        }
    }

    init_all();
    buffer_config_count_lines(count_lines);
    buffer_config_n_units(n_units);

    server_run();

    final_all();

    exit(EXIT_SUCCESS);
}

