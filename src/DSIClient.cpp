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

#include "DSIClient.h"
#include "DSIProto.h"

// TODO List
// - Handle stale sessions (no server Tickle for > x mins)
// - Better common dispatch for session messages (Attention, etc...)
// - Login code for additional protocols
// - Is there a deadlock in here someplace...?
// - Handle server options in DSIOpenSession

/////////////////////////////////////////////////////////////////////////////////
// DSI Buffer
/////////////////////////////////////////////////////////////////////////////////
// TODO: Is a 2x resize strategy the best way to go?
#define CONFIRM_BUF_SIZE(_newBytes)              \
  GuaranteeSize(m_DataLen + _newBytes);       \
  m_DataLen += _newBytes;

CDSIBuffer::CDSIBuffer(uint32_t size /*=0*/) :
  m_pBuffer(NULL),
  m_pIndex(NULL),
  m_BufferSize(size),
  m_DataLen(0),
  m_HeadRoom(sizeof(DSIHeader))
{
  if (size)
  {
    m_pBuffer = (uint8_t*)malloc(size + m_HeadRoom);
    m_pIndex = m_pBuffer + m_HeadRoom;
  }
}

CDSIBuffer::~CDSIBuffer()
{
  free(m_pBuffer);
}

void CDSIBuffer::Write(void* pData, uint32_t size)
{
  CONFIRM_BUF_SIZE(size);
  memcpy(m_pIndex, pData, size);
  m_pIndex+=size;
}

void CDSIBuffer::Write(uint8_t val)
{
  CONFIRM_BUF_SIZE(sizeof(val));
  m_pIndex[0] = val;
  m_pIndex++;
}

void CDSIBuffer::Write(uint16_t val)
{
  CONFIRM_BUF_SIZE(sizeof(val));
  *((uint16_t*)m_pIndex) = htons(val);
  m_pIndex+=sizeof(uint16_t);
}

void CDSIBuffer::Write(uint32_t val)
{
  CONFIRM_BUF_SIZE(sizeof(val));
  *((uint32_t*)m_pIndex) = htonl(val);
  m_pIndex+=sizeof(uint32_t);
}

void CDSIBuffer::Write(uint64_t val)
{
  CONFIRM_BUF_SIZE(sizeof(val));

#ifdef BIG_ENDIAN
  uint32_t* pb = (uint32_t*)m_pIndex;
  uint32_t* p = (uint32_t*)&val;
  pb[0] = ntohl(*(p + 1));
  pb[1] = ntohl(*p);
#else
  memcpy(m_pIndex, p, sizeof(val));
#endif
  m_pIndex+=sizeof(uint64_t);
}

void CDSIBuffer::Write(const char* pVal)
{
  uint8_t len = strlen(pVal);
  CONFIRM_BUF_SIZE(sizeof(len) + len);
  m_pIndex[0] = len;
  memcpy(++m_pIndex, pVal, len);
  m_pIndex+=len;
}

void CDSIBuffer::WritePathSpec(const char* pVal, uint16_t len /*=0*/)
{
  if (pVal && !len) // If caller did not provide length 'hint' we need to check it
    len = strlen(pVal);

  CONFIRM_BUF_SIZE(sizeof(len) + len + 5);

  m_pIndex[0] = 3; // kFPUTF8Name
  *((uint32_t*)++m_pIndex) = htonl(0x08000103); // 'Unknown'
  m_pIndex += sizeof(uint32_t);

  *((uint16_t*)m_pIndex) = htons(len);
  m_pIndex+=sizeof(uint16_t);
  
  if (pVal)
  {
    // Copy the path spec into the buffer
    memcpy(m_pIndex, pVal, len);
    // AFP path specifiers use NULL-chars as separators
    // Replace all the slashes with NULLs
    char* p = (char*)m_pIndex;
    for (int i = 0; i < len; i++)
    {
      if (p[i] == '/')
        p[i] = '\0';
    }
  }
  m_pIndex+=len;
}

void CDSIBuffer::Skip(uint32_t bytes)
{
  if ((m_pIndex + bytes) > (m_pIndex + m_DataLen))
    m_pIndex = m_pIndex + m_DataLen;
  else
    m_pIndex += bytes;
}

uint8_t CDSIBuffer::Read8()
{
  uint8_t val = *((uint8_t*)m_pIndex);
  m_pIndex++;
  return val;
}  

