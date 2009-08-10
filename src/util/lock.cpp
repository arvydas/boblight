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

#include "lock.h"

CLock::CLock(CMutex& mutex) : m_mutex(mutex)
{
  m_haslock = false;
  Enter();
}

CLock::CLock(CCondition& condition) : m_mutex(condition)
{
  m_haslock = false;
  Enter();
}

CLock::~CLock()
{
  Leave();
}

void CLock::Enter()
{
  if (!m_haslock)
  {
    m_mutex.Lock();
    m_haslock = true;
  }
}

void CLock::Leave()
{
  if (m_haslock)
  {
    m_mutex.Unlock();
    m_haslock = false;
  }
}