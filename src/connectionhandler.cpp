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

#include "connectionhandler.h"
#include "util/log.h"

using namespace std;

CConnectionHandler::CConnectionHandler(CClientsHandler& clients): m_clients(clients)
{
}

void CConnectionHandler::Process()
{
  if (m_address.empty())
    log("starting connection handler on *:%i", m_port);
  else
    log("starting connection handler on %s:%i", m_address.c_str(), m_port);

  if (!m_socket.Open(m_address, m_port, 1000000))
  {
    logerror("%s", m_socket.GetError().c_str());
    return;
  }

  while(!m_stop)
  {
    //TODO: don't alloc and dealloc every second when accept times out
    //TODO: move this into clients handler, it already does a select call on all the client sockets
    CClient* client = new CClient;

    int returnv = m_socket.Accept(client->m_socket);
    
    if (returnv == SUCCESS)
    {
      log("%s:%i connected", client->m_socket.GetAddress().c_str(), client->m_socket.GetPort());
      m_clients.AddClient(client);
    }
    else
    {
      delete client;
      if (returnv == FAIL)
      {
        log("%s", m_socket.GetError().c_str());
        break;
      }
    }
  }

  log("connection handler stopped");
}

