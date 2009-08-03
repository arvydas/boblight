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

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream> //debug

#include "udpsocket.h"
#include "misc.h"

using namespace std;

CUdpPacket::CUdpPacket()
{
  m_len = 0;
  memset(m_data, 0, sizeof(m_data));
  memset(&m_addr, 0, sizeof(m_addr));
}

void CUdpPacket::SetData(unsigned char* data, int len)
{
  m_len = sizeof(m_data) < len ? sizeof(m_data) : len;
  memcpy(m_data, data, m_len);
}

int CUdpPacket::GetData(unsigned char*& data)
{
  data = m_data;
  return m_len;
}

string CUdpPacket::GetAddr()
{
  return inet_ntoa(m_addr.sin_addr);
}

int CUdpPacket::GetPort()
{
  return ntohs(m_addr.sin_port);
}

void CUdpPacket::SetAddrPort(string address, int port)
{
  m_addr.sin_family = AF_INET;
  m_addr.sin_addr.s_addr = inet_addr(address.c_str());
  m_addr.sin_port = htons(port);
}

string CUdpPacket::GetStrData()
{
  char buff[PACKETSIZE + 1];
  memcpy(buff, m_data, m_len);
  buff[m_len] = 0;

  return buff;
}

int64_t CUdpPacket::GetTime()
{
  return m_time;
}

void CUdpPacket::SetTime(int64_t time)
{
  m_time = time;
}

void CUdpPacket::SetStrData(string data)
{
  m_len = data.length() < sizeof(m_data) ? data.length() : sizeof(m_data);
  memcpy(m_data, data.c_str(), m_len);
}

CUdpSocket::CUdpSocket()
{
  m_sock = -1;
}

CUdpSocket::~CUdpSocket()
{
  CloseSocket();
}

bool CUdpSocket::WritePacket(CUdpPacket& packet)
{
  unsigned char* data;
  int   len;

  if (m_sock == -1)
  {
    m_error = "socket closed";
    return false;
  }

  len = packet.GetData(data);
  
  int byteswritten = sendto(m_sock, data, len, 0, reinterpret_cast<struct sockaddr*>(&packet.m_addr), sizeof(packet.m_addr));

  if (byteswritten != len)
  {
    m_error = GetErrno();
    return false;
  }
  return true;
}

int CUdpSocket::ReadPacket(CUdpPacket& packet, int64_t usecs /*= -1*/)
{
  unsigned char  buff[PACKETSIZE];
  fd_set         socket;
  struct         timeval timeout;

  if (m_sock == -1)
  {
    m_error = "socket closed";
    return -1;
  }

  if (usecs > 0)
  {
    timeout.tv_sec = usecs / 1000000;
    timeout.tv_usec = usecs % 1000000;
    FD_ZERO(&socket);
    FD_SET(m_sock, &socket);
    int returnv = select(m_sock + 1, &socket, NULL, NULL, &timeout);

    if (returnv == -1)
    {
      m_error = GetErrno();
      return -1;
    }

    if (returnv == 0)
    {
      m_error = "read timed out";
      return 0;
    }
  }
  
  socklen_t addrsize = sizeof(packet.m_addr);
  int bytesread = recvfrom(m_sock, buff, sizeof(buff), 0, reinterpret_cast<struct sockaddr*>(&packet.m_addr), &addrsize);

  if (bytesread == -1)
  {
    m_error = GetErrno();
    return -1;
  }

  packet.SetData(buff, bytesread);
  packet.SetTime(m_clock.GetTime());

  return bytesread;
}

bool CUdpSocket::OpenSocket(string addr/* = ""*/, int port /*= -1*/)
{
  m_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (m_sock == -1)
  {
    m_error = "socket(): " + GetErrno();
    return false;
  }

  if (addr.empty() || port < 0)
    return true;
  
  memset(&m_server, 0, sizeof(m_server));

  m_server.sin_family = AF_INET;
  m_server.sin_port = htons(port);

  if (addr == "*")
    m_server.sin_addr.s_addr = htonl(INADDR_ANY);
  else
    m_server.sin_addr.s_addr = inet_addr(addr.c_str());

  if (bind(m_sock, reinterpret_cast<struct sockaddr*>(&m_server), sizeof(m_server)) == -1)
  {
    m_error = "bind(): " + GetErrno();
    CloseSocket();
    return false;
  }
  return true;
}

void CUdpSocket::CloseSocket()
{
  if (m_sock != -1)
  {
    close(m_sock);
    m_sock = -1;
  }
}

string CUdpSocket::GetError()
{
  return m_error;
}