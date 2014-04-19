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

#include "config.h"

#if defined(HAVE_FCNTL_NONBLOCK) || defined(HAVE_FCNTL_NDELAY)
#  if defined(HAVE_FCNTL_H)
#    include <fcntl.h>
#  endif
#  if defined(HAVE_UNISTD_H)
#    include <unistd.h>
#  endif
/* If a system has neither O_NONBLOCK nor NDELAY, upgrade is recommended... but not required. */
#  if !defined(O_NONBLOCK)
#    define O_NONBLOCK O_NDELAY
#    define HAVE_FCNTL_NONBLOCK HAVE_FCNTL_NDELAY
#  endif
#endif

