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

bool CGrabberXGetImage::Run(volatile bool& stop)
{
  XImage* xim;
  unsigned long pixel;
  int rgb[3];

  while(!stop)
  {
    UpdateDimensions();
    boblight_setscanrange(m_boblight, m_rootattr.width, m_rootattr.height);

    for (int y = 0; y < m_rootattr.height && !stop; y += m_rootattr.height / m_size)
    {
      for (int x = 0; x < m_rootattr.width && !stop; x += m_rootattr.width / m_size)
      {
        xim = XGetImage(m_dpy, m_rootwin, x, y, 1, 1, AllPlanes, ZPixmap);
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
      }
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