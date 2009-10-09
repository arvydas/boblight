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

//debug stuff
#include <X11/Xlib.h>
#include <X11/Xutil.h>

//have to sort out these includes, might not need all of them
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
}

#define BOBLIGHT_DLOPEN
#include "lib/libboblight.h"
#include "util/misc.h"
#include "flagmanager-v4l.h"

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

  av_register_all();
  avdevice_register_all();
  
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

  SetupDebugWindow();
  
  AVFormatParameters formatparams = {};
  AVInputFormat*     inputformat;
  AVFormatContext*   formatcontext;

  formatparams.channel = g_flagmanager.m_channel;
  formatparams.width = g_flagmanager.m_width;
  formatparams.height = g_flagmanager.m_height;
  formatparams.standard = g_flagmanager.m_standard.empty() ? NULL : g_flagmanager.m_standard.c_str();

  inputformat = av_find_input_format("video4linux2");
  if (!inputformat)
    inputformat = av_find_input_format("video4linux");

  if (!inputformat)
  {
    PrintError("didn't find video4linux2 or video4linux input format");
    return 1;
  }

  returnv = av_open_input_file(&formatcontext, g_flagmanager.m_device.c_str(), inputformat, 0, &formatparams);

  if (returnv)
  {
    PrintError("unable to open " + g_flagmanager.m_device);
    return 1;
  }

  if(av_find_stream_info(formatcontext) < 0)
  {
    PrintError("unable to find stream info");
    return 1;
  }

  dump_format(formatcontext, 0, g_flagmanager.m_device.c_str(), false);

  int videostream = -1;
  for (int i = 0; i < formatcontext->nb_streams; i++)
  {
    if (formatcontext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
    {
      videostream = i;
      break;
    }
  }

  if (videostream == -1)
  {
    PrintError("unable to find a video stream");
    return 1;
  }

  AVCodecContext* codeccontext = formatcontext->streams[videostream]->codec;
  AVCodec *codec;

  codec = avcodec_find_decoder(codeccontext->codec_id);
  if (!codec)
  {
    PrintError("unable to find decoder");
    return 1;
  }

  //if (codec->capabilities & CODEC_CAP_TRUNCATED)
    //codeccontext->flags |= CODEC_FLAG_TRUNCATED;

  if (avcodec_open(codeccontext, codec) < 0)
  {
    PrintError("unable to open codec");
    return 1;
  }

  AVPacket pkt;
  AVFrame* inputframe;
  AVFrame* outputframe;
  int64_t  prev;

  inputframe = avcodec_alloc_frame();
  outputframe = avcodec_alloc_frame();

  struct SwsContext *sws = sws_getContext(codeccontext->width, codeccontext->height, codeccontext->pix_fmt, g_flagmanager.m_width, g_flagmanager.m_height, PIX_FMT_RGB24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
  if (!sws)
  {
    PrintError("unable to get sws context");
    return 1;
  }  

  xim = XGetImage(dpy, RootWindow(dpy, DefaultScreen(dpy)), 0, 0, g_flagmanager.m_width, g_flagmanager.m_height, AllPlanes, ZPixmap);

  uint8_t *buffer;
  int numBytes;
  // Determine required buffer size and allocate buffer
  numBytes=avpicture_get_size(PIX_FMT_RGB24, codeccontext->width, codeccontext->height);
  buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
  avpicture_fill((AVPicture *)outputframe, buffer, PIX_FMT_RGB24, codeccontext->width, codeccontext->height);

  
  while(av_read_frame(formatcontext, &pkt) >= 0)
  {
    if (pkt.stream_index == videostream)
    {
      int framefinished;
      avcodec_decode_video(codeccontext, inputframe, &framefinished, pkt.data, pkt.size);

      if (framefinished)
      {
        sws_scale(sws, inputframe->data, inputframe->linesize, 0, codeccontext->height, outputframe->data, outputframe->linesize);
      }
    }

    int rgb[3] = {0, 0, 0};
    int count = 0;
    
    for (int x = 0; x < g_flagmanager.m_width; x++)
    {
      for (int y = 0; y < g_flagmanager.m_height; y++)
      {
        int r = outputframe->data[0][y * outputframe->linesize[0] + x * 3 + 0];
        int g = outputframe->data[0][y * outputframe->linesize[0] + x * 3 + 1];
        int b = outputframe->data[0][y * outputframe->linesize[0] + x * 3 + 2];

        rgb[0] += r;
        rgb[1] += g;
        rgb[2] += b;
        count++;

        int pixel = (r & 0xFF) << 16;
        pixel |= (g & 0xFF) << 8;
        pixel |= b & 0xFF;

        XPutPixel(xim, x, y, pixel);
      }
    }

    XPutImage(dpy, window, gc, xim, 0, 0, 0, 0, g_flagmanager.m_width, g_flagmanager.m_height);
    XSync(dpy, False);
    
    cout <<  AV_TIME_BASE / (double)(pkt.pts - prev) << " " << rgb[0] / count << " " << rgb[1] / count << " " << rgb[2] / count << "\n";

    prev = pkt.pts;
    
    av_free_packet(&pkt);
  }
}

void SetupDebugWindow()
{
  dpy = XOpenDisplay(NULL);
  if (!dpy) throw string("fail");

  window = XCreateSimpleWindow(dpy, RootWindow(dpy, DefaultScreen(dpy)), 0, 0, g_flagmanager.m_width, g_flagmanager.m_height, 0, 0, 0);
  gc = XCreateGC(dpy, window, 0, NULL);

  XMapWindow(dpy, window);
  XSync(dpy, False);


}
