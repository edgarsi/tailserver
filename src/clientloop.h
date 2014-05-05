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

#ifndef CLIENTLOOP_H
#define CLIENTLOOP_H 1

#include <stdbool.h>

void clientloop_config_follow (bool follow);

void clientloop_run (const char* file_path, bool wait_file_at_startup, bool reopen_file, double sleep_seconds);

void clientloop_init ();
void clientloop_final ();

#endif /* CLIENTLOOP_H */


