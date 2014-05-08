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
   
*/

#include "config.h"

#include "stdout.h"

#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <stdlib.h>

#include "safe_write.h"

#define debug(...)
/*#define debug(x) {fputs(x, stderr);}*/
#define fdebugf(...)
/*#define fdebugf fprintf*/

bool stdout_write_blocking (const char* buf, ssize_t size)
{
    ssize_t n;

    n = safe_write_forever(STDOUT_FILENO, buf, size);

    if (n < size) {
        if (n == 0) {
            /* Sometimes stdout is lost but there is no EPIPE */
        } else
        if (n < 0 && errno == EPIPE){
            /* The pipe of stdout is lost. Let the program end gracefully. */
        } else {
            perror(_("Writing to stdout encountered an error"));
        }
        debug("stdout broken\n");
        return false;
    }

    return true;
}

