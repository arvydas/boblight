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

#ifndef GRABBER
#define GRABBER

#include <string>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "util/timer.h"
#include "vblanksignal.h"
#include "util/clock.h"

extern volatile bool xerror;

class CGrabber
{
  public:
    CGrabber(void* boblight);
    ~CGrabber();

    std::string& GetError() { return m_error; }
    
    void SetInterval(double interval) { m_interval = interval; }
    void SetSize(int size)            { m_size = size; }

    bool Setup();
    virtual bool ExtendedSetup();
    virtual bool Run(volatile bool& stop);

    void SetDebug(const char* display);
    
  protected:

    void              UpdateDimensions();
    bool              Wait();

    std::string       m_error;
    
    void*             m_boblight;
    
    Display*          m_dpy;
    Window            m_rootwin;
    XWindowAttributes m_rootattr;
    int               m_size;

    bool              m_debug;
    Display*          m_debugdpy;
    Window            m_debugwindow;
    int               m_debugwindowwidth;
    int               m_debugwindowheight;
    GC                m_debuggc;

    void              UpdateDebugFps();
    CClock            m_fpsclock;
    long double       m_lastupdate;
    long double       m_lastmeasurement;
    long double       m_measurements;
    int               m_nrmeasurements;
    
    double            m_interval;
    CTimer            m_timer;
    CVblankSignal     m_vblanksignal;    
    
};

#endif //GRABBER