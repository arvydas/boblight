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

#ifndef CONFIGURATION
#define CONFIGURATION

#include <string>
#include <vector>

#include "connectionhandler.h"
#include "light.h"
#include "client.h"
#include "device/device.h"

class CConfigLine
{
  public:
    CConfigLine(std::string linestr, int iline)
    {
      linenr = iline;
      line = linestr;
    }
    
    int linenr;
    std::string line;
};

class CConfigGroup
{
  public:
    std::vector<CConfigLine> lines;
};

class CConfig
{
  public:
    bool LoadConfigFromFile(std::string file);
    bool CheckConfig();

    bool BuildConfig(CConnectionHandler& connectionhandler, CClientsHandler& clients, std::vector<CDevice*>& devices,
                     std::vector<CAsyncTimer>& timers, std::vector<CLight>& lights);

  private:
    std::string m_filename;

    std::vector<CConfigLine>  m_globalconfiglines;
    std::vector<CConfigGroup> m_devicelines;
    std::vector<CConfigGroup> m_colorlines;
    std::vector<CConfigGroup> m_lightlines;

    void PrintConfig();
    void TabsToSpaces(std::string& line);

    bool CheckGlobalConfig();
    bool CheckDeviceConfig();
    bool CheckColorConfig();
    bool CheckLightConfig();

    int  GetLineWithKey(std::string key, std::vector<CConfigLine>& lines, std::string& line);

    void BuildConnectionHandlerConfig(CConnectionHandler& connectionhandler);
    bool BuildColorConfig(std::vector<CColor>& colors);
    void BuildTimers(std::vector<CAsyncTimer>& timers);
    bool BuildDeviceConfig(std::vector<CDevice*>& devices, std::vector<CAsyncTimer>& timers, CClientsHandler& clients);
    bool BuildLightConfig(std::vector<CLight>& lights, std::vector<CDevice*>& devices, std::vector<CColor>& colors);

    bool BuildPopen(CDevice*& device, std::vector<CAsyncTimer>& timers, int devicenr, CClientsHandler& clients);
    bool BuildRS232(CDevice*& device, std::vector<CAsyncTimer>& timers, int devicenr, CClientsHandler& clients);
    bool BuildLtbl(CDevice*& device, std::vector<CAsyncTimer>& timers, int devicenr, CClientsHandler& clients);
    bool BuildSound(CDevice*& device, int devicenr, CClientsHandler& clients);

    CAsyncTimer* GetTimer(int interval, std::vector<CAsyncTimer>& timers);

    bool SetDeviceName(CDevice* device, int devicenr);
    bool SetDeviceOutput(CDevice* device, int devicenr);
    bool SetDeviceChannels(CDevice* device, int devicenr);
    bool SetDeviceRate(CDevice* device, int devicenr);
    void SetDevicePrefix(CDevice* device, int devicenr);
    bool SetDevicePeriod(CDevice* device, int devicenr);
    bool SetDeviceBuffer(CDevice* device, int devicenr);

    void SetLightSpeed(CLight& light, std::vector<CConfigLine>& lines);
    void SetLightUse(CLight& light, std::vector<CConfigLine>& lines);
    void SetLightInterpolation(CLight& light, std::vector<CConfigLine>& lines);
    bool SetLightName(CLight& light, std::vector<CConfigLine>& lines, int lightnr);
    bool SetLightArea(CLight& light, std::vector<CConfigLine>& lines);
};

#endif //CONFIGURATION