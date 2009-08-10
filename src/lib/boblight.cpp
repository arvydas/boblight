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

#include <string>
#include <stdint.h>
#include <iostream> //debug

#include "boblight.h"
#include "util/misc.h"
#include "protocolversion.h"

using namespace std;

CLight::CLight()
{
  m_speed = 100.0;
  m_interpolation = false;
  m_use = true;
  m_value = 1.0;
  m_saturation = 1.0;
  m_valuerange[0] = 0.0;
  m_valuerange[1] = 1.0;
  m_threshold = 0;

  m_width = -1;
  m_height = -1;

  memset(m_rgbd, 0, sizeof(m_rgbd));
}

void CLight::GetRGB(float* rgb)
{
  if (m_rgbd[3] == 0)
  {
    for (int i = 0; i < 3; i++)
      rgb[i] = 0.0f;

    memset(m_rgbd, 0, sizeof(m_rgbd));
    return;
  }
  
  for (int i = 0; i < 3; i++)
    rgb[i] = Clamp((float)m_rgbd[i] / (float)m_rgbd[3] / 255.0f, 0.0f, 1.0f);

  memset(m_rgbd, 0, sizeof(m_rgbd));
  //return;

  //we need some hsv adjustments
  if (m_value != 1.0 || m_saturation != 1.0 || m_valuerange[0] != 0.0 || m_valuerange[1] != 1.0)
  {
    //rgb - hsv conversion, thanks wikipedia!
    float hsv[3];
    float max = Max(rgb[0], rgb[1], rgb[2]);
    float min = Min(rgb[0], rgb[1], rgb[2]);

    if (min == max) //grayscale
    {
      hsv[0] = -1.0f; //undefined
      hsv[1] = 0.0; //no saturation
      hsv[2] = min; //value
    }
    else
    {
      if (max == rgb[0]) //red zone
      {
        hsv[0] = (60.0f * ((rgb[1] - rgb[2]) / (max - min)) + 360.0f);
        while (hsv[0] > 360.0f)
          hsv[0] -= 360.0f;
      }
      else if (max == rgb[1]) //green zone
      {
        hsv[0] = 60.0f * ((rgb[2] - rgb[1]) / (max - min)) + 120.0f;
      }
      else if (max == rgb[2]) //blue zone
      {
        hsv[0] = 60.0f * ((rgb[0] - rgb[1]) / (max - min)) + 240.0f;
      }
      
      hsv[1] = (max - min) / max; //saturation
      hsv[2] = max; //value
    }

    //saturation and value adjustment
    hsv[1] = Clamp(hsv[1] * m_saturation, 0.0f, 1.0f);
    hsv[2] = Clamp(hsv[2] * m_value, m_valuerange[0], m_valuerange[1]);

    if (hsv[0] == -1.0f) //grayscale
    {
      for (int i = 0; i < 3; i++)
        rgb[i] = hsv[2];
    }
    else
    {
      int hi = (int)(hsv[0] / 60.0f) % 6;
      float f = (hsv[0] / 60.0f) - (float)(int)(hsv[0] / 60.0f);

      float v = hsv[2];
      float p = v * (1.0f - hsv[1]);
      float q = v * (1.0f - f * v);
      float t = v * (1.0f - (1.0f - f) * v);

      if (hi == 0)
      { rgb[0] = v; rgb[1] = t; rgb[2] = p; }
      else if (hi == 1)
      { rgb[0] = q; rgb[1] = v; rgb[2] = p; }
      else if (hi == 2)
      { rgb[0] = p; rgb[1] = v; rgb[2] = t; }
      else if (hi == 3)
      { rgb[0] = p; rgb[1] = q; rgb[2] = v; }
      else if (hi == 4)
      { rgb[0] = t; rgb[1] = p; rgb[2] = v; }
      else if (hi == 5)
      { rgb[0] = v; rgb[1] = p; rgb[2] = q; }               
    }

    for (int i = 0; i < 3; i++)
      rgb[i] = Clamp(rgb[i], 0.0f, 1.0f);
  }
}

