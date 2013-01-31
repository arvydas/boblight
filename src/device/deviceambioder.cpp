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

#define AMBIODER_PWM_PERIOD 255
/*
support for Ambioder device: http://gabriel-lg.github.com/Ambioder/
protocol:
commands are sent as 6/8-byte sequences:
hex: [0x0X 0x1X] 0x2X 0x3X 0x4X 0x5X 0x6X 0x7X
the least significant nibbles are combined to form (in order):
pwm period, red, green blue.
*/

static const uint8_t ambioder_off[] = {0x0F, 0x1F, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70};

CDeviceAmbioder::CDeviceAmbioder(CClientsHandler& clients) : CDeviceRS232(clients)
{
  m_buff = new uint8_t[8];
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
  memcpy(m_buff, ambioder_off, 8);
  if (m_serialport.Write(m_buff, 8) == -1)
  {
    LogError("%s: %s", m_name.c_str(), m_serialport.GetError().c_str());
    return false;
  }

  return true;
}

bool CDeviceAmbioder::WriteOutput()
{
  //get the channel values from the clienshandler
  int64_t now = GetTimeUs();
  m_clients.FillChannels(m_channels, now, this);

  //create the command sequence
  if(m_channels.size() >= 3)
  {
	uint8_t period = AMBIODER_PWM_PERIOD;
	uint8_t red = Clamp(Round32(m_channels[0].GetValue(now) * AMBIODER_PWM_PERIOD), 0, AMBIODER_PWM_PERIOD);
	uint8_t green = Clamp(Round32(m_channels[1].GetValue(now) * AMBIODER_PWM_PERIOD), 0, AMBIODER_PWM_PERIOD);
	uint8_t blue = Clamp(Round32(m_channels[2].GetValue(now) * AMBIODER_PWM_PERIOD), 0, AMBIODER_PWM_PERIOD);
	m_buff[0] = 0x00 | ((period >> 4) & 0x0F);
	m_buff[1] = 0x10 | (period & 0x0F);
	m_buff[2] = 0x20 | ((red >> 4) & 0x0F);
	m_buff[3] = 0x30 | (red & 0x0F);
	m_buff[4] = 0x40 | ((green >> 4) & 0x0F);
	m_buff[5] = 0x50 | (green & 0x0F);
	m_buff[6] = 0x60 | ((blue >> 4) &0x0F);
	m_buff[7] = 0x70 | (blue & 0x0F);
  }
  else
  {
	  memcpy(m_buff, ambioder_off, 8);
  }
  //write output to the serial port
  if (m_serialport.Write(m_buff, 8) == -1)
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
  memcpy(m_buff, ambioder_off, 8);
  if (m_serialport.Write(m_buff, 8) == -1)
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
