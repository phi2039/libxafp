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

/////////////////////////////////////////////////////////////////////////////////
// AFP Protocol Definitions/Constants
/////////////////////////////////////////////////////////////////////////////////
// AFP Command Codes
enum 
{
  //  FPGetSrvrInfo = 15, // Does not seem to be implemented by servers, since DSIGetStatus does the same thing...
  //  FPAccess
  //  FPAddAPPL // Deprecated (10.6.x)
  //  FPAddComment // Deprecated (10.6.x)
  //  FPAddIcon // Deprecated (10.6.x)
  //  FPByteRangeLock // Deprecated (AFP 3.x)
  //  FPByteRangeLockExt
  //  FPCatSearch
  //  FPCatSearchExt
  //  FPChangePassword
  //  FPCloseDir // Deprecated
  //  FPCloseDT // Deprecated (10.6.x)
  FPCloseFork = 4,
  FPCloseVol = 2,
  //  FPCopyFile
  FPCreateDir = 6,
  FPCreateFile = 7,
  //  FPCreateID // Deprecated (AFP 3.x)
  FPDelete = 8,
  //  FPDeleteID // Deprecated (AFP 3.x)
  //  FPDisconnectOldSession
  //  FPEnumerate
  //  FPEnumerateExt
  FPEnumerateExt2 = 68,
  //  FPExchangeFiles
  //  FPFlush
  //  FPFlushFork
  //  FPGetACL
  //  FPGetAPPL
  //  FPGetAuthMethods
  //  FPGetComment
  //  FPGetExtAttr
  FPGetFileDirParms = 34,
  //  FPGetForkParms
  //  FPGetIcon
  //  FPGetIconInfo
  //  FPGetSessionToken
  //  FPGetSrvrInfo
  //  FPGetSrvrMsg
  //  FPGetSrvrParms
  //  FPGetUserInfo
  //  FPGetVolParms
  //  FPListExtAttrs
  FPLogin = 18,
  //  FPLoginExt
  FPLoginCont = 19,
  FPLogout = 20,
  //  FPMapID
  //  FPMapName
  //  FPMoveAndRename
  //  FPOpenDir // Deprecated
  //  FPOpenDT
  FPOpenFork = 26,
  FPOpenVol = 24,
  //  FPRead
  FPReadExt = 60,
  //  FPRemoveAPPL
  //  FPRemoveComment
  //  FPRemoveExtAttr
  //  FPRename
  //  FPResolveID
  //  FPSetACL
  //  FPSetDirParms
  //  FPSetExtAttr
  //  FPSetFileDirParms
  //  FPSetFileParms
  //  FPSetForkParms
  //  FPSetVolParms
  //  FPSpotlightRPC
  //  FPSyncDir
  //  FPSyncFork
  //  FPWrite // Deprecated
  FPWriteExt = 61,
  FPZzzzz = 122  
};

// AFP Error Codes
enum
{
  kFPNoErr = 0,
  kASPSessClosed = -1072,
  kFPAccessDenied = -5000,
  kFPAuthContinue = -5001,
  kFPBadUAM = -5002,
  kFPBadVersNum = -5003,
  kFPBitmapErr = -5004,
  kFPCantMove = -5005,
  kFPDenyConflict = -5006,
  kFPDirNotEmpty = -5007,
  kFPDiskFull = -5008,
  kFPEOFErr = -5009,
  kFPFileBusy = -5010,
  kFPFlatVol = -5011,
  kFPItemNotFound = -5012,
  kFPLockErr = -5013,
  kFPMiscErr = -5014,
  kFPNoMoreLocks = -5015,
  kFPNoServer = -5016,
  kFPObjectExists = -5017,
  kFPObjectNotFound = -5018,
  kFPParamErr = -5019,
  kFPRangeNotLocked = -5020,
  kFPRangeOverlap = -5021,
  kFPSessClosed = -5022,
  kFPUserNotAuth = -5023,
  kFPCallNotSupported = -5024,
  kFPObjectTypeErr = -5025,
  kFPTooManyFilesOpen = -5026,
  kFPServerGoingDown = -5027,
  kFPCantRename = -5028,
  kFPDirNotFound = -5029,
  kFPIconTypeError = -5030,
  kFPVolLocked = -5031,
  kFPObjectLocked = -5032,
  kFPContainsSharedErr = -5033,
  kFPIDNotFound = -5034,
  kFPIDExists = -5035,
  kFPDiffVolErr = -5036,
  kFPCatalogChanged = -5037,
  kFPSameObjectErr = -5038,
  kFPBadIDErr = -5039,
  kFPPwdSameErr = -5040,
  kFPPwdTooShortErr = -5041,
  kFPPwdExpiredErr = -5042,
  kFPInsideSharedErr = -5043,
  kFPInsideTrashErr = -5044,
  kFPPwdNeedsChangeErr = -5045,
  kFPPwdPolicyErr = -5046
  //  kFPDiskQuotaExceeded = –5047
};