int CBoblight::Connect(const char* address, int port, int usectimeout)
{
  CMessage message;
  CTcpData data;
  int64_t  now;
  int64_t  target;
  string   word;

  //set address
  m_usectimeout = usectimeout;
  if (address == NULL)
    m_address = "127.0.0.1";
  else
    m_address = address;

  //set port
  if (port >= 0)
    m_port = port;
  else
    m_port = 19333;

  //try to open a tcp connection
  if (m_socket.Open(m_address, m_port, m_usectimeout) != SUCCESS)
  {
    m_error = m_socket.GetError();
    return 0;
  }

  //write hello to the server, we should get hello back
  if (!WriteDataToSocket("hello\n"))
    return 0;

  if (!ReadDataToQueue())
    return 0;

  message = m_messagequeue.GetMessage();
  if (!ParseWord(message, "hello"))
  {
    m_error = m_address + ":" + ToString(m_port) + " sent gibberish";
    return 0;
  }

  //get the protocol version from the server
  if (!WriteDataToSocket("get version\n"))
    return 0;

  if (!ReadDataToQueue())
    return 0;

  message = m_messagequeue.GetMessage();
  
  if (!ParseWord(message, "version") || !GetWord(message.message, word))
  {
    m_error = m_address + ":" + ToString(m_port) + " sent gibberish";
    return 0;
  }

  //if we don't get the same protocol version back as we have, we can't work together
  if (word != PROTOCOLVERSION)
  {
    m_error = "version mismatch, " + m_address + ":" + ToString(m_port) + " has version \"" + message.message +
              "\", libboblight has version \"" + PROTOCOLVERSION + "\"";
    return 0;
  }

  //get lights info, like number, names and area
  if (!WriteDataToSocket("get lights\n"))
    return 0;

  if (!ReadDataToQueue())
    return 0;

  message = m_messagequeue.GetMessage();
  if (!ParseLights(message))
  {
    m_error = m_address + ":" + ToString(m_port) + " sent gibberish";
    return 0;
  }
  
  return 1;
}

//reads from socket until timeout or one message has arrived
bool CBoblight::ReadDataToQueue()
{
  CTcpData data;
  int64_t  now = m_clock.GetTime();
  int64_t  target = now + m_usectimeout * m_clock.GetFreq() / 1000000;
  int      nrmessages = m_messagequeue.GetNrMessages();

  while (now < target && m_messagequeue.GetNrMessages() == nrmessages)
  {
    if (m_socket.Read(data) != SUCCESS)
    {
      m_error = m_socket.GetError();
      return false;
    }

    m_messagequeue.AddData(data.GetData());

    now = m_clock.GetTime();
  }

  if (nrmessages == m_messagequeue.GetNrMessages())
  {
    m_error = m_address + ":" + ToString(m_port) + " read timed out";
    return false;
  }
  return true;
}

bool CBoblight::WriteDataToSocket(std::string strdata)
{
  CTcpData data;
  data.SetData(strdata);

  if (m_socket.Write(data) != SUCCESS)
  {
    m_error = m_socket.GetError();
    return false;
  }
  
  return true;
}

//TODO:need to clean this, doesn't make sense this way
bool CBoblight::ParseWord(CMessage& message, std::string wordtocmp)
{
  string word;
  return ParseWord(message, wordtocmp, word);
}

bool CBoblight::ParseWord(CMessage& message, std::string wordtocmp, std::string readword)
{
  if (!GetWord(message.message, readword) || readword != wordtocmp)
    return false;

  return true;
}

bool CBoblight::ParseLights(CMessage& message)
{
  string word;
  int nrlights;

  //first word in the message is "lights", second word is the number of lights
  if (!ParseWord(message, "lights") || !GetWord(message.message, word) || !StrToInt(word, nrlights) || nrlights < 1)
    return false;

  for (int i = 0; i < nrlights; i++)
  {
    CLight light;

    //read some data to the message queue if we have no messages
    if (m_messagequeue.GetNrMessages() == 0)
    {
      if (!ReadDataToQueue())
        return false;
    }

    message = m_messagequeue.GetMessage();

    //first word sent is "light
    if (!ParseWord(message, "light") || !GetWord(message.message, light.m_name))
    {
      return false;
    }

    //second one is "area"
    if (!ParseWord(message, "area"))
      return false;

    //area is defined as points from 1 to 1000, separated by .
    while (GetWord(message.message, word))
    {
      struct boblight_point point;
      if (sscanf(word.c_str(), "%i.%i", &point.x, &point.y) != 2 || point.x < 0 || point.x > 999 || point.y < 0 || point.y > 999)
        return false;

      light.m_area.push_back(point);
    }

    //it defines a polygon, so we need at least 3 points
    if (light.m_area.size() < 3)
      return false;

    m_lights.push_back(light);
  }    
  return true;
}

