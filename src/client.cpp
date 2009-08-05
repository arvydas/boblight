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

#include <iostream> //debug

#include <locale.h>

#include "config.h"

#include "client.h"
#include "util/lock.h"
#include "util/log.h"
#include "protocolversion.h"

#define WAITTIME 100000

using namespace std;

CClient::CClient()
{
  m_priority = 255;
  m_connecttime = -1;
}

int CClient::LightNameToInt(std::string& lightname)
{
  for (int i = 0; i < m_lights.size(); i++)
  {
    if (m_lights[i].GetName() == lightname)
      return i;
  }
  return -1;
}

CClientsHandler::CClientsHandler(std::vector<CLight>& lights) : m_lights(lights)
{
}

void CClientsHandler::Process()
{
  log("Starting clients handler");

  while (!m_stop)
  {
    //see if there's a client we can read from
    int sock = GetReadableClient();
    if (sock == -1) //nope
      continue;

    //get the client the sock fd belongs to
    CClient* client = GetClientFromSock(sock);
    if (client == NULL) //guess it belongs to nobody
      continue;
    
    CTcpData data;
    int returnv = client->m_socket.Read(data);
    if (returnv == FAIL)
    {
      log("%s", client->m_socket.GetError().c_str());
      RemoveClient(client);
      continue;
    }

    client->m_messagequeue.AddData(data.GetData(), data.GetSize());

    if (!HandleMessages(client))
      RemoveClient(client);
  }

  log("Stopping clients handler");

  CLock lock(m_mutex);
  while(m_clients.size())
  {
    RemoveClient(m_clients.front());
  }    
}

void CClientsHandler::AddClient(CClient* client)
{
  CLock lock(m_mutex);

  if (m_clients.size() >= FD_SETSIZE) //maximum number of clients reached
  {
    log("number of clients reached maximum %i", FD_SETSIZE);
    CTcpData data;
    data.SetData("full\n");
    client->m_socket.Write(data);
    delete client;
    return;
  }

  client->m_lights = m_lights;
  m_clients.push_back(client);
}

int CClientsHandler::GetReadableClient()
{
  vector<int> sockets;
  CLock lock(m_mutex);

  if (m_clients.size() == 0) //no clients so we just sleep
  {
    lock.Leave();
    usleep(WAITTIME);
    return -1;
  }

  for (int i = 0; i < m_clients.size(); i++)
  {
    sockets.push_back(m_clients[i]->m_socket.GetSock());
  }

  lock.Leave();
  
  int returnv, highestsock = -1;
  fd_set rsocks;
  struct timeval tv;

  FD_ZERO(&rsocks);

  for (int i = 0; i < sockets.size(); i++)
  {
    FD_SET(sockets[i], &rsocks);
    if (sockets[i] > highestsock)
      highestsock = sockets[i];
  }

  tv.tv_sec = WAITTIME / 1000000;
  tv.tv_usec = (WAITTIME % 1000000);

  returnv = select(highestsock + 1, &rsocks, NULL, NULL, &tv);

  if (returnv == 0) //select timed out
  {
    return -1;
  }
  else if (returnv == -1) //select had an error
  {
    log("select() %s", GetErrno().c_str());
    usleep(WAITTIME);
    return -1;
  }

  for (int i = 0; i < sockets.size(); i++)
  {
    if (FD_ISSET(sockets[i], &rsocks))
      return sockets[i];
  }

  return -1;
}

//gets a client from a socket fd
CClient* CClientsHandler::GetClientFromSock(int sock)
{
  CLock lock(m_mutex);
  for (int i = 0; i < m_clients.size(); i++)
  {
    if (m_clients[i]->m_socket.GetSock() == sock)
      return m_clients[i];
  }
  return NULL;
}

void CClientsHandler::RemoveClient(int sock)
{
  CLock lock(m_mutex);
  for (int i = 0; i < m_clients.size(); i++)
  {
    if (m_clients[i]->m_socket.GetSock() == sock)
    {
      log("removing %s:%i", m_clients[i]->m_socket.GetAddress().c_str(), m_clients[i]->m_socket.GetPort());
      delete m_clients[i];
      m_clients.erase(m_clients.begin() + i);
      return;
    }
  }
}

