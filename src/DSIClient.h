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

#include "TCPClient.h"
#include "Threads.h"

/////////////////////////////////////////////////////////////////////////////////
// DSI (Data Stream Interface) Protocol Layer
/////////////////////////////////////////////////////////////////////////////////
enum
{
  kDSISessionClosed = -200
};

class CFPServerInfo
{
public:
  CFPServerInfo();
  virtual ~CFPServerInfo();
  bool Parse(uint8_t* pBuf, uint32_t len);
  const char* GetServerName() {return m_ServerName;}
  const char* GetMachineType() {return m_MachineType;}
protected:
  // From FPGetSrvrInfo block
  int16_t m_Flags;
  char m_ServerName[33];
  char m_MachineType[17];
  std::vector<std::string> m_AFPVersions;
  std::vector<std::string> m_UAMs;
  uint8_t m_ServerSignature[16];
};

struct DSIHeader
{
  uint8_t flags;
  uint8_t command;
  uint16_t requestID;
  union
  {
    uint32_t errorCode;
    uint32_t writeOffset;
  };
  uint32_t totalDataLength;
  uint32_t reserved;
};

class CDSIBuffer
{
public:
  CDSIBuffer(uint32_t size = 0); // If size == 0, allocation is delayed
  virtual ~CDSIBuffer();
  
  operator void*() {return m_pBuffer;}
  operator uint8_t*() {return (uint8_t*)m_pBuffer;}
  
  void* GetData() {if (!m_pBuffer) return NULL; return m_pBuffer + m_HeadRoom;}
  uint32_t GetDataLen() {return m_DataLen;}
  uint32_t SetDataLen(uint32_t len) {m_DataLen = (len > m_BufferSize) ? m_BufferSize : len; return m_DataLen;}
  
  void Write(void* pData, uint32_t size);
  void Write(uint8_t val);
  void Write(uint16_t val);
  void Write(uint32_t val);
  void Write(uint64_t val);
  void Write(const char* pVal);
  
  uint8_t Read8();
  uint16_t Read16();
  uint32_t Read32();
  uint32_t ReadString(char* pBuf, uint32_t size);
  uint32_t Read(void* pBuf, uint32_t size);

  void Resize(uint32_t newSize);
  
  uint8_t* GetHeader() {return m_pBuffer;}
protected:
  void GuaranteeSize(uint32_t newSize); // TODO: inline...
  uint8_t* m_pBuffer; // Buffer Location
  uint8_t* m_pIndex; // Next read/write position
  uint32_t m_BufferSize;
  uint32_t m_DataLen;
  
  const uint32_t m_HeadRoom;
};

struct DSIRequest
{
  uint16_t id;
  CThreadSyncEvent* pEvent;
  uint32_t result;
  CDSIBuffer* pResponseBuffer; // Optional
  // Used for ongoing replies...
  uint32_t totalBytes;
  uint32_t bytesRemaining;
  uint32_t pieces;
};

class CDSISession : public CTCPSession, public ITCPReceiveCallback  // TODO: Technically, DSI can operate over protocols other than TCP...we are being lazy for now...
{
public:
  CDSISession();
  virtual ~CDSISession();
  bool Open(const char* pHostName, uint32_t port, unsigned int timeout = 3000);
  bool IsOpen() {return m_IsOpen;}
  void Close();

  static bool GetStatus(const char* pHostName, unsigned int port, CFPServerInfo& info); // Atomic operation (outside sesion)

  uint16_t GetNewRequestId(){return m_LastRequestId++;}
  
  int32_t SendCommand(CDSIBuffer& payload, CDSIBuffer* pResponse = NULL);

  // ITCPReceiveCallback Implementation
  void OnReceive(CTCPPacketReader& reader); 
protected:
  int32_t SendMessage(uint8_t messageId, uint16_t requestId, CDSIBuffer* pPayload = NULL, uint32_t writeOffset = 0);
  bool AddRequest(DSIRequest* pRequest);
  DSIRequest* RemoveRequest(uint16_t id);
  void SignalAll(int err);
  
  bool m_IsOpen;
  int16_t m_LastRequestId;
  CFPServerInfo m_ServerInfo;
  std::map<int, DSIRequest*> m_Requests;
  DSIRequest* m_pOngoingReply;
};