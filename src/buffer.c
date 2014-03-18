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

#include "buffer.h"


/* Number of items to tail. */
static uintmax_t n_units;

/* If true, interpret the numeric argument as the number of lines.
   Otherwise, interpret it as the number of bytes.  */
static bool count_lines;


void buffer_config_count_lines(bool p_count_lines)
{
    count_lines = p_count_lines;
}

void buffer_config_n_units(uintmax_t p_n_units)
{
    n_units = p_n_units;
}

void buffer_init()
{
    n_units = DEFAULT_N_LINES;
    count_lines = true;
}

void buffer_final()
{
    release memory
}

