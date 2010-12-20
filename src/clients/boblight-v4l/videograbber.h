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

#ifndef VIDEOGRABBER
#define VIDEOGRABBER

//avutil.h needs this
#define __STDC_CONSTANT_MACROS

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

#include <string>
#include <stdint.h>

class CVideoGrabber
{
  public:
    CVideoGrabber();
    ~CVideoGrabber();

    void Setup();
    void Run(volatile bool& stop, void* boblight);
    void Cleanup();

    std::string GetError() { return m_error; }
    
  private:
    AVFormatParameters m_formatparams;
    AVFormatContext*   m_formatcontext;
    int                m_videostream;

    AVCodecContext*    m_codeccontext;
    AVCodec*           m_codec;

    AVFrame*           m_inputframe;
    AVFrame*           m_outputframe;

    SwsContext*        m_sws;
    uint8_t*           m_framebuffer;
    bool               m_needsscale;

    std::string        m_error;

    int                m_fd;
    
    Display*           m_dpy;
    Window             m_window;
    XImage*            m_xim;
    GC                 m_gc;

    bool               CheckSignal();
    void               FrameToBoblight(uint8_t* outputptr, int linesize, void* boblight);
};

#endif //VIDEOGRABBER
