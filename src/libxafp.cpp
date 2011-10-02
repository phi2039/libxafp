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
  
  pCtx->volumes = new volume_map();
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
  
  delete pCtx->volumes;
  delete pCtx->session;
  
  free(pCtx->server_dns_name);
  free(pCtx->username);
  free(pCtx->password);
  free(pCtx);
}

// TODO: Return useful error codes...

// Volume Mount/Unmount
//////////////////////////////////////
int xafp_mount(xafp_client_handle hnd, const char* pVolumeName, xafp_mount_flags flags)
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
  
  int res = pCtx->session->OpenVolume(pVolumeName);
  if (res < 0)
    return -3;
  
  pCtx->volumes->insert(volume_map_entry(pVolumeName, res));
  
  return 0;
}

void xafp_unmount(xafp_client_handle hnd, const char* pVolumeName)
{
  if (!hnd || !pVolumeName)
    return;
  
  int volId = xafp_find_volume_id(hnd, pVolumeName);

  if (volId)
  {
    _client_context* pCtx = (_client_context*)hnd;
    pCtx->session->CloseVolume(volId);    
    pCtx->volumes->erase(pVolumeName);
  }
}

// Node Listing/Iteration
//////////////////////////////////////
xafp_node_iterator xafp_get_dir_iter(xafp_client_handle hnd, const char* pPath)
{
  if (!hnd)
    return NULL;
  
  // TODO: Make this more reliable and complete
  std::string path = (pPath[0] == '/') ? pPath + 1 : pPath; // Strip the leading '/' if there is one

  int pos = path.find('/');
  std::string volume = path.substr(0, pos);
  
  int volId = xafp_find_volume_id(hnd, volume.c_str());
  if (volId)
  {
    _client_context* pCtx = (_client_context*)hnd;
    std::string dir = path.substr(pos);    

    CAFPNodeList* pList = NULL;
    if (pCtx->session->GetNodeList(&pList, volId, path.c_str() + pos) == kNoError)
    {
      _fs_node_iterator* pIter = new _fs_node_iterator;
      pIter->list = pList;
      pIter->iter = pList->GetIterator();
      return (xafp_node_iterator)pIter;
    }
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
int xafp_stat(xafp_client_handle hnd, const char* pPath, struct stat* pStat)
{
  if (!hnd || !pPath)
    return -1;
  
  // TODO: Make this more reliable and complete (implement wrapper class?)
  std::string path = (pPath[0] == '/') ? pPath + 1 : pPath; // Strip the leading '/' if there is one
  int pos = path.find('/');
  std::string volume = path.substr(0, pos);
  int volId = xafp_find_volume_id(hnd, volume.c_str());
  if (volId)
  {
    _client_context* pCtx = (_client_context*)hnd;
    CNodeParams* pParams = NULL;
    if (!pCtx->session->Stat(volId, path.substr(pos).c_str(), &pParams)) // Path does NOT include share/volume name
    {
      // TODO: Maybe just call Exists() if caller does not provide stat buffer
      if (pStat) // Caller wants the file info back, otherwise they were just seeing if it existed...
      {
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
    return -3;
  }
  return -2;
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
  int volId = xafp_find_volume_id(hnd, volume.c_str());
  if (volId)
  {
    _client_context* pCtx = (_client_context*)hnd;
    uint16_t mode = ((flags & xafp_open_flag_read) ? kFPForkRead : 0) | ((flags & xafp_open_flag_write) ? kFPForkWrite : 0); // Translate mode flags from xafp to AFP
    return pCtx->session->OpenFile(volId, path.substr(pos).c_str(), mode); // Path does NOT include share/volume name
  }
  return -2;
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
  int volId = xafp_find_volume_id(hnd, volume.c_str());
  if (volId)
  {
    _client_context* pCtx = (_client_context*)hnd;
    int dirId = pCtx->session->Create(volId, path.substr(pos).c_str(), true);
    if (dirId > 0)
      return 0; // TODO: Return dirId once error codes are normalized
    return -3;
  }
  return -2;
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
  int volId = xafp_find_volume_id(hnd, volume.c_str());
  if (volId)
  {
    _client_context* pCtx = (_client_context*)hnd;
    if (!pCtx->session->Create(volId, path.substr(pos).c_str()))
      return 0;
    return -3;
  }
  return -2;  
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
  int volId = xafp_find_volume_id(hnd, volume.c_str());
  if (volId)
  {
    _client_context* pCtx = (_client_context*)hnd;
    if (pCtx->session->Delete(volId, path.substr(pos).c_str()) == kNoError)
      return 0;
    return -3;
  }
  return -2;
}

// Session Pool
///////////////////////////////////////////////////////
xafp_session_pool_handle xafp_create_session_pool(int timeout /*=300*/)
{
  // Allocate pool structure
  
  // Initialize
  
  return NULL;
}

xafp_client_handle xafp_get_context(xafp_session_pool_handle pool, const char* pServer, const char* pUser /*=NULL*/, const char* pPass /*=NULL*/, unsigned int port /*=548*/)
{
  // Look for existing session that matches parameters
  
  // Create a new one if none exists
  
  // Increment the reference count for the session
  return NULL;
}

void xafp_free_context(xafp_session_pool_handle pool, xafp_client_handle ctx)
{
  // Decrement the reference count for the session
  
  // If the count goes to zero, start the timer
}

void xafp_destroy_session_pool(xafp_session_pool_handle pool)
{
  // Clean-up any remaining sessions
}
