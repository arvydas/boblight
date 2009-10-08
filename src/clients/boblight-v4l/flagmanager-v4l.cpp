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

#include "flagmanager-v4l.h"
#include "config.h"

using namespace std;

CFlagManagerV4l::CFlagManagerV4l()
{
  //d = device
  m_flags += "d:";

  //default device
  m_device = "/dev/video0";
}

void CFlagManagerV4l::ParseFlagsExtended(int& argc, char**& argv, int& c, char*& optarg)
{
  if (c == 'd')
  {
    m_device = optarg;
  }
}

void CFlagManagerV4l::PrintHelpMessage()
{
  cout << "\n";
  cout << "boblight-v4l revision:" << REVISION << "\n";
  cout << "\n";
  cout << "Usage: boblight-v4l [OPTION]\n";
  cout << "\n";
  cout << "  options:\n";
  cout << "\n";
  cout << "  -p  priority, from 0 to 255, default is 128\n";
  cout << "  -s  address:[port], set the address and optional port to connect to\n";
  cout << "  -o  add libboblight option, syntax: [light:]option=value\n";
  cout << "  -l  list libboblight options\n";
  cout << "  -f  fork\n";
  cout << "  -d  set the device to use, default is /dev/video0\n";
  cout << "\n";
}
