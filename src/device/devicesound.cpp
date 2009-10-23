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

#include "util/log.h"
#include "util/misc.h"
#include "util/mutex.h"
#include "util/lock.h"
#include "devicesound.h"

using namespace std;

class CPortAudioInit
{
  public:
    int Init()
    {
      CLock lock(m_mutex);
      return Pa_Initialize();
    }

    int DeInit()
    {
      CLock lock(m_mutex);
      return Pa_Terminate();
    }

  private:
    CMutex m_mutex;
}
g_portaudioinit;

CDeviceSound::CDeviceSound(CClientsHandler& clients) : CDevice(clients)
{
  m_opened = false;
  m_initialized = false;
}

bool CDeviceSound::SetupDevice()
{
  int err = g_portaudioinit.Init();
  if (err != paNoError)
  {
    log("%s error: %s", m_name.c_str(), Pa_GetErrorText(err));
    return false;
  }
  m_initialized = true;

  int nrdevices = Pa_GetDeviceCount();
  if (nrdevices < 0)
  {
    log("%s error: %s",m_name.c_str(), Pa_GetErrorText(nrdevices));
    return false;
  }
  else if (nrdevices == 0)
  {
    log("%s no portaudio devices found", m_name.c_str());
    return false;
  }

  log("%s found %i portaudio devices", m_name.c_str(), nrdevices);

  const PaDeviceInfo *deviceinfo;
  for (int i = 0; i < nrdevices; i++)
  {
    deviceinfo = Pa_GetDeviceInfo(i);
    if (deviceinfo->maxOutputChannels)
    {
      log("Device %2i: channels:%3i name:%s", i, deviceinfo->maxOutputChannels, deviceinfo->name);
    }
  }

  int devicenr = -1;
  for (int i = 0; i < nrdevices; i++)
  {
    deviceinfo = Pa_GetDeviceInfo(i);
    if (m_output == string(deviceinfo->name))
    {
      devicenr = i;
      break;
    }
  }
  
  if (devicenr == -1)
  {
    log("%s device %s not found", m_name.c_str(), m_output.c_str());
    return false;
  }
  else if (deviceinfo->maxOutputChannels < 1)
  {
    log("%s device %s has no channels", m_name.c_str(), m_output.c_str());
    return false;
  }
  else
  {
    log("%s using device %i", m_name.c_str(), devicenr);
  }

  return false;
}

bool CDeviceSound::WriteOutput()
{
  return false;
}

void CDeviceSound::CloseDevice()
{
  if(m_initialized)
  {
    int err = g_portaudioinit.DeInit();
    if (err != paNoError)
      log("%s error: %s", m_name.c_str(), Pa_GetErrorText(err));

    m_initialized = false;
  }
    
  return;
}


int CDeviceSound::PaStreamCallback(const void *input, void *output, unsigned long framecount,
				   const PaStreamCallbackTimeInfo* timeinfo, PaStreamCallbackFlags statusflags,
				   void *userdata) 
{
  float* out = (float*)output;
  
  CDeviceSound* thisdevice = (CDeviceSound*)userdata;
  return 0;
}

