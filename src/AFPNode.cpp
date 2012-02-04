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
#include "AFPNode.h"

// AFP File and Directory Parameter Handling
/////////////////////////////////////////////////////////////////////////////////
// Node dates are represented by the number of seconds since '01/01/2000 00:00:00.0 UTC'
#define AFPTimeToTime_t(t) (t + ((2000 - 1970) * 365 * 86400) + (7 * 86400)) // Adjust for differences in epoch, including leap years

CNodeParams::CNodeParams()
{
  memset(&m_Info, 0, sizeof(m_Info));
}

CNodeParams::~CNodeParams()
{
  free(m_Info.name);
}

int CNodeParams::Parse(uint32_t bitmap, uint8_t* pData, uint32_t size)
{
  uint8_t* p = pData;
  
  if (bitmap & kFPAttributeBit)
  {
    m_Info.attributes = ntohs(*(uint16_t*)p);
    p += sizeof(uint16_t);
  }
  if (bitmap & kFPParentDirIDBit)
  {
    m_Info.parentId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t);
  }
  // TODO: Read server time at login and adjust all dates
  if (bitmap & kFPCreateDateBit)
  {
    m_Info.createDate = AFPTimeToTime_t(ntohl(*(uint32_t*)p));
    p += sizeof(uint32_t);
  }
  if (bitmap & kFPModDateBit)
  {
    m_Info.modDate = AFPTimeToTime_t(ntohl(*(uint32_t*)p));
    p += sizeof(uint32_t);
  }
  if (bitmap & kFPBackupDateBit)
  {
    m_Info.backupDate = AFPTimeToTime_t(ntohl(*(uint32_t*)p));
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
    m_Info.nodeId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t);
  }
  
  return (p - pData); // Read Offset
}

CDirParams::CDirParams(uint32_t bitmap, uint8_t* pData, uint32_t size) :
CNodeParams()
{
  Parse(bitmap, pData, size);
  m_Info.isDirectory = true;
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
    m_Info.dirInfo.offspringCount = ntohs(*(uint16_t*)p);
    p += sizeof(uint16_t);
  }
  if (bitmap & kFPOwnerIDBit)
  {
    m_Info.dirInfo.ownerId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t);
  }
  if (bitmap & kFPGroupIDBit)
  {
    m_Info.dirInfo.groupId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t);
  }
  if (bitmap & kFPAccessRightsBit)
  {
    m_Info.dirInfo.accessRights = ntohl(*(uint32_t*)p);
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
    m_Info.name = (char*)malloc(len + 1);
    memcpy(m_Info.name, pName, len);
    m_Info.name[len] = '\0'; // NULL-terminate the string
    p += sizeof (uint16_t); // The actual name is stored at an offset, so we only 'consume' one byte
    
    p += sizeof (uint32_t); // FIXME: There is padding in these packets. Why?
  }
  else // Make sure there is at least something here
  {
    m_Info.name = (char*)malloc(1);
    m_Info.name[0] = '\0';
  }
  
  if (bitmap & kFPUnixPrivsBit)
  {
    m_Info.unixPrivs.userId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
    m_Info.unixPrivs.groupId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
    m_Info.unixPrivs.perms = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
    m_Info.unixPrivs.userRights = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
  }
  
  return (p - pData); // Bytes 'Consumed'
}

CFileParams::CFileParams(uint32_t bitmap, uint8_t* pData, uint32_t size) :
CNodeParams()
{
  Parse(bitmap, pData, size);
  m_Info.isDirectory = false;
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
    m_Info.fileInfo.dataForkLen = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t);
  }
  if (bitmap & kFPRsrcForkLenBit)
  {
    m_Info.fileInfo.resourceForkLen = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t);
  }
  if (bitmap & kFPExtDataForkLenBit)
  {
    // TODO: Clean-up, and skip if 32-bit value is less-than 4GB
#ifdef BIG_ENDIAN
    uint32_t* pb = (uint32_t*)&m_Info.fileInfo.dataForkLen;
    pb[0] = ntohl(*((uint32_t*)(p + sizeof(uint32_t))));
    pb[1] = ntohl(*(uint32_t*)p);
#else
    memcpy(&m_Info.fileInfo.dataForkLen, p, sizeof(m_Info.fileInfo.dataForkLen));
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
    m_Info.name = (char*)malloc(len + 1);
    memcpy(m_Info.name, pName, len);
    m_Info.name[len] = '\0'; // NULL-terminate the string
    p += sizeof (uint16_t); // The actual name is stored at an offset, so we only 'consume' one byte
    
    p += sizeof (uint32_t); // FIXME: There is padding in these packets. Why?
  }
  if (bitmap & kFPExtRsrcForkLenBit)
  {
    // TODO: Clean-up, and skip if 32-bit value is less-than 4GB
#ifdef BIG_ENDIAN
    uint32_t* pb = (uint32_t*)&m_Info.fileInfo.resourceForkLen;
    pb[0] = ntohl(*((uint32_t*)(p + sizeof(uint32_t))));
    pb[1] = ntohl(*(uint32_t*)p);
#else
    memcpy(&m_Info.fileInfo.resourceForkLen, p, sizeof(m_Info.fileInfo.resourceForkLen));
#endif
    p += sizeof(uint64_t);
  }
  if (bitmap & kFPUnixPrivsBit)
  {
    m_Info.unixPrivs.userId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
    m_Info.unixPrivs.groupId = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
    m_Info.unixPrivs.perms = ntohl(*(uint32_t*)p);
    p += sizeof(uint32_t); // Would it be just a little better to use offsets?
    m_Info.unixPrivs.userRights = ntohl(*(uint32_t*)p);
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