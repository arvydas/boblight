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

#include <iostream>
#include <fstream>

#include "util/log.h"
#include "util/misc.h"
#include "configuration.h"

#include "device/devicepopen.h"
#include "device/deviceltbl.h"
#include "device/devicers232.h"
#include "device/devicesound.h"

using namespace std;

#define SECTNOTHING 0
#define SECTGLOBAL  1
#define SECTDEVICE  2
#define SECTCOLOR   3
#define SECTLIGHT   4

bool CConfig::LoadConfigFromFile(string file)
{
  int linenr = 0;
  int currentsection = SECTNOTHING;
  char buff[100000];

  m_filename = file;
  
  log("opening %s", file.c_str());

  ifstream configfile(file.c_str());
  if (configfile.fail())
  {
    log("%s: %s", file.c_str(), GetErrno().c_str());
    return false;
  }
  
  while (!configfile.eof())
  {
    configfile.getline(buff, sizeof(buff));
    if (configfile.fail())
    {
      log("%s: %s", file.c_str(), GetErrno().c_str());
      return false;
    }
    linenr++;
    string line = buff;
    string key;
    if (!GetWord(line, key))
      continue;

    //ignore comments
    if (key.substr(0, 1) == "#")
      continue;
    
    if (key == "[global]")
    {
      currentsection = SECTGLOBAL;
      continue;
    }
    else if (key == "[device]")
    {
      currentsection = SECTDEVICE;
      m_devicelines.resize(m_devicelines.size() + 1);
      continue;
    }
    else if (key == "[color]")
    {
      currentsection = SECTCOLOR;
      m_colorlines.resize(m_colorlines.size() + 1);
      continue;
    }
    else if (key == "[light]")
    {
      currentsection = SECTLIGHT;
      m_lightlines.resize(m_lightlines.size() + 1);
      continue;
    }
    
    if (currentsection == SECTNOTHING)
      continue;

    CConfigLine configline(buff, linenr);

    if (currentsection == SECTGLOBAL)
    {
      m_globalconfiglines.push_back(configline);
    }
    else if (currentsection == SECTDEVICE)
    {
      m_devicelines.back().lines.push_back(configline);
    }
    else if (currentsection == SECTCOLOR)
    {
      m_colorlines.back().lines.push_back(configline);
    }
    else if (currentsection == SECTLIGHT)
    {
      m_lightlines.back().lines.push_back(configline);
    }
  }

  //PrintConfig();

  return true;
}

void CConfig::PrintConfig()
{
  log("[global]");

  for (int i = 0; i < m_globalconfiglines.size(); i++)
  {
    //replace any tabs with spaces
    string line = m_globalconfiglines[i].line;
    TabsToSpaces(line);

    //print the line to the log
    log("%i %s", m_globalconfiglines[i].linenr, line.c_str());
  }

  log("%i devices", m_devicelines.size());
  for (int i = 0; i < m_devicelines.size(); i++)
  {
    log("[device] %i", i);
    for (int j = 0; j < m_devicelines[i].lines.size(); j++)
    {
      string line = m_devicelines[i].lines[j].line;
      TabsToSpaces(line);
      log("%i %s", m_devicelines[i].lines[j].linenr, line.c_str());
    }
  }

  log("%i colors", m_colorlines.size());
  for (int i = 0; i < m_colorlines.size(); i++)
  {
    log("[color] %i", i);
    for (int j = 0; j < m_colorlines[i].lines.size(); j++)
    {
      string line = m_colorlines[i].lines[j].line;
      TabsToSpaces(line);
      log("%i %s", m_colorlines[i].lines[j].linenr, line.c_str());
    }
  }

  log("%i lights", m_lightlines.size());
  for (int i = 0; i < m_lightlines.size(); i++)
  {
    log("[light] %i", i);
    for (int j = 0; j < m_lightlines[i].lines.size(); j++)
    {
      string line = m_lightlines[i].lines[j].line;
      TabsToSpaces(line);
      log("%i %s", m_lightlines[i].lines[j].linenr, line.c_str());
    }
  }
}