uint16_t CDSIBuffer::Read16()
{
  uint16_t val = ntohs(*(uint16_t*)m_pIndex);
  m_pIndex+=sizeof(uint16_t);
  return val;
}

uint32_t CDSIBuffer::Read32()
{
  uint32_t val = ntohl(*(uint32_t*)m_pIndex);
  m_pIndex+=sizeof(uint32_t);
  return val;
}

uint32_t CDSIBuffer::ReadString(char* pBuf, uint32_t size)
{
  // TODO: Clamp read size to available data size
  uint8_t len = Read8();
  uint8_t toRead = (len > size - 1) ? size - 1 : len;
  memcpy(pBuf, m_pIndex, toRead);
  pBuf[toRead] = '\0';
  m_pIndex += len;
  return toRead;
}

uint32_t CDSIBuffer::Read(void* pBuf, uint32_t size)
{
  // TODO: Clamp read size to available data size
  memcpy(pBuf, m_pIndex, size);
  m_pIndex += size;
  return size;
}

void CDSIBuffer::Resize(uint32_t newSize)
{
  if (newSize == 0)
    newSize = 16;
  
  uint8_t* pNewBuf = (uint8_t*)malloc(newSize + m_HeadRoom);
  m_DataLen = (m_DataLen < newSize) ? m_DataLen : newSize;
  if (m_pBuffer)
  {
    memcpy(pNewBuf, m_pBuffer, m_DataLen + m_HeadRoom);
    free(m_pBuffer);
  }
  m_BufferSize = newSize;
  m_pBuffer = pNewBuf;
  m_pIndex = m_pBuffer + m_DataLen + m_HeadRoom;
}

void CDSIBuffer::GuaranteeSize(uint32_t newSize)
{
  if (newSize > m_BufferSize)
  {
    if (newSize < (m_BufferSize * 2))
      newSize = m_BufferSize * 2;
    Resize(newSize);
  }
}

/////////////////////////////////////////////////////////////////////////////////
// FPServerInfo
/////////////////////////////////////////////////////////////////////////////////
CFPServerInfo::CFPServerInfo()
{
  // TODO: Initialize
}

CFPServerInfo::~CFPServerInfo()
{
}

bool CFPServerInfo::Parse(uint8_t* pBuf, uint32_t len)
{
  uint16_t os;
  uint8_t count;
  uint8_t* p = pBuf;
  
  os = ntohs((*(uint16_t*)p)); // MachineType Offset
  memset(m_MachineType, 0, sizeof(m_MachineType));
  CopyPascalString(m_MachineType, pBuf + os);
  p += 2;
  
  os = ntohs((*(uint16_t*)p)); // AFPVersionCount Offset
  count = *(pBuf + os);
  char* pVer = (char*)(pBuf + os + 1); // [Base] + [AFPVersionCount Offset] + [AFPVersionCount Size]
  for (uint8_t i = 0; i < count; i++)
  {
    uint8_t len = *((uint8_t*)pVer);
    std::string s(pVer + 1, len);
    m_AFPVersions.push_back(s);
    pVer += (len + 1);
  }
  p += 2;
  
  os = ntohs((*(uint16_t*)p)); // UAMCount Offset
  count = *(pBuf + os);
  char* pUAM = (char*)(pBuf + os + 1);
  for (uint8_t i = 0; i < count; i++)
  {
    uint8_t len = *((uint8_t*)pUAM);
    std::string s(pUAM + 1, len);
    m_UAMs.push_back(s);
    pUAM += (len + 1);
  }
  p += 4; // Ignore VolumIconAndMask
  
  m_Flags = ntohs((*(uint16_t*)p));
  p += 2;
  
  CopyPascalString(m_ServerName, p);
  if (*p & 0x1) // Is Odd?
    p+= (*p + 1); 
  else
    p += (*p + 2);
  
  os = ntohs((*(uint16_t*)p)); // ServerSignature Offset
  memcpy(m_ServerSignature, (pBuf + os), 16);
  
  return true;
}

/////////////////////////////////////////////////////////////////////////////////
// DSI Requests (Base/Sync/Async)
/////////////////////////////////////////////////////////////////////////////////
CDSIRequest::CDSIRequest(uint16_t id, CDSIBuffer* pResponseBuf/*=NULL*/) : 
  m_Id(id), 
  m_Result(kNoError), 
  m_TotalBytes(0), 
  m_BytesRemaining(0), 
  m_Pieces(0), 
  m_pResponse(pResponseBuf) 
{

};

