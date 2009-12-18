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

#include "signaltimer.h"
#include "lock.h"

CSignalTimer::CSignalTimer(volatile bool* stop /*= NULL*/): CTimer(stop)
{
  m_signaled = false;
}

void CSignalTimer::Wait()
{
  int64_t sleeptime;

  if (m_timerstop)
    if (*m_timerstop)
      return;

  CLock lock(m_condition);

  //keep looping until we have a timestamp that's not too old
  do
  {
    m_time += m_interval;
    sleeptime = m_time - m_clock.GetTime();
  }
  while(sleeptime <= m_interval * -2LL);
  
  if (sleeptime > m_interval * 2LL) //failsafe, m_time must be bork if we get here
  {
    sleeptime = m_interval * 2LL;
    Reset();
  }

  if (m_signaled)
    Reset();
  else
    WaitCondition(sleeptime);

  m_signaled = false;
}

void CSignalTimer::Signal()
{
  CLock lock(m_condition);
  m_signaled = true;
  m_condition.Broadcast();
}


void CSignalTimer::WaitCondition(int64_t sleeptime)
{
  int64_t secs  = sleeptime / 1000000LL;
  int64_t usecs = sleeptime % 1000000LL;

  for (int i = 0; i < secs; i++)
  {
    if (m_condition.Wait(1000000LL) || m_signaled)
    {
      Reset();
      return;
    }
    
    if (m_timerstop)
      if (*m_timerstop)
        return;
  }

  if (m_condition.Wait(usecs) || m_signaled)
    Reset();
}
