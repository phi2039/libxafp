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

struct context_record
{
  _client_context* ctx;
  bool isIdle;
  time_t idleTime;
  time_t lastCheck;
};

typedef std::string context_id;
typedef std::multimap<context_id,context_record> context_map;

class CSessionManager
{
public:
  CSessionManager(int sessionTimeout=300);
  virtual ~CSessionManager();
  
  // Find an idle client context or create a new one
  _client_context* GetContext(const char* pHost, unsigned int port=548, const char* pUser=NULL, const char* pPass=NULL);
  void FreeContext(_client_context* pCtx);
  
protected:
  int m_SessionTimeout;
  context_map m_Contexts;
  pthread_mutex_t m_MapMutex;
  
  int MonitorThreadProc();
  bool StartMonitorThread();
  void StopMonitorThread();
  static void* MonitorThreadStub(void* p);
  pthread_t m_MonitorThread;
  bool m_MonitorThreadQuitFlag;
  
  
};