CDSIRequest::~CDSIRequest() 
{
};

int CDSIRequest::SaveResponse(CTCPPacketReader& reader, uint32_t totalBytes)
{
  // TODO: Handle case where we don't find what we expected (maybe client timed-out...) - another use for buffer 'skip' code
  // TODO: Handle case where this is called, but we already have an ongoing operation in-process
  if (totalBytes && m_pResponse)
  {
    // Reset the read context
    m_Pieces = 0;
    m_TotalBytes = totalBytes;
    m_BytesRemaining = totalBytes;
    
    m_pResponse->Resize(m_TotalBytes); // Make sure we have somewhere to put the data (including possible ongoing reads)

    int bytesRead = reader.Read(m_pResponse->GetData(), m_TotalBytes);
    if (bytesRead > 0) // All OK
    {  
      // TODO: Shouldn't the current data len always be zero when we start? Why do we need GetDataLen() here?
      m_pResponse->SetDataLen(m_pResponse->GetDataLen() + bytesRead); // Size so far (might not be final size)
      m_BytesRemaining -= (bytesRead > m_BytesRemaining) ? m_BytesRemaining : bytesRead;
      
      // If the requested reply block is larger than the underlying layer's PDU,
      // we will have to wait for multiple packets. These will be sent serially, with no
      // other interleaved requests/replies. To handle this, we need to either store the
      // current reply context and continue it on the next callback, or invoke a sync read,
      // since we know the next block of data is sequential.
      // NOTE: We are trusting the server re: the data length here. If it is wrong, we will 
      // wait forever... There is no other way to know when an operation has failed...
      // (except the underlying protocol...)
      // TODO: We should probably make sure the result code is success before we plan to read forever...
      m_Pieces++;
      if (bytesRead < m_TotalBytes)
        XAFP_LOG(XAFP_LOG_FLAG_DSI_PROTO, "DSI Protocol: Began receiving multi-PDU read response (expected: %d, read: %d)",m_TotalBytes, m_BytesRemaining);
    }
  }
  
  return 0;
}

int CDSIRequest::AppendResponse(CTCPPacketReader& reader)
{
  uint8_t* pDest = (uint8_t*)m_pResponse->GetData() + m_TotalBytes - m_BytesRemaining;
  int bytesRead = reader.Read(pDest, m_BytesRemaining);
  
  if (bytesRead > 0)
    UpdateOngoing(bytesRead);
  
  return bytesRead;
}

bool CDSIRequest::IsOngoing()
{
  // A response is ongoing if there is supposed to be a payload, and some of it remains to be read
  return ((m_TotalBytes > 0) && (m_BytesRemaining > 0));
}

uint32_t CDSIRequest::UpdateOngoing(uint32_t bytesRead)
{
  m_BytesRemaining -= (bytesRead > m_BytesRemaining) ? m_BytesRemaining : bytesRead;
  m_Pieces++;
  return m_BytesRemaining;
}

// Sync
//////////
CDSISyncRequest::CDSISyncRequest(uint16_t id, CDSIBuffer* pResponseBuf/*=NULL*/) : CDSIRequest(id, pResponseBuf)
{
  m_pEvent = new CThreadSyncEvent();
}
CDSISyncRequest::~CDSISyncRequest()
{
  delete m_pEvent;
}

int CDSISyncRequest::Wait(int timeout/*=-1*/)
{
  // TODO: Implement error code for timeout
  if (m_pEvent->Wait(timeout))
    return -1; // The call timed-out
  
  return m_Result;
}

void CDSISyncRequest::Cancel(uint32_t reason)
{
  m_Result = reason;
  m_pEvent->Set();    
}
void CDSISyncRequest::Complete(uint32_t result)
{
  m_Result = result;
  m_pEvent->Set();
}

void CDSISyncRequest::Signal()
{
  m_pEvent->Set();
}  CThreadSyncEvent* m_pEvent;

// Async
//////////
CDSIAsyncRequest::CDSIAsyncRequest(uint16_t id, CDSIAsyncCallback* pCallback) : 
  CDSIRequest(id, &m_Buffer),
  m_pCallback(pCallback)
{
}

CDSIAsyncRequest::~CDSIAsyncRequest()
{
  delete m_pCallback;  
}

