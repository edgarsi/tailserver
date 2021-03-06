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

#include "config.h"

#include "xfreopen.h"

#include <errno.h>
#include "error.h"
#include "stdio--.h"
#include <stdlib.h>

bool
xfreopen (char const *filename, char const *mode, FILE *fp)
{
  if (!freopen (filename, mode, fp))
    {
      char const *f = (filename ? filename
                       : (fp == stdin ? _("stdin")
                          : (fp == stdout ? _("stdout")
                             : (fp == stderr ? _("stderr")
                                : _("unknown stream")))));
      error (0, errno, _("failed to reopen %s with mode %s"),
             f, mode);
      return false;
   }
  return true;
}
