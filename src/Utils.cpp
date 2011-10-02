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
  "FPGetAuthMethods",
  "FPLoginExt",
  "FPGetSessionToken",
  "FPDisconnectOldSession",
  "FPEnumerateExt",
  "FPCatSearchExt",
  "FPEnumerateExt2",
  "FPGetExtAttr",
  "FPSetExtAttr", 
/*   afpRemoveExtAttr , afpListExtAttrs,
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
  if (id > 70 )
    return "???";  
  return AFPCommandNames[id];
}
