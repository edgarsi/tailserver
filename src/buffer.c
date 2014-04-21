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
   Note that since MySQL does not include the "or any later version" clause, 
   therefore the acting license of tailserver is forced to be GPL2 by the magic of law.
   
*/

#include "buffer.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "intmax.h"
#include "memcpy.h"
#include "offsetof.h"


#define debug(...)
/*#define debug(x) fputs(x, stderr)*/
#define fdebugf(...)
/*#define fdebugf fprintf*/


/* Number of items to tail. */
static uintmax_t n_units;
static bool n_units_configured;

/* If true, interpret the numeric argument as the number of lines.
   Otherwise, interpret it as the number of bytes.  */
static bool count_lines;
static bool count_lines_configured;


typedef struct linebuffer_t
{
    char buffer[BUFSIZ];
    size_t nbytes;
    size_t nlines;
    struct linebuffer_t* next;
} linebuffer_t;

static linebuffer_t* first;
static linebuffer_t* last;
static linebuffer_t* append; /* Empty buffer for new inputs */ 
static size_t total_lines;
static size_t total_bytes;

static linebuffer_t* tail_buffer;
static size_t tail_offset;
static size_t lines_up_to_tail_buffer;
static size_t bytes_up_to_tail_buffer;


void buffer_config_count_lines (bool p_count_lines)
{
    count_lines = p_count_lines;
    count_lines_configured = true;
}

void buffer_config_n_units (uintmax_t p_n_units)
{
    n_units = p_n_units;
    n_units_configured = true;
}

/* TODO: Count the unfinished line which does not end with a newline */
static void keep_track_of_tail_lines (uintmax_t n_lines)
{
    /* First, skip over unneeded buffers.  */
    while (total_lines - lines_up_to_tail_buffer - tail_buffer->nlines > n_lines) {
        lines_up_to_tail_buffer += tail_buffer->nlines;
        tail_buffer = tail_buffer->next;
    }

    /* Find the correct beginning.  */
    /* The overhead or redoing this in every call is acceptable because it's only high when data input is slow. */
    {
        char const *beg = tail_buffer->buffer;
        char const *buffer_end = tail_buffer->buffer + tail_buffer->nbytes;
        if (total_lines - lines_up_to_tail_buffer > n_lines) {
            /* Skip 'total_lines' - 'lines_up_to_tail_buffer' - 'n_lines' newlines.  We made sure that
             'total_lines' - 'lines_up_to_tail_buffer' - 'n_lines' <= 'tail_buffer->nlines'.  */
            size_t j;
            for (j = total_lines - lines_up_to_tail_buffer - n_lines; j; --j) {
                beg = memchr(beg, '\n', buffer_end - beg);
                assert(beg);
                ++beg;
            }
        }
        tail_offset = beg - tail_buffer->buffer;
    }
}

static void keep_track_of_tail_bytes (uintmax_t n_bytes)
{
    /* First, skip over unneeded buffers.  */
    fdebugf(stderr, "%d - %d - %d > %d\n", total_bytes, bytes_up_to_tail_buffer, tail_buffer->nbytes, n_bytes);
    while (total_bytes - bytes_up_to_tail_buffer - tail_buffer->nbytes > n_bytes) {
        bytes_up_to_tail_buffer += tail_buffer->nbytes;
        tail_buffer = tail_buffer->next;
        fdebugf(stderr, "%d - %d - %d > %d\n", total_bytes, bytes_up_to_tail_buffer, tail_buffer->nbytes, n_bytes);
    }
    fdebugf(stderr, "using the buffer %llu\n", tail_buffer);

    /* Find the correct beginning. 
     * We made sure that 'total_bytes' - 'bytes_up_to_tail_buffer' - 'n_bytes' <= 'tail_buffer->nbytes'.  */
    if (total_bytes - bytes_up_to_tail_buffer > n_bytes) {
        tail_offset = total_bytes - bytes_up_to_tail_buffer - n_bytes;
    } else {
        tail_offset = 0;
    }
    
    fdebugf(stderr, "tail at %d + %d, total bytes %d\n", bytes_up_to_tail_buffer, tail_offset, total_bytes);
}

static void keep_track_of_tail ()
{
    if (count_lines) {
        keep_track_of_tail_lines(n_units);
    } else {
        keep_track_of_tail_bytes(n_units);
    }

    /* Don't allow zero sized chunks unless buffers are empty (or zero units tailed, for weird reasons).
     * Allow the user to detect the end of buffer by a zero chunk size. */
    if (tail_offset == tail_buffer->nbytes && tail_buffer->next) {
        bytes_up_to_tail_buffer += tail_buffer->nbytes;
        lines_up_to_tail_buffer += tail_buffer->nlines;
        tail_buffer = tail_buffer->next;
        tail_offset = 0;
        
        fdebugf(stderr, "tail shifted to %d + %d, total bytes %d\n", bytes_up_to_tail_buffer, tail_offset, total_bytes);
    }
}

