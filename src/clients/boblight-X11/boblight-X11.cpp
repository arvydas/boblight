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
#include "lib/libboblight.h"

#include <stdint.h>
#include <iostream>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "util/misc.h"
#include "util/timer.h"
#include "vblanksignal.h"
#include "config.h"
#include "clients/flags.h"

#include "grabber-base.h"
#include "grabber-xgetimage.h"
#include "grabber-xrender.h"

#define XGETIMAGE 0
#define XRENDER   1

using namespace std;

bool ParseFlags(int argc, char *argv[], double& interval, int& pixels, int& method);
int  Run(vector<string>& options, int priority, char* address, int port, int pixels, double interval, int method);
void PrintHelpMessage();
void SignalHandler(int signum);
int  ErrorHandler(Display* dpy, XErrorEvent* error);

volatile bool stop = false;

int main (int argc, char *argv[])
{
  //load the boblight lib, if it fails we get a char* from dlerror()
  char* boblight_error = boblight_loadlibrary(NULL);
  if (boblight_error)
  {
    PrintError(boblight_error);
    return 1;
  }

  bool   list = false;     //if we have to print the boblight options
  bool   help = false;     //if we have to print the help message
  int    priority = 128;   //priority of us as a client of boblightd
  string straddress;       //address of boblightd
  char*  address;          //set to NULL for default, or straddress.c_str() otherwise
  int    port = -1;        //port, -1 is default port
  double interval = 0.1;   //interval of the grabber in seconds
  int    pixels = -1;      //number of pixels/rows to use
  int    method = XRENDER; //what method we use to capture pixels
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

  if (!ParseFlags(argc, argv, interval, pixels, method))
    return 1;

  if (pixels == -1) //set default pixels
  {
    if (method == XGETIMAGE)
      pixels = 16;
    else
      pixels = 64;
  }  
  
  if (straddress.empty())
    address = NULL;
  else
    address = const_cast<char*>(straddress.c_str());

  //set up signal handlers
  signal(SIGTERM, SignalHandler);
  signal(SIGINT, SignalHandler);

  //keeps running until some unrecoverable error happens
  return Run(options, priority, address, port, pixels, interval, method);

}

bool ParseFlags(int argc, char *argv[], double& interval, int& pixels, int& method)
{
  int c;
  optind = 0; //ParseBoblightFlags already did getopt

  while ((c = getopt (argc, argv, "i:u:x")) != -1)
  {
    if (c == 'i')
    {
      bool vblank = false;
      if (optarg[0] == 'v') //starting interval with v means vblank interval
      {
        optarg++;
        vblank = true;
      }

      if (!StrToFloat(optarg, interval) || interval <= 0.0)
      {
        PrintError("Wrong value " + string(optarg) + " for interval");
        return false;
      }

      if (vblank)
      {
        if (interval < 1.0)
        {
          PrintError("Wrong value " + string(optarg) + " for vblank interval");
          return false;
        }
        interval *= -1.0; //negative interval means vblank
        optarg--;
      }
    }
    else if (c == 'u')
    {
      if (!StrToInt(optarg, pixels) || pixels <= 0)
      {
        PrintError("Wrong value " + string(optarg) + " for pixels");
        return false;
      }
    }
    else if (c == 'x')
    {
      method = XGETIMAGE;
    }
    else if (c == '?')
    {
      if (optopt == 'u' || optopt == 'i')
      {
        PrintError("Option " + ToString((char)optopt) + " requires an argument");
        return false;
      }
    }
  }

  return true;
}

int Run(vector<string>& options, int priority, char* address, int port, int pixels, double interval, int method)
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

    CGrabber* grabber;
    
    if (method == XGETIMAGE)
      grabber = reinterpret_cast<CGrabber*>(new CGrabberXGetImage(boblight));
    else if (method == XRENDER)
      grabber = reinterpret_cast<CGrabber*>(new CGrabberXRender(boblight));

    grabber->SetInterval(interval);
    grabber->SetSize(pixels);

    if (!grabber->Setup())
    {
      PrintError(grabber->GetError());
      delete grabber;
      boblight_destroy(boblight);
      return 1;
    }

    if (!grabber->Run(stop))
    {
      PrintError(grabber->GetError());
      delete grabber;
      boblight_destroy(boblight);
      return 1;
    }
    else
    {
      if (!grabber->GetError().empty())
        PrintError(grabber->GetError());
    }

    delete grabber;

    boblight_destroy(boblight);
  }

  cout << "Exiting\n";
  
  return 0;
}

void PrintHelpMessage()
{
  cout << "\n";
  cout << "boblight-X11 " << VERSION << "\n";
  cout << "\n";
  cout << "Usage: boblight-X11 [OPTION]\n";
  cout << "\n";
  cout << "  options:\n";
  cout << "\n";
  cout << "  -p  priority, from 0 to 255, default is 128\n";
  cout << "  -s  address:[port], set the address and optional port to connect to\n";
  cout << "  -o  add libboblight option, syntax: [light:]option=value\n";
  cout << "  -l  list libboblight options\n";
  cout << "  -i  set the interval in seconds, default is 0.1\n";
  cout << "      prefix the value with v to wait for a number of vertical blanks instead\n";
  cout << "  -u  set the number of pixels/rows to use\n";
  cout << "      default is 64 for xrender and 16 for xgetimage\n";
  cout << "  -x  use XGetImage instead of XRender\n";
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

int ErrorHandler(Display* dpy, XErrorEvent* error)
{
  if (error->error_code == BadMatch)
  {
    cout << "caught BadMatch, resolution probably changed\n";
    xerror = true;
  }
  else
  {
    cout << "caught error " << error->error_code << "\n";
    stop = true;
  }
}