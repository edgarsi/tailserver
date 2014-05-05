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

#ifndef CLIENTCONN_H
#define CLIENTCONN_H 1

#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>

/* clientconn understands the protocol but does not make communication decisions */

bool clientconn_connect (const char* file_path, bool wait_for_file, double sleep_seconds);
void clientconn_disconnect (); /* always safe to call */

int clientconn_socket (); /* only when connected */

/* Server buffer is what we can read without any real blocking */
/* While buffer is not emptied, _follow() returns 0. After that, _buffer() returns 0. No magic. */
ssize_t clientconn_read_server_buffer (char* buf, ssize_t size);
ssize_t clientconn_read_server_follow (char* buf, ssize_t size);
bool clientconn_is_all_server_buffer_read ();
bool clientconn_eof ();

/* Any data written causes server to stop send after-buffer data */
ssize_t clientconn_write (const char* buf, ssize_t size);


#endif /* CLIENTCONN_H */
