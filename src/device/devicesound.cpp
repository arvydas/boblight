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
#include "util/sleep.h"
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
  m_started = false;
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
  const PaHostApiInfo* hostapiinfo;
  for (int i = 0; i < nrdevices; i++)
  {
    deviceinfo = Pa_GetDeviceInfo(i);
    if (deviceinfo->maxOutputChannels)
    {
      hostapiinfo = Pa_GetHostApiInfo(deviceinfo->hostApi);
      log("n:%2i channels:%3i api:%s name:%s", i, deviceinfo->maxOutputChannels, hostapiinfo->name, deviceinfo->name);
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

  PaStreamParameters outputparameters = {};
  outputparameters.channelCount       = m_channels.size();
  outputparameters.device             = devicenr;
  outputparameters.sampleFormat       = paFloat32;
  outputparameters.suggestedLatency   = deviceinfo->defaultLowOutputLatency;

  int formatsupported = Pa_IsFormatSupported(NULL, &outputparameters, m_rate);
  if (formatsupported != paFormatIsSupported)
  {
    log("%s format not supported: %s", m_name.c_str(), Pa_GetErrorText(formatsupported));
    return false;
  }

  err = Pa_OpenStream(&m_stream, NULL, &outputparameters, m_rate, m_period, paNoFlag, PaStreamCallback, (void*)this);
  if (err != paNoError)
  {
    log("%s error: %s", m_name.c_str(), Pa_GetErrorText(err));
    return false;
  }
  m_opened = true;

  err = Pa_StartStream(m_stream);
  if (err != paNoError)
  {
    log("%s error: %s", m_name.c_str(), Pa_GetErrorText(err));
    return false;
  }
  m_started = true;

  return true;
}

bool CDeviceSound::WriteOutput()
{
  USleep(1000000LL);

  return true;
}

void CDeviceSound::CloseDevice()
{
  int err;
  if (m_started)
  {
    err = Pa_StopStream(m_stream);
    if (err != paNoError)
      log("%s error: %s", m_name.c_str(), Pa_GetErrorText(err));

    m_started = false;
  }

  if (m_opened)
  {
    err = Pa_CloseStream(m_stream);
    if (err != paNoError)
      log("%s error: %s", m_name.c_str(), Pa_GetErrorText(err));

    m_opened = false;
  }

  if (m_initialized)
  {
    err = g_portaudioinit.DeInit();
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
  CDeviceSound* thisdevice = (CDeviceSound*)userdata;
  float* out = (float*)output;
  
  memset(out, 0, framecount * thisdevice->m_channels.size() * sizeof(float));

  printf("callback %i\n", framecount);

  if (thisdevice->m_stop)
    return paAbort;
  else
    return paContinue;
}