void CConfig::TabsToSpaces(std::string& line)
{
  size_t pos = line.find('\t');
  while (pos != string::npos)
  {
    line.replace(pos, 1, 1, ' ');
    pos++;
    if (pos >= line.size()) break;
    pos = line.find('\t', pos);
  }
}

bool CConfig::CheckConfig()
{
  bool valid = true;
  log("checking config lines");

  if (!CheckGlobalConfig())
    valid = false;

  if (!CheckDeviceConfig())
    valid = false;

  if (!CheckColorConfig())
    valid = false;

  if (!CheckLightConfig())
    valid = false;
  
  if (valid)
    log("config lines valid");
  
  return valid;
}

bool CConfig::CheckGlobalConfig()
{
  bool valid = true;
  
  for (int i = 0; i < m_globalconfiglines.size(); i++)
  {
    string line = m_globalconfiglines[i].line;
    string key;
    string value;

    GetWord(line, key);

    if (!GetWord(line, value))
    {
      log("%s error on line %i: no value for key %s", m_filename.c_str(), m_globalconfiglines[i].linenr, key.c_str());
      valid = false;
      continue;
    }

    if (key == "interface")
    {
      continue; //not much to check here
    }
    else if (key == "port")
    {
      int port;
      if (!StrToInt(value, port) || port < 0 || port > 65535)
      {
        log("%s error on line %i: wrong value %s for key %s", m_filename.c_str(), m_globalconfiglines[i].linenr, value.c_str(), key.c_str());
        valid = false;
      }
    }
    else
    {
      log("%s error on line %i: unknown key %s", m_filename.c_str(), m_globalconfiglines[i].linenr, key.c_str());
      valid = false;
    }
  }

  return valid;
}

bool CConfig::CheckDeviceConfig()
{
  bool valid = true;

  for (int i = 0; i < m_devicelines.size(); i++)
  {
    for (int j = 0; j < m_devicelines[i].lines.size(); j++)
    {
      string line = m_devicelines[i].lines[j].line;
      int linenr = m_devicelines[i].lines[j].linenr;
      string key;
      string value;

      GetWord(line, key);

      if (!GetWord(line, value))
      {
        log("%s error on line %i: no value for key %s", m_filename.c_str(), linenr, key.c_str());
        valid = false;
        continue;
      }

      if (key == "name" || key == "output" || key == "type")
      {
        continue; //can't check these here
      }
      else if (key == "rate" || key == "channels" || key == "interval" || key == "period" || key == "buffer")
      {
        int ivalue;
        if (!StrToInt(value, ivalue) || ivalue < 1)
        {
          log("%s error on line %i: wrong value %s for key %s", m_filename.c_str(), linenr, value.c_str(), key.c_str());
          valid = false;
        }          
      }
      else if (key == "prefix")
      {
        int ivalue;
        while(1)
        {
          if (!HexStrToInt(value, ivalue) || (ivalue > 0xFF))
          {
            log("%s error on line %i: wrong value %s for key %s", m_filename.c_str(), linenr, value.c_str(), key.c_str());
            valid = false;
          }
          if (!GetWord(line, value))
            break;
        }
      }
      else
      {
        log("%s error on line %i: unknown key %s", m_filename.c_str(), linenr, key.c_str());
        valid = false;
      }      
    }
  }

  return valid;
}

