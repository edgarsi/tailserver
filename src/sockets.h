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

#ifndef SOCKETS_H
#define SOCKETS_H 1

#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>

void sockets_config_file (const char* file_path);
void sockets_config_tailer_block_forever (bool block);

void sockets_write_blocking (char* buf, ssize_t size);

bool sockets_init ();
void sockets_final ();


#endif /* SOCKETS_H */
