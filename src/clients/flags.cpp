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

#include "flags.h"
#include "util/misc.h"

#define BOBLIGHT_DLOPEN_EXTERN
#include "../lib/libboblight.h"

using namespace std;

bool ParseBoblightFlags(int argc, char *argv[], vector<string>& options, int& priority, string& address, int& port, bool& list, bool& help)
{
  string option;
  int c;

  opterr = 0; //we don't want to print error messages
  
  while ((c = getopt (argc, argv, "p:s:o:lh")) != -1)
  {
    if (c == 'p') //priority
    {
      option = optarg;
      if (!StrToInt(option, priority) || priority < 0 || priority > 255)
      {
        PrintError("Wrong option " + static_cast<string>(optarg) + " for argument -p");
        return false;
      }
    }
    else if (c == 's') //address[:port]
    {
      option = optarg;
      address = option.substr(0, option.find(':'));

      if (option.find(':') != string::npos) //check if we have a port
      {
        option = option.substr(option.find(':') + 1);
        string word;
        if (!StrToInt(option, port) || port < 0 || port > 65535)
        {
          PrintError("Wrong option " + static_cast<string>(optarg) + " for argument -s");
          return false;
        }
      }
    }
    else if (c == 'o') //option for libboblight
    {
      options.push_back(optarg);
    }
    else if (c == 'l') //list libboblight options
    {
      list = true;
    }
    else if (c == 'h') //print help message
    {
      help = true;
    }
    else if (c == '?')
    {
      if (optopt == 'p' || optopt == 's' || optopt == 'o')
      {
        PrintError("Option " + ToString((char)optopt) + "requires an argument");
        return false;
      }
    }
  }

  return true;
}

//go through all options and print the descriptions to stdout
void ListBoblightOptions()
{
  void* boblight = boblight_init();
  int nroptions = boblight_getnroptions(boblight);

  for (int i = 0; i < nroptions; i++)
  {
    cout << boblight_getoptiondescript(boblight, i) << "\n";
  }

  boblight_destroy(boblight);
}

bool ParseBoblightOptions(void* boblight, vector<string>& options)
{
  int nrlights = boblight_getnrlights(boblight);
  
  for (int i = 0; i < options.size(); i++)
  {
    string option = options[i];
    string lightname;
    string optionname;
    string optionvalue;
    int    lightnr = -1;

    //check if we have a lightname, otherwise we use all lights
    if (option.find(':') != string::npos)
    {
      lightname = option.substr(0, option.find(':'));
      if (option.find(':') == option.size() - 1) //check if : isn't the last char in the string
      {
        PrintError("wrong option \"" + option + "\", syntax is [light:]option=value");
        return false;
      }
      option = option.substr(option.find(':') + 1); //shave off the lightname

      //check which light this is
      bool lightfound = false;
      for (int j = 0; j < nrlights; j++)
      {
        if (lightname == boblight_getlightname(boblight, j))
        {
          lightfound = true;
          lightnr = j;
          break;
        }
      }
      if (!lightfound)
      {
        PrintError("light \"" + lightname + "\" used in option \"" + options[i] + "\" doesn't exist");
        return false;
      }
    }

    //check if '=' exists and it's not at the end of the string
    if (option.find('=') == string::npos || option.find('=') == option.size() - 1)
    {
      PrintError("wrong option \"" + option + "\", syntax is [light:]option=value");
      return false;
    }

    optionname = option.substr(0, option.find('='));
    optionvalue = option.substr(option.find('=') + 1);

    option = optionname + " " + optionvalue;

    if (!boblight_setoption(boblight, lightnr, option.c_str()))
    {
      PrintError(boblight_geterror(boblight));
      return false;
    }
  }

  return true;
}