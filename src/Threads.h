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

#include <pthread.h>

// Simple Thread Synchronization Event Wrapper
//////////////////////////////////////////////
class CThreadSyncEvent
{
public:
  CThreadSyncEvent();
  virtual ~CThreadSyncEvent();
  int Wait(int timeout = -1);
  void Set();
  void Reset();
protected:
  pthread_mutex_t m_Mutex;
  pthread_cond_t m_Cond;
  bool m_Signaled;
};

// Simple Mutex Wrapper
//////////////////////////////////////////////
class CMutexLock
{
public:
  inline CMutexLock(pthread_mutex_t m) :
    m_Mutex(m)
  {
    pthread_mutex_lock(&m_Mutex);
  }
  inline ~CMutexLock()
  {
    pthread_mutex_unlock(&m_Mutex);  
  }
protected:
  pthread_mutex_t m_Mutex;
};

