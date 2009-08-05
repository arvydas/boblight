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

#include "deviceltbl.h"
#include "util/log.h"
#include "util/misc.h"

CDeviceLtbl::CDeviceLtbl(CClientsHandler& clients, CAsyncTimer& timer) : CDeviceRS232(clients, timer)
{
  m_buff = NULL;
  m_isopened = false;
}

bool CDeviceLtbl::SetupDevice()
{
  if (!m_serialport.Open(m_output, m_rate))
  {
    log("%s %s", m_name.c_str(), m_serialport.GetError().c_str());
    return false;
  }

  if (OpenController())
  {
    m_buff = new unsigned char[m_channels.size()];
    return true;
  }

  return false;
}

bool CDeviceLtbl::WriteOutput()
{
  unsigned char prefix[4] = {0x55, 0xAA, 0x00, m_channels.size()};

  int64_t now = m_clock.GetTime();
  m_clients.FillChannels(m_channels, now);

  bool isused = false;
  for (int i = 0; i < m_channels.size(); i++)
  {
    if (m_channels[i].IsUsed())
      isused = true;
    
    int value = Clamp(Round<int>(m_channels[i].GetValue(now) * 65535.0f), 0, 65535);

    m_buff[i * 2 + 0] = (unsigned char)((value >> 8) & 0xFF);
    m_buff[i * 2 + 1] = (unsigned char)((value >> 0) & 0xFF);
  }

  if (isused)
  {
    if (!m_isopened)
    {
      if (!OpenController())
        return false;
    }
    
    if (m_serialport.Write(prefix, 4) == -1 || m_serialport.Write(m_buff, m_channels.size() * 2) == -1)
    {
      log("%s %s", m_name.c_str(), m_serialport.GetError().c_str());
      return false;
    }
  }
  else if (m_isopened)
  {
    if (!CloseController())
      return false;
  }
    
  m_timer.Wait();

  return true;
}

void CDeviceLtbl::CloseDevice()
{
  CloseController();

  if (m_buff)
  {
    delete m_buff;
    m_buff = NULL;
  }

  m_serialport.Close();
}

bool CDeviceLtbl::OpenController()
{
  unsigned char buff[512];
  unsigned char prefix[2]    = {0x55, 0xAA};
  unsigned char open[2]      = {0x83, 0x00};
  unsigned char getvalues[4] = {0x81, 0x02, 0x00, m_channels.size()};

  if (m_isopened)
    return true;
  
  memset(buff, 0x55, 255);
  buff[255] = 0xFF;
  buff[256] = 0xFF;
  buff[257] = 0xFF;

  if (m_serialport.Write(buff, 258) == -1 || m_serialport.Write(prefix, 2) == -1 || m_serialport.Write(open, 2) == -1)
  {
    log("%s %s", m_name.c_str(), m_serialport.GetError().c_str());
    return false;
  }

  if (m_serialport.Write(prefix, 2) == -1 || m_serialport.Write(getvalues, 4) == -1)
  {
    log("%s %s", m_name.c_str(), m_serialport.GetError().c_str());
    return false;
  }

  if (!WaitForPrefix())
    return false;
  
  if (m_serialport.Read(buff, 2, 1000000) == -1)
  {
    log("%s %s", m_name.c_str(), m_serialport.GetError().c_str());
    return false;
  }

  int startchannel = buff[0];
  int nrchannels = buff[1];

  if (startchannel >= (int)m_channels.size() || startchannel < 0 || nrchannels <= 0 || nrchannels > (int)m_channels.size())
  {
    log("%s %s: sent gibberish", m_name.c_str(), m_output.c_str());
    return false;
  }    

  if (m_serialport.Read(buff, nrchannels * 2, 1000000) == -1)
  {
    log("%s %s", m_name.c_str(), m_serialport.GetError().c_str());
    return false;
  }

  for (int i = startchannel; i < nrchannels; i++)
  {
    int value = (int)(buff[i * 2]) << 8;
    value |= (int)(buff[i * 2 + 1]);

    m_channels[i].SetFallback(Clamp((float)value / 65535.0f, 0.0f, 1.0f));
  }

  m_isopened = true;
  log("controller on %s opened", m_output.c_str());
  
  return true;
}

bool CDeviceLtbl::CloseController()
{
  unsigned char prefix[2] = {0x55, 0xAA};
  unsigned char close[2]  = {0x84, 0x00};

  if (!m_isopened)
    return true;
  
  if (m_serialport.Write(prefix, 2) == -1 || m_serialport.Write(close, 2) == -1)
  {
    log("%s %s", m_name.c_str(), m_serialport.GetError().c_str());
    return false;
  }

  m_isopened = false;

  log("controller on %s closed", m_output.c_str());
  
  return true;
}

bool CDeviceLtbl::WaitForPrefix()
{
  unsigned char prefix[2] = {0, 0};

  while(1)
  {
    prefix[0] = prefix[1];
    if (m_serialport.Read(prefix + 1, 1, 1000000) == -1)
    {
      log("%s %s", m_name.c_str(), m_serialport.GetError().c_str());
      return false;
    }

    if (prefix[0] == 0x55 && prefix[1] == 0xAA)
      return true;
  }
}
