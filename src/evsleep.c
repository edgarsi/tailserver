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

#include "evsleep.h"

static void sleeper_timeout_cb (EV_P_ ev_timer *w)
{
    ev_break(EV_A_ EVBREAK_ALL);
}

void evsleep (EV_P_ double seconds)
{
    ev_timer sleeper;
    ev_timer_init(&sleeper, sleeper_timeout_cb, seconds, 0.0);
    ev_timer_start(EV_A_ &sleeper);

    ev_run(EV_A_ 0);

    ev_timer_stop(EV_A_ &sleeper);
}
