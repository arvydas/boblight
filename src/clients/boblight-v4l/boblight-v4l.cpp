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
#include <stdint.h>

#define BOBLIGHT_DLOPEN
#include "lib/libboblight.h"
#include "util/misc.h"
#include "flagmanager-v4l.h"
#include "videograbber.h"

CFlagManagerV4l g_flagmanager;

using namespace std;

//debug stuff
void SetupDebugWindow();
Display* dpy;
Window window;
XImage* xim;
GC gc;

int main(int argc, char *argv[])
{
  //load the boblight lib, if it fails we get a char* from dlerror()
  char* boblight_error = boblight_loadlibrary(NULL);
  if (boblight_error)
  {
    PrintError(boblight_error);
    return 1;
  }

  int returnv;

  //try to parse the flags and bitch to stderr if there's an error
  try
  {
    g_flagmanager.ParseFlags(argc, argv);
  }
  catch (string error)
  {
    PrintError(error);
    g_flagmanager.PrintHelpMessage();
    return 1;
  }

  if (g_flagmanager.m_printhelp) //print help message
  {
    g_flagmanager.PrintHelpMessage();
    return 1;
  }

  if (g_flagmanager.m_printboblightoptions) //print boblight options (-o [light:]option=value)
  {
    g_flagmanager.PrintBoblightOptions();
    return 1;
  }

  if (g_flagmanager.m_fork)
  {
    if (fork())
      return 0;
  }

  CVideoGrabber videograbber;

  try
  {
    videograbber.Setup();
  }
  catch(string error)
  {
    PrintError(error);
    return 1;
  }

  videograbber.Run();
  videograbber.Cleanup();
  
  return 1;
}
