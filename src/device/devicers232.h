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

#ifndef CDEVICERS232
#define CDEVICERS232

#include "device.h"
#include "util/serialport.h"

class CDeviceRS232 : public CDevice
{
  public:
    CDeviceRS232(CClientsHandler& clients, CAsyncTimer& timer);

    void SetPrefix(std::vector<unsigned char> prefix) { m_prefix = prefix; }
    void SetType(int type);

  protected:

    bool SetupDevice();
    bool WriteOutput();
    void CloseDevice();
    
    CAsyncTimer& m_timer;
    CSerialPort  m_serialport;

    unsigned char*             m_buff;
    std::vector<unsigned char> m_prefix;
};

#endif //CDEVICERS232
