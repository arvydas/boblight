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

#ifndef CDEVICE
#define CDEVICE

#define NOTHING 0
#define MOMO    1
#define ATMO    2
#define POPEN   3
#define LTBL    4
#define SOUND   5

#include <string>
#include <vector>
#include <stdint.h>

//#include "client.h"
#include "util/thread.h"
#include "util/clock.h"
#include "util/timer.h"

class CClientsHandler; //forward declaration

class CChannel
{
  public:
    CChannel();
    void SetLight(int light)             { m_light = light; }
    int  GetLight()                      { return m_light;  }
    void SetColor(int color)             { m_color = color; }
    int  GetColor()                      { return m_color;  }
    
    void SetValue(float value)           { m_wantedvalue = value; }
    void SetFallback(float fallback)     { m_fallback = fallback; }
    void SetValueToFallback()            { m_wantedvalue = m_fallback; }
    void SetSpeed(float speed)           { m_speed = speed; }
    void SetGamma(float gamma)           { m_gamma = gamma; }
    void SetAdjust(float adjust)         { m_adjust = adjust; }
    void SetBlacklevel(float blacklevel) { m_blacklevel = blacklevel; }

    float GetValue(int64_t time);

  private:
    int   m_color;
    int   m_light;
    
    float m_speed;
    float m_wantedvalue;
    float m_fallback;
    float m_gamma;
    float m_adjust;
    float m_blacklevel;

    float   m_currentvalue;
    int64_t m_lastupdate; //when m_currentvalue was last updated

    CClock  m_clock;
};

class CDevice : public CThread
{
  public:
    CDevice(CClientsHandler& clients);
    
    void SetName(std::string name)     { m_name = name; }
    void SetOutput(std::string output) { m_output = output; }
    void SetBuffer(int buffer)         { m_buffer = buffer; }
    void SetPeriod(int period)         { m_period = period; }
    virtual void SetType(int type)     { m_type = type; }
    void SetRate(int rate)             { m_rate = rate; }
    void SetNrChannels(int nrchannels) { m_channels.resize(nrchannels); }
    void SetChannel(CChannel& channel, int channelnr) { m_channels[channelnr] = channel; }
    void SetPrefix(std::vector<unsigned char> prefix) { m_prefix = prefix; }

    int GetNrChannels()   { return m_channels.size(); }
      
    std::string GetName() { return m_name; }

  protected:
    void Process();
    virtual bool SetupDevice() { return false; }
    virtual bool WriteOutput() { return false; }
    virtual void CloseDevice() { return; }

    std::string m_name;
    std::string m_output;
    int         m_buffer;
    int         m_period;
    int         m_type;
    int         m_rate;

    CClock      m_clock;

    std::vector<CChannel> m_channels;
    std::vector<unsigned char> m_prefix;

    CClientsHandler& m_clients;
};

#include "client.h"

#endif //CDEVICE