const char* CBoblight::GetLightName(int lightnr)
{
  if (lightnr < 0) //negative lights don't exist, so we set it to an invalid number to get the error message
    lightnr = m_lights.size();

  if (CheckLightExists(lightnr))
    return m_lights[lightnr].m_name.c_str();

  return NULL;
}

int CBoblight::SetPriority(int priority)
{
  string data = "set priority " + ToString(priority) + "\n";

  return WriteDataToSocket(data);
}

int CBoblight::SetSpeed(int lightnr, float speed)
{
  string data;

  if (!CheckLightExists(lightnr))
    return 0;
  
  if (lightnr < 0)
  {
    for (int i = 0; i < m_lights.size(); i++)
    {
      m_lights[i].m_speed = Clamp(speed, 0.0, 100.0);
      data += "set light " + m_lights[i].m_name + " speed " + ToString(m_lights[i].m_speed) + "\n";
    }
  }
  else
  {
    m_lights[lightnr].m_speed = Clamp(speed, 0.0, 100.0);
    data += "set light " + m_lights[lightnr].m_name + " speed " + ToString(m_lights[lightnr].m_speed) + "\n";
  }

  return WriteDataToSocket(data);    
}

int CBoblight::SetInterpolation(int lightnr, int on)
{
  string data;

  if (!CheckLightExists(lightnr))
    return 0;

  if (lightnr < 0)
  {
    for (int i = 0; i < m_lights.size(); i++)
    {
      m_lights[i].m_interpolation = on;
      data += "set light " + m_lights[i].m_name + " interpolation " + ToString(m_lights[i].m_interpolation) + "\n";
    }
  }
  else
  {
    m_lights[lightnr].m_interpolation = on;
    data += "set light " + m_lights[lightnr].m_name + " interpolation " + ToString(m_lights[lightnr].m_interpolation) + "\n";
  }

  return WriteDataToSocket(data);    
}

int CBoblight::SetUse(int lightnr, int on)
{
  string data;

  if (!CheckLightExists(lightnr))
    return 0;

  if (lightnr < 0)
  {
    for (int i = 0; i < m_lights.size(); i++)
    {
      m_lights[i].m_use = on;
      data += "set light " + m_lights[i].m_name + " use " + ToString(m_lights[i].m_use) + "\n";
    }
  }
  else
  {
    m_lights[lightnr].m_use = on;
    data += "set light " + m_lights[lightnr].m_name + " use " + ToString(m_lights[lightnr].m_use) + "\n";
  }

  return WriteDataToSocket(data);    
}

int CBoblight::SetSaturation(int lightnr, float saturation)
{
  if (!CheckLightExists(lightnr))
    return 0;

  if (lightnr < 0)
  {
    for (int i = 0; i < m_lights.size(); i++)
    {
      m_lights[i].m_saturation = saturation;
    }
  }
  else
  {
    m_lights[lightnr].m_saturation = saturation;
  }

  return 1;
}

int CBoblight::SetValue(int lightnr, float value)
{
  if (!CheckLightExists(lightnr))
    return 0;

  if (lightnr < 0)
  {
    for (int i = 0; i < m_lights.size(); i++)
    {
      m_lights[i].m_value = value;
    }
  }
  else
  {
    m_lights[lightnr].m_value = value;
  }

  return 1;
}

int CBoblight::SetValueRange(int lightnr, float* valuerange)
{
  if (!CheckLightExists(lightnr))
    return 0;

  if (lightnr < 0)
  {
    for (int i = 0; i < m_lights.size(); i++)
    {
      m_lights[i].m_valuerange[0] = Clamp(valuerange[0], 0.0, 1.0);
      m_lights[i].m_valuerange[1] = Clamp(valuerange[1], 0.0, 1.0);
      m_lights[i].m_valuerange[0] = Clamp(m_lights[i].m_valuerange[0], 0.0, m_lights[i].m_valuerange[1]);
    }
  }
  else
  {
    m_lights[lightnr].m_valuerange[0] = Clamp(valuerange[0], 0.0, 1.0);
    m_lights[lightnr].m_valuerange[1] = Clamp(valuerange[1], 0.0, 1.0);
    m_lights[lightnr].m_valuerange[0] = Clamp(m_lights[lightnr].m_valuerange[0], 0.0, m_lights[lightnr].m_valuerange[1]);
  }

  return 1;
}