bool CConfig::CheckColorConfig()
{
  bool valid = true;

  for (int i = 0; i < m_colorlines.size(); i++)
  {
    for (int j = 0; j < m_colorlines[i].lines.size(); j++)
    {
      string line = m_colorlines[i].lines[j].line;
      int linenr = m_colorlines[i].lines[j].linenr;
      string key;
      string value;

      GetWord(line, key);

      if (!GetWord(line, value))
      {
        log("%s error on line %i: no value for key %s", m_filename.c_str(), linenr, key.c_str());
        valid = false;
        continue;
      }

      if (key == "name")
      {
        continue;
      }
      else if (key == "gamma" || key == "adjust" || key == "blacklevel")
      {
        float fvalue;
        if (!StrToFloat(value, fvalue) || fvalue < 0.0 || (key != "gamma" && fvalue > 1.0))
        {
          log("%s error on line %i: wrong value %s for key %s", m_filename.c_str(), linenr, value.c_str(), key.c_str());
          valid = false;
        }
      }
      else if (key == "rgb")
      {
        int rgb;
        if (!HexStrToInt(value, rgb) || (rgb & 0xFF000000))
        {
          log("%s error on line %i: wrong value %s for key %s", m_filename.c_str(), linenr, value.c_str(), key.c_str());
          valid = false;
        }
      }
      else
      {
        log("%s error on line %i: unknown key %s", m_filename.c_str(), linenr, key.c_str());
        valid = false;
      }      
    }
  }

  return valid;
}

bool CConfig::CheckLightConfig()
{
  bool valid = true;

  for (int i = 0; i < m_lightlines.size(); i++)
  {
    for (int j = 0; j < m_lightlines[i].lines.size(); j++)
    {
      string line = m_lightlines[i].lines[j].line;
      int linenr = m_lightlines[i].lines[j].linenr;
      string key;
      string value;

      GetWord(line, key);

      if (!GetWord(line, value))
      {
        log("%s error on line %i: no value for key %s", m_filename.c_str(), linenr, key.c_str());
        valid = false;
        continue;
      }

      if (key == "name")
      {
        continue;
      }
      else if (key == "area")
      {
        int x, y, nrpoints = 0;
        do
        {
          if (sscanf(value.c_str(), "%i.%i", &x, &y) != 2 || x < 1 || x > 1000 || y < 1 || y > 1000)
          {
            log("%s error on line %i: wrong value %s for key %s", m_filename.c_str(), linenr, value.c_str(), key.c_str());
            valid = false;
          }
          nrpoints++;
        }
        while (GetWord(line, value));
        if (nrpoints < 3)
        {
          log("%s error on line %i: key %s requires at least 3 points", m_filename.c_str(), linenr, key.c_str());
          valid = false;
        }
      }
      else if (key == "color")
      {
        int k;
        for (k = 0; k < 2; k++)
        {
          if (!GetWord(line, value))
          {
            log("%s error on line %i: not enough values for key %s", m_filename.c_str(), linenr, key.c_str());
            valid = false;
            break;
          }
        }
        if (k == 2)
        {
          int channel;
          if (!StrToInt(value, channel) || channel <= 0)
          {
            log("%s error on line %i: wrong value %s for key %s", m_filename.c_str(), linenr, value.c_str(), key.c_str());
            valid = false;
          }
        }
      }
      else
      {
        log("%s error on line %i: unknown key %s", m_filename.c_str(), linenr, key.c_str());
        valid = false;
      }      
    }
  }

  return valid;
}

int CConfig::GetLineWithKey(std::string key, std::vector<CConfigLine>& lines, std::string& line)
{
  for (int i = 0; i < lines.size(); i++)
  {
    string word;
    line = lines[i].line;
    GetWord(line, word);

    if (word == key)
    {
      return lines[i].linenr;
    }
  }

  return -1;
}

bool CConfig::BuildConfig(CConnectionHandler& connectionhandler, CClientsHandler& clients, std::vector<CDevice*>& devices,
                 std::vector<CAsyncTimer>& timers, std::vector<CLight>& lights)
{
  log("building config");
  
  BuildConnectionHandlerConfig(connectionhandler);

  vector<CColor> colors;
  if (!BuildColorConfig(colors))
    return false;

  BuildTimers(timers);
  
  if (!BuildDeviceConfig(devices, timers, clients))
    return false;

  if (!BuildLightConfig(lights, devices, colors))
    return false;
  
  log("built config successfully");
}

