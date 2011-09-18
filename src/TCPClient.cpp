/*
 *      Copyright (C) 2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Common.h"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include "TCPClient.h"

// TODO List
// - Improve start/stop sync for receive thread
// - Improve error handling/recovery
// - Implement ability to skip bytes in the receive buffer, so that DSISession can handle bad callers
// - Switch to recv() so that callers can specify a timeout
// - Handle disconnects
// - Are we still getting dropped out of poll() with zero bytes available?
// - Make sure error codes/results are consistent. Data processing methods should return sizes or errors.

/////////////////////////////////////////////////////////////////////////////////
// TCP Packet Reader
//  Abstraction of read()/recv()
/////////////////////////////////////////////////////////////////////////////////
CTCPPacketReader::CTCPPacketReader(int socket, uint32_t maxData, int timeout /*=0*/) : // No waiting by default
  m_Socket(socket),
  m_BytesLeft(maxData)
{
  
}

int  CTCPPacketReader::Read(void* pBuffer, uint32_t bytesToRead)
{
  if (bytesToRead > m_BytesLeft)
    bytesToRead = m_BytesLeft; // Limit caller to what is known to be available
  
  if (read(m_Socket, pBuffer, bytesToRead) != bytesToRead)
  {
    int err = kNoError;
    socklen_t errlen = sizeof(err);
    getsockopt(m_Socket, SOL_SOCKET, SO_ERROR, (void*)&err, &errlen);
    XAFP_LOG(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Failed to read %d bytes from inbound queue - error: %s", bytesToRead, strerror(err));
    return kTCPReceiveError;
  }
  m_BytesLeft -= bytesToRead;
  
  return bytesToRead;
}

/////////////////////////////////////////////////////////////////////////////////
// TCP Session
/////////////////////////////////////////////////////////////////////////////////
CTCPSession::CTCPSession() :
  m_Socket(-1),
  m_ReceiveThread(NULL),
  m_ReceiveThreadQuitFlag(false)
{
  
}

CTCPSession::~CTCPSession()
{
  Disconnect();
}

bool CTCPSession::Connect(const char* pHostName, unsigned int port, ITCPReceiveCallback* pCallback /*=NULL*/, uint32_t timeout /*=3000*/)
{
  XAFP_LOG(XAFP_LOG_FLAG_INFO, "TCP Protocol: Trying to connect to: %s:%d (Mode:%s)", pHostName, port, (pCallback == NULL) ? "Sync" : "Async");

  if (m_Socket >= 0)
  {
    XAFP_LOG(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Cannot connect to: %s:%d - a connection is already established", pHostName, port);
    return false; // Already an open session
  }
  
  m_pReceiveCallback = pCallback;
  
  // Resolve host name
  addrinfo addrHints, *pAddrList;
  char svc[33];
  int err = 0;
  
  memset(&addrHints, 0, sizeof(addrHints));
  addrHints.ai_family   = AF_UNSPEC;
  addrHints.ai_socktype = SOCK_STREAM;
  addrHints.ai_protocol = IPPROTO_TCP;
  snprintf(svc, sizeof(svc), "%d", port);
  
  err = getaddrinfo(pHostName, svc, &addrHints, &pAddrList);  
  if (err)
  {
    XAFP_LOG(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Cannot connect to: %s:%d - hostname lookup error (%s)", pHostName, port, gai_strerror(err));
    return false;
  }
  // Try to connect to the host using one of the returned addresses
  for (addrinfo* pAddr = pAddrList; pAddr; pAddr = pAddr->ai_next)
  {
    // Create a TCP socket
    m_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_Socket == -1)
      break;
    
    // Set the socket to non-blocking, so that we can allow a failed connection to time-out
    fcntl(m_Socket, F_SETFL, fcntl(m_Socket, F_GETFL) | O_NONBLOCK);    
    if(connect(m_Socket, pAddr->ai_addr, pAddr->ai_addrlen) == -1) 
    {
      if(errno == EINPROGRESS)
      {
        pollfd p;
        p.fd = m_Socket;
        p.events = POLLOUT;
        p.revents = 0;
        
        // Wait for the socket to connect
        switch(poll(&p, 1, timeout))
        {
          case 0: // The connection timed-out
            XAFP_LOG(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Cannot connect to: %s:%d - Connection timed-out", pHostName, port);            
            err = ETIMEDOUT;
            break;
          case -1: // An error occurred in poll()
            err = errno;
            XAFP_LOG(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Cannot connect to: %s:%d - error in poll() (%s)", pHostName, port, strerror(err));            
            break;
          default:
            socklen_t errlen = sizeof(int);
            getsockopt(m_Socket, SOL_SOCKET, SO_ERROR, (void*)&err, &errlen);
            if (err)
              XAFP_LOG(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Socket error while connecting to: %s:%d (%s)", pHostName, port, strerror(err));                          
        }
      }
      else
      {
        err = errno;
        if (err)
          XAFP_LOG(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Socket error while connecting to: %s:%d (%s)", pHostName, port, strerror(err));                          
      }
    }
    else
      err = 0;
    
    if (err) // Something went wrong
    {
      close(m_Socket);
      m_Socket = -1;
    }
    else
      break; // We are connected
  }
  freeaddrinfo(pAddrList);
  
  if (m_Socket == -1) // We were not able to connect
    return false;
  
  // Set socket as blocking
  fcntl(m_Socket, F_SETFL, fcntl(m_Socket, F_GETFL) & ~O_NONBLOCK);
  
  int val = 1;
  setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
  
  // If the caller wants asynchronous reads, kick-off a reader thread, otherwise reads will have to be synchronous
  if (m_pReceiveCallback)
    StartReceiveThread();
  
  XAFP_LOG(XAFP_LOG_FLAG_INFO, "TCP Protocol: Connected to: %s:%d (id: 0x%08x)", pHostName, port, this);
  return true;
}

void CTCPSession::Disconnect()
{
  StopReceiveThread();
  
  if (m_Socket >= 0)
  {
    XAFP_LOG(XAFP_LOG_FLAG_INFO, "TCP Protocol: Disconnected (id: 0x%08x)", this);
    close(m_Socket);
  }
  m_Socket = -1;

}

bool CTCPSession::IsConnected()
{
  // TODO: This should actually query the socket...  
  // TODO: Be more optimistic with operations (i.e. assume success and handle failures, rather than checking things every time)
  return (m_Socket >= 0);
}

int CTCPSession::SendData(void* pData, int size)
{
  // TODO: This is a good example of optimistic checking...change this to just call write() and then examine the response code...
  // Otherwise, the socket gets checked twice...
  if (!IsConnected())
  {
    XAFP_LOG_0(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Caller attempted to send data on disconnected socket");
    return kTCPNotConnected;
  }
  
  XAFP_LOG(XAFP_LOG_FLAG_TCP_PROTO, "TCP Protocol: Sending %d bytes", size);
  int dataLen = write(m_Socket, pData, size);
  if (dataLen == -1)
  {
    XAFP_LOG(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Error in write() (%s)",strerror(errno));
    return kTCPSendError;
  }
  return dataLen;
}

// Read method for Synchronous clients
int CTCPSession::ReceiveData(CTCPPacketReader** ppReader, uint32_t timeout /*=3000*/)
{
  if (m_pReceiveCallback)
  {
    XAFP_LOG_0(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Caller attempted Sync read on Async session");
    return kTCPInvalidState; // We are set-up for async operation, sync reads are prohibited
  }
  
  if (!ppReader)
  {
    XAFP_LOG_0(XAFP_LOG_FLAG_ERROR, "TCP Protocol: NULL output parameter (ppReader) provided to ReceiveData()");    
    return kParamError;
  }
  
  int dataLen = PollSocket(timeout);
  if (dataLen < 0)
  {
    // PollSocket() should report the error. Don't do it again.
    //XAFP_LOG(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Error in PollSocket() - result: %d", dataLen); 
    *ppReader = NULL;
  }
  else
  {
    XAFP_LOG(XAFP_LOG_FLAG_TCP_PROTO, "TCP Protocol: Received %d bytes", dataLen);
    CTCPPacketReader* pReader = new CTCPPacketReader(m_Socket, dataLen);
    *ppReader = pReader;
  }
  return dataLen;
}

int CTCPSession::PollSocket(int timeout /*=-1*/)
{
  int err = kNoError;
  
  pollfd p;
  p.fd = m_Socket;
  p.events = POLLIN; // There is data to read
  p.revents = 0;
  
  // Wait for data on the socket
  switch(poll(&p, 1, timeout))
  {
    case 0: // The operation timed-out
      XAFP_LOG(XAFP_LOG_FLAG_ERROR, "TCP Protocol: poll() timed-out after %dms",timeout);
      return kTCPTimeout;
    case -1: // An error occurred in poll()
      XAFP_LOG(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Error in poll() (%s)",strerror(errno));
      return kTCPReceiveError;
    default:
      socklen_t errlen = sizeof(int);
      getsockopt(m_Socket, SOL_SOCKET, SO_ERROR, (void*)&err, &errlen);
      if (err)
      {
        XAFP_LOG(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Socket error while polling", strerror(err));                          
        return kTCPReceiveError;
      }
  }

  // Fetch the length of the inbound fifo
  int dataLen = 0;
  if (ioctl(m_Socket, FIONREAD, &dataLen) == -1)
  {
    socklen_t errlen = sizeof(err);
    getsockopt(m_Socket, SOL_SOCKET, SO_ERROR, (void*)&err, &errlen);
    if (err)
    {
      XAFP_LOG(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Socket error while fetching inbound queue len (%s)", strerror(err));
      return kTCPReceiveError;
    }
  }  
  return dataLen;
}

// Socket Read Thread (For Async operation)
////////////////////////////////////////////////////////
int CTCPSession::ReceiveThreadProc()
{
  int dataLen = 0;
  int timeout = -1; // Forever...
  
  XAFP_LOG_0(XAFP_LOG_FLAG_INFO, "TCP Protocol: Async receive thread started...");

  // Poll the socket and process packets
  while (true)
  {
    if (m_ReceiveThreadQuitFlag) // shutdown() will unblock any open requests, so this should be sufficient for quitting
      break;
    
    // Wait for data on the socket
    dataLen = PollSocket(timeout);
    if (dataLen < 0) // Something went wrong
    {
      XAFP_LOG(XAFP_LOG_FLAG_ERROR, "TCP Protocol: Receive thread error while polling. Result: %d", dataLen);
      continue; // TODO: try to repair the problem, or bail...
    }
    else if (dataLen == 0)
    {
      XAFP_LOG_0(XAFP_LOG_FLAG_DEBUG, "TCP Protocol: Zero-length data");
      continue;
    }
    else // Notify client via callback
    {      
      // Invoke callback
      CTCPPacketReader reader(m_Socket, dataLen); // Create a reader interface for the callback (so they can read data directly)
      XAFP_LOG(XAFP_LOG_FLAG_TCP_PROTO, "TCP Protocol: Received %d bytes (Async) - Notifying client", dataLen);
      m_pReceiveCallback->OnReceive(reader);
    }
  }
  XAFP_LOG_0(XAFP_LOG_FLAG_INFO, "TCP Protocol: Async receive thread stopped...");      
  return 0; 
}

bool CTCPSession::StartReceiveThread()
{
  if (m_ReceiveThread)
    return false; // Already running

  XAFP_LOG_0(XAFP_LOG_FLAG_INFO, "TCP Protocol: Starting Async receive thread");
  
  return (pthread_create(&m_ReceiveThread, NULL, CTCPSession::ReceiveThreadStub, (void*)this) == 0);
}

void CTCPSession::StopReceiveThread()
{
  if (!m_ReceiveThread)
    return; // Not running
  
  XAFP_LOG_0(XAFP_LOG_FLAG_INFO, "TCP Protocol: Stopping Async receive thread");
  m_ReceiveThreadQuitFlag = true;
  shutdown(m_Socket, SHUT_RD); // Unblock any poll() operations
  pthread_join(m_ReceiveThread, NULL); // Wait for thread to complete
}

void* CTCPSession::ReceiveThreadStub(void* p)
{
  return (void*)((CTCPSession*)p)->ReceiveThreadProc();
}