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
   Socket handling from mysql client.c (GPL2=)
   Note that since MySQL does not include the "or any later version" clause, the license of tailserver 
   is forced to be GPL2 by the magic of law.
   
*/

#include "config.h"

#include "buffer.h"
#include "clientpipes.h"
#include "clientloop.h"

#include <stdbool.h>
#include <getopt.h>
#include <limits.h>
#include <stdlib.h>
#include "intmax.h"
#include "xstrtol.h"
#include "error.h"
#include "xnanosleep.h"
#include "xstrtod.h"

/* Eludes me why everyone (see mysql, postgre, ngircd, redis) tries to implement their own socket event handling */
#include <ev.h>

#define debug(...)
/*#define debug(x) {fputs(x, stderr);}*/
#define fdebugf(...)
/*#define fdebugf fprintf*/

#define PROGRAM_NAME "tailclient"
const char *program_name = PROGRAM_NAME;

/* Default number of items to tail. */
const unsigned int DEFAULT_N_LINES = 10;

#include "system.h"

static ev_signal sigint_watcher;

/* For long options that have no equivalent short option, use a
   non-character as a pseudo short option, starting with CHAR_MAX + 1.  */
enum
{
  LONG_FOLLOW_OPTION = CHAR_MAX + 1
};

static struct option const long_options[] =
{
    {"bytes", required_argument, NULL, 'c'},
    {"lines", required_argument, NULL, 'n'},
    {"follow", optional_argument, NULL, LONG_FOLLOW_OPTION},
    {"wait", no_argument, NULL, 'w'},
    {"sleep-interval", required_argument, NULL, 's'},
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
Connect to the SOCKET_FILE which is created by tailserver.\n\
Output the last %u lines offered by tailserver.\n\
"), DEFAULT_N_LINES);

        emit_mandatory_arg_note ();

        fputs(_("\
  -c, --bytes=K             output the last K bytes\n\
"), stdout);
        printf(_("\
  -n, --lines=K             output the last K lines, instead of the last %u\n\
"), DEFAULT_N_LINES);
        fputs(_("\
  -w, --wait                if SOCKET_FILE is inaccessible at startup,\n\
                            wait until otherwise\n\
"), stdout);
        fputs (_("\
  -f, --follow              output new data that appear\n\
"), stdout);
        fputs (_("\
  -F                        same as --follow but whenever SOCKET_FILE is or\n\
                            becomes inaccessible at any point in time,\n\
                            wait until otherwise\n\
"), stdout);
        fputs (_("\
  -s, --sleep-interval=N    with -w, -f, or -F, sleep for approximately\n\
                            N seconds (default 1.0) between tries to access\n\
                            SOCKET_FILE\n\
"), stdout);
        fputs(HELP_OPTION_DESCRIPTION, stdout);
        fputs(VERSION_OPTION_DESCRIPTION, stdout);
        fputs(_("\
\n\
K (the number of bytes or lines) may have a multiplier suffix:\n\
b 512, kB 1000, K 1024, MB 1000*1000, M 1024*1024,\n\
GB 1000*1000*1000, G 1024*1024*1024, and so on for T, P, E, Z, Y.\n\
"), stdout);
        fputs(_("\
\n\
The amount of last data output is limited by how much tailserver buffers (see \n\
'man tailserver' for -c and -n).\n\
"), stdout);
        
        emit_ancillary_info();
    }
    exit(status);
}

static void final_all ();

static void libev_error_handler (const char* msg)
{
    perror(msg);
    final_all();
    exit(EXIT_FAILURE);
}

static void sigint_cb (EV_P_ ev_signal* w ATTRIBUTE_UNUSED, int revents ATTRIBUTE_UNUSED)
{
    debug("sigint\n");
    final_all();
    exit(EXIT_SUCCESS);
}

static void event_loop_init ()
{
    ev_default_loop(
            EVFLAG_NOENV | EVFLAG_NOINOTIFY | EVFLAG_NOSIGMASK
            | EVBACKEND_SELECT | EVBACKEND_POLL);

    ev_set_syserr_cb(libev_error_handler);

    /* Exit gracefully on SIGINT */
    ev_signal_init(&sigint_watcher, sigint_cb, SIGINT);
    ev_signal_start(EV_DEFAULT_UC_ &sigint_watcher);

    /* SIGPIPE is called when write on any fd fails with EPIPE, even sockets. We can't have that... */
    /* So, let the program stay alive until an attempt to write to the broken STDOUT is made. */
    signal(SIGPIPE, SIG_IGN);
}

static void event_loop_final ()
{
    /* Nothing */
}

static void init_all ()
{
    if ( ! buffer_init()) {
        exit(EXIT_FAILURE);
    }
    event_loop_init();
    clientpipes_init();
    clientloop_init();
}

static void final_all ()
{
    clientloop_final();
    clientpipes_final();
    event_loop_final();
    buffer_final();
}

int main (int argc, char** argv)
{
    bool ok;
    int optc;

    /* Configure */

    atexit(close_stdout);

    bool count_lines = true;
    uintmax_t n_units = DEFAULT_N_LINES;
    bool follow = false;
    bool wait_file_at_startup = false;
    bool reopen_file = false;
    double sleep_seconds = 1.0;

    while ((optc = getopt_long(argc, argv, "c:n:fFws:", long_options, NULL)) != -1) {
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

        case 'f':
        case LONG_FOLLOW_OPTION:
            follow = true;
            break;

        case 'F':
            follow = true;
            reopen_file = true;
            break;

        case 'w':
            wait_file_at_startup = true;
            break;

        case 's':
            {
                double s;
                if (!(xstrtod(optarg, NULL, &s) && 0 <= s))
                    error(EXIT_FAILURE, 0, _("%s: invalid number of seconds"), optarg);
                sleep_seconds = s;
            }
            break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR(program_name, "<" PACKAGE_URL ">");

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
            error(EXIT_FAILURE, 0, _("File path not given. Call with --help for options."));
        }

        buffer_config_count_lines(count_lines);
        buffer_config_n_units(n_units);
        clientloop_config_follow(follow);

        /* Launch */

        init_all();

        clientloop_run(file_path, wait_file_at_startup, reopen_file, sleep_seconds);

        final_all();
    }

    exit(EXIT_SUCCESS);
}

