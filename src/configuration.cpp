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

using namespace std;

#define SECTNOTHING 0
#define SECTGLOBAL  1
#define SECTDEVICE  2
#define SECTCOLOR   3
#define SECTLIGHT   4

//builds the config
bool CConfig::LoadConfigFromFile(string file)
{
  int linenr = 0;
  int currentsection = SECTNOTHING;

  m_filename = file;
  
  log("opening %s", file.c_str());

  //try to open the config file
  ifstream configfile(file.c_str());
  if (configfile.fail())
  {
    log("%s: %s", file.c_str(), GetErrno().c_str());
    return false;
  }

  //read lines from the config file and store them in the appropriate sections
  while (!configfile.eof())
  {
    string buff;
    getline(configfile, buff);
    if (configfile.fail())
    {
      break;
    }
    linenr++;

    string line = buff;
    string key;
    //if the line doesn't have a word it's not important
    if (!GetWord(line, key))
      continue;

    //ignore comments
    if (key.substr(0, 1) == "#")
      continue;

    //check if we entered a section
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

    //we're not in a section
    if (currentsection == SECTNOTHING)
      continue;

    CConfigLine configline(buff, linenr);

    //store the config line in the appropriate section
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

//checks lines in the config file to make sure the syntax is correct
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

    GetWord(line, key); //we already checked each line starts with one word

    if (!GetWord(line, value)) //every line here needs to have another word
    {
      log("%s error on line %i: no value for key %s", m_filename.c_str(), m_globalconfiglines[i].linenr, key.c_str());
      valid = false;
      continue;
    }

    if (key == "interface")
    {
      continue; //not much to check here
    }
    else if (key == "port") //check tcp/ip port
    {
      int port;
      if (!StrToInt(value, port) || port < 0 || port > 65535)
      {
        log("%s error on line %i: wrong value %s for key %s", m_filename.c_str(), m_globalconfiglines[i].linenr, value.c_str(), key.c_str());
        valid = false;
      }
    }
    else //we don't know this one
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

      GetWord(line, key); //we already checked each line starts with one word

      if (!GetWord(line, value)) //every line here needs to have another word
      {
        log("%s error on line %i: no value for key %s", m_filename.c_str(), linenr, key.c_str());
        valid = false;
        continue;
      }

      if (key == "name" || key == "output" || key == "type")
      {
        continue; //can't check these here
      }
      else if (key == "rate" || key == "channels" || key == "interval" || key == "period")
      { //these are of type integer not lower than 1
        int ivalue;
        if (!StrToInt(value, ivalue) || ivalue < 1)
        {
          log("%s error on line %i: wrong value %s for key %s", m_filename.c_str(), linenr, value.c_str(), key.c_str());
          valid = false;
        }          
      }
      else if (key == "prefix")
      { //this is in hex from 00 to FF, separated by spaces, like: prefix FF 7F A0 22
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
      else if (key == "latency")//this is a double
      {
        double latency;
        if (!StrToFloat(value, latency) || latency <= 0.0)
        {
          log("%s error on line %i: wrong value %s for key %s", m_filename.c_str(), linenr, value.c_str(), key.c_str());
          valid = false;
        }
      }
      else //don't know this one
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

      GetWord(line, key); //we already checked each line starts with one word

      if (!GetWord(line, value)) //every line here needs to have another word
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
      { //these are floats from 0.0 to 1.0, except gamma which is from 0.0 and up
        float fvalue;
        if (!StrToFloat(value, fvalue) || fvalue < 0.0 || (key != "gamma" && fvalue > 1.0))
        {
          log("%s error on line %i: wrong value %s for key %s", m_filename.c_str(), linenr, value.c_str(), key.c_str());
          valid = false;
        }
      }
      else if (key == "rgb")
      { //rgb lines in hex notation, like: rgb FF0000 for red and rgb 0000FF for blue
        int rgb;
        if (!HexStrToInt(value, rgb) || (rgb & 0xFF000000))
        {
          log("%s error on line %i: wrong value %s for key %s", m_filename.c_str(), linenr, value.c_str(), key.c_str());
          valid = false;
        }
      }
      else //don't know this one
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

      GetWord(line, key); //we already checked each line starts with one word

      if (!GetWord(line, value)) //every line here needs to have another word
      {
        log("%s error on line %i: no value for key %s", m_filename.c_str(), linenr, key.c_str());
        valid = false;
        continue;
      }

      if (key == "name")
      {
        continue;
      }
      else if (key == "vscan" || key == "hscan") //check the scanrange, should be two floats from 0.0 to 100.0
      {
        string scanrange;
        float scan[2];
        if (!GetWord(line, scanrange))
        {
          log("%s error on line %i: not enough values for key %s", m_filename.c_str(), linenr, key.c_str());
          valid = false;
          continue;
        }

        scanrange = value + " " + scanrange;

        if (sscanf(scanrange.c_str(), "%f %f", scan, scan + 1) != 2
            || scan[0] < 0.0 || scan[0] > 100.0 || scan[1] < 0.0 || scan[1] > 100.0 || scan[0] > scan[1])
        {
          log("%s error on line %i: wrong value %s for key %s", m_filename.c_str(), linenr, scanrange.c_str(), key.c_str());
          valid = false;
          continue;
        }
      }
      else if (key == "color")
      { //describes a color for a light, on a certain channel of a device
        //we can only check if it has enough keys and channel is a positive int from 1 or higher
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
      else //don't know this one
      {
        log("%s error on line %i: unknown key %s", m_filename.c_str(), linenr, key.c_str());
        valid = false;
      }      
    }
  }

  return valid;
}

//gets a config line that starts with key
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

//builds the config
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

  return true;
}

