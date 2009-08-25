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

#include <stdint.h>
#include <iostream>

#include "util/misc.h"
#include "grabber-base.h"

using namespace std;

volatile bool xerror = false;

CGrabber::CGrabber(void* boblight)
{
  m_boblight = boblight;

  m_dpy = NULL;
}

CGrabber::~CGrabber()
{
  if (m_dpy)
  {
    XCloseDisplay(m_dpy);
    m_dpy = NULL;
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