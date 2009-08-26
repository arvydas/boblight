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

using namespace std;

#include "grabber-xgetimage.h"
#include "util/misc.h"

#define BOBLIGHT_DLOPEN_EXTERN
#include "lib/libboblight.h"

CGrabberXGetImage::CGrabberXGetImage(void* boblight) : CGrabber(boblight)
{
}

CGrabberXGetImage::~CGrabberXGetImage()
{
  if (m_debug)
    XDestroyImage(m_debugxim);
}

bool CGrabberXGetImage::ExtendedSetup()
{
  //set up an ximage of m_size*m_size when debug mode is enabled
  if (m_debug)
    m_debugxim = XGetImage(m_debugdpy, RootWindow(m_debugdpy, DefaultScreen(m_debugdpy)), 0, 0, m_size, m_size, AllPlanes, ZPixmap);
}

bool CGrabberXGetImage::Run(volatile bool& stop)
{
  XImage* xim;
  unsigned long pixel;
  int rgb[3];

  boblight_setscanrange(m_boblight, m_size, m_size);

  while(!stop)
  {
    UpdateDimensions();

    for (int y = 0; y < m_size && !stop; y++)
    {
      for (int x = 0; x < m_size && !stop; x++)
      {
        int xpos = (m_rootattr.width  - 1) * x / m_size;
        int ypos = (m_rootattr.height - 1) * y / m_size;
        
        xim = XGetImage(m_dpy, m_rootwin, xpos, ypos, 1, 1, AllPlanes, ZPixmap);
        if (xerror) //size of the root window probably changed and we read beyond it
        {
          xerror = false;
          sleep(1);
          UpdateDimensions();
          continue;
        }

        pixel = XGetPixel(xim, 0, 0);
        XDestroyImage(xim);

        rgb[0] = (pixel >> 16) & 0xff;
        rgb[1] = (pixel >>  8) & 0xff;
        rgb[2] = (pixel >>  0) & 0xff;

        boblight_addpixelxy(m_boblight, x, y, rgb);

        //put pixel on debug image
        if (m_debug)
          XPutPixel(m_debugxim, x, y, pixel);
      }
    }

    //put debug image on debug window
    if (m_debug)
    {
      XPutImage(m_debugdpy, m_debugwindow, m_debuggc, m_debugxim, 0, 0, 0, 0, m_size, m_size);
      XSync(m_debugdpy, False);
    }

    if (!boblight_sendrgb(m_boblight))
    {
      m_error = boblight_geterror(m_boblight);
      return true;
    }

    if (!Wait())
    {
      m_error = m_vblanksignal.GetError();
      return false;
    }
  }

  m_error.clear();
  
  return true;
}