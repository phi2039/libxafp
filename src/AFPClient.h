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

#include "DSIClient.h"

/////////////////////////////////////////////////////////////////////////////////
// AFP Protocol Layer
/////////////////////////////////////////////////////////////////////////////////
class CNodeParams
{
public:
  CNodeParams();
  virtual ~CNodeParams();
  int Parse(uint32_t bitmap, uint8_t* pData, uint32_t size);
  inline uint32_t GetNodeId() {return m_NodeId;}
  inline uint32_t GetUserId() {return m_UnixPrivs.userId;}
  inline uint32_t GetGroupId() {return m_UnixPrivs.userId;}
  inline uint32_t GetPermissions() {return m_UnixPrivs.userRights;}
  inline const char* GetName() {return (m_pName == NULL) ? "" : m_pName;}
  inline const time_t GetModDate() {return (time_t)m_ModDate;}
  inline uint16_t GetAttributes() {return m_Attributes;}
  inline bool IsDirectory() {return m_IsDirectory;}
protected:
  bool m_IsDirectory;
  uint16_t m_Attributes;
  uint32_t m_ParentId;
  // TODO: Read server time at login and adjust all dates
  time_t m_CreateDate;
  time_t m_ModDate;
  time_t m_BackupDate;
  uint32_t m_NodeId;
  char* m_pName;
  struct
  {
    uint32_t userId;
    uint32_t groupId;
    uint32_t perms;
    uint32_t userRights;
  } m_UnixPrivs;
};

// AFP File and Directory Parameter Handling
/////////////////////////////////////////////////////////////////////////////////
class CDirParams : public CNodeParams
{
public:
  CDirParams() : CNodeParams() {}
  CDirParams(uint32_t bitmap, uint8_t* pData, uint32_t size);
  int Parse(uint32_t bitmap, uint8_t* pData, uint32_t size);
  inline uint32_t GetOffspringCount() {return m_OffspringCount;}
protected: 
  uint16_t m_OffspringCount;
  uint32_t m_OwnerId;
  uint32_t m_GroupId;
  // TODO: Do we need this, or can we just use UNIX privs?
  uint32_t m_AccessRights;
};

class CFileParams : public CNodeParams
{
public:
  CFileParams() : CNodeParams() {}
  CFileParams(uint32_t bitmap, uint8_t* pData, uint32_t size);
  int Parse(uint32_t bitmap, uint8_t* pData, uint32_t size);
protected: 
  uint64_t m_DataForkLen;
  uint64_t m_ResourceForkLen;
};

class CAFPNodeList
{
public:
  CAFPNodeList(CDSIBuffer* pBuf, uint32_t dirBitmap, uint32_t fileBitmap, int count);
  virtual ~CAFPNodeList();
  int GetSize(){return m_Count;}
  
  class Iterator
  {
  public:
    Iterator(CDSIBuffer* pBuf, uint32_t dirBitmap, uint32_t fileBitmap);
    CNodeParams* MoveNext();
    CNodeParams* GetCurrent() {return m_pCurrent;}
  protected:
    uint8_t* m_pData;
    uint8_t* m_pEnd;
    CDirParams m_DirParams;
    uint32_t m_DirBitmap;
    CFileParams m_FileParams;
    uint32_t m_FileBitmap;
    CNodeParams* m_pCurrent;
  };
  Iterator* GetIterator() {return &m_Iter;}
protected:
  CDSIBuffer* m_pBuffer;
  int m_Count;
  uint32_t m_DirBitmap;
  uint32_t m_FileBitmap;
  Iterator m_Iter;
};

// AFP Session Handling
/////////////////////////////////////////////////////////////////////////////////
class CAFPSession : public CDSISession
{
public:
  CAFPSession();
  virtual ~CAFPSession();
  bool Login(const char* pUsername, const char* pPassword);
  void Logout();
  int OpenVolume(const char* pVolName);
  void CloseVolume(int volId);
  bool IsLoggedIn() {return m_LoggedIn;}
  
  int GetDirectory(int volumeID, const char* pPath);
  int GetNodeList(CAFPNodeList** ppList, int volumeID, int dirID);
  
  int OpenFile(int volumeId, int dirId, const char* pName);
  void CloseFile(int forkId);
  
  int ReadFile(int forkId, uint64_t offset, void* pBuf, uint32_t len);
protected:
  bool LoginDHX2(const char* pUsername, const char* pPassword);
  bool LoginClearText(const char* pUsername, const char* pPassword);
  int OpenDir(int volumeID, int parentID, const char* pName);
  
  bool ParseFileDirParams(CDSIBuffer* pBuffer);
  bool ParseDirParams(uint16_t bitmap, CDSIBuffer* pBuffer, CDirParams& params);
  bool ParseFileParams(uint16_t bitmap, CDSIBuffer* pBuffer, CFileParams& params);
  
  bool m_LoggedIn;
};