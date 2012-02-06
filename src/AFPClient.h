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

#include "AFPNode.h"

/////////////////////////////////////////////////////////////////////////////////
// AFP Protocol Layer
/////////////////////////////////////////////////////////////////////////////////

class CAFPUserAuthInfo
{
public:
  virtual const char* GetUserName() = 0;  
};

class CAFPCleartextAuthInfo : public CAFPUserAuthInfo
{
public:
  CAFPCleartextAuthInfo(const char* username, const char* password)
  {
    strncpy(m_UserName, username, 255);
    strncpy(m_Password, password, 8);
  }
  const char* GetUserName() {return m_UserName;}
  const char* GetPassword() {return m_Password;}
protected:
  char m_UserName[256]; // 255 char max + NULL
  char m_Password[9]; // 8 char max + NULL
};

struct AFPServerVolume
{
  bool hasPassword;
  bool hasConfigInfo;
  std::string name;
};

typedef std::vector<AFPServerVolume> AFPServerVolumeList;
typedef AFPServerVolumeList::iterator AFPServerVolumeIterator;

class CAFPServerParameters
{
public:
  CAFPServerParameters(CDSIBuffer& buf);
  virtual ~CAFPServerParameters();  
  bool IsValid() {return m_IsValid;}
  
  bool GetVolumeList(AFPServerVolumeList& list);
protected:
  bool Parse(CDSIBuffer& buf);
  bool m_IsValid;
  
  time_t m_ServerTimeOffset;
  AFPServerVolumeList m_Volumes;
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
  int GetVolumeStatus(const char* pVolName); // Retrieve mount status/mode for a volume
  bool GetVolumeList(AFPServerVolumeList& list, bool reload=false);
  void CloseVolume(int volId);
  bool IsLoggedIn() {return m_LoggedIn;}
  
  int GetDirectoryId(int volumeId, const char* pPathSpec, int refId = 2);
  int GetNodeList(CAFPNodeList** ppList, int volumeId, const char* pPathSpec, int refId = 2);
  
  int OpenFile(int volumeId, const char* pPathSpec, uint16_t mode = kFPForkRead, int refId = 2);
  int ReadFile(int forkId, uint64_t offset, void* pBuf, uint32_t len);
  int WriteFile(int forkId, uint64_t offset, void* pBuf, uint32_t len);
  int FlushFile(int forkId);
  void CloseFile(int forkId);

  int Create(int volumeId, const char* pPathSpec, bool dir = false, int refId = 2);
  int Delete(int volumeId, const char* pPathSpec, int refId = 2);
  int Move(int volumeId, const char* pPathSpec, const char* pNewPathSpec);
             
  int Stat(int volumeId, const char* pPathSpec, CNodeParams** ppParams, int refId = 2);
  int Exists(int volumeId, const char* pPathSpec, int refId) {return Stat(volumeId, pPathSpec, NULL, refId);}
  
protected:
  bool LoginDHX2(const char* pUsername, const char* pPassword);
  bool LoginClearText(CAFPCleartextAuthInfo* authInfo);
  bool LoginGuest();

  bool RequestServerParams();

  // Overrides from DSISession
  virtual void OnAttention(uint16_t attData);
  
  // Session State Information
  bool m_LoggedIn;
  CAFPUserAuthInfo* m_pAuthInfo;
  CAFPServerParameters* m_pServerParams;
  
  std::map<std::string, int> m_MountedVolumes;
  
};
