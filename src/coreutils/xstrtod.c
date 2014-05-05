/* error-checking interface to strtod-like functions

   Copyright (C) 1996, 1999-2000, 2003-2006, 2009-2013 Free Software
   Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Jim Meyering.  */

#include "config.h"

#include "xstrtod.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* An interface to a string-to-floating-point conversion function that
   encapsulates all the error checking one should usually perform.
   Like strtod, but upon successful
   conversion put the result in *RESULT and return true.  Return
   false and don't modify *RESULT upon any failure. */

bool
xstrtod (char const *str, char const **ptr, double *result)
{
  double val;
  char *terminator;
  bool ok = true;

  errno = 0;
  val = strtod (str, &terminator);

  /* Having a non-zero terminator is an error only when PTR is NULL. */
  if (terminator == str || (ptr == NULL && *terminator != '\0'))
    ok = false;
  else
    {
      /* Allow underflow (in which case CONVERT returns zero),
         but flag overflow as an error. */
      if (val <= 0.0 && errno == ERANGE)
        ok = false;
    }

  if (ptr != NULL)
    *ptr = terminator;

  *result = val;
  return ok;
}
