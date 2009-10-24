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

#ifndef CDEVICESOUND
#define CDEVICESOUND

#include <vector>
#include <portaudio.h>
#include "device.h"
#include "util/condition.h"

class CDeviceSound : public CDevice
{
  public:
    CDeviceSound(CClientsHandler& clients);

    void SetLatency(float latency) { m_latency = latency; }
    void SetPeriod(int period)     { m_period = period; }
 
    bool SetupDevice();
    bool WriteOutput();
    void CloseDevice();

  private:
    PaStream*  m_stream;

    bool       m_opened;
    bool       m_initialized;
    bool       m_started;
    CCondition m_callbacksignal;

    double     m_latency;
    int        m_period;
 
    int        m_pwmphase;
    int        m_pwmperiod;
    int        m_pwmcount;

    std::vector<int16_t> m_outputvalues;

    static int PaStreamCallback(const void *input, void *output, unsigned long framecount,
			        const PaStreamCallbackTimeInfo* timeinfo, PaStreamCallbackFlags statusflags,
				void *userdata);
    void       FillOutput(int16_t* out, unsigned long framecount);
};

#endif //CDEVICESOUND
