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
#include <iostream>

extern CFlagManagerV4l g_flagmanager;

using namespace std;

CVideoGrabber::CVideoGrabber()
{
  av_register_all();
  avdevice_register_all();
  av_log_set_level(AV_LOG_DEBUG);

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
  
  m_dpy = NULL;
}

CVideoGrabber::~CVideoGrabber()
{
  Cleanup();
}

void CVideoGrabber::Setup()
{
  int returnv;

  memset(&m_formatparams, 0, sizeof(m_formatparams));

  m_formatparams.channel = g_flagmanager.m_channel;
  m_formatparams.width = g_flagmanager.m_width;
  m_formatparams.height = g_flagmanager.m_height;
  m_formatparams.standard = g_flagmanager.m_standard.empty() ? NULL : g_flagmanager.m_standard.c_str();
  m_formatparams.pix_fmt = PIX_FMT_BGR24;

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
                         g_flagmanager.m_width, g_flagmanager.m_height, PIX_FMT_BGR24, SWS_FAST_BILINEAR, NULL, NULL, NULL);

  if (!m_sws)
    throw string("Unable to get sws context");

  int buffsize = avpicture_get_size(PIX_FMT_RGB24, g_flagmanager.m_width, g_flagmanager.m_height);
  m_framebuffer = (uint8_t*)av_malloc(buffsize);

  avpicture_fill((AVPicture *)m_outputframe, m_framebuffer, PIX_FMT_BGR24, g_flagmanager.m_width, g_flagmanager.m_height);

  if (g_flagmanager.m_debug)
  {
    m_dpy = XOpenDisplay(g_flagmanager.m_debugdpy);
    if (!m_dpy)
      throw string("Unable to open display");

    m_window = XCreateSimpleWindow(m_dpy, RootWindow(m_dpy, DefaultScreen(m_dpy)), 0, 0,
                                   g_flagmanager.m_width, g_flagmanager.m_height, 0, 0, 0);
    m_gc = XCreateGC(m_dpy, m_window, 0, NULL);

    //lazy way of creating an ximage
    m_xim = XGetImage(m_dpy, RootWindow(m_dpy, DefaultScreen(m_dpy)), 0, 0, g_flagmanager.m_width,
                      g_flagmanager.m_height, AllPlanes, ZPixmap);
    
    XMapWindow(m_dpy, m_window);
    XSync(m_dpy, False);
  }
}

void CVideoGrabber::Run()
{
  AVPacket pkt;
  int      nrpixels = g_flagmanager.m_width * g_flagmanager.m_height;

  while(av_read_frame(m_formatcontext, &pkt) >= 0) //read videoframe
  {
    if (pkt.stream_index == m_videostream)
    {
      int framefinished;
      avcodec_decode_video(m_codeccontext, m_inputframe, &framefinished, pkt.data, pkt.size);

      if (framefinished)
      {
        sws_scale(m_sws, m_inputframe->data, m_inputframe->linesize, 0, m_codeccontext->height, m_outputframe->data, m_outputframe->linesize);

        int      rgb[3] = {0, 0, 0};
        uint8_t* buffptr;
        
        for (int y = 0; y < g_flagmanager.m_height; y++)
        {
          buffptr = m_framebuffer + m_outputframe->linesize[0] * y;
          for (int x = 0; x < g_flagmanager.m_width; x++)
          {
            int b = *(buffptr++);
            int g = *(buffptr++);
            int r = *(buffptr++);

            rgb[0] += r;
            rgb[1] += g;
            rgb[2] += b;

            if (m_dpy)
            {
              int pixel;
              pixel  = (r & 0xFF) << 16;
              pixel |= (g & 0xFF) << 8;
              pixel |=  b & 0xFF;

              //I'll probably get the annual inefficiency award for this
              XPutPixel(m_xim, x, y, pixel);
            }
          }
        }

        if (m_dpy)
        {
          XPutImage(m_dpy, m_window, m_gc, m_xim, 0, 0, 0, 0, g_flagmanager.m_width, g_flagmanager.m_height);
          XSync(m_dpy, False);
        }

        cout << rgb[0] / nrpixels << " " << rgb[1] / nrpixels << " " << rgb[2] / nrpixels << "\n";
      }
    }

    av_free_packet(&pkt);
  }
}

void CVideoGrabber::Cleanup()
{
  if (m_dpy)
  {
    XDestroyImage(m_xim);
    XDestroyWindow(m_dpy, m_window);
    XFreeGC(m_dpy, m_gc);
    XCloseDisplay(m_dpy);
    m_dpy = NULL;
  }

  if (m_framebuffer)
  {
    av_free(m_framebuffer);
    m_framebuffer = NULL;
  }

  if (m_sws)
  {
    sws_freeContext(m_sws);
    m_sws = NULL;
  }

  if (m_inputframe)
  {
    av_free(m_inputframe);
    m_inputframe = NULL;
  }

  if (m_outputframe)
  {
    av_free(m_outputframe);
    m_outputframe = NULL;
  }

  if (m_codeccontext)
  {
    avcodec_close(m_codeccontext);
    m_codeccontext = NULL;
  }

  if (m_formatcontext)
  {
    av_close_input_file(m_formatcontext);
    m_formatcontext = NULL;
  }
}
