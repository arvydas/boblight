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

#ifndef FLAGMANAGERX11
#define FLAGMANAGERX11

#define XGETIMAGE 0
#define XRENDER   1

#include "clients/flagmanager.h"

class CFlagManagerX11 : public CFlagManager
{
  public:
    CFlagManagerX11();
    
    void        ParseFlagsExtended(int& argc, char**& argv, int& c, char*& optarg);
    void        PrintHelpMessage();

    double      m_interval;
    int         m_pixels;
    int         m_method;
    bool        m_debug;
    const char* m_debugdpy;

  private:
    std::string m_strdebugdpy;
};

#endif //FLAGMANAGERX11