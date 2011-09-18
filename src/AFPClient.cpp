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

#include <openssl/dh.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/md2.h>

#include "AFPClient.h"
#include "AFPProto.h"

// AFP File and Directory Parameter Handling
/////////////////////////////////////////////////////////////////////////////////
// Node dates are represented by the number of seconds since '01/01/2000 00:00:00.0 UTC'
#define AFPTimeToTime_t(t) (t + ((2000 - 1970) * 365 * 86400) + (7 * 86400)) // Adjust for differences in epoch, including leap years

CNodeParams::CNodeParams() :
  m_pName(NULL)
{
  // TODO: Initialize remaining members
}

CNodeParams::~CNodeParams()
{
  free(m_pName);
}

int CNodeParams::Parse(uint32_t bitmap, uint8_t* pData, uint32_t size)
{
  uint8_t* p = pData;
  
  if (bitmap & kFPAttributeBit)
  {
    m_Attributes = ntohs(*(uint16_t*)p);
    p += sizeof(uint16_t);
  }
  if (bitmap & kFPParentDirIDBit)
  {
    m_ParentId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t);
  }
  if (bitmap & kFPCreateDateBit)
  {
    m_CreateDate = AFPTimeToTime_t(ntohl(*(uint32_t*)p));
    p += sizeof(uint32_t);
  }
  if (bitmap & kFPModDateBit)
  {
    m_ModDate = AFPTimeToTime_t(ntohl(*(uint32_t*)p));
    p += sizeof(uint32_t);
  }
  if (bitmap & kFPBackupDateBit)
  {
    m_BackupDate = AFPTimeToTime_t(ntohl(*(uint32_t*)p));
    p += sizeof(uint32_t);
  }
  if (bitmap & kFPFinderInfoBit)
  {
    p += 32; // This information is not useful to us
  }
  if (bitmap & kFPLongNameBit)
    p += (*p + 1); // Skip the string. We are using the UTF8 name.
  if (bitmap & kFPShortNameBit)
    p += (*p + 1); // Skip the string. We are using the UTF8 name.
  if (bitmap & kFPNodeIDBit)
  {
    m_NodeId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t);
  }
  
  return (p - pData); // Read Offset
}

CDirParams::CDirParams(uint32_t bitmap, uint8_t* pData, uint32_t size) :
  CNodeParams()
{
  Parse(bitmap, pData, size);
  m_IsDirectory = true;
}

int CDirParams::Parse(uint32_t bitmap, uint8_t* pData, uint32_t size)
{
  int offset = CNodeParams::Parse(bitmap, pData, size);
  if (offset < 0)
    return -1;
  
  if (offset >= size) // Skip parsing if the base class 'used' all the data
    return offset;
  
  // Directory-specific params
  uint8_t* p = pData + offset;
  if (bitmap & kFPOffspringCountBit)
  {
    m_OffspringCount = ntohs(*(uint16_t*)p);
    p += sizeof(uint16_t);
  }
  if (bitmap & kFPOwnerIDBit)
  {
    m_OwnerId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t);
  }
  if (bitmap & kFPGroupIDBit)
  {
    m_GroupId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t);
  }
  if (bitmap & kFPAccessRightsBit)
  {
    m_AccessRights = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t);
  }
  
  // These params (UNICODE Name/UNIX Params) fall after the file/dir-specific ones, so we need to handle them in both places (easier than another method)
  // TODO: Inline this or add a method...
  if (bitmap & kFPUTF8NameBit)
  {
    uint16_t loc = ntohs(*((uint16_t*)p));
    uint8_t* pName = pData + loc;
    //uint32_t hint = *((uint32_t*)pName);
    pName += sizeof(uint32_t);
    uint16_t len = ntohs(*((uint16_t*)pName));
    pName += sizeof(uint16_t);
    m_pName = (char*)malloc(len + 1);
    memcpy(m_pName, pName, len);
    m_pName[len] = '\0'; // NULL-terminate the string
    p += sizeof (uint16_t); // The actual name is stored at an offset, so we only 'consume' one byte
    
    p += sizeof (uint32_t); // FIXME: There is padding in these packets. Why?
  }
  
  if (bitmap & kFPUnixPrivsBit)
  {
    m_UnixPrivs.userId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
    m_UnixPrivs.groupId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
    m_UnixPrivs.perms = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
    m_UnixPrivs.userRights = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
  }
  
  return (p - pData); // Bytes 'Consumed'
}

