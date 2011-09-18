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

typedef struct _client_context* xafp_client_handle;

typedef struct _fs_node_iterator* xafp_node_iterator;

typedef struct _fs_node* xafp_node_handle;

typedef int xafp_file_handle;

// TODO: define common error codes

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

// Client Interface Definition
///////////////////////////////////////////////////////
xafp_client_handle xafp_create_context(const char* pServer, unsigned int port=548, const char* pUser=NULL, const char* pPass=NULL);
xafp_client_handle xafp_create_context(const char* pServer, const char* pUser, const char* pPass);
void xafp_destroy_context(xafp_client_handle pCtx);

int xafp_mount(xafp_client_handle hnd, const char* pVolumeName, xafp_mount_flags flags);
void xafp_unmount(xafp_client_handle hnd, const char* pVolumeName);

xafp_node_iterator xafp_get_dir_iter(xafp_client_handle hnd, const char* pPath);
xafp_node_handle xafp_next(xafp_node_iterator iter);
void xafp_free_iter(xafp_node_iterator iter);

xafp_file_handle xafp_open_file(xafp_client_handle hnd, const char* pPath, xafp_open_flags flags);
int xafp_read_file(xafp_client_handle hnd, xafp_file_handle file, void* pBuf, unsigned int len);
void xafp_close_file(xafp_client_handle hnd, xafp_file_handle file);

// xafp_create_file
// xafp_create_dir

// xafp_remove_file
// xafp_remove_dir