void CConfig::BuildConnectionHandlerConfig(CConnectionHandler& connectionhandler)
{
  //set up where to bind the listening socket
  //config for this should already be valid here, of course we can't check yet if the interface actually exists
  string interface;
  int port;
  for (int i = 0; i < m_globalconfiglines.size(); i++)
  {
    string line = m_globalconfiglines[i].line;
    string word;
    GetWord(line, word);

    if (word == "interface")
    {
      GetWord(line, interface);
    }
    else if (word == "port")
    {
      GetWord(line, word);
      StrToInt(word, port);
    }
  }
  connectionhandler.SetInterface(interface, port);
}

bool CConfig::BuildColorConfig(std::vector<CColor>& colors)
{
  for (int i = 0; i < m_colorlines.size(); i++)
  {
    CColor color;
    for (int j = 0; j < m_colorlines[i].lines.size(); j++)
    {
      string line = m_colorlines[i].lines[j].line;
      string key, value;

      GetWord(line, key);
      GetWord(line, value);

      if (key == "name")
      {
        color.SetName(value);
      }
      else if (key == "rgb")
      {
        int irgb;
        float frgb[3];
        HexStrToInt(value, irgb);

        for (int k = 0; k < 3; k++)
          frgb[k] = (float)((irgb >> ((2 - k) * 8)) & 0xFF) / 255.0;

        color.SetRgb(frgb);
      }
      else if (key == "gamma")
      {
        float gamma;
        ConvertFloatLocale(value);
        StrToFloat(value, gamma);
        color.SetGamma(gamma);
      }
      else if (key == "adjust")
      {
        float adjust;
        ConvertFloatLocale(value);
        StrToFloat(value, adjust);
        color.SetAdjust(adjust);
      }
      else if (key == "blacklevel")
      {
        float blacklevel;
        ConvertFloatLocale(value);
        StrToFloat(value, blacklevel);
        color.SetBlacklevel(blacklevel);
      }
    }

    if (color.GetName().empty())
    {
      log("%s error: color %i has no name", m_filename.c_str(), i + 1);
      return false;
    }

    colors.push_back(color);
  }

  return true;
}

void CConfig::BuildTimers(std::vector<CAsyncTimer>& timers)
{
  for (int i = 0; i < m_devicelines.size(); i++)
  {
    string line;

    int linenr = GetLineWithKey("interval", m_devicelines[i].lines, line);
    if (linenr == -1) continue;

    string strinterval;
    GetWord(line, strinterval);

    int interval;
    StrToInt(strinterval, interval);

    bool timerfound = false;
    for (int j = 0; j < timers.size(); j++)
    {
      if (timers[j].GetInterval() == interval)
      {
        timerfound = true;
        break;
      }
    }

    if (!timerfound)
    {
      CAsyncTimer timer;
      timer.SetInterval(interval);
      timers.push_back(timer);
    }
  }
}    

bool CConfig::BuildDeviceConfig(std::vector<CDevice*>& devices, std::vector<CAsyncTimer>& timers, CClientsHandler& clients)
{
  for (int i = 0; i < m_devicelines.size(); i++)
  {
    string line;
    int linenr = GetLineWithKey("type", m_devicelines[i].lines, line);
    string type;

    GetWord(line, type);

    if (type == "popen")
    {
      CDevice* device = NULL;
      if (!BuildPopen(device, timers, i, clients))
      {
        if (device)
          delete device;
        return false;
      }
      devices.push_back(device);
    }
    else if (type == "momo" || type == "atmo")
    {
      CDevice* device = NULL;
      if (!BuildRS232(device, timers, i, clients))
      {
        if (device)
          delete device;
        return false;
      }
      devices.push_back(device);
    }
    else if (type == "ltbl")
    {
      CDevice* device = NULL;
      if (!BuildLtbl(device, timers, i, clients))
      {
        if (device)
          delete device;
        return false;
      }
      devices.push_back(device);
    }
    else
    {
      log("%s error on line %i: unknown type %s", m_filename.c_str(), linenr, type.c_str());
      return false;
    }
  }

  return true;
}

