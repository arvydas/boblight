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

#include "flagmanager-v4l.h"
#include "videograbber.h"

#define BOBLIGHT_DLOPEN_EXTERN
#include "lib/libboblight.h"

#include <string.h>

extern CFlagManagerV4l g_flagmanager;

CVideoGrabber::CVideoGrabber()
{
  av_register_all();
  avdevice_register_all();

  memset(&m_formatparams, 0, sizeof(m_formatparams));
  m_inputformat = NULL;
  m_formatcontext = NULL;
  m_codeccontext = NULL;
  m_codec = NULL;
  m_inputframe = NULL;
  m_outputframe = NULL;
  m_sws = NULL;
  m_framebuffer = NULL;
}

CVideoGrabber::~CVideoGrabber()
{
}

void CVideoGrabber::Setup()
{
  
}
