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

#include <time.h>
#include "mutex.h"

CMutex::CMutex()
{
  pthread_mutex_init(&m_mutex, NULL);
  m_owner = NULL;
  m_refcount = 0;
}

CMutex::~CMutex()
{
  pthread_mutex_destroy(&m_mutex);
}

void CMutex::Unlock()
{
  if (pthread_equal(m_owner, pthread_self()))
  {
    if (m_refcount > 0)
    {
      m_refcount--;
    }
    else
    {
      m_owner = NULL;
      pthread_mutex_unlock(&m_mutex);
    }
  }
}

bool CMutex::TryLock()
{
  pthread_t thread = pthread_self();
  
  if (pthread_equal(m_owner, thread))
  {
    m_refcount++;
    return true;
  }
  else
  {
    if (pthread_mutex_trylock(&m_mutex) == 0)
    {
      m_owner = thread;
      return true;
    }
    else
    {
      return false;
    }
  }      
}

bool CMutex::Lock(int64_t usecs /*= -1*/)
{
  pthread_t thread = pthread_self();
  
  if (pthread_equal(m_owner, thread))
  {
    m_refcount++;
    return true;
  }
  else if (usecs < 0)
  {
    pthread_mutex_lock(&m_mutex);
    m_owner = thread;
    return true;
  }
  else
  {
    //get the current time
    struct timespec currtime;
    clock_gettime(CLOCK_REALTIME, &currtime);

    //add the number of microseconds
    currtime.tv_sec += usecs / 1000000;
    currtime.tv_nsec += (usecs % 1000000) * 1000;
    
    if (currtime.tv_nsec >= 1000000000)
    {
      currtime.tv_sec += currtime.tv_nsec / 1000000000;
      currtime.tv_nsec %= 1000000000;
    }

    //try to lock the mutex
    if (pthread_mutex_timedlock(&m_mutex, &currtime) == 0)
    {
      m_owner = thread;
      return true;
    }
    else
    {
      return false;
    }
  }
}

bool CMutex::OwnLock()
{
  return pthread_equal(m_owner, pthread_self());
}