CFileParams::CFileParams(uint32_t bitmap, uint8_t* pData, uint32_t size) :
  CNodeParams()
{
  Parse(bitmap, pData, size);
  m_IsDirectory = false;
}

int CFileParams::Parse(uint32_t bitmap, uint8_t* pData, uint32_t size)
{
  int offset = CNodeParams::Parse(bitmap, pData, size);
  if (offset < 0)
    return -1;
  
  if (offset >= size) // Skip parsing if the base class 'used' all the data
    return offset;
  
  // File-specific params
  uint8_t* p = pData + offset;
  if (bitmap & kFPDataForkLenBit)
  {
    m_DataForkLen = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t);
  }
  if (bitmap & kFPRsrcForkLenBit)
  {
    m_ResourceForkLen = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t);
  }
  if (bitmap & kFPExtDataForkLenBit)
  {
    // TODO: Clean-up, and skip if 32-bit value is less-than 4GB
#ifdef BIG_ENDIAN
    uint32_t* pb = (uint32_t*)&m_DataForkLen;
    pb[0] = ntohl(*((uint32_t*)(p + sizeof(uint32_t))));
    pb[1] = ntohl(*(uint32_t*)p);
#else
    memcpy(&m_DataForkLen, p, sizeof(m_DataForkLen));
#endif
    p += sizeof(uint64_t);
  }
  // These params (UNICODE Name/UNIX Params) fall after the file/dir-specific ones, so we need to handle them in both places (easier than another method)
  // TODO: Inline this or add a method...
  if (bitmap & kFPUTF8NameBit)
  {
    uint16_t loc = ntohs(*((uint16_t*)p));
    uint8_t* pName = pData + loc;
    //uint32_t hint = *((uint32_t*)pName);
    pName += sizeof(uint32_t);
    uint16_t len = ntohs(*((uint16_t*)pName));
    pName += sizeof(uint16_t);
    m_pName = (char*)malloc(len + 1);
    memcpy(m_pName, pName, len);
    m_pName[len] = '\0'; // NULL-terminate the string
    p += sizeof (uint16_t); // The actual name is stored at an offset, so we only 'consume' one byte
    
    p += sizeof (uint32_t); // FIXME: There is padding in these packets. Why?
  }
  if (bitmap & kFPExtRsrcForkLenBit)
  {
    // TODO: Clean-up, and skip if 32-bit value is less-than 4GB
#ifdef BIG_ENDIAN
    uint32_t* pb = (uint32_t*)&m_ResourceForkLen;
    pb[0] = ntohl(*((uint32_t*)(p + sizeof(uint32_t))));
    pb[1] = ntohl(*(uint32_t*)p);
#else
    memcpy(&m_ResourceForkLen, p, sizeof(m_ResourceForkLen));
#endif
    p += sizeof(uint64_t);
  }
  if (bitmap & kFPUnixPrivsBit)
  {
    m_UnixPrivs.userId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
    m_UnixPrivs.groupId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
    m_UnixPrivs.perms = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
    m_UnixPrivs.userRights = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
  }  
  return (p - pData);
}


// AFP Node List/Iterator
/////////////////////////////////////////////////////////////////////////////////
// TODO: Only one iterator is available at a time...should this change?
CAFPNodeList::CAFPNodeList(CDSIBuffer* pBuf, uint32_t dirBitmap, uint32_t fileBitmap,int count) :
  m_pBuffer(pBuf),
  m_DirBitmap(dirBitmap),
  m_FileBitmap(fileBitmap),
  m_Count(count),
  m_Iter(pBuf, dirBitmap, fileBitmap)
{
  
}

CAFPNodeList::~CAFPNodeList()
{
  delete m_pBuffer;
}

