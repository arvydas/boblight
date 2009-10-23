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

#include "asynctimer.h"
#include "misc.h"
#include "lock.h"
#include "sleep.h"

CAsyncTimer::~CAsyncTimer()
{
  StopTimer();
}

void CAsyncTimer::StartTimer(int64_t usecs /*= -1*/)
{
  if (usecs > 0) 
    m_interval = usecs;
  
  StartThread();
}

void CAsyncTimer::Process()
{
  int64_t target = m_clock.GetTime();
  int64_t now;
  int64_t sleeptime;

  while(!m_stop)
  {
    target += m_interval;

    now = m_clock.GetTime();
    sleeptime = target - now;
    if (sleeptime > m_interval * 2) //failsafe
    {
      sleeptime = m_interval * 2;
      target = now;
    }
    USleep(sleeptime, &m_stop);

    m_signal.Broadcast();
  }
}

void CAsyncTimer::Wait(volatile bool* stop /*= NULL*/)
{
  int64_t waittime  = m_interval * 2;
  int     count     = waittime / 1000000LL;
  int     remainder = waittime % 1000000LL;

  for (int i = 0; i < count; i++)
  {
    CLock lock(m_signal);
    if (m_signal.Wait(1000000))
      return;

    if (stop)
    {
      if (*stop) return;
    }
  }

  CLock lock(m_signal);
  m_signal.Wait(remainder);
}

void CAsyncTimer::StopTimer()
{
  StopThread();
}
