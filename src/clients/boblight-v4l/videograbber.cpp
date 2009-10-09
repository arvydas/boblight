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

using namespace std;

CVideoGrabber::CVideoGrabber()
{
  av_register_all();
  avdevice_register_all();

  memset(&m_formatparams, 0, sizeof(m_formatparams));
  m_inputformatv4l = NULL;
  m_inputformatv4l2 = NULL;
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
  int returnv;
  
  memset(&m_formatparams, 0, sizeof(m_formatparams));

  m_formatparams.channel = g_flagmanager.m_channel;
  m_formatparams.width = g_flagmanager.m_width;
  m_formatparams.height = g_flagmanager.m_height;
  m_formatparams.standard = g_flagmanager.m_standard.empty() ? NULL : g_flagmanager.m_standard.c_str();

  m_inputformatv4l  = av_find_input_format("video4linux");
  m_inputformatv4l2 = av_find_input_format("video4linux2");
  if (!m_inputformatv4l && !m_inputformatv4l2)
  {
    throw string("Didn't find video4linux2 or video4linux input format");
  }

  returnv = -1;
  if (m_inputformatv4l2)
  {
    returnv = av_open_input_file(&m_formatcontext, g_flagmanager.m_device.c_str(), m_inputformatv4l2, 0, &m_formatparams);
  }

  if (returnv)
  {
    if (m_inputformatv4l)
    {
      returnv = av_open_input_file(&m_formatcontext, g_flagmanager.m_device.c_str(), m_inputformatv4l, 0, &m_formatparams);
    }

    if (returnv)
    {
      throw string ("Unable to open " + g_flagmanager.m_device);
    }
  }

  if(av_find_stream_info(m_formatcontext) < 0)
    throw string ("Unable to find stream info");

  dump_format(m_formatcontext, 0, g_flagmanager.m_device.c_str(), false);

  m_videostream = -1;
  for (int i = 0; i < m_formatcontext->nb_streams; i++)
  {
    if (m_formatcontext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
    {
      m_videostream = i;
      break;
    }
  }

  if (m_videostream == -1)
    throw string("Unable to find a video stream");

  m_codeccontext = m_formatcontext->streams[m_videostream]->codec;
  m_codec = avcodec_find_decoder(m_codeccontext->codec_id);

  if (!m_codec)
    throw string("Unable to find a codec");

  returnv = avcodec_open(m_codeccontext, m_codec);
  if (returnv < 0)
    throw string("Unable to open codec");

  m_inputframe = avcodec_alloc_frame();
  m_outputframe = avcodec_alloc_frame();

  m_sws = sws_getContext(m_codeccontext->width, m_codeccontext->height, m_codeccontext->pix_fmt, 
                         g_flagmanager.m_width, g_flagmanager.m_height, PIX_FMT_RGB24, SWS_POINT, NULL, NULL, NULL);

  if (!m_sws)
    throw string("Unable to get sws context");

  int buffsize = avpicture_get_size(PIX_FMT_RGB24, g_flagmanager.m_width, g_flagmanager.m_height);
  m_framebuffer = (uint8_t*)av_malloc(buffsize);

  avpicture_fill((AVPicture *)m_outputframe, m_framebuffer, PIX_FMT_RGB24, g_flagmanager.m_width, g_flagmanager.m_height);
}