CAFPNodeList::Iterator::Iterator(CDSIBuffer* pBuf, uint32_t dirBitmap, uint32_t fileBitmap) :
  m_DirBitmap(dirBitmap),
  m_FileBitmap(fileBitmap),
  m_pCurrent(NULL)
{
  m_pData = ((uint8_t*)pBuf->GetData()) + 6;
  m_pEnd = m_pData + pBuf->GetDataLen() - 6;
  
  //m_pCurrent = MoveNext(); // Parse the first node
};

CNodeParams* CAFPNodeList::Iterator::MoveNext()
{
  if (m_pData >= m_pEnd)
  {
    m_pCurrent = NULL;
    return NULL; // We already reached the end
  }
  
  uint16_t len = ntohs(*((uint16_t*)m_pData)); // Entry size
  if ((m_pData + len) > m_pEnd) // Make sure we don't fall off the end of the world...
  {
    m_pData = m_pEnd;
    m_pCurrent = NULL;
    return NULL;
  }
  
  if (*(m_pData + sizeof(uint16_t)) & 0x80) // Directory bit + pad(7bits) -- followed by another byte of padding
  {
    m_DirParams.Parse(m_DirBitmap, m_pData+4, len-4);
    m_pCurrent = &m_DirParams;
  }
  else
  {
    m_FileParams.Parse(m_FileBitmap, m_pData+4, len-4);
    m_pCurrent = &m_FileParams;
  }
  m_pData += len;
  return m_pCurrent;
}


// AFP Session Handling
/////////////////////////////////////////////////////////////////////////////////
CAFPSession::CAFPSession() :
  m_LoggedIn(false)
{
  
}

CAFPSession::~CAFPSession()
{
  Logout();
  Close();
}

bool CAFPSession::Login(const char* pUsername, const char* pPassword)
{
  m_LoggedIn = LoginClearText(pUsername, pPassword);
  return m_LoggedIn;
}   

bool CAFPSession::LoginClearText(const char* pUsername, const char* pPassword)
{
  CDSIBuffer reqBuf(strlen(kAFPVersion_3_1) + strlen(kClearTextUAMStr) + strlen(pUsername) + 8 + 5);
  reqBuf.Write((uint8_t)FPLogin); // Command
  reqBuf.Write((char*)kAFPVersion_3_1); // AFP Version
  reqBuf.Write((char*)kClearTextUAMStr); // UAM
  reqBuf.Write((char*)pUsername);
  reqBuf.Write((uint8_t)0); // Pad
  char pwd[10];
  memset(pwd,0,8);
  memcpy(pwd, pPassword, strlen(pPassword));
  reqBuf.Write((void*)pwd, 8); // Password parameter is 8 bytes long

  uint32_t err = SendCommand(reqBuf);
  return (err == kNoError);
}

void CAFPSession::Logout()
{
  if (!IsLoggedIn())
    return;
  
  m_LoggedIn = false;
  
  CDSIBuffer reqBuf(2);
  reqBuf.Write((uint8_t)FPLogout); // Command
  reqBuf.Write((uint8_t)0); // Pad
  SendCommand(reqBuf);
}

int CAFPSession::OpenVolume(const char* pVolName)
{
  if (!IsLoggedIn())
    return kFPUserNotAuth;
 
  CDSIBuffer reqBuf(4 + 1 + strlen(pVolName));
  reqBuf.Write((uint8_t)FPOpenVol); // Command
  reqBuf.Write((uint8_t)0); // Pad
  uint16_t bitmap = kFPVolIDBit;
  reqBuf.Write((uint16_t)bitmap); // Bitmap
  reqBuf.Write((char*)pVolName);
  
  CDSIBuffer replyBuf;
  uint32_t err = SendCommand(reqBuf, &replyBuf);
  if (err == kNoError)
  {
    bitmap = replyBuf.Read16();
    if (bitmap & kFPVolIDBit)
    {
      uint16_t volID = replyBuf.Read16();
      return volID;      
    }
  }
  return kNoError;  
}

void CAFPSession::CloseVolume(int volId)
{
  CDSIBuffer reqBuf(4);
  reqBuf.Write((uint8_t)FPCloseVol); // Command
  reqBuf.Write((uint8_t)0); // Pad
  reqBuf.Write((uint16_t)volId); // Bitmap
  
  SendCommand(reqBuf);
}

