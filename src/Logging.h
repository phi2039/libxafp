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

enum 
{
  XAFP_LOG_FLAG_ERROR     = 0x1,
  XAFP_LOG_FLAG_WARNING   = 0x1 << 1,
  XAFP_LOG_FLAG_INFO      = 0x1 << 2,
  XAFP_LOG_FLAG_DEBUG     = 0x1 << 3,
  XAFP_LOG_FLAG_TCP_PROTO = 0x1 << 4,
  XAFP_LOG_FLAG_DSI_PROTO = 0x1 << 5,
  XAFP_LOG_FLAG_AFP_PROTO = 0x1 << 6,
};

enum
{
  XAFP_LOG_LEVEL_NONE     = 0x0,
  XAFP_LOG_LEVEL_ERROR    = XAFP_LOG_FLAG_ERROR,
  XAFP_LOG_LEVEL_WARNING  = XAFP_LOG_LEVEL_ERROR | XAFP_LOG_FLAG_WARNING,
  XAFP_LOG_LEVEL_INFO     = XAFP_LOG_LEVEL_ERROR | XAFP_LOG_FLAG_WARNING | XAFP_LOG_FLAG_INFO,
  XAFP_LOG_LEVEL_DEBUG    = XAFP_LOG_LEVEL_ERROR | XAFP_LOG_FLAG_WARNING | XAFP_LOG_FLAG_INFO | XAFP_LOG_FLAG_DEBUG
};

void XafpSetLogLevel(int level);

typedef int (*XafpLogFuncPtr)(const char* format, ...);
void XafpSetLogFunc(XafpLogFuncPtr func);

// TODO: Allow for totally disabling this
extern XafpLogFuncPtr xafplog;
extern int g_xafplevel;
#define XAFP_LOG(level,fmt, ...) if (g_xafplevel & level) xafplog(fmt "\n", __VA_ARGS__)
#define XAFP_LOG_0(level,fmt) if (g_xafplevel & level) xafplog(fmt "\n")
