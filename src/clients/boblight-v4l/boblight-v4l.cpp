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

  av_register_all();
  avdevice_register_all();

  AVFormatParameters formatparams = {};
  AVInputFormat*     inputformat;
  AVFormatContext*   formatcontext;

  formatparams.channel = 1;
  formatparams.width = 64;
  formatparams.height = 64;
  formatparams.standard = "ntsc";

  inputformat = av_find_input_format("video4linux2");
  if (!inputformat)
    inputformat = av_find_input_format("video4linux");

  if (!inputformat)
  {
    PrintError("didn't find video4linux2 or video4linux input formats");
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

  if (codec->capabilities & CODEC_CAP_TRUNCATED)
    codeccontext->flags |= CODEC_FLAG_TRUNCATED;

  if (avcodec_open(codeccontext, codec) < 0)
  {
    PrintError("unable to open codec");
    return 1;
  }

  AVPacket pkt;
  AVFrame* frame;
  int64_t  prev;

  frame = avcodec_alloc_frame();

  struct SwsContext *sws = sws_getContext(codeccontext->width, codeccontext->height, codeccontext->pix_fmt, 64, 64, PIX_FMT_RGB24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
  if (!sws)
  {
    PrintError("unable to get sws context");
    return 1;
  }  

  uint8_t* output[64];
  int stride[64];

  for (int i = 0; i < 64; i++)
  {
    output[i] = new uint8_t[64];
    stride[i] = 64;
  }
  
  while(av_read_frame(formatcontext, &pkt) >= 0)
  {
    if (pkt.stream_index == videostream)
    {
      int framefinished;
      avcodec_decode_video(codeccontext, frame, &framefinished, pkt.data, pkt.size);

      if (framefinished)
      {
        sws_scale(sws, frame->data, frame->linesize, 0, codeccontext->height, output, stride);
      }
    }

    int rgb[3] = {0, 0, 0};
    int count = 0;
    
    for (int x = 0; x < 64; x++)
    {
      for (int y = 0; y < 64; y++)
      {
        rgb[0] += output[y][x * 3 + 0];
        rgb[1] += output[y][x * 3 + 1];
        rgb[2] += output[y][x * 3 + 2];
        count++;
      }
    }

    cout << rgb[0] / count << " " << rgb[1] / count << " " << rgb[2] / count << "\n";
    
    av_free_packet(&pkt);
  }
}