// The Directory ID of the root is always 2
// TODO: Is this thread-safe?
// TODO: How do we specify the whole path at once?
int CAFPSession::GetDirectory(int volumeID, const char* pPath)
{
  char* pBuf = (char*)malloc(strlen(pPath) + 1);
  strcpy(pBuf, pPath);
  char* pElement = strtok(pBuf, "/");
  int parentId = 2;
  int dirId = 0;
  // Recurse into directory tree
  while(pElement)
  {
    dirId = OpenDir(volumeID, parentId, pElement);
    pElement = strtok(NULL, "/");
    parentId = dirId;
  }
  free(pBuf);
  
  return dirId;
}

int CAFPSession::OpenDir(int volumeID, int parentID, const char* pName)
{
  if (!IsLoggedIn())
    return 0;
  
  CDSIBuffer reqBuf(13 + 4 + 1 + strlen(pName));
  reqBuf.Write((uint8_t)FPGetFileDirParms); // Command
  reqBuf.Write((uint8_t)0); // Pad
  reqBuf.Write((uint16_t)volumeID);
  reqBuf.Write((uint32_t)parentID);
  reqBuf.Write((uint16_t)NULL); // File Bitmap
  uint16_t bitmap = kFPNodeIDBit;
  reqBuf.Write((uint16_t)bitmap); // Directory Bitmap
  reqBuf.Write((uint8_t)3); // kFPUTF8Name
  reqBuf.Write((uint32_t)0x08000103);
  uint16_t nameLen = strlen(pName);
  reqBuf.Write((uint16_t)nameLen); // Name Length
  reqBuf.Write((void*)pName, nameLen);

  CDSIBuffer replyBuf;
  uint32_t err = SendCommand(reqBuf, &replyBuf);
  if (err == kNoError)
  {
    // TODO: Validate returned bitmap vs provided one
    replyBuf.Read16(); // Skip File Bitmap (we already specified)
    replyBuf.Read16(); // Skip Directory Bitmap (we already specified)
    bool isDir = replyBuf.Read8() & 0x80; // Directory bit + pad(7bits)
    replyBuf.Read8(); // Pad
    if (isDir)
    {
      CDirParams params(bitmap, (uint8_t*)replyBuf.GetData() + 6, replyBuf.GetDataLen() - 6);
      return params.GetNodeId();
    }
  }  
  return 0; // TODO: Need error codes for callers...
}

int CAFPSession::GetNodeList(CAFPNodeList** ppList, int volumeID, int dirID)
{
  if (!IsLoggedIn())
    return kFPUserNotAuth;

  if (!ppList)
    return kParamError;
  
  // TODO: handle instances that require multiple calls (too many entries for one block)
  CDSIBuffer reqBuf(29); // TODO: This method of sizing the request block is WAAAY too error-prone...
  // TODO: FPEnumerateExt2 requires AFPv3.1 - need to add support for FPEnumerateExt for AFPv3.0
  reqBuf.Write((uint8_t)FPEnumerateExt2); // Command
  reqBuf.Write((uint8_t)0); // Pad
  reqBuf.Write((uint16_t)volumeID);
  reqBuf.Write((uint32_t)dirID);
  uint16_t fileBitmap = kFPAttributeBit | kFPUTF8NameBit | kFPUnixPrivsBit | kFPModDateBit | kFPExtDataForkLenBit;
  reqBuf.Write((uint16_t)fileBitmap); // File Bitmap
  uint16_t dirBitmap = kFPAttributeBit | kFPUTF8NameBit | kFPUnixPrivsBit | kFPModDateBit | kFPOffspringCountBit;
  reqBuf.Write((uint16_t)dirBitmap); // Directory Bitmap
  reqBuf.Write((uint16_t)20); // Max Results
  reqBuf.Write((uint32_t)1); // Start Index
  reqBuf.Write((uint32_t)5280); // Max Reply Size
  reqBuf.Write((uint8_t)3); // kFPUTF8Name
  reqBuf.Write((uint32_t)0x08000103); // Unknown
  reqBuf.Write((uint16_t)0); // Name Length
  
  CDSIBuffer* pReplyBuf = new CDSIBuffer();
  uint32_t err = SendCommand(reqBuf, pReplyBuf);
  if (err == kNoError)
  {
    // TODO: Validate returned bitmap vs provided one
    pReplyBuf->Read16(); // Skip File Bitmap (we already specified)
    pReplyBuf->Read16(); // Skip Directory Bitmap (we already specified)
    int count = pReplyBuf->Read16(); // Results Returned
    *ppList = new CAFPNodeList(pReplyBuf, dirBitmap, fileBitmap, count);
    return kNoError;
  }
  else
  {
    delete pReplyBuf;
    return err;
  }
}