void CDSIAsyncRequest::Cancel(uint32_t reason)
{
  m_Result = reason;
}

void CDSIAsyncRequest::Complete(uint32_t result)
{
  if (!m_pCallback)
    return;
  
  m_Result = result;

  DSIAsyncResult res;
  res.err = result;
  res.pResponse = m_pResponse;
  m_pCallback->Call(&res);

  // Self-destruct
  delete this;
}

/////////////////////////////////////////////////////////////////////////////////
// DSI Session
/////////////////////////////////////////////////////////////////////////////////
CDSISession::CDSISession() :
  m_LastRequestId(0),
  m_IsOpen(false),
  m_pOngoingReply(NULL)
{
  
}

CDSISession::~CDSISession()
{
  Close();
}

bool CDSISession::Open(const char* pHostName, unsigned int port, unsigned int timeout /*=3000*/)
{
  XAFP_LOG(XAFP_LOG_FLAG_INFO, "DSI Protocol: Opening session for %s:%d", pHostName, port);

  if (!GetStatus(pHostName, port, m_ServerInfo))
    return false; // Report's its own errors
  
  if (!Connect(pHostName, port, (ITCPReceiveCallback*)this, timeout))
    return false; // Report's its own errors

  CDSIBuffer reqBuf(6);
  reqBuf.Write((uint8_t)0x01);  // Session Option Type (Attention Quantum)
  reqBuf.Write((uint8_t)4);     // Option Data Size
  reqBuf.Write((uint32_t)1024); // Option Value
  
  // Add Request to map so receive handler can notify us
  CDSIBuffer response; // TODO: Remove this once the handler is able to ignore payloads
  CDSISyncRequest req(GetNewRequestId(), &response);
  AddRequest(&req);

  int err = SendMessage(DSIOpenSession, req.GetId(), &reqBuf); // Report's its own errors
  if (!err)
  {
    // The request went out, now wait for a reply
    err = req.Wait();
    if (!err)
    {
      XAFP_LOG(XAFP_LOG_FLAG_INFO, "DSI Protocol: Opened session for %s:%d", pHostName, port);
      m_IsOpen = true;
    }
    else
    {
      XAFP_LOG(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Failed to open session for %s:%d. Error: %d", pHostName, port, err);
    }
  }
  else if (err == kTCPSendError)
  {
    // Something went wrong while sending the request
    XAFP_LOG(XAFP_LOG_FLAG_ERROR, "DSI Protocol: TCP error while sending session request to %s:%d. Error: %d", pHostName, port, err);
    // TODO: Do we want to do this?
    Disconnect();
  }
  else
  {
    XAFP_LOG(XAFP_LOG_FLAG_ERROR, "DSI Protocol: An error occurred while sending session request to %s:%d. Error: %d", pHostName, port, err);
  }
  
  return (!err);
}

void CDSISession::Close()
{
  m_IsOpen = false;
  if (!IsConnected())
    return;
  
  SendMessage(DSICloseSession, GetNewRequestId());
  CTCPSession::Disconnect();
  XAFP_LOG_0(XAFP_LOG_FLAG_INFO, "DSI Protocol: Closed session");
}

int32_t CDSISession::SendCommand(CDSIBuffer& payload, CDSIBuffer* pResponse /*=NULL*/, uint32_t writeOffset /*=0*/)
{
  if (!IsOpen())
  {
    XAFP_LOG_0(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Attempting to use closed session");
    return kDSISessionClosed;
  }
  
// Sync Request handling  
  CDSISyncRequest req(GetNewRequestId(), pResponse);
  AddRequest(&req); // Add Request to map so receive handler can notify us
  
  // TODO: This is a messy way to get the AFP command Id...
  XAFP_LOG(XAFP_LOG_FLAG_DSI_PROTO, "DSI Protocol: Sending command (%s), requestId: %d", AFPProtoCommandToString(*((uint8_t*)payload.GetData())), req.GetId());

  int err = kNoError;
  if (writeOffset)
  {
    err = SendMessage(DSIWrite, req.GetId(), &payload, writeOffset);
  }
  else
  {
    err = SendMessage(DSICommand, req.GetId(), &payload);
  }
  
  if (!err)
  {
    // The DSICommand message always expects a response, even if the caller does not (no return payload)
    // Wait for the response and return the result to the caller
    return req.Wait();
  }
  else
  {
    // TODO: This is a messy way to get the AFP command Id...
    XAFP_LOG(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Failed to send command (%s), requestId: %d, result: %d", AFPProtoCommandToString(*((uint8_t*)payload.GetData())), req.GetId(), err);
  }
  
  return err;
}

int32_t CDSISession::SendCommandAsync(CDSIBuffer& payload, CDSIAsyncCallback* pCallback, uint32_t writeOffset /*=0*/)
{
  if (!IsOpen())
  {
    XAFP_LOG_0(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Attempting to use closed session");
    return kDSISessionClosed;
  }
  
  // The DSICommand message always expects a response, even if the caller does not (no return payload)
  if (!pCallback)
  {
    XAFP_LOG_0(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Invalid callback passed to SendCommandAsync()");
    return kFPParamErr;
  }
  
  // Async Request handling
  CDSIAsyncRequest* pReq =  new CDSIAsyncRequest(GetNewRequestId(), pCallback); // This object will self-destruct upon completion
  if (!AddRequest(pReq)) // Add Request to map so receive handler can notify us (do this before calling SendMessage, to make sure it is there in time)
  {
    // This really should not happen...ever
    delete pReq;
    return kFPParamErr;
  }
  
  // TODO: This is a messy way to get the AFP command Id...
  XAFP_LOG(XAFP_LOG_FLAG_DSI_PROTO, "DSI Protocol: Sending async command (%s), requestId: %d", AFPProtoCommandToString(*((uint8_t*)payload.GetData())), pReq->GetId());
  
  int err = kNoError;
  if (writeOffset)
  {
    err = SendMessage(DSIWrite, pReq->GetId(), &payload, writeOffset);
  }
  else
  {
    err = SendMessage(DSICommand, pReq->GetId(), &payload);
  }
  
  if (err)
  {
    XAFP_LOG(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Failed to send async command (%s), requestId: %d, result: %d", AFPProtoCommandToString(*((uint8_t*)payload.GetData())), pReq->GetId(), err);
    delete pReq; // This will also clean-up the callback (closure) object
  }
  return err;
}

// Async Read callback
// TODO: Should we add a 'hint' here to let the reader know that we are waiting for
// a certain number of additional bytes? (to prevent unecessary loops)...
void CDSISession::OnReceive(CTCPPacketReader& reader)
{
  DSIHeader hdr;  
  // Technically, we should probably be locking more in here, but we know that there is only one read thread, so 
  // there will never be concurrent calls into this method...
  
  // Ongoing Reply...
  /////////////////////////////////////////////////////
  // TODO: Should we time out after some period...?
  if (m_pOngoingReply) // Do we have a previously-incomplete reply block?
  {
    int err = kNoError;
    // Continue working on the incomplete read...
    int bytesRead = m_pOngoingReply->AppendResponse(reader);    
    if (bytesRead > 0) // All OK
    {
      if (m_pOngoingReply->GetBytesRemaining()) // Not quite done yet...
        return; // Wait some more
    }
    else if(!bytesRead)
    {
      XAFP_LOG(XAFP_LOG_FLAG_ERROR, "DSI Protocol: TCPPacketReader returned zero bytes (expected: %d)", reader.GetBytesLeft());
    }
    else
    {
      XAFP_LOG(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Unable to read response - Error: %d", bytesRead);
      err = kTCPReceiveError;
    }
    
    // We either completed the reply block, or an error occurred. Signal waiting caller
    XAFP_LOG(XAFP_LOG_FLAG_DSI_PROTO, "DSI Protocol: Finished receiving multi-PDU read response (expected: %d, read: %d, pieces: %d)",m_pOngoingReply->GetTotalBytes(), m_pOngoingReply->GetTotalBytes() - m_pOngoingReply->GetBytesRemaining(), m_pOngoingReply->GetPieces());
    m_pOngoingReply->Complete(err); 
    m_pOngoingReply = NULL;
    return;
  }
  
  // 'Standard' Message Block(s) or beginning of a multi-PDU reply
  /////////////////////////////////////////////////////
  while (reader.GetBytesLeft())
  {
    // Read the message header
    int err = reader.Read(&hdr, sizeof(DSIHeader));
    if (err < 0)
    {
      XAFP_LOG(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Unable to read response header - Error: %d", err);
      // TODO: Nothing after this is going to work...figure that out
      return;
    }
    
    translate_header(hdr); // Change from network to host representation
    
    if (!hdr.flags) // This is a server-initiated request (as opposed to a reply to one of our requests)
    {
      switch (hdr.command)
      {
        case DSITickle: // Used by server for 'heartbeat' monitoring
          // TODO: Track time since last notice to detect timeouts
          XAFP_LOG_0(XAFP_LOG_FLAG_DSI_PROTO, "DSI Protocol: Received Server 'Tickle'. Sending reply...");
          SendMessage(DSITickle, GetNewRequestId()); // Send keep-alive reply to server
          break;
        case DSIAttention: // Used by server to notify clients of events
          uint16_t attData;
          // TODO: Implement reconnect handler and heed delay specified by server in AFPUserBytes
          // TODO: Skip unexpected bytes, if present
          // TODO: Retrieve quantum size during login
          if (hdr.totalDataLength > 2)
            XAFP_LOG(XAFP_LOG_FLAG_ERROR,"DSI Protocol: ****Unexpected Size for Attention Data**** (%d bytes)", hdr.totalDataLength);
          reader.Read(&attData, sizeof(uint16_t)); // TODO: Where should we handle read() failures
          attData = ntohs(attData); // Handle byte-ordering
          XAFP_LOG(XAFP_LOG_FLAG_INFO, "DSI Protocol: Received Server Attention. Flags - Shutdown:%s, ServerCrash:%s, Message:%s, NoReconnect:%s",
                   (kShutDownNotifyMask & attData) ? "yes" : "no",
                   (kAllowReconnectMask & attData) ? "yes" : "no",
                   (kMsgNotifyMask & attData) ? "yes" : "no",
                   (kDisconnectNotifyMask & attData) ? "yes" : "no"
                   );
          // TODO: Retrieve the server message when there is one
          OnAttention(attData);
          break;
        case DSICloseSession: // Notification from server that the session will be closed. 
          // Signal all waiting callers and tell them something happened
          XAFP_LOG_0(XAFP_LOG_FLAG_INFO, "DSI Protocol: Server Closed Session. Canceling all pending requests");
          SignalAll(kDSISessionClosed);
          // TODO: Clean-up session (and possibly re-open?)
          break;  
        default:
          XAFP_LOG(XAFP_LOG_FLAG_ERROR,"DSI Protocol: Received unknown request - command: %d", hdr.command);
      }
    }
    else // This is a reply to a previously-sent message
    {
      CDSIRequest* pRequest = NULL;
      
      switch (hdr.command)
      {
        case DSICloseSession: // This should not happen, but handle it anyway...
          XAFP_LOG_0(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Unexpected reply message - CloseSession");        
          // Signal all waiting callers and tell them something happened
          SignalAll(kDSISessionClosed);
          // TODO: Clean-up session (should already be done...)
          break;
        case DSIOpenSession:
        case DSICommand:
        case DSIWrite:
        case DSIGetStatus:
          XAFP_LOG(XAFP_LOG_FLAG_DSI_PROTO, "DSI Protocol: Received Reply - Message:%s, RequestId:%d, result:%d", DSIProtoCommandToString(hdr.command), hdr.requestID, hdr.errorCode);
          // Find the request in our request map
          pRequest = RemoveRequest(hdr.requestID);
          if (pRequest)
          {
            // Tranfer data into caller-supplied buffer, if one was provided
            // If not, they did not expect any data back, just a result code
            // TODO: Make sure all data in the message is read/skipped before moving on, to prevent 
            // clogging-up the pipe...
            pRequest->SaveResponse(reader, hdr.totalDataLength);
            if (pRequest->IsOngoing())
            {
              m_pOngoingReply = pRequest; 
              return; // Wait for more data before signaling requestor...
            }
            // Signal waiting requestor
            // NOTE: This object may self-destruct (or go out of scope) immediately. Do not use the reference again after calling 'Complete'
            pRequest->Complete(hdr.errorCode);
          }
          else
          {
            XAFP_LOG(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Unexpected reply - RequestId: %d", hdr.requestID);
            // TODO: Need a better flush/skip/seek method
            void* p = malloc(hdr.totalDataLength);
            reader.Read(p, hdr.totalDataLength);
            free(p);
          }
          break;
        default:
          // TODO: Skip payload data and try to recover
          XAFP_LOG(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Received unknown reply - command: %d, payload: %d", hdr.command, hdr.totalDataLength);
      }
    }
  }
}

void CDSISession::OnAttention(uint16_t attData)
{
  // Do nothing. Default implementation.
}

bool CDSISession::AddRequest(CDSIRequest* pRequest)
{
  // TODO: Prevent anyone from changing the map while we use it
  uint16_t id = pRequest->GetId();
  if (m_Requests.find(id) == m_Requests.end())
  {
    m_Requests[pRequest->GetId()] = pRequest;
    return true;
  }
  // This should only happen if the id-generation code is broken...
  XAFP_LOG(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Received request with duplicate id [%d]. Ignoring this request.", id);
  return false;
}

CDSIRequest* CDSISession::RemoveRequest(uint16_t id)
{
  // TODO: Prevent anyone from changing the map while we use it
  CDSIRequest* pReq = m_Requests[id];
  m_Requests.erase(id);
  return pReq;
}

void CDSISession::SignalAll(int err)
{
  XAFP_LOG(XAFP_LOG_FLAG_INFO, "DSI Protocol: Signaling %d waiting callers", m_Requests.size());

  // TODO: Prevent anyone from adding to the map while we iterate
  std::map<int, CDSIRequest*>::iterator it;
  for (it = m_Requests.begin(); it != m_Requests.end(); it++)
  {
    // TODO: Add error code
    (*it).second->Cancel(1);
  }
}

int32_t CDSISession::SendMessage(uint8_t messageId, uint16_t requestId, CDSIBuffer* pPayload /*=NULL*/, uint32_t writeOffset /*=0*/)
{
  if (!IsConnected())
  {
    XAFP_LOG_0(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Attempting to use closed session");
    return kTCPNotConnected;
  }
  
  // We want to limit calls to the TCP layer, so try and use the buffer provided by the caller to hold the header.
  // If they did not provide one, we still only make one call, but we need to have our own space
  DSIHeader hdr; // Local instance, if no buffer was provided by caller
  DSIHeader* pHeader;
  if (pPayload)
    pHeader = (DSIHeader*)pPayload->GetHeader();
  else
    pHeader = &hdr;
  
  // Build the header
  pHeader->flags = 0; // Request
  pHeader->command = messageId;
  pHeader->requestID = htons(requestId);
  pHeader->writeOffset = htonl(writeOffset);
  uint32_t payloadLen = (pPayload == NULL) ? 0 : pPayload->GetDataLen();
  pHeader->totalDataLength = htonl(payloadLen);
  pHeader->reserved = 0;
  
  XAFP_LOG(XAFP_LOG_FLAG_DSI_PROTO, "DSI Protocol: Sending message: %s, requestId: %d", DSIProtoCommandToString(messageId), requestId);
  
  int sendSize = sizeof(DSIHeader) + payloadLen;
  if (SendData(pHeader, sendSize) != sendSize)
    return kTCPSendError;
  
  return kNoError;
}

// This uses the TCPSession's Synchronous Mode
bool CDSISession::GetStatus(const char* pHostName, unsigned int port, CFPServerInfo& info)
{
  CTCPSession s;
  if (!s.Connect(pHostName, port)) // Synchronous connection
    return false;
  
  DSIHeader hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.command = DSIGetStatus;
  
  int bytes = s.SendData(&hdr, sizeof(hdr));
  if (bytes)
  {
    CTCPPacketReader* pReader = NULL;
    bytes = s.ReceiveData(&pReader, 3000);
    if (bytes <= 0)
    {
      XAFP_LOG(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Unable to retrieve server status - Error: %d", bytes);          
    }
    else
    {
      pReader->Read(&hdr, sizeof(hdr));
      if (ntohl(hdr.errorCode) == kNoError)
      {
        uint8_t* pBuf =  (uint8_t*)malloc(bytes);
        pReader->Read(pBuf, bytes);
        if (info.Parse(pBuf, bytes))
        {
          XAFP_LOG(XAFP_LOG_FLAG_INFO, "DSI Protocol: Retrieved server status - Name: %s, Type: %s", info.GetServerName(), info.GetMachineType());
          return true;
        }
        else
          XAFP_LOG_0(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Error encountered while parsing server status");
      }
      else
      {
        XAFP_LOG(XAFP_LOG_FLAG_ERROR, "DSI Protocol: Error encountered while retrieving server status - Error: %d", ntohl(hdr.errorCode));          
        
      }
    }
  }
  s.Disconnect();
  
  return false;
}