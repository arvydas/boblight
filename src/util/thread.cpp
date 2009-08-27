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

#include "thread.h"
#include "lock.h"
#include "misc.h"

CThread::CThread()
{
  m_thread = NULL;
}

CThread::~CThread()
{
  StopThread();
}

void CThread::StartThread()
{
  m_stop = false;
  pthread_create(&m_thread, NULL, ThreadFunction, reinterpret_cast<void*>(this));
}

void* CThread::ThreadFunction(void* args)
{
  CThread* thread = reinterpret_cast<CThread*>(args);
  thread->Thread();
}

void CThread::Thread()
{
  Process();
}

void CThread::Process()
{
}

void CThread::StopThread()
{
  AsyncStopThread();
  if (m_thread)
  {
    pthread_join(m_thread, NULL);
    m_thread = NULL;
  }
}

void CThread::AsyncStopThread()
{
  m_stop = true;
  m_sleepcondition.Broadcast();
}

void CThread::JoinThread()
{
  if (m_thread)
  {
    pthread_join(m_thread, NULL);
    m_thread = NULL;
  }
}

void CThread::sleep(int secs)
{
  usleep(secs * 1000000);
}

//when sleeping for longer than 1 second, wait on a condition variable so we can stop quickly
void CThread::usleep(int usecs)
{
  if (m_stop)
    return;
  
  if (usecs < 1000000)
  {
    ::usleep(usecs);
  }
  else
  {
    CLock lock(m_sleepcondition);
    m_sleepcondition.Wait(usecs);
  }
}