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

#ifndef CLIENTPIPES_H
#define CLIENTPIPES_H 1

#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>

bool clientpipes_write_blocking (const char* buf, ssize_t size);
bool clientpipes_broken (); /* if any write fails, assume stdout is broken */

void clientpipes_init ();
void clientpipes_final ();


#endif /* CLIENTPIPES_H */
