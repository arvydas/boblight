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

#ifndef CLIGHT
#define CLIGHT

#include <stdint.h>
#include <string>
#include <vector>
#include <string.h>

#include "util/misc.h"

class CColor
{
  public:
    CColor();

    void        SetName(std::string name) { m_name = name; }
    std::string GetName()                 { return m_name; }

    void SetGamma(float gamma)           { m_gamma = gamma; }
    void SetAdjust(float adjust)         { m_adjust = adjust; }
    void SetBlacklevel(float blacklevel) { m_blacklevel = blacklevel; }
    void SetRgb(float (&rgb)[3])         { memcpy(m_rgb, rgb, sizeof(m_rgb)); }
    
    float  GetGamma()      { return m_gamma; }
    float  GetAdjust()     { return m_adjust; }
    float  GetBlacklevel() { return m_blacklevel; }
    float* GetRgb()        { return m_rgb; }
    
  private:
    std::string m_name;
    
    float m_rgb[3];
    float m_gamma;
    float m_adjust;
    float m_blacklevel;
};
    
struct point
{
  int x;
  int y;
};

class CLight
{
  public:
    CLight();

    void        SetName(std::string name)      { m_name = name; }
    std::string GetName()                      { return m_name; }
    
    void  SetRgb(float (&rgb)[3], int64_t time);

    void  SetUse(bool use)                     { m_use = use; }
    void  SetInterpolation(bool interpolation) { m_interpolation = interpolation; }
    void  SetSpeed(float speed)                { m_speed = Clamp(speed, 0.0, 100.0); }
    bool  GetUse()                             { return m_use; }
    bool  GetInterpolation()                   { return m_interpolation; }
    float GetSpeed()                           { return m_speed; }

    void  AddColor(CColor& color)              { m_colors.push_back(color); }
    int   GetNrColors()                        { return m_colors.size(); };
    
    float GetGamma(int colornr)                { return m_colors[colornr].GetGamma(); }
    float GetAdjust(int colornr)               { return m_colors[colornr].GetAdjust(); }
    float GetBlacklevel(int colornr)           { return m_colors[colornr].GetBlacklevel(); }
    float GetColorValue(int colornr, int64_t time);

    void  SetArea(std::vector<point>& area)   { m_area = area; };
    std::vector<point>& GetArea()             { return m_area; };
    
  private:
    std::string m_name;
    
    int64_t m_time;        //current write time
    int64_t m_prevtime;    //previous write time

    float   m_rgb[3];
    float   m_prevrgb[3];
    float   m_speed;

    bool    m_interpolation;
    bool    m_use;

    std::vector<CColor> m_colors;
    std::vector<point>  m_area;

    float   FindMultiplier(float *rgb, float ceiling);
    float   FindMultiplier(float *rgb, float *ceiling);
};

#endif //CLIGHT