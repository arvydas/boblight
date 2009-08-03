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

#include "condition.h"
#include <iostream> //debug
CCondition::CCondition()
{
  pthread_cond_init(&m_cond, NULL);
}

CCondition::~CCondition()
{
  pthread_cond_destroy(&m_cond);
}

void CCondition::Send()
{
  pthread_cond_broadcast(&m_cond);
}

//should have locked before getting here
bool CCondition::Wait(int64_t usecs /*= -1*/)
{
  int result = 0;
  
  //save the refcount and owner
  int refcount = m_refcount;
  pthread_t owner = m_owner;
  //set the mutex class to an unlocked state
  m_refcount = 0;
  m_owner = NULL;

  //wait for the condition signal
  if (usecs < 0)
  {
    pthread_cond_wait(&m_cond, &m_mutex);
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

    result = pthread_cond_timedwait(&m_cond, &m_mutex, &currtime);
  }    

  //restore refcount and owner
  m_refcount = refcount;
  m_owner = owner;

  return result == 0;
}