char* buffer_end ()
{
    return append->buffer;
}

size_t buffer_available_for_append ()
{
    return sizeof(append->buffer);
}

void buffer_set_appended (size_t size)
{
    if (size == 0) {
        return;
    }

    debug("data being appended to buffer\n");
    fdebugf(stderr, "size: %d\n", size);

    /* Input is always read into a fresh buffer.  */
    append->nbytes = size;
    total_bytes += append->nbytes;

    /* Count the number of newlines just read.  */
    if (count_lines) {
        char const *buffer_end = append->buffer + append->nbytes;
        char const *p = append->buffer;
        while ((p = memchr(p, '\n', buffer_end - p))) {
            ++p;
            ++append->nlines;
        }
        total_lines += append->nlines;
    }

    /* If there is enough room in the last buffer read, just append the new
     one to it.  This is because when reading from a pipe, 'n_read' can
     often be very small.  */
    if (append->nbytes + last->nbytes < BUFSIZ) { /* TODO: What's wrong with "<="? */
        memcpy(&last->buffer[last->nbytes], append->buffer, append->nbytes);
        last->nbytes += append->nbytes;
        last->nlines += append->nlines;
        keep_track_of_tail();
    } else {
        debug("new buffer\n");
        /* If there's not enough room, link the new buffer onto the end of
         the list, then either free up the oldest buffer for the next
         read if that would leave enough lines, or else malloc a new one.
         Some compaction mechanism is possible but probably not
         worthwhile.  */
        last = last->next = append;
        keep_track_of_tail();
        bool first_buffer_droppable = 
                (count_lines
                        ? total_lines - first->nlines > n_units
                        : total_bytes - first->nbytes > n_units);
        if (first_buffer_droppable) {
            debug("dropping first\n");
            assert(first != tail_buffer);
            fdebugf(stderr, "first buffer has %d bytes, resides at %llu and will be given to append, taking next at %llu\n", 
                    first->nbytes, first, first->next);
            append = first;
            total_bytes -= first->nbytes;
            total_lines -= first->nlines;
            bytes_up_to_tail_buffer -= first->nbytes;
            lines_up_to_tail_buffer -= first->nlines;
            first = first->next;
        } else {
            /* TODO: Handle errors! We might be in a RAM jail. */
            debug("malloc\n");
            append = malloc(sizeof(linebuffer_t));
        }
        append->next = NULL;
        fdebugf(stderr, "first %llu (%llu) tail %llu (%llu) last %llu (%llu) append %llu (%llu)\n", 
                first, first->next, tail_buffer, tail_buffer->next, last, last->next, append, append->next);
    }
    append->nbytes = append->nlines = 0;
    debug("append ended fine\n");
}

size_t buffer_size ()
{
    return total_bytes;
}

const char* buffer_get_tail_chunk ()
{
    fdebugf(stderr, "tailing from buffer %llu\n", tail_buffer);
    return tail_buffer->buffer;
}

size_t buffer_get_tail_offset ()
{
    return tail_offset;
}

size_t buffer_chunk_size (const char* chunk)
{
    if (chunk) {
        const linebuffer_t* tmp = (const linebuffer_t*)(chunk - offsetof(linebuffer_t, buffer));
        return tmp->nbytes;
    } else {
        return 0;
    }
}

const char* buffer_advance_chunk (const char* chunk)
{
    if (chunk) {
        const linebuffer_t* tmp = (const linebuffer_t*)(chunk - offsetof(linebuffer_t, buffer));
        if (tmp->next) {
            fdebugf(stderr, "advancing tail to %llu\n", tmp->next);
            return tmp->next->buffer;
        } else {
            fdebugf(stderr, "tail has ended\n");
            return NULL;
        }
    } else {
        return NULL;
    }
}


void buffer_init()
{
    if (!n_units_configured) {
        n_units = DEFAULT_N_LINES;
    }
    if (!count_lines_configured) {
        count_lines = true;
    }

    /* TODO: Search for all malloc calls... */
    first = last = tail_buffer = malloc(sizeof(linebuffer_t));
    first->nbytes = first->nlines = 0;
    first->next = NULL;

    append = malloc(sizeof(linebuffer_t));
    append->nbytes = first->nlines = 0;
    append->next = NULL;
}

void buffer_final()
{
    debug("buffer finalizing\n");
    while (first) {
        linebuffer_t* tmp = first->next;
        free(first);
        first = tmp;
    }
    free(append);
}