bool CConfig::BuildLightConfig(std::vector<CLight>& lights, std::vector<CDevice*>& devices, std::vector<CColor>& colors)
{
  CLight globallight; //default values

  for (int i = 0; i < m_lightlines.size(); i++)
  {
    CLight light = globallight;

    if (!SetLightName(light, m_lightlines[i].lines, i))
      return false;

    if (!SetLightArea(light, m_lightlines[i].lines))
      return false;

    for (int j = 0; j < m_lightlines[i].lines.size(); j++)
    {
      string line = m_lightlines[i].lines[j].line;
      string key;
      GetWord(line, key);

      if (key != "color")
        continue;

      string colorname, devicename, devicechannel;
      GetWord(line, colorname);
      GetWord(line, devicename);
      GetWord(line, devicechannel);

      bool colorfound = false;
      for (int k = 0; k < colors.size(); k++)
      {
        if (colors[k].GetName() == colorname)
        {
          colorfound = true;
          light.AddColor(colors[k]);
          break;
        }
      }
      if (!colorfound)
      {
        log("%s error on line %i: no color with name %s", m_filename.c_str(), m_lightlines[i].lines[j].linenr, colorname.c_str());
        return false;
      }

      int ichannel;
      StrToInt(devicechannel, ichannel);
      
      bool devicefound = false;
      for (int k = 0; k < devices.size(); k++)
      {
        if (devices[k]->GetName() == devicename)
        {
          if (ichannel > devices[k]->GetNrChannels())
          {
            log("%s error on line %i: channel %i wanted but device %s has %i channels", m_filename.c_str(),
                m_lightlines[i].lines[j].linenr, ichannel, devices[k]->GetName().c_str(), devices[k]->GetNrChannels());
            return false;
          }
          devicefound = true;
          CChannel channel;
          channel.SetColor(light.GetNrColors() - 1);
          channel.SetLight(i);
          devices[k]->SetChannel(channel, ichannel - 1);
          break;
        }
      }
      if (!devicefound)
      {
        log("%s error on line %i: no device with name %s", m_filename.c_str(), m_lightlines[i].lines[j].linenr, devicename.c_str());
        return false;
      }
    }
    lights.push_back(light);
  }

  return true;
}

CAsyncTimer* CConfig::GetTimer(int interval, std::vector<CAsyncTimer>& timers)
{
  for (int i = 0; i < timers.size(); i++)
  {
    if (interval == timers[i].GetInterval())
    {
      return &timers[i];
    }
  }
  return NULL;
}

bool CConfig::BuildPopen(CDevice*& device, std::vector<CAsyncTimer>& timers, int devicenr, CClientsHandler& clients)
{
  string line, strvalue;
  int interval;

  int linenr = GetLineWithKey("interval", m_devicelines[devicenr].lines, line);
  if (linenr == -1)
  {
    log("%s error: device %i has type popen but no interval", m_filename.c_str(), devicenr + 1);
    return false;
  }

  GetWord(line, strvalue);
  StrToInt(strvalue, interval);

  CAsyncTimer* timer = GetTimer(interval, timers);
  if (timer == NULL)
  {
    log("error: no timer with interval %i found (this should never happen, so we're screwed)", interval);
    return false;
  }

  device = reinterpret_cast<CDevice*>(new CDevicePopen(clients, *timer));

  if (!SetDeviceName(device, devicenr))
    return false;

  if (!SetDeviceOutput(device, devicenr))
    return false;

  if (!SetDeviceChannels(device, devicenr))
    return false;

  device->SetType(POPEN);
  
  return true;
}