int CAFPSession::OpenFile(int volumeId, int dirId, const char* pName)
{
  if (!IsLoggedIn())
    return 0;

  uint16_t nameLen = strlen(pName);
  CDSIBuffer reqBuf(17 + nameLen + 1);
  reqBuf.Write((uint8_t)FPOpenFork); // Command
  reqBuf.Write((uint8_t)0); // Flag (File Fork)
  reqBuf.Write((uint16_t)volumeId);
  reqBuf.Write((uint32_t)dirId);
  reqBuf.Write((uint16_t)0); // No need to return any parameters
  reqBuf.Write((uint16_t)kFPForkRead); // Open read-only
  reqBuf.Write((uint8_t)3); // kFPUTF8Name
  reqBuf.Write((uint32_t)0x08000103);
  reqBuf.Write((uint16_t)nameLen); // Name Length
  reqBuf.Write((void*)pName, nameLen);
  
  CDSIBuffer replyBuf;
  uint32_t err = SendCommand(reqBuf, &replyBuf);
  if (err == kNoError)
  {
    replyBuf.Read16(); // Skip bitmap. We didn't request any params
    uint16_t forkId = replyBuf.Read16();
    return forkId;
  } 
  return 0;
}

void CAFPSession::CloseFile(int forkId)
{
  CDSIBuffer reqBuf(4);
  reqBuf.Write((uint8_t)FPCloseFork); // Command
  reqBuf.Write((uint8_t)0); // Flag (File Fork)
  reqBuf.Write((uint16_t)forkId);

  SendCommand(reqBuf, NULL);
}

int CAFPSession::ReadFile(int forkId, uint64_t offset, void* pBuf, uint32_t len)
{
  CDSIBuffer reqBuf(20);
  reqBuf.Write((uint8_t)FPReadExt); // Command
  reqBuf.Write((uint8_t)0); // Pad
  reqBuf.Write((uint16_t)forkId);
  reqBuf.Write((uint64_t)offset); // Offset
  reqBuf.Write((uint64_t)len); // Bytes to Read

  // TODO: implement stateful read tracking to allow callers to read incrementally
  CDSIBuffer replyBuf;
  uint32_t err = SendCommand(reqBuf, &replyBuf);
  if (err == kNoError)
  {
    // TODO: This is a useless copy...come up with a better way
    memcpy(pBuf, replyBuf.GetData(), len);
    return len;
  }
  
  return 0;
}

bool CAFPSession::ParseFileDirParams(CDSIBuffer* pBuffer)
{
  return false;
}

bool CAFPSession::ParseDirParams(uint16_t bitmap, CDSIBuffer* pBuffer, CDirParams& params)
{
  return false;
}

bool CAFPSession::ParseFileParams(uint16_t bitmap, CDSIBuffer* pBuffer, CFileParams& params)
{
  return false;
}

