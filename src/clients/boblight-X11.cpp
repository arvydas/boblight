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
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "util/misc.h"
#include "util/timer.h"
#include "util/vblanksignal.h"
#include "config.h"
#include "flags.h"

using namespace std;

bool ParseFlags(int argc, char *argv[], double& interval, int& pixels);
int  Run(vector<string>& options, int priority, char* address, int port, int pixels, double interval);
bool Grabber(void* boblight, int pixels, double interval);
void PrintHelpMessage();
void SignalHandler(int signum);
int  ErrorHandler(Display* dpy, XErrorEvent* error);

volatile bool stop = false;
volatile bool xerror = false;

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

  //set up signal handlers
  signal(SIGTERM, SignalHandler);
  signal(SIGINT, SignalHandler);

  //keeps running until some unrecoverable error happens
  return Run(options, priority, address, port, pixels, interval);

}

bool ParseFlags(int argc, char *argv[], double& interval, int& pixels)
{
  int c;
  optind = 0; //ParseBoblightFlags already did getopt

  while ((c = getopt (argc, argv, "i:u:")) != -1)
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

int Run(vector<string>& options, int priority, char* address, int port, int pixels, double interval)
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

    if (!Grabber(boblight, pixels, interval)) //if grabber returns false we give up
    {
      boblight_destroy(boblight);
      return 1;
    }

    boblight_destroy(boblight);
  }

  cout << "Exiting\n";
  
  return 0;
}

bool Grabber(void* boblight, int pixels, double interval)
{
  Display*          dpy;
  Window            rootwin;
  XWindowAttributes rootattr;
  XImage*           xim;
  unsigned long     pixel;
  int               rgb[3];
  int               usedpixels;
  CTimer            timer;
  CVblankSignal     vblanksignal;

  //positive interval means seconds, negative means vblank interval
  if (interval > 0.0)
  {
    timer.SetInterval(Round<int64_t>(interval * 1000000.0));
  }
  else
  {
    if (!vblanksignal.Setup())
    {
      PrintError(vblanksignal.GetError());
      return false;
    }
  }
  
  dpy = XOpenDisplay(NULL);
  if (dpy == NULL)
  {
    PrintError("Unable to open display");
    return false;
  }

  //set error handler in case we read beyond the dimensions of the display
  XSetErrorHandler(ErrorHandler);
  
  while(!stop)
  {
    rootwin = RootWindow(dpy, DefaultScreen(dpy));
    XGetWindowAttributes(dpy, rootwin, &rootattr);

    //we want at least four pixels
    usedpixels = Min(rootattr.width / 2, rootattr.height / 2, pixels);
    
    boblight_setscanrange(boblight, rootattr.width / usedpixels, rootattr.height / usedpixels);

    for (int y = 0; y < rootattr.height && !stop; y += rootattr.height / usedpixels)
    {
      for (int x = 0; x < rootattr.width && !stop; x += rootattr.width / usedpixels)
      {
        xim = XGetImage(dpy, rootwin, x, y, 1, 1, AllPlanes, ZPixmap);
        if (xerror) //size of the root window probably changed and we read beyond it
        {
          xerror = false;
          sleep(1);
          XGetWindowAttributes(dpy, rootwin, &rootattr);
          continue;
        }
        
        pixel = XGetPixel(xim, 0, 0);
        XDestroyImage(xim);

        rgb[0] = (pixel >> 16) & 0xff;
        rgb[1] = (pixel >>  8) & 0xff;
        rgb[2] = (pixel >>  0) & 0xff;

        boblight_addpixelxy(boblight, x / usedpixels, y / usedpixels, rgb);
      }
    }

    if (!boblight_sendrgb(boblight))
    {
      PrintError(boblight_geterror(boblight));
      return true;
    }

    if (interval > 0.0)
    {
      timer.Wait();
    }
    else
    {
      if (!vblanksignal.Wait(Round<unsigned int>(interval * -1.0)))
      {
        PrintError(vblanksignal.GetError());
        return false;
      }
    }
  }

  XCloseDisplay(dpy);

  return true;
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
  cout << "  -u  set the number of pixels/rows to use, default is 16\n";
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