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
// DSI Protocol Definitions/Constants
/////////////////////////////////////////////////////////////////////////////////
// DSI Protocol Messages
enum
{
  DSICloseSession = 1,
  DSICommand = 2,
  DSIGetStatus = 3,
  DSIOpenSession = 4,
  DSITickle = 5,
  DSIWrite = 6,
  DSIAttention = 8
};

// Server Attention Flags (DSIAttention message)
enum 
{
  kShutDownNotifyMask   = 0x8000,
  kAllowReconnectMask   = 0x4000,
  kMsgNotifyMask        = 0x2000,
  kDisconnectNotifyMask = 0x1000,
};
