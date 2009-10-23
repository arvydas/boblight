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

#include <unistd.h>
#include <stdint.h>

inline void USleep(int64_t usecs, volatile bool* stop = NULL)
{
  if (usecs <= 0)
    return;

  //usleep() can only sleep up to 999999 microseconds

  int count     = usecs / 1000000;
  int remainder = usecs % 1000000;

  for (int i = 0; i < count; i++)
  {
    sleep(1); 

    if (stop)
    {
      if (*stop) return;
    }
  }

  usleep(remainder);
}

#endif //USLEEP