bool CConfig::BuildRS232(CDevice*& device, std::vector<CAsyncTimer>& timers, int devicenr, CClientsHandler& clients)
{
  string line, strvalue, type;
  int interval;

  GetLineWithKey("type", m_devicelines[devicenr].lines, line);
  GetWord(line, type);
  
  int linenr = GetLineWithKey("interval", m_devicelines[devicenr].lines, line);
  if (linenr == -1)
  {
    log("%s error: device %i has type %s but no interval", m_filename.c_str(), devicenr + 1, type.c_str());
    return false;
  }

  GetWord(line, strvalue);
  StrToInt(strvalue, interval);

  CAsyncTimer* timer = GetTimer(interval, timers);
  if (timer == NULL)
  {
    log("error: no timer with interval %i found (this should never happen, so we're screwed)", interval);
    return false;
  }

  device = reinterpret_cast<CDevice*>(new CDeviceRS232(clients, *timer));

  if (!SetDeviceName(device, devicenr))
    return false;

  if (!SetDeviceOutput(device, devicenr))
    return false;

  if (!SetDeviceChannels(device, devicenr))
    return false;

  if (!SetDeviceRate(device, devicenr))
    return false;

  SetDevicePrefix(device, devicenr);

  if (type == "momo")
    device->SetType(MOMO);
  else if (type == "atmo")
    device->SetType(ATMO);
  
  return true;
}

bool CConfig::BuildLtbl(CDevice*& device, std::vector<CAsyncTimer>& timers, int devicenr, CClientsHandler& clients)
{
  string line, strvalue;
  int interval;

  int linenr = GetLineWithKey("interval", m_devicelines[devicenr].lines, line);
  if (linenr == -1)
  {
    log("%s error: device %i has type ltbl but no interval", m_filename.c_str(), devicenr + 1);
    return false;
  }

  GetWord(line, strvalue);
  StrToInt(strvalue, interval);

  CAsyncTimer* timer = GetTimer(interval, timers);
  if (timer == NULL)
  {
    log("error: no timer with interval %i found (this should never happen, so we're screwed)", interval);
    return false;
  }

  device = reinterpret_cast<CDevice*>(new CDeviceLtbl(clients, *timer));

  if (!SetDeviceName(device, devicenr))
    return false;

  if (!SetDeviceOutput(device, devicenr))
    return false;

  if (!SetDeviceChannels(device, devicenr))
    return false;

  if (!SetDeviceRate(device, devicenr))
    return false;

  device->SetType(LTBL);
  
  return true;
}

bool CConfig::BuildSound(CDevice*& device, int devicenr, CClientsHandler& clients)
{
  string line, strvalue;

  device = reinterpret_cast<CDevice*>(new CDeviceSound(clients));

  if (!SetDeviceName(device, devicenr))
    return false;

  if (!SetDeviceOutput(device, devicenr))
    return false;

  if (!SetDeviceChannels(device, devicenr))
    return false;

  if (!SetDeviceRate(device, devicenr))
    return false;

  if (!SetDevicePeriod(device, devicenr))
    return false;

  if (!SetDeviceBuffer(device, devicenr))
    return false;

  device->SetType(SOUND);
  
  return true;
}

bool CConfig::SetDeviceName(CDevice* device, int devicenr)
{
  string line, strvalue;
  int linenr = GetLineWithKey("name", m_devicelines[devicenr].lines, line);
  if (linenr == -1)
  {
    log("%s error: device %i has no name", m_filename.c_str(), devicenr + 1);
    return false;
  }
  GetWord(line, strvalue);
  device->SetName(strvalue);

  return true;
}

bool CConfig::SetDeviceOutput(CDevice* device, int devicenr)
{
  string line, strvalue;
  int linenr = GetLineWithKey("output", m_devicelines[devicenr].lines, line);
  if (linenr == -1)
  {
    log("%s error: device %s has no output", m_filename.c_str(), device->GetName().c_str());
    return false;
  }
  GetWord(line, strvalue);
  device->SetOutput(strvalue + line);

  return true;
}

bool CConfig::SetDeviceChannels(CDevice* device, int devicenr)
{
  string line, strvalue;
  int linenr = GetLineWithKey("channels", m_devicelines[devicenr].lines, line);
  if (linenr == -1)
  {
    log("%s error: device %s has no channels", m_filename.c_str(), device->GetName().c_str());
    return false;
  }
  GetWord(line, strvalue);

  int nrchannels;
  StrToInt(strvalue, nrchannels);
  device->SetNrChannels(nrchannels);

  return true;
}  