void CConfig::BuildConnectionHandlerConfig(CConnectionHandler& connectionhandler)
{
  //set up where to bind the listening socket
  //config for this should already be valid here, of course we can't check yet if the interface actually exists
  string interface; //empty string means bind to *
  int port = 19333; //default port
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

    //we need at least a name for a color
    if (color.GetName().empty())
    {
      log("%s error: color %i has no name", m_filename.c_str(), i + 1);
      return false;
    }

    colors.push_back(color);
  }

  return true;
}

//builds a pool of CAsyncTimer, which is a timer that runs in its own thread
//this way devices with the same interval can run off the same timer so they're synchonized :)
void CConfig::BuildTimers(std::vector<CAsyncTimer>& timers)
{
  //loop though the device config lines
  for (int i = 0; i < m_devicelines.size(); i++)
  {
    string line;

    //see if this device has an interval value, not all types have to have that
    int linenr = GetLineWithKey("interval", m_devicelines[i].lines, line);
    if (linenr == -1) continue; //this one doesn't

    string strinterval;
    GetWord(line, strinterval);

    int interval;
    StrToInt(strinterval, interval); //we already checked interval is a positive integer

    //check if we already have a timer with this interval
    bool timerfound = false;
    for (int j = 0; j < timers.size(); j++)
    {
      if (timers[j].GetInterval() == interval)
      {
        timerfound = true;
        break;
      }
    }

    //we didn't find one, so we have to add it
    if (!timerfound)
    {
      CAsyncTimer timer;
      timer.SetInterval(interval);
      timers.push_back(timer);
    }
  }
}    

//builds a pool of devices
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
    else if (type == "sound")
    {
      CDevice* device = NULL;
      if (!BuildSound(device, i, clients))
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

//builds a pool of lights
bool CConfig::BuildLightConfig(std::vector<CLight>& lights, std::vector<CDevice*>& devices, std::vector<CColor>& colors)
{
  CLight globallight; //default values

  for (int i = 0; i < m_lightlines.size(); i++)
  {
    CLight light = globallight;

    if (!SetLightName(light, m_lightlines[i].lines, i))
      return false;

    SetLightScanRange(light, m_lightlines[i].lines);

    //check the colors on a light
    for (int j = 0; j < m_lightlines[i].lines.size(); j++)
    {
      string line = m_lightlines[i].lines[j].line;
      string key;
      GetWord(line, key);

      if (key != "color")
        continue;

      //we already checked these in the syntax check
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
      if (!colorfound) //this color doesn't exist
      {
        log("%s error on line %i: no color with name %s", m_filename.c_str(), m_lightlines[i].lines[j].linenr, colorname.c_str());
        return false;
      }

      int ichannel;
      StrToInt(devicechannel, ichannel);

      //loop through the devices, check if one with this name exists and if the channel on it exists
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

//gets a timer with a specific interval
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

  device = new CDevicePopen(clients, *timer);

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

  CDeviceRS232* rs232device = new CDeviceRS232(clients, *timer);
  device = rs232device;

  if (!SetDeviceName(rs232device, devicenr))
    return false;

  if (!SetDeviceOutput(rs232device, devicenr))
    return false;

  if (!SetDeviceChannels(rs232device, devicenr))
    return false;

  if (!SetDeviceRate(rs232device, devicenr))
    return false;

  SetDevicePrefix(rs232device, devicenr);

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

  device = new CDeviceLtbl(clients, *timer);

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
  CDeviceSound* sounddevice = new CDeviceSound(clients);
  device = sounddevice;

  if (!SetDeviceName(sounddevice, devicenr))
    return false;

  if (!SetDeviceOutput(sounddevice, devicenr))
    return false;

  if (!SetDeviceChannels(sounddevice, devicenr))
    return false;

  if (!SetDeviceRate(sounddevice, devicenr))
    return false;

  if (!SetDevicePeriod(sounddevice, devicenr))
    return false;

  if (!SetDeviceLatency(sounddevice, devicenr))
    return false;

  sounddevice->SetType(SOUND);

  //check if output is in api:device format
  int pos = sounddevice->GetOutput().find(':');
  if (pos == string::npos || sounddevice->GetOutput().size() == pos)
  {
    log("%s error: device %s output %s is not in api:device format",
        m_filename.c_str(), sounddevice->GetName().c_str(), sounddevice->GetOutput().c_str());
    return false;
  }

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

void CConfig::SetDevicePrefix(CDeviceRS232* device, int devicenr)
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

bool CConfig::SetDevicePeriod(CDeviceSound* device, int devicenr)
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

bool CConfig::SetDeviceLatency(CDeviceSound* device, int devicenr)
{
  string line, strvalue;
  int linenr = GetLineWithKey("latency", m_devicelines[devicenr].lines, line);
  if (linenr == -1)
  {
    log("%s error: device %s has no latency", m_filename.c_str(), device->GetName().c_str());
    return false;
  }
  GetWord(line, strvalue);

  float latency;
  StrToFloat(strvalue, latency);
  device->SetLatency(latency);

  return true;
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

void CConfig::SetLightScanRange(CLight& light, std::vector<CConfigLine>& lines)
{
  string line, strvalue;
  int linenr = GetLineWithKey("hscan", lines, line);
  if (linenr != -1)
  {
    float hscan[2];
    sscanf(line.c_str(), "%f %f", hscan, hscan + 1);
    light.SetHscan(hscan);
  }

  linenr = GetLineWithKey("vscan", lines, line);
  if (linenr != -1)
  {
    float vscan[2];
    sscanf(line.c_str(), "%f %f", vscan, vscan + 1);
    light.SetVscan(vscan);
  }
}  
