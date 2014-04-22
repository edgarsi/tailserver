/* a wrapper for frepoen
   Copyright (C) 2008-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef XFREOPEN_H
#define XFREOPEN_H 1

#include <stdio.h>
#include <stdbool.h>

bool xfreopen (char const *filename, char const *mode, FILE *fp);


#endif /* XFREOPEN_H */
