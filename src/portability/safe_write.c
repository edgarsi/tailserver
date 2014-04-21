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

#include "safe_write.h"

#include <errno.h>


#define PANIC_LOOP_ITERATIONS 1000000 /* educated guess */

ssize_t safe_write (int fd, const char* buf, ssize_t size)
{
    ssize_t n;
    size_t safety_counter = 0;

    /* Using active blocking because EINTR (for all) and EAGAIN (for pipe fds at least) may always happen. */
    /* Plus, blockability of some fds may not be controllable, such as pipes shared with other processes. */
    do {
        if (++safety_counter > PANIC_LOOP_ITERATIONS) {
            break;
        }

        n = write(fd, buf, size);
        
    } while (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK));

    if (safety_counter > PANIC_LOOP_ITERATIONS) {
        perror(_("System is actively blocking a write attempt"));
    }

    return n;
}

