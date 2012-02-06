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

class CNodeParams
{
public:
  CNodeParams();
  virtual ~CNodeParams();
  int Parse(uint32_t bitmap, uint8_t* pData, uint32_t size);
  xafp_node_info* GetInfo() {return &m_Info;}
protected:
  xafp_node_info m_Info;
};

// AFP File and Directory Parameter Handling
/////////////////////////////////////////////////////////////////////////////////
class CDirParams : public CNodeParams
{
public:
  CDirParams() : CNodeParams() {m_Info.isDirectory = true;}
  CDirParams(uint32_t bitmap, uint8_t* pData, uint32_t size);
  int Parse(uint32_t bitmap, uint8_t* pData, uint32_t size);
};

class CFileParams : public CNodeParams
{
public:
  CFileParams() : CNodeParams() {m_Info.isDirectory = false;}
  CFileParams(uint32_t bitmap, uint8_t* pData, uint32_t size);
  int Parse(uint32_t bitmap, uint8_t* pData, uint32_t size);
};

class CNodeParamIterator
{
public:
  virtual CNodeParams* MoveNext() = 0;
  virtual CNodeParams* GetCurrent() = 0;
};

class CAFPNodeList
{
public:
  CAFPNodeList(CDSIBuffer* pBuf, uint32_t dirBitmap, uint32_t fileBitmap, int count);
  virtual ~CAFPNodeList();
  int GetSize(){return m_Count;}
  
  class Iterator : CNodeParamIterator
  {
  public:
    Iterator(CDSIBuffer* pBuf, uint32_t dirBitmap, uint32_t fileBitmap);
    CNodeParams* MoveNext();
    CNodeParams* GetCurrent() {return m_pCurrent;}
  protected:
    uint8_t* m_pData;
    uint8_t* m_pEnd;
    CDirParams m_DirParams;
    uint32_t m_DirBitmap;
    CFileParams m_FileParams;
    uint32_t m_FileBitmap;
    CNodeParams* m_pCurrent;
  };
  Iterator* GetIterator() {return &m_Iter;}
protected:
  CDSIBuffer* m_pBuffer;
  int m_Count;
  uint32_t m_DirBitmap;
  uint32_t m_FileBitmap;
  Iterator m_Iter;
};