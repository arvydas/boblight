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

#ifndef FLAGMANAGER
#define FLAGMANAGER

#include <string>
#include <vector>

class CArguments
{
  public:
    CArguments(int argc, char** argv);
    ~CArguments();

    int    m_argc;
    char** m_argv;
};

class CFlagManager
{
  public:
    CFlagManager();

    bool         m_printhelp;              //if we need to print the help message
    bool         m_printboblightoptions;   //if we need to print the boblight options

    const char*  m_paddress;
    int          m_port;                   //port to connect to
    int          m_priority;

    void         ParseFlags(int tempargc, char** tempargv);
    virtual void PrintHelpMessage() {};

    void         PrintBoblightOptions();
    void         ParseBoblightOptions(void* boblight);

  private:
    std::string  m_flags;
    std::string  m_address;                //address to connect to

    std::vector<std::string> m_options;    //boblight options

    virtual void ParseFlagsExtended(int& argc, char**& argv){};
    virtual void PostGetopt(int optind, int argc, char** argv) {};
};

#endif //FLAGMANAGER