/*
 // http://developer.apple.com/library/mac/#documentation/Networking/Conceptual/AFP/AFPSecurity/AFPSecurity.html
 // DHX2 Initialization Vectors (IV)
 uint8_t C2SIV[] = { 0x4c, 0x57, 0x61, 0x6c, 0x6c, 0x61, 0x63, 0x65};
 uint8_t S2CIV[] = { 0x43, 0x4a, 0x61, 0x6c, 0x62, 0x65, 0x72, 0x74};
 
 bool CAFPSession::LoginDHX2(const char* pUsername, const char* pPassword)
 {
 CDSIPacket pkt(DSICommand, GetNewRequestId(), strlen(kAFPVersion_3_1) + strlen(kDHX2UAMStr) + strlen(pUsername) + 4);
 pkt.Write((uint8_t)FPLogin); // Command
 pkt.Write((char*)kAFPVersion_3_1); // AFP Version
 pkt.Write((char*)kDHX2UAMStr); // UAM
 pkt.Write((char*)pUsername);
 uint32_t err;
 CDSIPacket reply;
 SendReceiveAFPPacket(pkt, reply, *this, err);
 if (err)
 if (err != (uint32_t)kFPAuthContinue)
 return false;
 
 //Reply block will contain:
 // *   Transaction ID (2 bytes, MSB)
 // *   g: primitive mod p sent by the server to the client(4 bytes, MSB)
 // *   length of large values in bytes (2 bytes, MSB)
 // *   p (minimum 64 bytes, indicated by length value, MSB)
 // *   Mb (minimum 64 bytes, indicated by length value, MSB)
 
 // Populate a Diffie Hellman key structure
 DH* pDH = DH_new();
 
 uint16_t id = reply.Read16();
 
 uint8_t g[4];
 reply.Read(g, 4);
 pDH->g = BN_bin2bn((unsigned char*)g, 4, NULL);
 
 uint16_t len = reply.Read16();
 
 uint8_t* p = (uint8_t*)malloc(len);
 reply.Read(p, len);
 pDH->p = BN_bin2bn(p, len, NULL);
 free(p);
 
 uint8_t* Mb = (uint8_t*)malloc(len);
 reply.Read(Mb, len); // Server's public key
 BIGNUM* bn_Mb = BN_bin2bn(Mb, len, NULL);
 free(Mb);
 
 // Generate our 'public' key (Ma)
 if (DH_generate_key(pDH))
 {
 // Compute the shared key value, K (sesion key)
 uint8_t* K = (uint8_t*)malloc(DH_size(pDH));
 int keySize = DH_compute_key(K, bn_Mb, pDH);
 
 // Calculate the MD5 hash of K
 EVP_MD_CTX mdctx;
 uint8_t K_md5[EVP_MAX_MD_SIZE];
 uint32_t md_len;
 EVP_MD_CTX_init(&mdctx);
 EVP_DigestInit_ex(&mdctx, EVP_md5(), NULL);
 EVP_DigestUpdate(&mdctx, K, keySize);
 EVP_DigestFinal_ex(&mdctx, K_md5, &md_len);
 EVP_MD_CTX_cleanup(&mdctx);    
 
 // Generate nonce bytes
 uint8_t nonce[16];
 RAND_bytes(nonce, 16);
 
 // Encrypt nonce using CAST 128 CBC cipher and key K_md5
 EVP_CIPHER_CTX ctx;
 EVP_CIPHER_CTX_init(&ctx);
 EVP_EncryptInit_ex(&ctx, EVP_cast5_cbc(), NULL, K_md5, C2SIV);
 EVP_CIPHER_CTX_set_key_length(&ctx, md_len);
 uint8_t* ai = (uint8_t*)malloc(len + 16);
 int ai_len = 0;
 int ret = EVP_EncryptUpdate(&ctx, ai, &ai_len, nonce, 16);
 int padLen = 0;
 ret = EVP_EncryptFinal_ex(&ctx, ai + ai_len, &padLen);
 //ai_len += padLen;
 EVP_CIPHER_CTX_cleanup(&ctx);
 
 // Write Sequence Packet 3 (Key Exchange)
 uint8_t* Ma = (uint8_t*)malloc(len);
 ret = BN_bn2bin(pDH->pub_key, Ma);
 pkt.Init(DSICommand, GetNewRequestId(), 4 + len + ai_len);
 pkt.Write((uint8_t)FPLoginCont);
 pkt.Write((uint16_t)id);
 pkt.Write(Ma, len);
 pkt.Write(ai, ai_len);
 free(Ma);
 free(ai);
 
 SendReceiveAFPPacket(pkt, reply, *this, err);
 if (err == (uint32_t)kFPAuthContinue)
 {
 
 }
 
 free(K);
 }
 DH_free(pDH); // Also frees component BIGNUMs
 free(Mb);
 
 return true;  
 }
 */