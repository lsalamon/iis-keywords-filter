/*
* stdafx.h - This file is the part of the IISKeywordsFilter.
* Copyright (C) 2010 Dean Lee <deanlee3@gmail.com> <http://www.deanlee.cn/>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* 
* Acknowledgements:
*   <> Many thanks to all of you, who have encouraged me to update my articles
*     and code, and who sent in bug reports and fixes.
* 
*/


#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>


#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#include <atlbase.h>
#include <time.h>
#include <atlstr.h>
#include <atlcoll.h>
#include <httpfilt.h>
#include <Mlang.h>
#include <WinInet.h>
#include "SWMRG.h"
#include "objectpool.h"

// TODO: reference additional headers your program requires here
