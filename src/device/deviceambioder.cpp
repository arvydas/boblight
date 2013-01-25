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

#include "deviceambioder.h"
#include "util/log.h"
#include "util/misc.h"
#include "util/timeutils.h"


/*
support for Ambioder device: http://gabriel-lg.github.com/Ambioder/
protocol:
commands are sent as 4-byte sequences:
binary: 00pppppp 01rrrrrr 10gggggg 11bbbbbb
*/

CDeviceAmbioder::CDeviceAmbioder(CClientsHandler& clients) : CDeviceRS232(clients)
{
  m_buff = new uint8_t[4];
}

bool CDeviceAmbioder::SetupDevice()
{
  m_timer.SetInterval(m_interval);

  if (!OpenSerialPort())
    return false;

  m_serialport.PrintToStdOut(m_debug); //print serial data to stdout when debug mode is on

  if (m_delayafteropen > 0)
    USleep(m_delayafteropen, &m_stop);

  //turn off the lights
  m_buff[0] = 0x00;
  m_buff[1] = 0x40;
  m_buff[2] = 0x80;
  m_buff[3] = 0xC0;
  if (m_serialport.Write(m_buff, 4) == -1)
  {
    LogError("%s: %s", m_name.c_str(), m_serialport.GetError().c_str());
    return false;
  }

  return true;
}

bool CDeviceAmbioder::OpenSerialPort()
{
  bool opened = m_serialport.Open(m_output, m_rate, 8, 2); //FIXME workaround for sync issue when uart transmitter stays saturated
  if (m_serialport.HasError())
  {
    LogError("%s: %s", m_name.c_str(), m_serialport.GetError().c_str());
    if (opened)
      Log("%s: %s had a non fatal error, it might still work, continuing", m_name.c_str(), m_output.c_str());
  }

  m_serialport.PrintToStdOut(m_debug); //print serial data to stdout when debug mode is on

  return opened;
}

bool CDeviceAmbioder::WriteOutput()
{
  //get the channel values from the clienshandler
  int64_t now = GetTimeUs();
  m_clients.FillChannels(m_channels, now, this);

  //create the command sequence
  m_buff[0] = 0x3F; //63 step pwm loop
  if(m_channels.size() > 0)
  {
    m_buff[1] = Clamp(Round32(m_channels[0].GetValue(now) * 63.0f), 0, 63) | 0x40;
  }
  else
  {
    m_buff[1] = 0x40;
  }
  if(m_channels.size() > 1)
  {
    m_buff[2] = Clamp(Round32(m_channels[1].GetValue(now) * 63.0f), 0, 63) | 0x80;
  }
  else
  {
    m_buff[2] = 0x80;
  }

  if(m_channels.size() > 2)
  {
    m_buff[3] = Clamp(Round32(m_channels[2].GetValue(now) * 63.0f), 0, 63) | 0xC0;
  }
  else
  {
    m_buff[3] = 0xC0;
  }
  //write output to the serial port
  if (m_serialport.Write(m_buff, 4) == -1)
  {
    LogError("%s: %s", m_name.c_str(), m_serialport.GetError().c_str());
    return false;
  }
    
  m_timer.Wait(); //wait for the timer to signal us

  return true;
}

void CDeviceAmbioder::CloseDevice()
{
  //turn off the lights
  m_buff[0] = 0x00;
  m_buff[1] = 0x40;
  m_buff[2] = 0x80;
  m_buff[3] = 0xC0;
  if (m_serialport.Write(m_buff, 4) == -1)
  {
    LogError("%s: %s", m_name.c_str(), m_serialport.GetError().c_str());
  }
  m_serialport.Write(m_buff, 4);

  m_serialport.Close();
}

CDeviceAmbioder::~CDeviceAmbioder()
{
    delete m_buff;
}
