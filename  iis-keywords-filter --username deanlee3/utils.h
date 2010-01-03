/*
* utils.h - This file is the part of the IISKeywordsFilter.
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
#define ATL_ISAPI_BUFFER_SIZE  (64* 1024)
template <DWORD dwSizeT=ATL_ISAPI_BUFFER_SIZE>
class CAtlIsapiBuffer
{
protected:
	char m_szBuffer[dwSizeT];
	LPSTR m_pBuffer;
	DWORD m_dwLen;
	DWORD m_dwAlloc;
	HANDLE m_hProcHeap;

public:
	CAtlIsapiBuffer() throw()
	{
		if (dwSizeT > 0)
			m_szBuffer[0] = 0;

		m_pBuffer = m_szBuffer;
		m_dwLen = 0;
		m_dwAlloc = dwSizeT;
		m_hProcHeap = GetProcessHeap();
	}

	CAtlIsapiBuffer(__in LPCSTR sz)
	{
		m_pBuffer = m_szBuffer;
		m_dwLen = 0;
		m_dwAlloc = dwSizeT;
		m_hProcHeap = GetProcessHeap();

		if (!Append(sz))
			AtlThrow(E_OUTOFMEMORY);
	}

	~CAtlIsapiBuffer() throw()
	{
		Free();
	}

	BOOL Alloc(__in DWORD dwSize) throw()
	{
		if (m_dwAlloc >= dwSize)
		{
			return TRUE;
		}
		if (m_pBuffer != m_szBuffer)
		{
			HeapFree(m_hProcHeap, 0, m_pBuffer);
			m_dwLen = 0;
			m_dwAlloc = 0;
		}
		m_pBuffer = (LPSTR)HeapAlloc(m_hProcHeap, 0, dwSize);
		if (m_pBuffer)
		{
			m_dwAlloc = dwSize;
			return TRUE;
		}
		return FALSE;
	}

	BOOL ReAlloc(__in DWORD dwNewSize) throw()
	{
		if (dwNewSize <= m_dwAlloc)
			return TRUE;

		if (m_pBuffer == m_szBuffer)
		{
			BOOL bRet = Alloc(dwNewSize);
			if (bRet)
			{
				Checked::memcpy_s(m_pBuffer, dwNewSize, m_szBuffer, m_dwLen);
			}
			return bRet;
		}

		LPSTR pvNew = (LPSTR )HeapReAlloc(m_hProcHeap, 0, m_pBuffer, dwNewSize);
		if (pvNew)
		{
			m_pBuffer = pvNew;
			m_dwAlloc = dwNewSize;
			return TRUE;
		}
		return FALSE;
	}

	void Free() throw()
	{
		if (m_pBuffer != m_szBuffer)
		{
			HeapFree(m_hProcHeap,0 , m_pBuffer);
			m_dwAlloc = dwSizeT;
			m_pBuffer = m_szBuffer;
		}
		Empty();
	}

	void Empty() throw()
	{
		if (m_pBuffer)
		{
			m_pBuffer[0]=0;
			m_dwLen  = 0;
		}
	}

	DWORD GetLength() throw()
	{
		return m_dwLen;
	}

	DWORD GetAllocLength() throw()
	{
		return m_dwAlloc;
	}

	BOOL Append(__in LPCSTR sz, __in int nLen = -1) throw()
	{
		if (!sz)
			return FALSE;

		if (nLen == -1)
			nLen = (int) strlen(sz);

		DWORD newLen = m_dwLen + nLen + 1;
		if (newLen <= m_dwLen || newLen <= (DWORD)nLen)
		{
			return FALSE;
		}
		if (newLen > m_dwAlloc)
		{
			if (!ReAlloc(m_dwAlloc + (nLen+1 > ATL_ISAPI_BUFFER_SIZE ? nLen+1 : ATL_ISAPI_BUFFER_SIZE)))
				return FALSE;
		}
		Checked::memcpy_s(m_pBuffer + m_dwLen, m_dwAlloc-m_dwLen, sz, nLen);
		m_dwLen += nLen;
		m_pBuffer[m_dwLen]=0;
		return TRUE;
	}

	operator LPCSTR() throw()
	{
		return m_pBuffer;
	}

	LPCSTR Data() throw()
	{
		return m_pBuffer;
	}
	CAtlIsapiBuffer& operator+=(__in LPCSTR sz)
	{
		if (!Append(sz))
			AtlThrow(E_OUTOFMEMORY);
		return *this;
	}
}; // class CAtlIsapiBuffer

class CBodyBuffer : public CAtlIsapiBuffer<>
{
public:
	CBodyBuffer() : CAtlIsapiBuffer<>(){}
	~CBodyBuffer(){}
	DECLARE_OBJECT_POOL(CBodyBuffer);
};


#ifdef _DEBUG
#define TRACE OutputArgumentedDebugString
#else
#define TRACE
#endif

void OutputArgumentedDebugString(LPCSTR szFormat, ... );
void DlcReportEventA(WORD wType, LPCSTR pszMessage);
char* FindHttpHeader( LPCSTR name, char*pInBuffer, DWORD size );
char * strnstr(const char *s, const char *find, size_t slen);
char* ReadFile(LPCTSTR pszFile);

