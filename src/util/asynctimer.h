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

#ifndef ASYNCTIMER
#define ASYNCTIMER

#include "timer.h"
#include "thread.h"
#include "condition.h"

class CAsyncTimer : public CThread, public CTimer
{
  public:
    ~CAsyncTimer();
    void StartTimer(int64_t usecs = -1);
    void Wait();
    void StopTimer();

  private:
    void Process();
    CCondition m_signal;
};

#endif //ASYNCTIMER