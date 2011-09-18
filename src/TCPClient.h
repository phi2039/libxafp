#pragma once
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

/////////////////////////////////////////////////////////////////////////////////
// TCP Protocol Layer
/////////////////////////////////////////////////////////////////////////////////

enum
{
  kTCPSendError     = -100,
  kTCPReceiveError  = -101,
  kTCPNotConnected  = -102,
  kTCPInvalidState  = -103,
  kTCPTimeout       = -104
};

class CTCPPacketReader
{
public:
  CTCPPacketReader(int socket, uint32_t maxData, int timeout = 0);
  int Read(void* pBuffer, uint32_t bytesToRead);
  int GetBytesLeft() {return m_BytesLeft;}
  void SkipBytes(uint32_t bytesToSkip);
protected:
  int m_Socket;
  uint32_t m_BytesLeft;
};

class ITCPReceiveCallback
{
public:
  virtual ~ITCPReceiveCallback() {};
  virtual void OnReceive(CTCPPacketReader& reader) = 0;
};

class CTCPSession
{
public:
  CTCPSession();
  virtual ~CTCPSession();
  bool Connect(const char* pHostName, unsigned int port, ITCPReceiveCallback* pCallback = NULL, uint32_t timeout = 3000);
  void Disconnect();
  int SendData(void* pData, int size);
  int ReceiveData(CTCPPacketReader** ppReader, uint32_t timeout = 3000); // This method is only available for synchronous clients
  bool IsConnected();
protected:
  int m_Socket;
  
  int PollSocket(int timeout);
  
  int ReceiveThreadProc();
  bool StartReceiveThread();
  void StopReceiveThread();
  static void* ReceiveThreadStub(void* p);
  pthread_t m_ReceiveThread;
  bool m_ReceiveThreadQuitFlag;
  ITCPReceiveCallback* m_pReceiveCallback;
};
