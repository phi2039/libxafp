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

// NOTE: Volumes are treated as directories, so they are mounted automatically as neccessary

// Context Handling
//////////////////////////////////////
// Function: xafp_create_context
// Returns: Handle on success (> 0), null on failure (==0)
xafp_client_handle xafp_create_context(const char* pServer, unsigned int port/*=548*/, const char* pUser/*=NULL*/, const char* pPass/*=NULL*/)
{
  _client_context* pCtx = (_client_context*)malloc(sizeof(_client_context));
  
  size_t len = strlen(pServer);
  pCtx->server_dns_name = (char*)malloc(len + 1);
  strncpy(pCtx->server_dns_name, pServer, len);
  pCtx->server_dns_name[len] = '\0';
  
  pCtx->port = port;
  
  if (pUser)
  {
    len = strlen(pUser);
    pCtx->username = (char*)malloc(len + 1);
    strncpy(pCtx->username, pUser, len);
    pCtx->username[len] = '\0';
  }
  else
    pCtx->username = NULL;
  
  if (pPass)
  {
    len = strlen(pPass);
    pCtx->password = (char*)malloc(len + 1);
    strncpy(pCtx->password, pPass, len);
    pCtx->password[len] = '\0';
  }
  else
    pCtx->password = NULL;
  
  pCtx->session = new CAFPSession();
  
  return pCtx;
}

xafp_client_handle xafp_create_context(const char* pServer, const char* pUser, const char* pPass)
{
  return xafp_create_context(pServer, 548, pUser, pPass);
}

void xafp_destroy_context(xafp_client_handle hnd)
{
  if (!hnd)
    return;
  
  _client_context* pCtx = (_client_context*)hnd;
  
  delete pCtx->session;
  
  free(pCtx->server_dns_name);
  free(pCtx->username);
  free(pCtx->password);
  free(pCtx);
}

// TODO: Return useful error codes...

// Volume Management
//////////////////////////////////////
// TODO: Add reference counting so that we can close unused volumes... For now, they stay open once we open them...

// Node Listing/Iteration
//////////////////////////////////////
xafp_node_iterator xafp_get_dir_iter(xafp_client_handle hnd, const char* pPath)
{
  if (!hnd)
    return NULL;
    
  // TODO: Make this more reliable and complete
  std::string path = (pPath[0] == '/') ? pPath + 1 : pPath; // Strip the leading '/' if there is one
  
  // Handle special case where caller requests a directory iterator for the server root (volume list)
  // TODO: Implement a more elegant method to do this... Really should be using an interface for abstraction.
  if (path.length() == 0)
  {
    if (xafp_get_volume(hnd, "") < 0) // Force login if the session is not already active
      return NULL; // Failed to open the session
      
    // Get a list of volumes from the server
    _client_context* pCtx = (_client_context*)hnd;
    
    AFPServerVolumeList volumes;
    if (!pCtx->session->GetVolumeList(volumes))
      return NULL;
    
    // Build a 'spoofed' reply packet that can be parsed as if it were a real enum reply
    uint16_t count = volumes.size();
    CDSIBuffer* pBuf = new CDSIBuffer(64 * count);
    
    uint16_t fileBitmap = 0;
    uint16_t dirBitmap = kFPAttributeBit | kFPUTF8NameBit | kFPUnixPrivsBit | kFPModDateBit | kFPOffspringCountBit;
    pBuf->Write((uint16_t)0); // File Bitmap
    pBuf->Write((uint16_t)dirBitmap); // Directory Bitmap
    pBuf->Write((uint16_t)count); // Count

    for (AFPServerVolumeIterator it = volumes.begin(); it < volumes.end(); it++)
    {
      pBuf->Write((uint16_t)((*it).name.length() + 6 + 35)); // Size
      pBuf->Write((uint8_t)0x80); // Directory Flag      
      pBuf->Write((uint8_t)0x00); // Pad
      pBuf->Write((uint16_t)(0x02 | 0x20 | 0x40 | 0x100)); // Attributes (kAttrIsExpFolder: 0x02, kFPWriteInhibitBit: 0x20, kFPRenameInhibitBit: 0x40, kFPDeleteInhibitBit: 0x100)
      pBuf->Write((uint32_t)Time_tToAFPTime(time(NULL))); // Modified Date
      pBuf->Write((uint16_t)0); // Offspring Count (Not sure if this needs to be > 0 or not...)
      pBuf->Write((uint16_t)31); // UTF-8 Name Offset
      pBuf->Write((uint32_t)0); // Pad (Not sure why this is needed, but it is...)
      pBuf->Write((uint32_t)0); // Unix UID (root)
      pBuf->Write((uint32_t)0); // Unix GID (root)
      pBuf->Write((uint32_t)(0x0040000 | AFPRightsToUnixRights(0x02020202))); // Permissions (r--r--r--) or 0x124
      pBuf->Write((uint32_t)0x02020202); // Access Rights (r--r--r--r--)
      pBuf->WritePathSpec((*it).name.c_str());
    }
    CAFPNodeList* pList = new CAFPNodeList(pBuf, dirBitmap, fileBitmap, count);
    _fs_node_iterator* pIter = new _fs_node_iterator;
    pIter->list = pList;
    pIter->iter = pList->GetIterator();
    return (xafp_node_iterator)pIter;
  }
  
  int pos = path.find('/');
  std::string volume = path.substr(0, pos);
  
  // Find the volume id (mount volume if necessary)
  int volId = xafp_get_volume(hnd, volume.c_str());
  if (volId <= 0)
    return NULL;
  
  _client_context* pCtx = (_client_context*)hnd;
  std::string dir;
  if (pos > 0)
    dir = path.substr(pos);
  else
    dir = path;
  
  CAFPNodeList* pList = NULL;
  if (pCtx->session->GetNodeList(&pList, volId, path.c_str() + pos) == kNoError)
  {
    _fs_node_iterator* pIter = new _fs_node_iterator;
    pIter->list = pList;
    pIter->iter = pList->GetIterator();
    return (xafp_node_iterator)pIter;
  }

  return NULL;
}

