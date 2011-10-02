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

#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct _client_context* xafp_client_handle;
typedef struct _fs_node_iterator* xafp_node_iterator;
typedef int xafp_file_handle;
typedef struct _session_pool* xafp_session_pool_handle;

// TODO: define common error codes

// Logging Interface Definition
///////////////////////////////////////////////////////
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

void xafp_set_log_level(int level);
typedef int (*xafp_log_func_ptr)(const char* format, ...);
void xafp_set_log_func(xafp_log_func_ptr func);

// Client Interface Definition
///////////////////////////////////////////////////////
enum xafp_mount_flags
{
  xafp_mount_flag_none   = 0x0,
  xafp_mount_flag_read  = 0x1 << 0,
  xafp_mount_flag_write = 0x1 << 1,
};

enum xafp_open_flags
{
  xafp_open_flag_read   = 0x1 << 0,
  xafp_open_flag_write  = 0x1 << 1,
};

enum xafp_node_type
{
  xafp_node_file        = 0x0,
  xafp_node_directory   = 0x1
};

enum xafp_node_attribute
{
  xafp_node_att_none    = 0x0,
  xafp_node_att_hidden  = 0x1
};

enum xafp_node_perms
{
  xafp_node_perms_exec    = 0x1,
  xafp_node_perms_read    = 0x2,
  xafp_node_perms_write   = 0x4
};

#define MAKE_XAFP_PERMS_OWNER(x) \
  (x)
#define MAKE_XAFP_PERMS_GROUP(x) \
  (x << 8)
#define MAKE_XAFP_PERMS_OTHER(x) \
  (x << 16)
#define MAKE_XAFP_PERMS_USER(x) \
  (x << 24)

struct xafp_node_info
{
  bool isDirectory;
  uint16_t attributes;
  uint32_t parentId;
  time_t createDate;
  time_t modDate;
  time_t backupDate;
  uint32_t nodeId;
  struct
  {
    uint32_t userId;
    uint32_t groupId;
    uint32_t perms;
    uint32_t userRights;
  } unixPrivs;
  union
  {
    struct
    {
      uint16_t offspringCount;
      uint32_t ownerId;
      uint32_t groupId; 
      uint32_t accessRights;
    }dirInfo;
    struct
    {
      uint64_t dataForkLen;
      uint64_t resourceForkLen;
    }fileInfo;
  };
  char* name;
};

xafp_client_handle xafp_create_context(const char* pServer, unsigned int port=548, const char* pUser=NULL, const char* pPass=NULL);
xafp_client_handle xafp_create_context(const char* pServer, const char* pUser, const char* pPass);
void xafp_destroy_context(xafp_client_handle pCtx);

int xafp_mount(xafp_client_handle hnd, const char* pVolumeName, xafp_mount_flags flags);
void xafp_unmount(xafp_client_handle hnd, const char* pVolumeName);

// Path spec includes share/volume name
xafp_node_iterator xafp_get_dir_iter(xafp_client_handle hnd, const char* pPath);
xafp_node_info* xafp_next(xafp_node_iterator iter);
void xafp_free_iter(xafp_node_iterator iter);

// Path spec includes share/volume name
int xafp_stat(xafp_client_handle hnd, const char* pPath, struct stat* pStat);

xafp_file_handle xafp_open_file(xafp_client_handle hnd, const char* pPath, int flags);
int xafp_read_file(xafp_client_handle hnd, xafp_file_handle file, unsigned int offset, void* pBuf, unsigned int len);
int xafp_write_file(xafp_client_handle hnd, xafp_file_handle file, unsigned int offset, void* pBuf, unsigned int len, bool flush = false);
void xafp_close_file(xafp_client_handle hnd, xafp_file_handle file);

// Path spec includes share/volume name
int xafp_create_dir(xafp_client_handle hnd, const char* pPath, uint32_t flags=0);
int xafp_create_file(xafp_client_handle hnd, const char* pPath, uint32_t flags=0);

int xafp_remove(xafp_client_handle hnd, const char* pPath, uint32_t flags=0);

// Session Pool Definition
///////////////////////////////////////////////////////
xafp_session_pool_handle xafp_create_session_pool(int timeout = 300);
xafp_client_handle xafp_get_context(xafp_session_pool_handle pool, const char* pServer, const char* pUser=NULL, const char* pPass=NULL, unsigned int port=548);
void xafp_free_context(xafp_session_pool_handle pool, xafp_client_handle ctx);
void xafp_destroy_session_pool(xafp_session_pool_handle pool);
