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

#include <stdint.h>
#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "util/misc.h"
#include "util/timer.h"
#include "config.h"
#include "flags.h"

using namespace std;

bool ParseFlags(int argc, char *argv[], double& interval, int& pixels);
int  Run(vector<string>& options, int priority, char* address, int port, int pixels, CAsyncTimer& timer);
bool Grabber(void* boblight, int pixels, CAsyncTimer& timer);
void PrintHelpMessage();

int main (int argc, char *argv[])
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
  double interval = 0.1; //interval of the grabber in seconds
  int    pixels = 16;      //number of pixels/rows to use
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

  if (!ParseFlags(argc, argv, interval, pixels))
    return 1;
  
  if (straddress.empty())
    address = NULL;
  else
    address = const_cast<char*>(straddress.c_str());

  CAsyncTimer timer;
  timer.StartTimer(Round<int64_t>(interval * 1000000.0));

  //keeps running until some unrecoverable error happens
  return Run(options, priority, address, port, pixels, timer);

}

bool ParseFlags(int argc, char *argv[], double& interval, int& pixels)
{
  int c;
  optind = 1; //ParseBoblightFlags already did getopt

  while ((c = getopt (argc, argv, "i:u:")) != -1)
  {
    if (c == 'i')
    {
      if (!StrToFloat(optarg, interval) || interval <= 0.0)
      {
        PrintError("Wrong value " + string(optarg) + " for interval");
        return false;
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

int Run(vector<string>& options, int priority, char* address, int port, int pixels, CAsyncTimer& timer)
{
  while(1)
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

    if (!Grabber(boblight, pixels, timer)) //if grabber returns false we give up
      return 1;

    boblight_destroy(boblight);
  }

  return 0;
}

bool Grabber(void* boblight, int pixels, CAsyncTimer& timer)
{
  Display*          dpy;
  Window            rootwin;
  XWindowAttributes rootattr;
  XImage*           xim;
  unsigned long     pixel;
  int               rgb[3];
  int               usedpixels;

  dpy = XOpenDisplay(NULL);
  if (dpy == NULL)
  {
    PrintError("Unable to open display");
    return false;
  }

  while(1)
  {
    rootwin = RootWindow(dpy, DefaultScreen(dpy));
    XGetWindowAttributes(dpy, rootwin, &rootattr);

    //we want at least four pixels
    usedpixels = Min(rootattr.width / 2, rootattr.height / 2, pixels);
    
    boblight_setscanrange(boblight, rootattr.width / usedpixels, rootattr.height / usedpixels);

    for (int y = 0; y < rootattr.height; y += rootattr.height / usedpixels)
    {
      for (int x = 0; x < rootattr.width; x += rootattr.width / usedpixels)
      {
        xim = XGetImage(dpy, rootwin, x, y, 1, 1, AllPlanes, ZPixmap);
        pixel = XGetPixel(xim, 0, 0);
        XDestroyImage(xim);

        rgb[0] = (pixel >> 16) & 0xff;
        rgb[1] = (pixel >>  8) & 0xff;
        rgb[2] = (pixel >>  0) & 0xff;

        boblight_addpixelxy(boblight, x / 16, y / 16, rgb);
      }
    }

    if (!boblight_sendrgb(boblight))
    {
      PrintError(boblight_geterror(boblight));
      return true;
    }
    
    timer.Wait();
  }
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
  cout << "  -p  priority, from 0 to 255\n";
  cout << "  -s  address:[port], set the address and optional port to connect to\n";
  cout << "  -o  add libboblight option, syntax: [light:]option=value\n";
  cout << "  -l  list libboblight options\n";
  cout << "  -i  set the interval in seconds\n";
  cout << "  -u  set the number of pixels/rows to use, default is 16\n";
  cout << "\n";
}  