bool CConfig::SetDeviceRate(CDevice* device, int devicenr)
{
  string line, strvalue;
  int linenr = GetLineWithKey("rate", m_devicelines[devicenr].lines, line);
  if (linenr == -1)
  {
    log("%s error: device %s has no rate", m_filename.c_str(), device->GetName().c_str());
    return false;
  }
  GetWord(line, strvalue);

  int rate;
  StrToInt(strvalue, rate);
  device->SetRate(rate);

  return true;
}

void CConfig::SetDevicePrefix(CDevice* device, int devicenr)
{
  string line, strvalue;
  std::vector<unsigned char> prefix;
  int linenr = GetLineWithKey("prefix", m_devicelines[devicenr].lines, line);
  if (linenr == -1)
  {
    return;
  }

  while(GetWord(line, strvalue))
  {
    int iprefix;
    HexStrToInt(strvalue, iprefix);
    prefix.push_back(iprefix);
  }
  device->SetPrefix(prefix);
}

bool CConfig::SetDevicePeriod(CDevice* device, int devicenr)
{
  string line, strvalue;
  int linenr = GetLineWithKey("period", m_devicelines[devicenr].lines, line);
  if (linenr == -1)
  {
    log("%s error: device %s has no period", m_filename.c_str(), device->GetName().c_str());
    return false;
  }
  GetWord(line, strvalue);

  int period;
  StrToInt(strvalue, period);
  device->SetPeriod(period);

  return true;
}

bool CConfig::SetDeviceBuffer(CDevice* device, int devicenr)
{
  string line, strvalue;
  int linenr = GetLineWithKey("buffer", m_devicelines[devicenr].lines, line);
  if (linenr == -1)
  {
    log("%s error: device %s has no buffer", m_filename.c_str(), device->GetName().c_str());
    return false;
  }
  GetWord(line, strvalue);

  int buffer;
  StrToInt(strvalue, buffer);
  device->SetBuffer(buffer);

  return true;
}

void CConfig::SetLightSpeed(CLight& light, std::vector<CConfigLine>& lines)
{
  string line, strvalue;
  int linenr = GetLineWithKey("speed", lines, line);
  if (linenr != -1)
  {
    float speed;
    GetWord(line, strvalue);
    StrToFloat(strvalue, speed);
    light.SetSpeed(speed);
  }
}

void CConfig::SetLightUse(CLight& light, std::vector<CConfigLine>& lines)
{
  string line, strvalue;
  int linenr = GetLineWithKey("use", lines, line);
  if (linenr != -1)
  {
    bool use;
    GetWord(line, strvalue);
    StrToBool(strvalue, use);
    light.SetUse(use);
  }
}

void CConfig::SetLightInterpolation(CLight& light, std::vector<CConfigLine>& lines)
{
  string line, strvalue;
  int linenr = GetLineWithKey("interpolation", lines, line);
  if (linenr != -1)
  {
    bool interpolation;
    GetWord(line, strvalue);
    StrToBool(strvalue, interpolation);
    light.SetInterpolation(interpolation);
  }
}

bool CConfig::SetLightName(CLight& light, std::vector<CConfigLine>& lines, int lightnr)
{
  string line, strvalue;
  int linenr = GetLineWithKey("name", lines, line);
  if (linenr == -1)
  {
    log("%s error: light %i has no name", m_filename.c_str(), lightnr + 1);
    return false;
  }

  GetWord(line, strvalue);
  light.SetName(strvalue);
  return true;
}

bool CConfig::SetLightArea(CLight& light, std::vector<CConfigLine>& lines)
{
  string line, strvalue;
  int linenr = GetLineWithKey("area", lines, line);
  if (linenr == -1)
  {
    log("%s error: light %s has no area", m_filename.c_str(), light.GetName().c_str());
    return false;
  }

  std::vector<point> area;

  while (GetWord(line, strvalue))
  {
    point point;
    sscanf(strvalue.c_str(), "%i.%i", &point.x, &point.y);
    point.x--;
    point.y--;
    
    area.push_back(point);           
  }
  light.SetArea(area);
  return true;
}  