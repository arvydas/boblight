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
  cout << "Help message to come here\n";
}
