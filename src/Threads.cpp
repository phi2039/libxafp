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

#include "Threads.h"

// Simple Thread Synchronization Event Wrapper
//////////////////////////////////////////////
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