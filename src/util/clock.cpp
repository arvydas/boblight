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

#include <stdlib.h>
#include <time.h>
#include "clock.h"
#include "misc.h"

CClock::CClock()
{
  m_freq = 1000000000;
}

int64_t CClock::GetFreq()
{
  return m_freq;
}

int64_t CClock::GetTime()
{
  struct timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);

  return ((int64_t)time.tv_sec * 1000000000) + (int64_t)time.tv_nsec;
}

long double CClock::GetSecTime()
{
  int64_t time = GetTime();

  return (long double)time / (long double)m_freq;
}