void CClientsHandler::RemoveClient(CClient* client)
{
  CLock lock(m_mutex);
  for (int i = 0; i < m_clients.size(); i++)
  {
    if (m_clients[i] == client)
    {
      log("removing %s:%i", m_clients[i]->m_socket.GetAddress().c_str(), m_clients[i]->m_socket.GetPort());
      delete m_clients[i];
      m_clients.erase(m_clients.begin() + i);
      return;
    }
  }
}

bool CClientsHandler::HandleMessages(CClient* client)
{
  while (client->m_messagequeue.GetNrMessages() > 0)
  {
    CMessage message = client->m_messagequeue.GetMessage();
    if (!ParseMessage(client, message))
      return false;
  }
  return true;
}

bool CClientsHandler::ParseMessage(CClient* client, CMessage& message)
{
  CTcpData data;
  string messagekey;
  if (!GetWord(message.message, messagekey))
  {
    log("%s:%i sent gibberish", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
    return false;
  }

  if (messagekey == "hello")
  {
    log("%s:%i said hello", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
    data.SetData("hello\n");
    if (client->m_socket.Write(data) != SUCCESS)
    {
      log("%s", client->m_socket.GetError().c_str());
      return false;
    }
    CLock lock(m_mutex);
    client->m_connecttime = message.time;
  }
  else if (messagekey == "ping")
  {
    return SendPing(client);
  }
  else if (messagekey == "get")
  {
    return ParseGet(client, message);
  }
  else if (messagekey == "set")
  {
    return ParseSet(client, message);
  }
  else
  {
    log("%s:%i sent gibberish", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
    return false;
  }

  return true;
}

bool CClientsHandler::ParseGet(CClient* client, CMessage& message)
{
  CTcpData data;
  string messagekey;
  if (!GetWord(message.message, messagekey))
  {
    log("%s:%i sent gibberish", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
    return false;
  }

  if (messagekey == "version")
  {
    return SendVersion(client);
  }
  else if (messagekey == "lights")
  {
    return SendLights(client);
  }
  else
  {
    log("%s:%i sent gibberish", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
    return false;
  }
}

bool CClientsHandler::SendVersion(CClient* client)
{
  CTcpData data;

  data.SetData("version " + static_cast<string>(PROTOCOLVERSION) + "\n");

  if (client->m_socket.Write(data) != SUCCESS)
  {
    log("%s", client->m_socket.GetError().c_str());
    return false;
  }
  return true;
}

bool CClientsHandler::SendLights(CClient* client)
{
  CTcpData data;
  
  data.SetData("lights " + ToString(client->m_lights.size()) + "\n");

  for (int i = 0; i < client->m_lights.size(); i++)
  {
    data.SetData("light " + client->m_lights[i].GetName() + " ", true);
    
    data.SetData("area ", true);
    for (int j = 0; j < client->m_lights[i].GetArea().size(); j++)
    {
      data.SetData(ToString(client->m_lights[i].GetArea()[j].x) + "." + ToString(client->m_lights[i].GetArea()[j].y) + " ", true);
    }
    data.SetData("\n", true);
  }

  if (client->m_socket.Write(data) != SUCCESS)
  {
    log("%s", client->m_socket.GetError().c_str());
    return false;
  }
  return true;
}

bool CClientsHandler::SendPing(CClient* client)
{
  CTcpData data;

  data.SetData("ping\n");

  if (client->m_socket.Write(data) != SUCCESS)
  {
    log("%s", client->m_socket.GetError().c_str());
    return false;
  }
  return true;
}

bool CClientsHandler::ParseSet(CClient* client, CMessage& message)
{
  CTcpData data;
  string messagekey;
  if (!GetWord(message.message, messagekey))
  {
    log("%s:%i sent gibberish", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
    return false;
  }

  if (messagekey == "priority")
  {
    int priority;
    string strpriority;
    if (!GetWord(message.message, strpriority) || !StrToInt(strpriority, priority))
    {
      log("%s:%i sent gibberish", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
      return false;
    }
    client->SetPriority(priority);
  }
  else if (messagekey == "light")
  {
    return ParseSetLight(client, message);
  }    
  else
  {
    log("%s:%i sent gibberish", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
    return false;
  }
  return true;
}

bool CClientsHandler::ParseSetLight(CClient* client, CMessage& message)
{
  string lightname;
  string lightkey;
  int lightnr;

  if (!GetWord(message.message, lightname) || !GetWord(message.message, lightkey) || (lightnr = client->LightNameToInt(lightname)) == -1)
  {
    log("%s:%i sent gibberish", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
    return false;
  }

  if (lightkey == "rgb")
  {
    float rgb[3];
    string value;

    ConvertFloatLocale(message.message); //workaround for locale mismatch (, and .)
    
    for (int i = 0; i < 3; i++)
    {
      if (!GetWord(message.message, value) || !StrToFloat(value, rgb[i]))
      {
        log("%s:%i sent gibberish", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
        return false;
      }
    }

    CLock lock(m_mutex);
    client->m_lights[lightnr].SetRgb(rgb, message.time);
  }
  else if (lightkey == "speed")
  {
    float speed;
    string value;

    ConvertFloatLocale(message.message); //workaround for locale mismatch (, and .)

    if (!GetWord(message.message, value) || !StrToFloat(value, speed))
    {
      log("%s:%i sent gibberish", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
      return false;
    }

    CLock lock(m_mutex);
    client->m_lights[lightnr].SetSpeed(speed);
  }
  else if (lightkey == "interpolation")
  {
    bool interpolation;
    string value;

    if (!GetWord(message.message, value) || !StrToBool(value, interpolation))
    {
      log("%s:%i sent gibberish", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
      return false;
    }

    CLock lock(m_mutex);
    client->m_lights[lightnr].SetInterpolation(interpolation);
  }
  else if (lightkey == "use")
  {
    bool use;
    string value;

    if (!GetWord(message.message, value) || !StrToBool(value, use))
    {
      log("%s:%i sent gibberish", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
      return false;
    }

    CLock lock(m_mutex);
    client->m_lights[lightnr].SetUse(use);
  }
  else
  {
    log("%s:%i sent gibberish", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
    return false;
  }

  return true;
}

void CClientsHandler::FillChannels(std::vector<CChannel>& channels, int64_t time)
{
  CLock lock(m_mutex);

  //get the oldest client with the highest priority
  for (int i = 0; i < channels.size(); i++)
  {
    int64_t clienttime = 0x7fffffffffffffffLL;
    int     priority   = 255;
    int     light = channels[i].GetLight();
    int     color = channels[i].GetColor();
    int     clientnr = -1;

    if (light == -1 || color == -1) //unused channel
      continue;

    for (int j = 0; j < m_clients.size(); j++)
    {
      if (m_clients[j]->m_priority == 255 || m_clients[j]->m_connecttime == -1 || !m_clients[j]->m_lights[light].GetUse())
        continue; //this client we don't use

      if (m_clients[j]->m_priority < priority || (priority == m_clients[j]->m_priority && m_clients[j]->m_connecttime < clienttime))
      {
        clientnr = j;
        clienttime = m_clients[j]->m_connecttime;
        priority = m_clients[j]->m_priority;
      }
    }
    
    if (clientnr == -1) //no client for the light on this channel
    {
      channels[i].SetUsed(false);
      channels[i].SetSpeed(m_lights[light].GetSpeed());
      channels[i].SetValueToFallback();
      channels[i].SetGamma(1.0);
      channels[i].SetAdjust(1.0);
      channels[i].SetBlacklevel(0.0);
      continue;
    }

    channels[i].SetUsed(true);
    channels[i].SetValue(m_clients[clientnr]->m_lights[light].GetColorValue(color, time));
    channels[i].SetSpeed(m_clients[clientnr]->m_lights[light].GetSpeed());
    channels[i].SetGamma(m_clients[clientnr]->m_lights[light].GetGamma(color));
    channels[i].SetAdjust(m_clients[clientnr]->m_lights[light].GetAdjust(color));
    channels[i].SetBlacklevel(m_clients[clientnr]->m_lights[light].GetBlacklevel(color));
  }
}