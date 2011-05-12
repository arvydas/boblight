/*
 * boblight
 * Copyright (C) Bob  2009 
 * 
 * boblight is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * boblight is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef USLEEP
#define USLEEP

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <unistd.h>
#include "timeutils.h"

inline void USleep(int64_t usecs, volatile bool* stop = NULL)
{
  if (usecs <= 0)
  {
    return;
  }
  else if (usecs > 1000000) //when sleeping for more than one second, keep checking stop every second
  {
    int64_t now = GetTimeUs();
    int64_t end = now + usecs;
    while (now < end)
    {
      if (stop && *stop)
        return;
      else if (end - now >= 1000000)
        sleep(1);
      else
        usleep(end - now);

      now = GetTimeUs();
    }
  }
  else
  {
    usleep(usecs);
  }
}

#endif //USLEEP

