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

#include "devicepopen.h"
#include "util/log.h"
#include "util/misc.h"

#include <iostream>

using namespace std;

CDevicePopen::CDevicePopen (CClientsHandler& clients, CAsyncTimer& timer) : m_timer(timer), CDevice(clients)
{
  m_process = NULL;
}

bool CDevicePopen::SetupDevice()
{
  m_process = popen(m_output.c_str(), "w");
  if (m_process == NULL)
  {
    log("%s: %s", m_output.c_str(), GetErrno().c_str());
    return false;
  }
  return true;
}

bool CDevicePopen::WriteOutput()
{
  int64_t now = m_clock.GetTime();
  m_clients.FillChannels(m_channels, now);

  for (int i = 0; i < m_channels.size(); i++)
  {
    if (fprintf(m_process, "%f ", m_channels[i].GetValue(now)) != 1)
    {
      log("%s %s: %s", m_name.c_str(), m_output.c_str(), GetErrno().c_str());
      return false;
    }
  }
  if (fprintf(m_process, "\n") == -1 || fflush(m_process) != 0)
  {
    log("%s %s: %s", m_name.c_str(), m_output.c_str(), GetErrno().c_str());
    return false;
  }
  
  m_timer.Wait();

  return true;
}

void CDevicePopen::CloseDevice()
{
  if (m_process)
  {
    pclose(m_process);
    m_process = NULL;
  }
}