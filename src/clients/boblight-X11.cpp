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
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "util/misc.h"
#include "util/timer.h"
#include "config.h"
#include "flags.h"

using namespace std;

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


  
  void*             boblight;
  CAsyncTimer       timer;

  Display*          dpy;
  Window            rootwin;
  XWindowAttributes rootattr;
  XImage*           xim;
  unsigned long     pixel;
  int               rgb[3];

  dpy = XOpenDisplay(NULL);
  if (dpy == NULL)
  {
    cout << "Unable to open display\n";
    return 1;
  }
  
  boblight = boblight_init();

  if (!boblight_connect(boblight, NULL, -1, 10000000))
  {
    cout << boblight_geterror(boblight) << "\n";
    boblight_destroy(boblight);
    return 1;
  }

  boblight_setpriority(boblight, 128);
  boblight_setoption(boblight, -1, "interpolation 1");
  boblight_setoption(boblight, -1, "value 10.0");
  boblight_setoption(boblight, -1, "saturation 5.0");
  boblight_setoption(boblight, -1, "speed 5.0");
  
  timer.SetInterval(40000);
  timer.StartTimer();

  while(1)
  {
    rootwin = RootWindow(dpy, DefaultScreen(dpy));
    XGetWindowAttributes(dpy, rootwin, &rootattr);

    boblight_setscanrange(boblight, rootattr.width / 16, rootattr.height / 16);

    for (int y = 0; y < rootattr.height; y += rootattr.height / 16)
    {
      for (int x = 0; x < rootattr.width; x += rootattr.width / 16)
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
      cout << boblight_geterror(boblight) << "\n";
      boblight_destroy(boblight);
      return 1;
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
  cout << "\n";
}  