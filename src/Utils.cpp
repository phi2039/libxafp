/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "Utils.h"

const char* DSICommandNames[] = {
  "0",
  "DSICloseSession",
  "DSICommand",
  "DSIGetStatus",
  "DSIOpenSession",
  "DSITickle",
  "DSIWrite",
  "7",
  "DSIAttention"
};

const char* AFPCommandNames[] = {
  "",
  "FPByteRangeLock", 
  "FPCloseVol",
  "FPCloseDir",
  "FPCloseFork",
  "FPCopyFile",
  "FPCreateDir",
  "FPCreateFile",
  "FPDelete",
  "FPEnumerate",
  "FPFlush",
  "FPFlushFork",
  "12",
  "13",
  "FPGetForkParms",
  "FPGetSrvrInfo",
  "FPGetSrvrParms",
  "FPGetVolParms",
  "FPLogin",
  "FPLoginCont",
  "FPLogout",
  "FPMapID",
  "FPMapName",
  "FPMoveAndRename",
  "FPOpenVol",
  "FPOpenDir",
  "FPOpenFork",
  "FPRead",
  "FPRename",
  "FPSetDirParms",
  "FPSetFileParms",
  "FPSetForkParms",
  "FPSetVolParms",
  "FPWrite",
  "FPGetFileDirParms",
	"FPSetFileDirParms",
  "FPChangePassword",
  "FPGetUserInfo",
  "FPGetSrvrMsg",  //38
  "39",
  "40",
  "41",
  "42",
  "43",
  "44",
  "45",
  "46",
  "47",
  "FPOpenDT",
  "FPCloseDT",
  "50",
  "FPGetIcon",
  "FPGetIconInfo",
  "53",
  "54",
  "55",
  "FPAddComment",
  "FPRemoveComment",
  "FPGetComment",
  "FPByteRangeLockExt", 
  "FPReadExt", // 60
  "FPWriteExt", // 61
/*   afpGetAuthMethods=62,
   afp_LoginExt=63,
   afpGetSessionToken=64,
   afpDisconnectOldSession=65,
   afpEnumerateExt=66,
   afpCatSearchExt = 67,
   afpEnumerateExt2 = 68, afpGetExtAttr, afpSetExtAttr, 
   afpRemoveExtAttr , afpListExtAttrs,
   afpZzzzz = 122,
   afpAddIcon=192,*/
  ""
};

const char* DSIProtoCommandToString(int id)
{
  if (id > 8)
    return "???";
  return DSICommandNames[id];
}

const char* AFPProtoCommandToString(int id)
{
  if (id > 61 )
    return "???";  
  return AFPCommandNames[id];
}

CThreadSyncEvent::CThreadSyncEvent() : 
  m_Signaled(false)
{
  pthread_mutex_init(&m_Mutex, NULL);
  pthread_cond_init(&m_Cond, NULL);
}

CThreadSyncEvent::~CThreadSyncEvent()
{
  pthread_mutex_destroy(&m_Mutex);
  pthread_cond_destroy(&m_Cond);
}

int CThreadSyncEvent::Wait(int timeout /*=-1*/)
{
  pthread_mutex_lock(&m_Mutex);
  if (!m_Signaled)
    pthread_cond_wait(&m_Cond, &m_Mutex);
  pthread_mutex_unlock(&m_Mutex);
  
  return 0;
}

void CThreadSyncEvent::Set()
{
  pthread_mutex_lock(&m_Mutex);
  m_Signaled = true;
  pthread_cond_signal(&m_Cond);
  pthread_mutex_unlock(&m_Mutex);    
}

void CThreadSyncEvent::Reset()
{
  m_Signaled = false;
}
