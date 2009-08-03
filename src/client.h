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

#ifndef CCLIENT
#define CCLIENT

#include <vector>
#include <stdint.h>

#include "util/misc.h"
#include "util/mutex.h"
#include "util/thread.h"
#include "util/tcpsocket.h"
#include "util/messagequeue.h"

#include "light.h"
#include "device/device.h"

class CClient
{
  public:
    CClient();
    CTcpClientSocket m_socket;
    CMessageQueue    m_messagequeue;
    int              m_priority;
    void             SetPriority(int priority) { m_priority = Clamp(priority, 0, 255); }
    int64_t          m_connecttime;

    std::vector<CLight> m_lights;

    int              LightNameToInt(std::string& lightname);
};

class CClientsHandler : public CThread
{
  public:
    CClientsHandler(std::vector<CLight>& lights);
    void AddClient(CClient* client);
    void FillChannels(std::vector<CChannel>& channels, int64_t time);
    
  private:
    void Process();
    
    std::vector<CClient*> m_clients;
    std::vector<CLight>&  m_lights;

    CMutex   m_mutex;
    int      GetReadableClient();
    CClient* GetClientFromSock(int sock);     //gets a client from a socket fd
    void     RemoveClient(int sock);          //removes a client based on socket
    void     RemoveClient(CClient* client);   //removes a client based on pointer
    bool     HandleMessages(CClient* client); //handles client messages
    bool     ParseMessage(CClient* client, CMessage& message); //parses client messages
    bool     ParseGet(CClient* client, CMessage& message);
    bool     ParseSet(CClient* client, CMessage& message);
    bool     ParseSetLight(CClient* client, CMessage& message);
    bool     SendVersion(CClient* client);
    bool     SendLights(CClient* client);
    bool     SendPing(CClient* client);
};

#endif //CCLIENT