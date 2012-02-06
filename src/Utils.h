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

#define CopyPascalString(_dst,_src)                 \
  memcpy(_dst,((char*)_src + 1),*((uint8_t*)_src)); \
  _dst[*((uint8_t*)_src)] = '\0';

// Node dates are represented by the number of seconds since '01/01/2000 00:00:00.0 UTC'
#define AFPTimeToTime_t(t) (t + ((2000 - 1970) * 365 * 86400) + (7 * 86400)) // Adjust for differences in epoch, including leap years
#define Time_tToAFPTime(t) (t - ((2000 - 1970) * 365 * 86400) + (7 * 86400)) // Adjust for differences in epoch, including leap years

enum
{
  kNoError = 0,
  kParamError = -1
};

const char* DSIProtoCommandToString(int id);
const char* AFPProtoCommandToString(int id);

class CAFPPath
{
public:
protected:
};
