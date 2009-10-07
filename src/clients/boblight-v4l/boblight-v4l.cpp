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
    PrintError("Unable to open " + g_flagmanager.m_device);
    return 1;
  }

  AVPacket pkt;

  while(av_read_frame(formatcontext, &pkt) >= 0)
  {
    cout << "test\n";

    av_free_packet(&pkt);
  }
}
