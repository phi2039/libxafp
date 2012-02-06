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
#include "SessionManager.h"

CSessionManager::CSessionManager(int sessionTimeout/*=300*/) :
  m_MonitorThread(NULL),
  m_MonitorThreadQuitFlag(false),
  m_SessionTimeout(sessionTimeout)
{
  // TODO: Implement self-start/stop
  StartMonitorThread();
  pthread_mutex_init(&m_MapMutex, NULL);
}

CSessionManager::~CSessionManager()
{  
  // TODO: Clean-up contexts left in map
  StopMonitorThread();
  
  pthread_mutex_destroy(&m_MapMutex);
}

void make_context_id(context_id& id, const char* pHost, unsigned int port, const char* pUser)
{
  char portNo[6];
  sprintf(portNo, "%d", port);
  id = "";
  id += pHost;
  id += ":";  
  id += portNo;
  id += ":";
  id += pUser ? pUser : "[guest]";
}

// Client Interface
/////////////////////////////////////////////////////////
_client_context* CSessionManager::GetContext(const char* pHost, unsigned int port/*=548*/, const char* pUser/*=NULL*/, const char* pPass/*=NULL*/)
{
  CMutexLock lock(m_MapMutex); // Lock Context Map
  
  context_id lookupId;
  make_context_id(lookupId, pHost, port, pUser);
  
  // Look through all of the entries for this id to see if one is open and idle
  std::pair<context_map::iterator,context_map::iterator> range = m_Contexts.equal_range(lookupId);
  for (context_map::iterator it = range.first; it != range.second; ++it)
  {
    // If this one is idle, use it...
    if (it->second.isIdle)
    {
      XAFP_LOG(XAFP_LOG_FLAG_SESS_MGR, "Session Manager: Reusing context [%s, 0x%08x]", lookupId.c_str(), it->second.ctx);
      it->second.isIdle = false; // No longer idle...
      // TODO: Do we need to verify that the connection is good...?
      return it->second.ctx; // We are done. 
    }
  }
  
  // No open and/or idle contexts were found. Create a new one.
  _client_context* ctx = xafp_create_context(pHost, port, pUser, pPass);
  
  // Add the new context to the map
  context_record rec = {ctx, false, 0, time(NULL)};  
  m_Contexts.insert(std::pair<context_id,context_record>(lookupId, rec));
  XAFP_LOG(XAFP_LOG_FLAG_INFO, "Session Manager: Created new context [%s, 0x%08x] - count: %d", lookupId.c_str(), ctx, m_Contexts.size());
  
  return ctx;
}

void CSessionManager::FreeContext(_client_context* pCtx)
{
  CMutexLock lock(m_MapMutex); // Lock Context Map

  // Find this context in the map
  for (context_map::iterator it = m_Contexts.begin(); it != m_Contexts.end(); it++)
  {
    // Once we find it, free it up
    if (it->second.ctx == pCtx)
    {
      XAFP_LOG(XAFP_LOG_FLAG_SESS_MGR, "Session Manager: Freeing context [%s, 0x%08x]", it->first.c_str(), pCtx);
      it->second.isIdle = true;
    }
  }
}

// Monitor Thread
/////////////////////////////////////////////////////////
int CSessionManager::MonitorThreadProc()
{
  XAFP_LOG_0(XAFP_LOG_FLAG_INFO, "Session Manager: Monitor thread started...");
  
  while (true)
  {
    // TODO: Use wait with timeout instead of sleep, so that we can trigger a speedier exit...
    sleep(1);

    if (m_MonitorThreadQuitFlag)
      break;
    
    std::vector<_client_context*> staleContexts;
    
    // Establish scope for mutex lock
    {
      CMutexLock lock(m_MapMutex); // Lock Context Map
      
      // Look at each context and decide if it needs to be cleaned-up
      for (context_map::iterator it = m_Contexts.begin(); it != m_Contexts.end(); it++)
      {
        context_record& rec = it->second;
        if (!rec.isIdle) // Context is in-use. Nothing to do...
        {
          rec.idleTime = 0;
        }
        else // Context is idle. Update the timer and see if it is expired...
        {
          time_t now = time(NULL);
          rec.idleTime += (now - rec.lastCheck); // Update context timer
          // Check to see if this context had been idle longer than the timeout
          if (rec.idleTime >= m_SessionTimeout)
          {
            // Remove the record from the map, and stage the context for deletion
            XAFP_LOG(XAFP_LOG_FLAG_INFO, "Session Manager: Removing expired context [%s, 0x%08x] - count %d", it->first.c_str(), rec.ctx, m_Contexts.size());
            staleContexts.push_back(rec.ctx);
            m_Contexts.erase(it);
          }
          else
            rec.lastCheck = now; // Still time left on this one
        }
      }
    }
    
    // Clean-up any expired contexts (do this after releasing the mutex to minimize locked time)
    for (std::vector<_client_context*>::iterator it = staleContexts.begin(); it != staleContexts.end(); it++)
      xafp_destroy_context((*it));    
  }
  XAFP_LOG_0(XAFP_LOG_FLAG_INFO, "Session Manager: Monitor thread stopped...");
  
  return 0;
}

bool CSessionManager::StartMonitorThread()
{
  if (m_MonitorThread)
    return false; // Already running
  
  XAFP_LOG_0(XAFP_LOG_FLAG_INFO, "Session Manager: Starting monitor thread");
  
  return (pthread_create(&m_MonitorThread, NULL, CSessionManager::MonitorThreadStub, (void*)this) == 0);
}

void CSessionManager::StopMonitorThread()
{
  if (!m_MonitorThread)
    return; // Not running
  
  XAFP_LOG_0(XAFP_LOG_FLAG_INFO, "Session Manager: Stopping monitor thread");
  m_MonitorThreadQuitFlag = true;

  pthread_join(m_MonitorThread, NULL); // Wait for thread to complete
}

void* CSessionManager::MonitorThreadStub(void* p)
{
  return (void*)((CSessionManager*)p)->MonitorThreadProc();
}