// TODO: Get rid of the extra layer here...
xafp_node_info* xafp_next(xafp_node_iterator iter)
{
  if (!iter)
    return NULL;
  
  _fs_node_iterator* pIter = (_fs_node_iterator*)iter;
  CNodeParams* pNode = pIter->iter->MoveNext();
  if (pNode)
    return pNode->GetInfo();
  return NULL;
}

void xafp_free_iter(xafp_node_iterator iter)
{
  if (!iter)
    return;
  
  _fs_node_iterator* pIter = (_fs_node_iterator*)iter;
  delete pIter->list;
  delete pIter;
}

// File I/O
//////////////////////////////////////
// TODO: Address structure size differences for 'stat' structure
int xafp_stat(xafp_client_handle hnd, const char* pPath, struct stat* pStat/*=NULL*/)
{
  if (!hnd || !pPath)
    return -1;
  
  // TODO: Make this more reliable and complete (implement wrapper class?)
  std::string path = (pPath[0] == '/') ? pPath + 1 : pPath; // Strip the leading '/' if there is one
  
  // Handle special case where caller stat's the server root
  if (path.length() == 0)
  {
    // Build an explicit stat structure for the server root
    if (pStat) // Caller wants the file info back, otherwise they were just seeing if it existed...
    {
      memset(pStat, 0, sizeof(*pStat));
      pStat->st_dev = 0; // Not really relevant...
      pStat->st_ino = 0; // Not really relevant...
      pStat->st_mode = 0x0040000 | AFPRightsToUnixRights(0x02020202);
      pStat->st_nlink = 0; 
      pStat->st_uid = 0; // root
      pStat->st_gid = 0; // root
      pStat->st_rdev = 0;
      pStat->st_size = 0;
      pStat->st_atime = Time_tToAFPTime(time(NULL));
      pStat->st_mtime = pStat->st_atime;
      pStat->st_ctime = pStat->st_atime;
    }
    return 0;
  }  
  
  int pos = path.find('/');
  std::string volume = path.substr(0, pos);
  // Find the volume id (mount volume if necessary)
  int volId = xafp_get_volume(hnd, volume.c_str());
  if (volId <= 0)
    return -2;
  
  _client_context* pCtx = (_client_context*)hnd;
  CNodeParams* pParams = NULL;
  if (!pCtx->session->Stat(volId, path.substr(pos).c_str(), &pParams)) // Path does NOT include share/volume name
  {
    if (pStat) // Caller wants the file info back, otherwise they were just seeing if it existed...
    {
      memset(pStat, 0, sizeof(*pStat));
      xafp_node_info* pInfo = pParams->GetInfo();
      pStat->st_dev = volId; // Substitute volume for device
      pStat->st_ino = pInfo->nodeId;
      pStat->st_mode = (pInfo->isDirectory ? 0x0040000 : 0x0100000) | AFPRightsToUnixRights(pInfo->unixPrivs.userRights);
      pStat->st_nlink = 0; ////////
      pStat->st_uid = pInfo->unixPrivs.userId;
      pStat->st_gid = pInfo->unixPrivs.groupId;
      pStat->st_rdev = 0;///////
      pStat->st_size = (pInfo->isDirectory ? 0 : pInfo->fileInfo.dataForkLen);
      pStat->st_atime = pInfo->modDate;
      pStat->st_mtime = pInfo->modDate;
      pStat->st_ctime = pInfo->createDate;
    }
    return 0;
  }
  delete pParams;
  return -3;
}