// Volume Bitmap Bits
enum {
  kFPVolAttributeBit = 0x1,
  kFPVolSignatureBit = 0x2,
  kFPVolCreateDateBit = 0x4,
  kFPVolModDateBit = 0x8,
  kFPVolBackupDateBit = 0x10,
  kFPVolIDBit = 0x20,
  kFPVolBytesFreeBit = 0x40,
  kFPVolBytesTotalBit = 0x80,
  kFPVolNameBit = 0x100,
  kFPVolExtBytesFreeBit = 0x200,
  kFPVolExtBytesTotalBit = 0x400,
  kFPVolBlockSizeBit = 0x800
};

// File and Directory Bitmap Bits
enum {
  kFPAttributeBit = 0x1,
  kFPParentDirIDBit = 0x2,
  kFPCreateDateBit = 0x4,
  kFPModDateBit = 0x8,
  kFPBackupDateBit = 0x10,
  kFPFinderInfoBit = 0x20,
  kFPLongNameBit = 0x40,
  kFPShortNameBit = 0x80,
  kFPNodeIDBit = 0x100,
  
  /* Bits that apply only to directories: */
  kFPOffspringCountBit = 0x0200,
  kFPOwnerIDBit = 0x0400,
  kFPGroupIDBit = 0x0800,
  kFPAccessRightsBit = 0x1000,
  
  /* Bits that apply only to files (same bits as previous group): */
  kFPDataForkLenBit = 0x0200,
  kFPRsrcForkLenBit = 0x0400,
  kFPExtDataForkLenBit = 0x0800, // In AFP version 3.0 and later
  kFPLaunchLimitBit = 0x1000, // Obsolete, per http://developer.apple.com/mac/library/documentation/Networking/Reference/AFP_Reference/Reference/reference.html#//apple_ref/doc/c_ref/kFPLaunchLimitBit
  
  /* Bits that apply to everything except where noted: */
  kFPProDOSInfoBit = 0x2000,      // Deprecated; AFP version 2.2 and earlier
  kFPUTF8NameBit = 0x2000,       // AFP version 3.0 and later
  kFPExtRsrcForkLenBit = 0x4000, // Files only; AFP version 3.0 and later
  kFPUnixPrivsBit = 0x8000,       // AFP version 3.0 and later
  kFPUUID = 0x10000              // Directories only; AFP version 3.2 and later (with ACL support)
};

// Node Attributes Bitmap Bits
enum 
{
  kFPInvisibleBit = 0x01,
  kFPMultiUserBit = 0x02,    // for files
  kAttrIsExpFolder = 0x02,   // for directories
  kFPSystemBit = 0x04,
  kFPDAlreadyOpenBit = 0x08, // for files
  kAttrMounted = 0x08,       // for directories
  kFPRAlreadyOpenBit = 0x10, // for files
  kAttrInExpFolder = 0x10,   // for directories
  kFPWriteInhibitBit = 0x20,
  kFPBackUpNeededBit = 0x40,
  kFPRenameInhibitBit = 0x80,
  kFPDeleteInhibitBit = 0x100,
  kFPCopyProtectBit = 0x400,
  kFPSetClearBit = 0x8000
};

// Access Rights Bitmap Bits
enum 
{
  kSPOwner =            0x1,
  kRPOwner =            0x2,
  kWROwner =            0x4,
  
  kSPGroup =          0x100,
  kRPGroup =          0x200,
  kWRGroup =          0x400,
  
  kSPOther =        0x10000,
  kRPOther =        0x20000,
  kWROther =        0x40000,
  
  kSPUser =       0x1000000,
  kRPUser =       0x2000000,
  kWRUser =       0x4000000,
  kBlankAcess =  0x10000000,
  kUserIsOwner = 0x80000000
};

enum
{
  kRightsUser = 0x8,//->0
  kRightsGroup = 0x4,//->16
  kRightsOther = 0x0  //->8
};

#define __AFPRightsToUnixRights(__x,_out_shift,_in_shift) \
  ((((__x >> _in_shift) & 0x1) | (((__x >> _in_shift) & 0x2) << 1) | (((__x >> _in_shift) & 0x4) >> 1)) << _out_shift)
#define AFPRightsToUnixRights(_x) \
  __AFPRightsToUnixRights(_x,8,0) | __AFPRightsToUnixRights(_x,4,8) | __AFPRightsToUnixRights(_x,0,16)

enum 
{
  kFPForkRead       = 0x1,
  kFPForkWrite      = 0x2,
  kFPForkDenyRead   = 0x4,
  kFPForkDenyWrite  = 0x8
};

// AFP Version Strings
#define kAFPVersion_2_1 "AFPVersion 2.1"
#define kAFPVersion_2_2 "AFP2.2"
#define kAFPVersion_2_3 "AFP2.3"
#define kAFPVersion_3_0 "AFPX03"
#define kAFPVersion_3_1 "AFP3.1"
#define kAFPVersion_3_2 "AFP3.2"
#define kAFPVersion_3_3 "AFP3.3"

// UAM Strings
#define kNoUserAuthStr   "No User Authent"
#define kClearTextUAMStr "Cleartxt Passwrd"
#define kRandNumUAMStr   "Randnum Exchange"
#define kTwoWayRandNumUAMStr "2-Way Randnum"
#define kDHCAST128UAMStr "DHCAST128"
#define kDHX2UAMStr      "DHX2"
#define kKerberosUAMStr  "Client Krb v2"
#define kReconnectUAMStr "Recon1"
