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

#include <string.h>

#include "util/misc.h"
#include "grabber-xrender.h"

#define BOBLIGHT_DLOPEN_EXTERN
#include "lib/libboblight.h"

using namespace std;

CGrabberXRender::CGrabberXRender(void* boblight) : CGrabber(boblight)
{
  m_srcformat = NULL;
  m_dstformat = NULL;
  m_pixmap = NULL;
  m_srcpicture = None;
  m_dstpicture = None;

  memset(&m_pictattr, 0, sizeof(m_pictattr));
  m_pictattr.repeat = RepeatNone;

  //m_transform.matrix[0][0] = Round<int>((double)root_window_attributes.width / (double)pixels * 65536.0);
  m_transform.matrix[0][0] = 65536;
  m_transform.matrix[0][1] = 0;
  m_transform.matrix[0][2] = 0;
  m_transform.matrix[1][0] = 0;
  //m_transform.matrix[1][1] = Round<int>((double)root_window_attributes.height / (double)pixels * 65536.0);
  m_transform.matrix[1][1] = 65536;
  m_transform.matrix[1][2] = 0;
  m_transform.matrix[2][0] = 0;
  m_transform.matrix[2][1] = 0;
  m_transform.matrix[2][2] = 65536;
}

CGrabberXRender::~CGrabberXRender()
{
  //TODO: fix memleak
}

bool CGrabberXRender::ExtendedSetup()
{
  if (!CheckExtensions())
    return false;

  m_pixmap = XCreatePixmap(m_dpy, m_rootwin, m_size, m_size, m_rootattr.depth);

  m_srcformat = XRenderFindVisualFormat(m_dpy, m_rootattr.visual);
  m_dstformat = XRenderFindVisualFormat(m_dpy, m_rootattr.visual); //how the hell do I get a visual from a pixmap?

  m_srcpicture = XRenderCreatePicture(m_dpy, m_rootwin, m_srcformat, CPRepeat, &m_pictattr);
  m_dstpicture = XRenderCreatePicture(m_dpy, m_pixmap,  m_dstformat, CPRepeat, &m_pictattr);

  XRenderSetPictureFilter (m_dpy, m_srcpicture, "bilinear", NULL, 0);

  return true;
}

bool CGrabberXRender::CheckExtensions()
{
  int render_event_base;
  int render_error_base;
  int render_major_ver;
  int render_minor_ver;

  if (!XRenderQueryExtension(m_dpy, &render_event_base, &render_error_base))
  {
    m_error = "XRender extension not found";
    return false;
  }
  if (!XRenderQueryVersion (m_dpy, &render_major_ver, &render_minor_ver))
  {
    m_error = "Unable to get XRender version";
    return false;
  }
  if (render_major_ver != 0 || render_minor_ver < 10)
  {
    m_error = "Incompatible XRender extension"; //TODO: verify this
    return false;
  }

  return true;
}

bool CGrabberXRender::Run(volatile bool& stop)
{
  XImage* xim;
  unsigned long pixel;
  int rgb[3];
  
  while(!stop)
  {
    UpdateDimensions();
    boblight_setscanrange(m_boblight, m_size, m_size);
    
    m_transform.matrix[0][0] = Round<int>((double)m_rootattr.width / (double)m_size * 65536.0);
    m_transform.matrix[1][1] = Round<int>((double)m_rootattr.height / (double)m_size * 65536.0);
    XRenderSetPictureTransform (m_dpy, m_srcpicture, &m_transform);
    
    XRenderComposite(m_dpy, PictOpSrc, m_srcpicture, None, m_dstpicture, 0, 0, 0, 0, 0, 0, m_size, m_size);
    XSync(m_dpy, False);

    usleep(1000);
    
    xim = XGetImage(m_dpy, m_pixmap, 0, 0, m_size, m_size, AllPlanes, ZPixmap);

    for (int y = 0; y < m_size && !stop; y++)
    {
      for (int x = 0; x < m_size && !stop; x++)
      {
        pixel = XGetPixel(xim, x, y);
        
        rgb[0] = (pixel >> 16) & 0xff;
        rgb[1] = (pixel >>  8) & 0xff;
        rgb[2] = (pixel >>  0) & 0xff;

        boblight_addpixelxy(m_boblight, x, y, rgb);
      }
    }

    XDestroyImage(xim);
    
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
    usleep(7000);
  }

  m_error.clear();
  
  return true;
}