int CBoblight::SetThreshold(int lightnr, int threshold)
{
  if (!CheckLightExists(lightnr))
    return 0;

  if (lightnr < 0)
  {
    for (int i = 0; i < m_lights.size(); i++)
    {
      m_lights[i].m_threshold = Clamp(threshold, 0, 255);
    }
  }
  else
  {
    m_lights[lightnr].m_threshold = Clamp(threshold, 0, 255);
  }

  return 1;
}  

bool CBoblight::CheckLightExists(int lightnr, bool printerror /*= true*/)
{
  if (lightnr >= (int)m_lights.size())
  {
    if (printerror)
    {
      m_error = "light " + ToString(lightnr) + " doesn't exist (have " + ToString(m_lights.size()) + " lights)";
    }
    return false;
  }
  return true;
}

void CBoblight::SetScanRange(int width, int height)
{
  //need to find a better way to do this stuff
  
  for (int i = 0; i < m_lights.size(); i++)
  {
    if (m_lights[i].m_width == width || m_lights[i].m_height == height)
      continue;

    m_lights[i].m_scanarea.clear();
    m_lights[i].m_scanarea.resize(width * height);
    m_lights[i].m_width = width;
    m_lights[i].m_height = height;

    DrawPolygon(m_lights[i]);
    FillPolygon(m_lights[i]);

    /*cout << "light " << m_lights[i].m_name << " " << m_lights[i].m_scanarea.size() / height << " " << m_lights[i].m_scanarea.size() / width << "\n";

    for (int j = 0; j < m_lights[i].m_area.size(); j++)
    {
      cout << m_lights[i].m_area[j].x << " " << m_lights[i].m_area[j].y << "\n";
    }

    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
      {
        cout << m_lights[i].m_scanarea[y * width + x];
      }
      cout << "\n";
    }*/
  }
}

void CBoblight::DrawPolygon(CLight& light)
{
  int width = light.m_width;
  int height = light.m_height;
  
  for (int i = 0; i < light.m_area.size(); i++)
  {
    int firstpoint = i;
    int secondpoint = (i + 1 < light.m_area.size()) ? i + 1 : 0;

    int line[4];
    line[0] = light.m_area[firstpoint].x;
    line[1] = light.m_area[firstpoint].y;
    line[2] = light.m_area[secondpoint].x;
    line[3] = light.m_area[secondpoint].y;

    TransformPoint(line + 0, width, height);
    TransformPoint(line + 2, width, height);

    int xsize = line[2] - line[0];
    int ysize = line[3] - line[1];

    int xadd = xsize > 0 ? 1 : -1;
    int yadd = ysize > 0 ? 1 : -1;

    if (ysize == 0)
    {
      for (int x = line[0]; x != line[2] + xadd; x += xadd)
      {
        light.m_scanarea[line[1] * width + x] = 1;
      }
    }
    else if (xsize == 0)
    {
      for (int y = line[1]; y != line[3] + yadd; y += yadd)
      {
        light.m_scanarea[y * width + line[0]] = 1;
      }
    }
    else if (Abs(ysize) < Abs(xsize))
    {
      for (int x = line[0]; x != line[2] + xadd; x += xadd)
      {
        int y = Round<int>((float)(x - line[0]) * (float)ysize / (float)xsize + (float)line[1]);
        light.m_scanarea[y * width + x] = 1;
      }
    }
    else
    {
      for (int y = line[1]; y != line[3] + yadd; y += yadd)
      {
        int x = Round<int>((float)(y - line[1]) * (float)xsize / (float)ysize + (float)line[0]);
        light.m_scanarea[y * width + x] = 1;
      }
    }
  }
}

void CBoblight::TransformPoint(int* point, int width, int height)
{
  point[0] = Round<int>((float)point[0] / 999.0f * ((float)width  - 1.0f));
  point[1] = Round<int>((float)point[1] / 999.0f * ((float)height - 1.0f));

  point[0] = Clamp(point[0], 0, width - 1);
  point[1] = Clamp(point[1], 0, height - 1);
}

