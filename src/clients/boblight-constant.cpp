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

#define BOBLIGHT_DLOPEN
#include "../lib/libboblight.h"

#include <iostream>
#include <string>
#include <vector>

#include "config.h"
#include "util/misc.h"

using namespace std;

void PrintHelpMessage();
bool ParseBoblightFlags(int argc, char *argv[], vector<string>& options, int& priority, string& address, int& port, bool& list, bool& help);
void ListBoblightOptions();
bool ParseBoblightOptions(void* boblight, vector<string>& options);

int main(int argc, char *argv[])
{
  //load the boblight lib, if it fails we get a char* from dlerror()
  char* boblight_error = boblight_loadlibrary(NULL);
  if (boblight_error)
  {
    PrintError(boblight_error);
    return 1;
  }

  bool   list = false;   //if we have to print the boblight options
  bool   help = false;   //if we have to print the help message
  int    priority = 128; //priority of us as a client of boblightd
  string straddress;     //address of boblightd
  char*  address;        //set to NULL for default, or straddress.c_str() otherwise
  int    port = -1;      //port, -1 is default port
  int    color;          //where we load in the hex color
  vector<string> options;

  //parse default boblight flags, if it fails we're screwed
  if (!ParseBoblightFlags(argc, argv, options, priority, straddress, port, list, help))
  {
    PrintHelpMessage();
    return 1;
  }

  //check if a color was given
  if (optind == argc)
  {
    PrintError("no color given");
    help = true;
  }
  //check if the color can be loaded
  else if (!HexStrToInt(argv[optind], color) || color & 0xFF000000)
  {
    PrintError("wrong value " + ToString(argv[optind]) + " for color");
    help = true;
  }
  
  if (help)
  {
    PrintHelpMessage();
    return 1;
  }
  else if (list)
  {
    ListBoblightOptions();
    return 1;
  }

  if (straddress.empty())
    address = NULL;
  else
    address = const_cast<char*>(straddress.c_str());

  //load the color into int array
  int rgb[3] = {(color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF};

  while(1)
  {
    //init boblight
    void* boblight = boblight_init();

    //try to connect, if we can't then bitch to stdout and destroy boblight
    if (!boblight_connect(boblight, NULL, -1, 5000000) || !boblight_setpriority(boblight, priority))
    {
      PrintError(boblight_geterror(boblight));
      boblight_destroy(boblight);
      sleep(10);
      continue;
    }

    cout << "Connection to boblightd opened\n";
    
    //if we can't parse the boblight option lines (given with -o) properly, just exit
    if (!ParseBoblightOptions(boblight, options))
    {
      boblight_destroy(boblight);
      return 1;
    }

    //set all lights to the color we want and send it
    boblight_addpixel(boblight, -1, rgb);
    if (!boblight_sendrgb(boblight))
    {
      PrintError(boblight_geterror(boblight));
      boblight_destroy(boblight);
      continue;
    }

    //keep checking the connection to boblightd every 10 seconds, if it breaks we try to connect again
    while(1)
    {
      if (!boblight_ping(boblight))
      {
        PrintError(boblight_geterror(boblight));
        boblight_destroy(boblight);
        break;
      }
      sleep(10);
    }
  }
}

void PrintHelpMessage()
{
  cout << "\n";
  cout << "boblight-constant " << VERSION << "\n";
  cout << "\n";
  cout << "Usage: boblight-constant [OPTION] color\n";
  cout << "\n";
  cout << "  color is in RRGGBB hex notation\n";
  cout << "\n";
  cout << "  options:\n";
  cout << "\n";
  cout << "  -p  priority, from 0 to 255\n";
  cout << "  -s  address:[port], set the address and optional port to connect to\n";
  cout << "  -o  add libboblight option, syntax: [light:]option=value\n";
  cout << "  -l  list libboblight options\n";
  cout << "\n";
}

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
        string error = "Option ";
        error += static_cast<char>(optopt);
        error += " requires an argument";
        PrintError(error);
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