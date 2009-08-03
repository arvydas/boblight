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

#ifndef CUDPSOCKET
#define CUDPSOCKET

#include <string>
#include <stdint.h>
#include <netinet/in.h>
#include "clock.h"

#define PACKETSIZE 65507

class CUdpPacket
{
  public:
    CUdpPacket();

    void        SetData(unsigned char* data, int len);
    int         GetData(unsigned char*& data);
    std::string GetStrData();
    void        SetStrData(std::string data);
    std::string GetAddr();
    int         GetPort();
    void        SetAddrPort(std::string address, int port);
    void        SetTime(int64_t time);
    int64_t     GetTime();

    struct sockaddr_in m_addr;

  private:
    unsigned char m_data[PACKETSIZE];
    int           m_len;
    int64_t       m_time;
};

class CUdpSocket
{
  public:
    CUdpSocket();
    ~CUdpSocket();

    void CloseSocket();
    bool OpenSocket(std::string addr = "", int port = -1);
    bool WritePacket(CUdpPacket& packet);
    int  ReadPacket(CUdpPacket& packet, int64_t usecs = -1);

    std::string GetError();

  private:
    int         m_sock;
    std::string m_error;
    CClock      m_clock;

    struct sockaddr_in m_server; //where we bind the socket on
};

#endif //CUDPSOCKET