void CBoblight::FillPolygon(CLight& light)
{
  for (int y = 0; y < light.m_height; y++)
  {
    for (int x = 0; x < light.m_width; x++)
    {
      if (IsPointInPolygon(light, x, y))
      {
        light.m_scanarea[y * light.m_width + x] = 2;
      }
    }
  }
}

bool CBoblight::IsPointInPolygon(CLight& light, int x, int y)
{
  int width = light.m_width;
  int height = light.m_height;
  
  if (x == 0 || x == width - 1 || y == 0 || y == height - 1)
    return false;

  if (light.m_scanarea[y * width + x])
    return false;

  bool edgefound;

  edgefound = false;
  for (int i = x; i < width; i++)
  {
    if (light.m_scanarea[y * width + i])
    {
      edgefound = true;
      break;
    }
  }
  if (!edgefound)
    return false;

  edgefound = false;
  for (int i = x; i >= 0; i--)
  {
    if (light.m_scanarea[y * width + i])
    {
      edgefound = true;
      break;
    }
  }
  if (!edgefound)
    return false;

  edgefound = false;
  for (int i = y; i < height; i++)
  {
    if (light.m_scanarea[i * width + x])
    {
      edgefound = true;
      break;
    }
  }
  if (!edgefound)
    return false;

  edgefound = false;
  for (int i = y; i >= 0; i--)
  {
    if (light.m_scanarea[i * width + x])
    {
      edgefound = true;
      break;
    }
  }
  if (!edgefound)
    return false;

  return true;
}

void CBoblight::AddPixel(int lightnr, int* rgb)
{
  if (lightnr < 0)
  {
    for (int i = 0; i < m_lights.size(); i++)
    {
      if (rgb[0] >= m_lights[i].m_threshold || rgb[1] >= m_lights[i].m_threshold || rgb[2] >= m_lights[i].m_threshold)
      {
        m_lights[i].m_rgbd[0] += Clamp(rgb[0], 0, 255);
        m_lights[i].m_rgbd[1] += Clamp(rgb[1], 0, 255);
        m_lights[i].m_rgbd[2] += Clamp(rgb[2], 0, 255);
      }
      m_lights[i].m_rgbd[3]++;
    }
  }
  else
  {
    if (rgb[0] >= m_lights[lightnr].m_threshold || rgb[1] >= m_lights[lightnr].m_threshold || rgb[2] >= m_lights[lightnr].m_threshold)
    {
      m_lights[lightnr].m_rgbd[0] += Clamp(rgb[0], 0, 255);
      m_lights[lightnr].m_rgbd[1] += Clamp(rgb[1], 0, 255);
      m_lights[lightnr].m_rgbd[2] += Clamp(rgb[2], 0, 255);
    }
    m_lights[lightnr].m_rgbd[3]++;
  }
}

void CBoblight::AddPixel(int* rgb, int x, int y)
{
  for (int i = 0; i < m_lights.size(); i++)
  {
    if (m_lights[i].m_scanarea[y * m_lights[i].m_width + x])
    {
      if (rgb[0] >= m_lights[i].m_threshold || rgb[1] >= m_lights[i].m_threshold || rgb[2] >= m_lights[i].m_threshold)
      {
        m_lights[i].m_rgbd[0] += Clamp(rgb[0], 0, 255);
        m_lights[i].m_rgbd[1] += Clamp(rgb[1], 0, 255);
        m_lights[i].m_rgbd[2] += Clamp(rgb[2], 0, 255);
      }
      m_lights[i].m_rgbd[3]++;
    }
  }
}

int CBoblight::SendRGB()
{
  string data;

  for (int i = 0; i < m_lights.size(); i++)
  {
    float rgb[3];
    m_lights[i].GetRGB(rgb);
    
    data += "set light " + m_lights[i].m_name + " rgb " + ToString(rgb[0]) + " " + ToString(rgb[1]) + " " + ToString(rgb[2]) + "\n";
  }

  return WriteDataToSocket(data);
}

int CBoblight::Ping()
{
  string word;
  
  if (!WriteDataToSocket("ping\n"))
    return 0;

  if (!ReadDataToQueue())
    return 0;

  CMessage message = m_messagequeue.GetMessage();

  if (!GetWord(message.message, word) || word != "ping")
  {
    m_error = m_address + ":" + ToString(m_port) + " sent gibberish";
    return 0;
  }

  return 1;
}