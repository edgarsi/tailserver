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

#ifndef BUFFER_H
#define BUFFER_H 1

#include <stdbool.h>
#include <stddef.h>
#include "intmax.h"

/* Default number of items to tail. */
extern const unsigned int DEFAULT_N_LINES;


/* Configure */
void buffer_config_count_lines (bool count_lines);
void buffer_config_n_units (uintmax_t n_units);

/* Write */
char* buffer_end ();
size_t buffer_available_for_append ();
bool buffer_set_appended (size_t size); /* Invalidates all pointers */

/* Read */
size_t buffer_tail_size ();
const char* buffer_get_tail_chunk ();
size_t buffer_get_tail_offset ();
size_t buffer_chunk_size (const char* chunk); /* Can only read small(!) size chunks at a time. Returns 0 at end (only). */
const char* buffer_advance_chunk (const char* chunk); /* Offset always zero after the first chunk. Returns NULL at end. */


bool buffer_init ();
void buffer_final ();


#endif /* BUFFER_H */
