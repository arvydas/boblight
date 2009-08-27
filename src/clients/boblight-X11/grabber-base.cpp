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

#include <assert.h>
#include <stdint.h>
#include <iostream>
#include <X11/Xatom.h>

#include "util/misc.h"
#include "grabber-base.h"

using namespace std;

volatile bool xerror = false;

CGrabber::CGrabber(void* boblight)
{
  m_boblight = boblight;
  m_dpy = NULL;
  m_debug = false;
}

CGrabber::~CGrabber()
{
  if (m_dpy)
  {
    XCloseDisplay(m_dpy);
    m_dpy = NULL;
  }

  if (m_debug)
  {
    XFreeGC(m_debugdpy, m_debuggc);
    XDestroyWindow(m_debugdpy, m_debugwindow);
    XCloseDisplay(m_debugdpy);
    m_debug = false;
  }
}

bool CGrabber::Setup()
{
  m_dpy = XOpenDisplay(NULL);
  if (m_dpy == NULL)
  {
    m_error = "unable to open display";
    return false;
  }

  m_rootwin = RootWindow(m_dpy, DefaultScreen(m_dpy));
  UpdateDimensions();

  if (m_interval > 0.0)
  {
    m_timer.SetInterval(Round<int64_t>(m_interval * 1000000.0));
  }
  else
  {
    if (!m_vblanksignal.Setup())
    {
      m_error = m_vblanksignal.GetError();
      return false;
    }
  }

  m_error.clear();

  return ExtendedSetup();
}

bool CGrabber::ExtendedSetup()
{
  return true;
}

void CGrabber::UpdateDimensions()
{
  XGetWindowAttributes(m_dpy, m_rootwin, &m_rootattr);
}

bool CGrabber::Run(volatile bool& stop)
{
  return false;
}

bool CGrabber::Wait()
{
  if (m_interval > 0.0)
  {
    m_timer.Wait();
  }
  else
  {
    if (!m_vblanksignal.Wait(Round<unsigned int>(m_interval * -1.0)))
    {
      m_error = m_vblanksignal.GetError();
      return false;
    }
  }

  return true;
}

void CGrabber::SetDebug(const char* display)
{
  //set up a display connection, a window and a gc so we can show what we're capturing
  
  m_debugdpy = XOpenDisplay(display);
  assert(m_debugdpy);
  m_debugwindowwidth = Max(200, m_size);
  m_debugwindowheight = Max(200, m_size);
  m_debugwindow = XCreateSimpleWindow(m_debugdpy, RootWindow(m_debugdpy, DefaultScreen(m_debugdpy)),
                                      0, 0, m_debugwindowwidth, m_debugwindowheight, 0, 0, 0);
  XMapWindow(m_debugdpy, m_debugwindow);
  XSync(m_debugdpy, False);

  m_debuggc = XCreateGC(m_debugdpy, m_debugwindow, 0, NULL);

  m_lastupdate = m_fpsclock.GetSecTime();
  m_lastmeasurement = m_lastupdate;
  m_measurements = 0.0;
  m_nrmeasurements = 0.0;
  
  m_debug = true;
}

void CGrabber::UpdateDebugFps()
{
  if (m_debug)
  {
    long double now = m_fpsclock.GetSecTime();
    long double fps = 1.0 / (now - m_lastmeasurement);
    m_measurements += fps;
    m_nrmeasurements++;
    m_lastmeasurement = now;

    if (now - m_lastupdate >= 1.0)
    {
      m_lastupdate = now;
      string strfps = ToString(m_measurements / m_nrmeasurements) + " fps";
      m_measurements = 0.0;
      m_nrmeasurements = 0;

      XTextProperty property;
      property.value = reinterpret_cast<unsigned char*>(const_cast<char*>(strfps.c_str()));
      property.encoding = XA_STRING;
      property.format = 8;
      property.nitems = strfps.length();

      XSetWMName(m_debugdpy, m_debugwindow, &property);
    }
  }
}
