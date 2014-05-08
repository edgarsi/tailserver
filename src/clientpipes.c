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

#include "clientpipes.h"

#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <stdlib.h>

#include "safe_write.h"
#include "stdout.h"

#define debug(...)
/*#define debug(x) {fputs(x, stderr);}*/
#define fdebugf(...)
/*#define fdebugf fprintf*/

static bool broken;

bool clientpipes_write_blocking (const char* buf, ssize_t size)
{
    if (stdout_write_blocking(buf, size)) {
        return true;
    } else {
        broken = true;
        return false;
    }
}

bool clientpipes_broken ()
{
    return broken;
}

void clientpipes_init ()
{
    setvbuf(stdout, NULL, _IONBF, 0);
}

void clientpipes_final ()
{
    if (close(STDIN_FILENO) != 0) {
        error(EXIT_FAILURE, errno, _("standard input"));
    }
}