// Function: xafp_open_file
// Returns: Handle on success (> 0), error code on failure (< 0)
xafp_file_handle xafp_open_file(xafp_client_handle hnd, const char* pPath, int flags)
{
  if (!hnd || !pPath)
    return -1;
  
  // TODO: Make this more reliable and complete
  std::string path = (pPath[0] == '/') ? pPath + 1 : pPath; // Strip the leading '/' if there is one
  int pos = path.find('/');
  std::string volume = path.substr(0, pos);
  // Find the volume id (mount volume if necessary)
  int volId = xafp_get_volume(hnd, volume.c_str());
  if (volId <= 0)
    return -2;
  
  _client_context* pCtx = (_client_context*)hnd;
  uint16_t mode = ((flags & xafp_open_flag_read) ? kFPForkRead : 0) | ((flags & xafp_open_flag_write) ? kFPForkWrite : 0); // Translate mode flags from xafp to AFP
  return pCtx->session->OpenFile(volId, path.substr(pos).c_str(), mode); // Path does NOT include share/volume name
}

// Function: xafp_read_file
// Returns: bytes read on success (> 0), error code on failure (< 0)
int xafp_read_file(xafp_client_handle hnd, xafp_file_handle file, unsigned int offset, void* pBuf, unsigned int len)
{
  if (!hnd || !file || !pBuf || !len)
    return 0;

  _client_context* pCtx = (_client_context*)hnd;
  return pCtx->session->ReadFile(file, offset, pBuf, len);
}

// Function: xafp_write_file
// Returns: bytes written on success (> 0), error code on failure (< 0)
int xafp_write_file(xafp_client_handle hnd, xafp_file_handle file, unsigned int offset, void* pBuf, unsigned int len, bool flush /*=false*/)
{
  if (!hnd || !file || !pBuf || !len)
    return 0;
  
  _client_context* pCtx = (_client_context*)hnd;
  if (pCtx->session->WriteFile(file, offset, pBuf, len) == len)
  {
    if (flush)
      pCtx->session->FlushFile(file);
    return len;
  }
  return -1;
}

// Function: xafp_close_file
// Returns: None
void xafp_close_file(xafp_client_handle hnd, xafp_file_handle file)
{
  if (!hnd || !file)
    return;
  
  _client_context* pCtx = (_client_context*)hnd;
  pCtx->session->CloseFile(file);
}

// Function: xafp_create_dir
// Returns: Zero(0) on success, error code on failure (< 0)
int xafp_create_dir(xafp_client_handle hnd, const char* pPath, uint32_t flags /*=0*/)
{
  if (!hnd || !pPath)
    return -1;
  
  // TODO: Make this more reliable and complete (implement wrapper class?)
  std::string path = (pPath[0] == '/') ? pPath + 1 : pPath; // Strip the leading '/' if there is one
  int pos = path.find('/');
  std::string volume = path.substr(0, pos);
  // Find the volume id (mount volume if necessary)
  int volId = xafp_get_volume(hnd, volume.c_str());
  if (volId <= 0)
    return -2;

  _client_context* pCtx = (_client_context*)hnd;
  int dirId = pCtx->session->Create(volId, path.substr(pos).c_str(), true);
  if (dirId > 0)
    return 0; // TODO: Return dirId once error codes are normalized
  return -3;
}

// Function: xafp_create_file
// Returns: Zero(0) on success, error code on failure (< 0)
int xafp_create_file(xafp_client_handle hnd, const char* pPath, uint32_t flags /*=0*/)
{
  if (!hnd || !pPath)
    return -1;
  
  // TODO: Make this more reliable and complete (implement wrapper class?)
  std::string path = (pPath[0] == '/') ? pPath + 1 : pPath; // Strip the leading '/' if there is one
  int pos = path.find('/');
  std::string volume = path.substr(0, pos);
  // Find the volume id (mount volume if necessary)
  int volId = xafp_get_volume(hnd, volume.c_str());
  if (volId <= 0)
    return -2;

  _client_context* pCtx = (_client_context*)hnd;
  if (!pCtx->session->Create(volId, path.substr(pos).c_str()))
    return 0;
  return -3;
}

