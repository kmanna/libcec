/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2013 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

#include "env.h"
#include "lib/platform/sockets/tcp.h"
#include "lib/platform/sockets/serversocket.h"

using namespace std;
using namespace PLATFORM;

bool CTcpServerSocket::Open(uint64_t UNUSED(iTimeoutMs))
{
  bool bReturn(false);
  struct addrinfo *address(NULL), *addr(NULL);
  if (!TcpResolveAddress("localhost", m_iPort, &m_iError, &address))
  {
    m_strError = strerror(m_iError);
    return bReturn;
  }

  for(addr = address; !bReturn && addr; addr = addr->ai_next)
  {
    m_socket = TcpCreateSocket(addr, &m_iError);
    if (m_socket != INVALID_SOCKET_VALUE)
    {
      bReturn = true;
      break;
    }
    else
    {
      m_strError = strerror(m_iError);
    }
  }

  if (bReturn)
  {
    m_iError = bind(m_socket, addr->ai_addr, addr->ai_addrlen);
    if (m_iError)
    {
      m_strError = strerror(m_iError);
      bReturn = false;
    }
  }

  freeaddrinfo(address);

  if (bReturn)
  {
    m_iError = listen(m_socket, 16);
    if (m_iError)
    {
      m_strError = strerror(m_iError);
      bReturn = false;
    }
  }

  return bReturn;
}

void CTcpServerSocket::Close(void)
{
  if (IsOpen())
    TcpSocketClose(m_socket);
  m_socket = INVALID_SOCKET_VALUE;
}

void CTcpServerSocket::Shutdown(void)
{
  Close();
}

bool CTcpServerSocket::IsOpen(void)
{
  return m_socket != INVALID_SOCKET_VALUE;
}

std::string CTcpServerSocket::GetError(void)
{
  std::string strError;
  strError = m_strError.empty() && m_iError != 0 ? strerror(m_iError) : m_strError;
  return strError;
}

int CTcpServerSocket::GetErrorNumber(void)
{
  return m_iError;
}

std::string CTcpServerSocket::GetName(void)
{
  std::string strName("localhost");
  return strName;
}

ISocket* CTcpServerSocket::Accept(void)
{
  struct sockaddr clientAddr;
  socklen_t iClientLen(sizeof(clientAddr));
  tcp_socket_t client = accept(m_socket, &clientAddr, &iClientLen);

  if (client != INVALID_SOCKET_VALUE)
  {
    CTcpClientSocket *socket = new CTcpClientSocket(client);
    return (ISocket*)socket;
  }

  m_strError = strerror(m_iError);
  return NULL;
}

tcp_socket_t CTcpServerSocket::TcpCreateSocket(struct addrinfo* addr, int* iError)
{
  tcp_socket_t fdSock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  if (fdSock == INVALID_SOCKET_VALUE)
  {
    *iError = errno;
    return (tcp_socket_t)INVALID_SOCKET_VALUE;
  }

  TcpSetNoDelay(fdSock);

  return fdSock;
}
