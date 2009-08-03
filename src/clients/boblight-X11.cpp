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

#include "util/timer.h"

using namespace std;

int main (int argc, char *argv[])
{
  char* boblight_error = boblight_loadlibrary(NULL);
  if (boblight_error)
  {
    cout << boblight_error << "\n";
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
  boblight_setinterpolation(boblight, -1, 1);
  boblight_setvalue(boblight, -1, 10.0f);
  boblight_setsaturation(boblight, -1, 5.0f);
  boblight_setspeed(boblight, -1, 5.0f);
  
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