// Function: xafp_remove
// Returns: Zero(0) on success, error code on failure (< 0)
// TODO: Implement recursive delete of contents (rm -rf)
int xafp_remove(xafp_client_handle hnd, const char* pPath, uint32_t flags /*=0*/)
{
  if (!hnd || !pPath)
    return -1;
  
  // TODO: Make this more reliable and complete (implement wrapper class?)
  std::string path = (pPath[0] == '/') ? pPath + 1 : pPath; // Strip the leading '/' if there is one
  int pos = path.find('/');
  std::string volume = path.substr(0, pos);
  // Find the volume id (mount volume if necessary)
  int volId = xafp_get_volume(hnd, volume.c_str());
  if (volId <= 0)
    return -2;

  _client_context* pCtx = (_client_context*)hnd;
  if (pCtx->session->Delete(volId, path.substr(pos).c_str()) == kNoError)
    return 0;
  return -3;
}

int xafp_rename_file(xafp_client_handle hnd, const char* pPath, const char* pNewPath)
{
  if (!hnd || !pPath|| !pNewPath)
    return -1;
  
  // TODO: Make this more reliable and complete (implement wrapper class?)
  std::string path = (pPath[0] == '/') ? pPath + 1 : pPath; // Strip the leading '/' if there is one
  std::string newPath = (pNewPath[0] == '/') ? pNewPath + 1 : pNewPath; // Strip the leading '/' if there is one
  
  int pos = path.find('/');
  int newPos = newPath.find('/');

  std::string volume = path.substr(0, pos);
  if (volume != newPath.substr(0, newPos))
    return -3; // Paths are on different volumes
  
  // Find the volume id (mount volume if necessary)
  int volId = xafp_get_volume(hnd, volume.c_str());
  if (volId <= 0)
    return -2;

  _client_context* pCtx = (_client_context*)hnd;
  return pCtx->session->Move(volId, path.substr(pos).c_str(), newPath.substr(newPos).c_str());
}

// Context/Session Pool
///////////////////////////////////////////////////////
xafp_context_pool_handle xafp_create_context_pool(int timeout /*=300*/)
{
  _context_pool* pPool = (_context_pool*)malloc(sizeof(_context_pool));
  pPool->manager = new CSessionManager();
  
  return pPool;
}

xafp_client_handle xafp_get_context(xafp_context_pool_handle hnd, const char* pServer, unsigned int port /*=548*/, const char* pUser /*=NULL*/, const char* pPass /*=NULL*/)
{
  if (hnd)
  {
    _context_pool* pPool = (_context_pool*)hnd;
    if (pPool->manager)
      return pPool->manager->GetContext(pServer, port, pUser, pPass);
  }
  
  return NULL;
}

xafp_client_handle xafp_get_context(xafp_context_pool_handle hnd, const char* pServer, const char* pUser /*=NULL*/, const char* pPass /*=NULL*/)
{
  return xafp_get_context(hnd, pServer, 548, pUser, pPass);
}

void xafp_free_context(xafp_context_pool_handle hnd, xafp_client_handle ctx)
{
  if (hnd)
  {
    _context_pool* pPool = (_context_pool*)hnd;
    if (pPool->manager)
      pPool->manager->FreeContext((_client_context*)ctx);
  }
}

void xafp_destroy_context_pool(xafp_context_pool_handle hnd)
{
  if (!hnd)
    return;
  
  _context_pool* pPool = (_context_pool*)hnd;
  delete pPool->manager;
  
  free(pPool);
}

// Internal functions
//////////////////////////////////////////////////////////
int xafp_get_volume(xafp_client_handle hnd, const char* pVolumeName)
{
  if (!hnd)
    return -4;
  
  if (!pVolumeName)
    return -5;
  
  _client_context* pCtx = (_client_context*)hnd;  
  
  // This interface is late-binding, so the first mount request triggers connect/login
  if (!pCtx->session->IsLoggedIn())
  {
    if (!pCtx->session->IsConnected())
    {
      if (!pCtx->session->Open(pCtx->server_dns_name, pCtx->port, 3000))
        return -1;
    }
    if (!pCtx->session->Login(pCtx->username,pCtx->password))
      return -2;
  }
  
  // Do not try to mount zero-length volume (but allow callers to use it to force login)
  // TODO: There must be a better way to do this...
  if (strlen(pVolumeName) == 0)
    return 0;
  
  // See if this volume is already mounted
  int volId = pCtx->session->GetVolumeStatus(pVolumeName);
  if (!volId)
  {
    // The volume was not mounted, mount it now
    volId = pCtx->session->OpenVolume(pVolumeName);
  }
  
  return volId;
}
