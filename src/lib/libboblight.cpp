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

#include "libboblight.h"
#include "boblight.h"

//C wrapper for C++ class

void* boblight_init()
{
  CBoblight* boblight = new CBoblight;
  return reinterpret_cast<void*>(boblight);
}

void boblight_destroy(void* vpboblight)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  delete boblight;
}

int boblight_connect(void* vpboblight, const char* address, int port, int usectimeout)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  return boblight->Connect(address, port, usectimeout);
}

int boblight_setpriority(void* vpboblight, int priority)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  return boblight->SetPriority(priority);
}

const char* boblight_geterror(void* vpboblight)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  return boblight->GetError();
}

int boblight_getnrlights(void* vpboblight)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  return boblight->GetNrLights();
}

const char* boblight_getlightname(void* vpboblight, int lightnr)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  return boblight->GetLightName(lightnr);
}

int boblight_setspeed(void* vpboblight, int lightnr, float speed)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  return boblight->SetSpeed(lightnr, speed);
}

int boblight_setinterpolation(void* vpboblight, int lightnr, int on)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  return boblight->SetInterpolation(lightnr, on);
}

int boblight_setuse(void* vpboblight, int lightnr, int on)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  return boblight->SetUse(lightnr, on);
}

int boblight_setvalue(void* vpboblight, int lightnr, float value)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  return boblight->SetValue(lightnr, value);
}

int boblight_setsaturation (void* vpboblight, int lightnr, float saturation)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  return boblight->SetSaturation(lightnr, saturation);
}

int boblight_setvaluerange (void* vpboblight, int lightnr, float* valuerange)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  return boblight->SetValueRange(lightnr, valuerange);
}

int boblight_setthreshold(void* vpboblight, int lightnr, int threshold)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  boblight->SetThreshold(lightnr, threshold);
}

void boblight_setscanrange(void* vpboblight, int width, int height)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  boblight->SetScanRange(width, height);
}

void boblight_addpixel(void* vpboblight, int lightnr, int* rgb)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  boblight->AddPixel(lightnr, rgb);
}

void boblight_addpixelxy(void* vpboblight, int x, int y, int* rgb)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  boblight->AddPixel(rgb, x, y);
}

int boblight_sendrgb(void* vpboblight)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  return boblight->SendRGB();
}

int boblight_ping(void* vpboblight)
{
  CBoblight* boblight = reinterpret_cast<CBoblight*>(vpboblight);
  return boblight->Ping();
}