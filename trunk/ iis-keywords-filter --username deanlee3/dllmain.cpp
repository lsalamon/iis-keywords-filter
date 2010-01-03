/*
* dllmain.cpp - This file is the part of the IISKeywordsFilter.
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
#include "stdafx.h"
#include "keywordsfilter.h"

CKeywordsFilter _filter;

BOOL APIENTRY DllMain( HMODULE hModule,
					  DWORD  ul_reason_for_call,
					  LPVOID lpReserved
					  )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
		_filter.s_tlslot = TlsAlloc();
		_filter.s_module = hModule;
		break;
	case DLL_THREAD_ATTACH:
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
		break;
	case DLL_THREAD_DETACH:
		CoUninitialize();
		break;
	case DLL_PROCESS_DETACH:
		TlsFree(_filter.s_tlslot);
		CoUninitialize();
		break;
	}
	return TRUE;
}

BOOL WINAPI __stdcall GetFilterVersion(HTTP_FILTER_VERSION *pVer)
{
	return _filter.GetFilterVersion(pVer);
}

DWORD WINAPI __stdcall HttpFilterProc(HTTP_FILTER_CONTEXT *pfc, DWORD NotificationType, VOID *pvData)
{
	return _filter.HttpFilterProc(pfc, NotificationType, pvData);
}

BOOL WINAPI TerminateFilter(DWORD dwFlags)
{
	return _filter.TerminateFilter(dwFlags);
}
