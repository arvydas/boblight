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

#ifndef CONNECTIONHANDLER
#define CONNECTIONHANDLER

#include <string>

#include "util/tcpsocket.h"
#include "util/thread.h"
#include "client.h"

class CConnectionHandler : public CThread
{
  public:
    CConnectionHandler(CClientsHandler& clients);
    void SetInterface(std::string address, int port) { m_address = address; m_port = port; }

  private:
    std::string m_address;
    int         m_port;
    
    CTcpServerSocket m_socket;
    CClientsHandler& m_clients;

    void Process();
};

#endif //CONNECTIONHANDLER