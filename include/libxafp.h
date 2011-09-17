#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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

struct xafp_client_context;
typedef struct xafp_client_context xafp_client_context;

struct xafp_node_iterator;
typedef struct xafp_node_iterator xafp_node_iterator;

struct xafp_node;
typedef struct xafp_node xafp_node;

// TODO: define common error codes

enum xafp_mount_flags
{
  xafp_mount_flag_read = 0x1 << 0,
  xafp_mount_flag_write = 0x1 << 1,
};

// Client Interface Definition
///////////////////////////////////////////////////////
xafp_client_context* xafp_create_context(const char* pServer, int port=548, const char* pUser=NULL, const char* pPass=NULL);
xafp_client_context* xafp_create_context(const char* pServer, const char* pUser, const char* pPass);
void xafp_destroy_context(xafp_client_context* pCtx);

int xafp_mount(xafp_client_context* pCtx, const char* pVolumeName, xafp_mount_flags flags);
void xafp_unmount(xafp_client_context* pCtx);

xafp_dir_iterator* xafp_list_dir(xafp_client_context* pCtx, const char* pPath);
xafp_node* xafp_next(xafp_node_iterator* pIter);

// xafp_open_file
// xafp_read_file
// xafp_close_file

// xafp_create_file
// xafp_create_dir

// xafp_remove_file
// xafp_remove_dir
