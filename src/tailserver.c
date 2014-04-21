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
   Note that since MySQL does not include the "or any later version" clause, the license of tailserver 
   is forced to be GPL2 by the magic of law.
   
*/

#include "config.h"

#include "buffer.h"
#include "pipes.h"
#include "sockets.h"

#include <stdbool.h>
#include <getopt.h>
#include <limits.h>
#include <stdlib.h>
#include "intmax.h"
#include "xstrtol.h"
#include "error.h"

/* Eludes me why everyone (see mysql, postgre, ngircd, redis) tries to implement their own socket event handling */
#include <ev.h>

#define PROGRAM_NAME "tailserver"
const char *program_name = PROGRAM_NAME;

#include "system.h"

static bool ignore_interrupts;
static ev_signal sigint_watcher;
/*static ev_signal sigpipe_watcher;*/

/* TODO: Option to stall reading input until either a single connection or timeout occurs 
   -w, --wait[=T] postpone reading input until a connection occurs or timeout (in seconds), if specified
   TODO: Option to choose either block or not block (dropping data seems stupid, closing connection OK) when some socket 
   can't keep up.  .... third option - buffer all in memory (but position tracking complicates; is it worth it?)
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
Copy stdin to stdout, and also serve it to UNIX domain socket connections.\n\
Give new SOCKET_FILE connections the last %d lines arrived to stdin so far.\n\
Continue giving all new lines that arrive (just like 'tail -f' does).\n\
"), DEFAULT_N_LINES);

        emit_mandatory_arg_note ();

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
        fputs(_("\n\
SOCKET_FILE will be rewritten if exists (in near future, this behaviour \n\
will change).\n"), stdout);
        fputs(_("\
\n\
K (the number of bytes or lines) may have a multiplier suffix:\n\
b 512, kB 1000, K 1024, MB 1000*1000, M 1024*1024,\n\
GB 1000*1000*1000, G 1024*1024*1024, and so on for T, P, E, Z, Y.\n\
"), stdout);
        emit_ancillary_info();
    }
    exit(status);
}

static void server_init ();
static void server_final ();

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

static void libev_error_handler (const char* msg)
{
    perror(msg);
    final_all();
    exit(EXIT_FAILURE);
}

static void sigint_cb (EV_P_ ev_signal* w ATTRIBUTE_UNUSED, int revents ATTRIBUTE_UNUSED)
{
    ev_break(EV_A_ EVBREAK_ALL);
}

/*
static void sigpipe_cb (EV_P_ ev_signal* w ATTRIBUTE_UNUSED, int revents ATTRIBUTE_UNUSED)
{
    fputs("sigpipe_cb\n", stderr);
    ev_break(EV_A_ EVBREAK_ALL);
}
*/

static void server_init ()
{
    ev_default_loop(
            EVFLAG_NOENV | EVFLAG_NOINOTIFY | EVFLAG_NOSIGMASK
            | EVBACKEND_SELECT | EVBACKEND_POLL);

    ev_set_syserr_cb(libev_error_handler);

    /* Either ignore SIGINT or exit gracefully, depending on user's choice */
    if (ignore_interrupts) {
        signal(SIGINT, SIG_IGN);
    } else {
        ev_signal_init(&sigint_watcher, sigint_cb, SIGINT);
        ev_signal_start(EV_DEFAULT_UC_ &sigint_watcher);
    }

    /* SIGPIPE is called when write on any fd fails with EPIPE, even sockets. We can't have that... */
    /* So, let the program stay alive until an attempt to write to the broken STDOUT is made. */
    signal(SIGPIPE, SIG_IGN);
}

static void server_run ()
{
    ev_run (EV_DEFAULT_UC_ 0);
}

static void server_final ()
{
    /* Nothing to do */
}

int main (int argc, char** argv)
{
    bool ok;
    int optc;

    /* Configure */

    atexit(close_stdout);

    bool count_lines = true;
    uintmax_t n_units = DEFAULT_N_LINES;
    ignore_interrupts = false;

    while ((optc = getopt_long(argc, argv, "c:n:i", long_options, NULL)) != -1) {
        switch (optc)
        {
        case 'c':
        case 'n':
            count_lines = (optc == 'n');

            {
                strtol_error s_err;
                s_err = xstrtoumax(optarg, NULL, 10, &n_units, "bkKmMGTPEZY0");
                if (s_err != LONGINT_OK) {
                    error(EXIT_FAILURE, 0, "%s: %s", optarg,
                            (optc == 'n' ?
                                    _("invalid number of lines") :
                                    _("invalid number of bytes")));
                }
            }
            break;

        case 'i':
            ignore_interrupts = true;
            break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR(NULL, "<" PACKAGE_URL ">");

        default:
            usage(EXIT_FAILURE);
        }
    }

    {
        const char* file_path;
        if (argc - optind > 1) {
            error(EXIT_FAILURE, 0, "%s", _("Multiple file paths given!"));
        }
        else
        if (argc - optind == 1) {
            file_path = argv[optind];
        } else {
            file_path = NULL;
            fputs(_("File path not given - tailserver will act like 'cat' command. \
Call with --help for options.\n"), stderr);
        }

        buffer_config_count_lines(count_lines);
        buffer_config_n_units(n_units);
        sockets_config_file(file_path);

        /* Launch */

        init_all();

        server_run();

        final_all();
    }

    exit(EXIT_SUCCESS);
}

