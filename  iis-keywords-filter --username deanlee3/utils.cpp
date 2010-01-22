/*
* utils.cpp - This file is the part of the IISKeywordsFilter.
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
#include "utils.h"

void OutputArgumentedDebugString(LPCSTR szFormat, ... )
{
	va_list argList;
	CHAR szHelper[1024] = {'\0'};
	strcpy(szHelper, "IISKeywordsFilter:");
	va_start(argList, szFormat);
	_vsnprintf(szHelper + strlen(szHelper), 1024, szFormat, argList);
	OutputDebugStringA(szHelper);
	va_end(argList);
}


void DlcReportEventA(WORD wType, LPCSTR pszMessage)
{
	HANDLE  hEventSource;
	LPCTSTR  lpszStrings[1];

	lpszStrings[0] = (LPCTSTR)pszMessage;

	// Get a handle to use with ReportEvent(). 
	hEventSource = RegisterEventSourceA(NULL, "IISKeywordsFilter");
	if (hEventSource != NULL) {
		// Write to event log. 
		ReportEventA(hEventSource, wType, 0, 0, NULL, 1, 0, (LPCSTR*) &lpszStrings[0], NULL);
		DeregisterEventSource(hEventSource);
	}
}

char * strnstr(const char *s, const char *find, size_t slen)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if (slen-- < 1 || (sc = *s++) == '\0')
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

char* FindHttpHeader( LPCSTR name, char*pInBuffer, DWORD size )
{
	char* pos = strnstr(pInBuffer, name, size);
	if (pos) {
		return pos + strlen(name);
	}
	return NULL;
}


char* ReadFile(LPCTSTR pszFile)
{
	char* buffer = NULL;
	HANDLE hFile = ::CreateFile(pszFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER fileSize;
		if (GetFileSizeEx(hFile, &fileSize)) {
			buffer = new char[(fileSize.QuadPart + 1) * sizeof(char*)];
			DWORD bytesRead = 0;
			if (::ReadFile(hFile, buffer, fileSize.QuadPart, &bytesRead, NULL)) {
				buffer[fileSize.QuadPart] = '\0';
				return buffer;
			}
			delete buffer;
		}
		::CloseHandle(hFile);
	}
	return NULL;
}