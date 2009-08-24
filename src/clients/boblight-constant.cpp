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
#include <signal.h>

#include "config.h"
#include "util/misc.h"
#include "flags.h"

using namespace std;

int  Run(vector<string>& options, int priority, char* address, int port, int color);
void PrintHelpMessage();
void SignalHandler(int signum);

volatile bool stop = false;

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

  //check if a color was given
  if (optind == argc)
  {
    PrintError("no color given");
    PrintHelpMessage();
    return 1;
  }
  
  //check if the color can be loaded
  if (!HexStrToInt(argv[optind], color) || color & 0xFF000000)
  {
    PrintError("wrong value " + ToString(argv[optind]) + " for color");
    PrintHelpMessage();
    return 1;
  }
  
  if (straddress.empty())
    address = NULL;
  else
    address = const_cast<char*>(straddress.c_str());

  //set up signal handlers
  signal(SIGTERM, SignalHandler);
  signal(SIGINT, SignalHandler);

  //keep running until we want to quit
  return Run(options, priority, address, port, color);
}

int Run(vector<string>& options, int priority, char* address, int port, int color)
{
  while(!stop)
  {
    //init boblight
    void* boblight = boblight_init();

    cout << "Connecting to boblightd\n";
    
    //try to connect, if we can't then bitch to stdout and destroy boblight
    if (!boblight_connect(boblight, address, port, 5000000) || !boblight_setpriority(boblight, priority))
    {
      PrintError(boblight_geterror(boblight));
      cout << "Waiting 10 seconds before trying again\n";
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

    //load the color into int array
    int rgb[3] = {(color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF};
   
    //set all lights to the color we want and send it
    boblight_addpixel(boblight, -1, rgb);
    if (!boblight_sendrgb(boblight))
    {
      PrintError(boblight_geterror(boblight));
      boblight_destroy(boblight);
      continue;
    }

    //keep checking the connection to boblightd every 10 seconds, if it breaks we try to connect again
    while(!stop)
    {
      if (!boblight_ping(boblight))
      {
        PrintError(boblight_geterror(boblight));
        break;
      }
      sleep(10);
    }

    boblight_destroy(boblight);
  }

  cout << "Exiting\n";
  
  return 0;
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
  cout << "  -p  priority, from 0 to 255, default is 128\n";
  cout << "  -s  address[:port], set the address and optional port to connect to\n";
  cout << "  -o  add libboblight option, syntax: [light:]option=value\n";
  cout << "  -l  list libboblight options\n";
  cout << "\n";
}

void SignalHandler(int signum)
{
  if (signum == SIGTERM)
  {
    cout << "caught SIGTERM\n";
    stop = true;
  }
  else if (signum == SIGINT)
  {
    cout << "caught SIGINT\n";
    stop = true;
  }
}
