/*
* KeywordsFilter.cpp - This file is the part of the IISKeywordsFilter.
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
#include "utils.h"
extern "C"
{
#include "mwm.h"
}

IMPLEMENT_OBJECT_POOL(UrlItem, 1024, 1, 1);
IMPLEMENT_OBJECT_POOL(CReqContext, 1024, 1, 1);
IMPLEMENT_OBJECT_POOL(CBodyBuffer, 512, 1, 1);


CReqContext* CReqContext::get(HTTP_FILTER_CONTEXT* pfc)
{
	return static_cast<CReqContext*>(pfc->pFilterContext);
}

CReqContext* CReqContext::create(HTTP_FILTER_CONTEXT* pfc)
{
	CReqContext* context = new CReqContext();
	pfc->pFilterContext = context;
	return context;
}

CReqContext::CReqContext(void)
{
	m_body = NULL;
	Init();
}

void CReqContext::Init()
{
	m_bodyPos = 0;
	doFilter = false;
	m_flag = IN_NONE;
	m_index = -1;
	m_szUrl[0] = '\0';
	m_item = NULL;
	m_encoding[0] = '\0';
	ZeroMemory(&rawHeader, sizeof(HTTP_FILTER_RAW_DATA));
}

CReqContext::~CReqContext(void)
{
}


void CReqContext::destroy(PHTTP_FILTER_CONTEXT pfc)
{
	CReqContext* pContext = get(pfc);
	if (pContext != NULL)
	{
		delete pContext;
	}
	pfc->pFilterContext = NULL;
}

HMODULE CKeywordsFilter::s_module = NULL;
DWORD CKeywordsFilter::s_tlslot = 0;

CKeywordsFilter::CKeywordsFilter()
{
	::InitializeCriticalSection(&m_crit);
}

CKeywordsFilter::~CKeywordsFilter()
{
	::DeleteCriticalSection(&m_crit);
}

BOOL CKeywordsFilter::GetFilterVersion(HTTP_FILTER_VERSION *pVer)
{
	/* Specify the types and order of notification */
	pVer->dwFlags = (SF_NOTIFY_ORDER_DEFAULT | SF_NOTIFY_NONSECURE_PORT | SF_NOTIFY_SECURE_PORT 
		| SF_NOTIFY_PREPROC_HEADERS 
		| SF_NOTIFY_SEND_RESPONSE
		| SF_NOTIFY_SEND_RAW_DATA 
		| SF_NOTIFY_END_OF_REQUEST 
		| SF_NOTIFY_END_OF_NET_SESSION);

	pVer->dwFilterVersion = MAKELONG( 0, 5 );
	strcpy_s(pVer->lpszFilterDesc, "IISKeywordsFilter, Version 2.0");
	DlcReportEventA(EVENTLOG_INFORMATION_TYPE, "IISKeywordsFilter started");
	TRACE("IISKeywordsFilter started\n");
	GetModuleFileName(s_module, m_szModulePath, _countof(m_szModulePath));
	::PathRemoveFileSpec(m_szModulePath);

	return loadConfig() && loadErrorHtml() && loadKeywords("us-ascii,gb2312,utf-8,big5");
}

DWORD CKeywordsFilter::HttpFilterProc(HTTP_FILTER_CONTEXT *pfc, DWORD NotificationType, VOID *pvData)
{
	DWORD   dwRet = SF_STATUS_REQ_NEXT_NOTIFICATION; 
	switch (NotificationType)
	{
	case SF_NOTIFY_PREPROC_HEADERS:
		dwRet = onPreprocHeaders(pfc, (PHTTP_FILTER_PREPROC_HEADERS)pvData);
		break;
	case SF_NOTIFY_SEND_RESPONSE:
		dwRet = onSendResponse(pfc, (PHTTP_FILTER_PREPROC_HEADERS)pvData);
		break;
	case SF_NOTIFY_SEND_RAW_DATA:
		dwRet = onSendRawData(pfc, (PHTTP_FILTER_RAW_DATA)pvData);
		break;
	case SF_NOTIFY_END_OF_REQUEST:
		dwRet = onEndOfRequest(pfc);
		break;
	case SF_NOTIFY_END_OF_NET_SESSION:
		dwRet = onEndOfNetSession(pfc);
		break;
	}
	return dwRet;
}


DWORD CKeywordsFilter::onEndOfNetSession(HTTP_FILTER_CONTEXT *pfc)
{
	TRACE("OnEndOfNetSession\n");
	CReqContext::destroy(pfc);
	return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

BOOL CKeywordsFilter::TerminateFilter(DWORD dwFlags)
{
	POSITION pos = m_urlMap.GetStartPosition();
	while (pos)
	{
		delete m_urlMap.GetNext(pos)->m_value;
	}
	m_urlMap.RemoveAll();
	delete m_errorHtml;
	DlcReportEventA(EVENTLOG_INFORMATION_TYPE, "IISKeywordsFilter stopped");
	return true;
}

void CKeywordsFilter::writeErrorToClient(HTTP_FILTER_CONTEXT *pfc, int keywordIndex)
{
	static char s_header[] = 
"HTTP/1.1 200 OK\n\
Content-Type: text/html; charset=UTF-8\n\
Content-Length: %d\
\r\n\r\n%s";
	CStringA error = m_errorHtml;
	CStringA keyword = m_patterns.GetAt(m_patterns.FindIndex(keywordIndex));
	error.Replace("{KEYWORD}", keyword);
	CStringA output;
	output.Format(s_header, error.GetLength(), (LPCSTR)error);
	DWORD size = output.GetLength();
	pfc->WriteClient(pfc, (LPVOID)((LPCSTR)output), (DWORD*)&size, 0 );
}

DWORD CKeywordsFilter::onPreprocHeaders(HTTP_FILTER_CONTEXT *pfc, PHTTP_FILTER_PREPROC_HEADERS pPreprocData)
{
	DWORD dwRet = SF_STATUS_REQ_NEXT_NOTIFICATION;
	bool doFilter = false;
	char url[INTERNET_MAX_URL_LENGTH];
	DWORD dwSize= _countof(url);

	UrlItem* pItem = NULL;
	if (pPreprocData->GetHeader(pfc, "Host:", url, &dwSize))
	{
		char* u = url + dwSize - 1;
		dwSize= _countof(url) - dwSize;
		if( pPreprocData->GetHeader(pfc, "URL", u, &dwSize ) )
		{
			char szExt[25] = {'\0'};
			char* ext = strrchr(url, '.');
			if (ext)
			{
				strncpy_s(szExt, ext, _TRUNCATE);
				TRACE("ext is :%s", szExt);
			}
			doFilter = true;
			m_swm.WaitToRead();
			if (m_urlMap.Lookup(url, pItem))
			{
				int timeInterval = (strcmp(szExt, ".htm") || strcmp(szExt, ".html")) ? m_config.dynamicRescanInterval : m_config.staticRescanInterval;
				if ((time(NULL) - pItem->timestamp) <= timeInterval)
				{
					doFilter = false;
					TRACE("don't do filter\n");

				}
			}
			m_swm.Done();
			if (doFilter)
			{
				CReqContext* pReq = CReqContext::get(pfc);
				if( !pReq ) 
				{
					pReq = CReqContext::create(pfc);
					return dwRet;
				}
				else
				{
					TRACE("Get context from cache\n");
					pReq->Init();

				}
				strcpy(pReq->m_szUrl, url);
				pReq->m_item = pItem;
			}
		}
	}

	if(!doFilter) 
	{
		pfc->ServerSupportFunction(pfc, SF_REQ_DISABLE_NOTIFICATIONS, 0, SF_NOTIFY_SEND_RESPONSE|SF_NOTIFY_SEND_RAW_DATA, 0);
		if (pItem && pItem->keywordIndex != -1)
		{
			writeErrorToClient(pfc, pItem->keywordIndex);
			dwRet = SF_STATUS_REQ_FINISHED;
		}
	}
	return dwRet;
}

DWORD CKeywordsFilter::onSendResponse(HTTP_FILTER_CONTEXT *pfc, PHTTP_FILTER_PREPROC_HEADERS pHeader)
{
	DWORD dwRet = SF_STATUS_REQ_NEXT_NOTIFICATION;
	CReqContext* pReq = CReqContext::get(pfc);
	if( !pReq ) 
	{
		return dwRet;
	}
	pReq->doFilter = false;
	const int bufSize = 1024;
	char buf[bufSize];
	DWORD dwBufSize = bufSize;
	switch(pHeader->HttpStatus) 
	{
	case 100:
		pReq->doFilter = false;
		return dwRet;
		break;
	case 200:
		if (!pReq->m_item)
		{
			if( pHeader->GetHeader(pfc, "Content-Type:", buf, &dwBufSize) ) 
			{
				TRACE("content-type:%s\n", buf);
				pReq->doFilter = strncmp( buf, "text/html", 9) == 0;
				if (pReq->doFilter && !pReq->m_item)
				{
					// add to map
					m_swm.WaitToWrite();
					if (!m_urlMap.Lookup(pReq->m_szUrl, pReq->m_item))
					{
						pReq->m_item = new UrlItem();
						m_urlMap.SetAt(pReq->m_szUrl, pReq->m_item);
					}
					m_swm.Done();
				}
				char* charset = strstr(buf, "charset=");
				if (charset)
				{
					m_swm.WaitToWrite();
					strcpy(pReq->m_item->encoding, charset + 8);
					m_swm.Done();
				}
			}
		}
		else
		{
			pReq->doFilter = true;
		}
		if(!pReq->doFilter) {
			TRACE("don't do filter for %s\n", pReq->m_szUrl);
			pfc->ServerSupportFunction( pfc, SF_REQ_DISABLE_NOTIFICATIONS, 0, SF_NOTIFY_SEND_RAW_DATA,0 );
		}
		break;
	default:
		pfc->ServerSupportFunction( pfc, SF_REQ_DISABLE_NOTIFICATIONS, 0, SF_NOTIFY_SEND_RAW_DATA,0 );
		break;
	} 
	return dwRet;
}

int match(void * id, int index, void *data)
{
	CReqContext* pNotifier = (CReqContext*)data;
	pNotifier->m_index = (int)id;
	return 1;
}

UINT CKeywordsFilter::detectBestCodePage(IMultiLanguage2* pML2, const char* src, int len, MIMECPINFO& minfo)
{
	if (!pML2)
	{
		pML2 = (IMultiLanguage2*)TlsGetValue(s_tlslot);
	}
	if (!pML2)
	{
		CComPtr<IMultiLanguage2> spML2;
		HRESULT hr = ::CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2, (LPVOID*)&spML2);
		if (SUCCEEDED(hr))
		{
			pML2 = spML2.Detach();
			TlsSetValue(s_tlslot, spML2.Detach());
		}
	}
	UINT uCodePage = 0;
	DetectEncodingInfo info[5] = {0};
	int infoSize = 5;
	pML2->DetectInputCodepage(0, 0, (CHAR*)src, &len, info, &infoSize);
	int max_conf = -1, best = -1;
	for ( int i = 0; i < infoSize; ++i ) {
		if (info[i].nCodePage == 65001)
		{
			best = i;
			break;
		}
		if ( info[i].nConfidence > max_conf ) {
			best = i;
			max_conf = info[i].nConfidence;
		}
	}
	if (best != -1)
	{
		pML2->GetCodePageInfo(info[best].nCodePage, info[best].nLangID, &minfo);
		uCodePage = info[best].nCodePage;
	}
	return uCodePage;
}


DWORD CKeywordsFilter::onSendRawData(HTTP_FILTER_CONTEXT* pfc, PHTTP_FILTER_RAW_DATA pRawData)
{
	TRACE("onSendRawData\n");
	DWORD dwRet = SF_STATUS_REQ_NEXT_NOTIFICATION;
	CReqContext* pReq = CReqContext::get(pfc);
	if( !pReq || !pReq->m_item || !pReq->doFilter) 
	{
		return dwRet;
	}
	TRACE("do onSendRawData\n");
	
	if (pReq->m_flag == CReqContext::IN_NONE)
	{
		pReq->m_flag = CReqContext::IN_HEAD;
		char* type = FindHttpHeader("Content-Type:", (char*)pRawData->pvInData, pRawData->cbInData);
		if (type && strncmp(type, " text/", 6) == 0)
		{
			pReq->m_body = new CBodyBuffer();
		}
		else
		{
			TRACE("it is'nt a valid text stream\n");
			pReq->m_flag = CReqContext::IN_ERROR;
		}
	}
	if (pReq->m_flag == CReqContext::IN_HEAD)
	{
		TRACE("IN_HEAD\n");
		char* headerEnd = strnstr((char*)pRawData->pvInData, "\r\n\r\n", pRawData->cbInData);
		if (headerEnd)
		{
			pReq->m_flag = CReqContext::IN_BODY;
			pReq->m_bodyPos = headerEnd + 4 - (char*)pRawData->pvInData;
		}
		pReq->m_body->Append((char*)pRawData->pvInData, pRawData->cbInData);
		pRawData->pvInData = NULL;
		pRawData->cbInData = NULL;
	}
	if (pReq->m_flag == CReqContext::IN_BODY)
	{
		TRACE("IN_BODY\n");
		if (pRawData->pvInData)
		{
			pReq->m_body->Append((char*)pRawData->pvInData, pRawData->cbInData);
			pRawData->pvInData = NULL;
			pRawData->cbInData = 0;
		}
	}
	return dwRet;
}

DWORD CKeywordsFilter::onEndOfRequest(HTTP_FILTER_CONTEXT* pfc)
{
	DWORD dwRet = SF_STATUS_REQ_NEXT_NOTIFICATION;
	CReqContext* pReq = CReqContext::get(pfc);
	if (!pReq || !pReq->doFilter || !pReq->m_body)
		return dwRet;

	if (pReq->m_item->encoding[0] == '\0')
	{
		if (m_config.autoDetectEncoding)
		{
			// detect the code page
			MIMECPINFO cpinfo;
			UINT codepage = detectBestCodePage(NULL, *pReq->m_body, pReq->m_body->GetLength(), cpinfo);
			CW2A encod(cpinfo.wszBodyCharset);
			TRACE("Detected encoding is :%s\n", encod.m_psz);
			strncpy_s(pReq->m_item->encoding, encod.m_psz, _TRUNCATE);
		}
		else
		{
			strncpy_s(pReq->m_item->encoding, m_config.defaultEncoding, _TRUNCATE);
		}
	}
	void *wumanber = NULL;
	m_swm.WaitToRead();
	if (!m_search.Lookup(pReq->m_item->encoding, wumanber))
	{
		TRACE("use defualt us-ascii wumanber\n");
		wumanber = m_search["us-ascii"];
	}
	m_swm.Done();
	pReq->doFilter = false;

	if (wumanber 
		&& mwmSearch(wumanber, (BYTE*)(pReq->m_body->Data() + pReq->m_bodyPos), 
			pReq->m_body->GetLength() - pReq->m_bodyPos, match, pReq))
	{
		TRACE("Found Keywords,do error report\n");
		m_swm.WaitToWrite();
		pReq->m_item->keywordIndex = pReq->m_index;
		pReq->m_item->timestamp = time(NULL);
		m_swm.Done();
		writeErrorToClient(pfc, pReq->m_index);
	}
	else
	{
		TRACE("Have not Found Keywords,do normal output\n");
		m_swm.WaitToWrite();
		pReq->m_item->keywordIndex = -1;
		pReq->m_item->timestamp = time(NULL);
		m_swm.Done();
		DWORD cbData = pReq->m_body->GetLength();
		pfc->WriteClient(pfc, (void*)pReq->m_body->Data(), &cbData, 0);
	}
	delete pReq->m_body;
	pReq->m_body = NULL;
	return dwRet;
}




bool CKeywordsFilter::loadKeywords(LPCSTR charsets)
{
	bool bRet = false;
	//::EnterCriticalSection(&m_crit);
	POSITION pos = m_search.GetStartPosition();
	while (pos)
	{
		mwmFree(m_search.GetNext(pos)->m_value);
	}
	m_search.RemoveAll();
	m_patterns.RemoveAll();

	CComPtr<IMultiLanguage2> spML2;
	HRESULT hr = ::CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2, (LPVOID*)&spML2);
	if (SUCCEEDED(hr))
	{
		TCHAR path[MAX_PATH] = {'\0'};
		::PathCombine(path, m_szModulePath, _T("keywords.txt"));
		char* buffer = ReadFile(path);
		if (buffer)
		{
			// defualt it : CP_USASCII Indicates the USASCII character set, Windows code page 1252. 
			UINT srcCodePage = 20127; //us-ascii
			MIMECPINFO mmInfo;
			srcCodePage = detectBestCodePage(spML2, buffer, strlen(buffer), mmInfo);
			TRACE("keywords codepage is %d\n", srcCodePage);
			DWORD dwMode = 0;
			UINT unicode_len = strlen(buffer) + 1;
			WCHAR* unicode_key = new WCHAR[unicode_len];
			if (SUCCEEDED(spML2->ConvertStringToUnicode(&dwMode, srcCodePage, buffer, NULL, unicode_key, &unicode_len)))
			{
				unicode_key[unicode_len] = _T('\0');
				char* c = new char[strlen(charsets) + 6 + 1];
				strcpy(c, charsets);
				strcat(c, ",utf-8");
				const char* sep = ",;\r\n";
				char* next_token = NULL;
				char* charset = strtok_s(c, sep, &next_token); 
				while (charset)
				{
					if (charset[0] != '\0')
					{
						strlwr(charset);
						void *wumanber = NULL;
						if (!m_search.Lookup(charset, wumanber))
						{
							MIMECSETINFO info;
							if (SUCCEEDED(spML2->GetCharsetInfo(CA2T(charset), &info)))
							{
								wumanber = loadKeywords(spML2, unicode_key, info.uiInternetEncoding, strcmp(charset, "utf-8") == 0);
								if (wumanber)
								{
									TRACE("Succeeded to load keywords for encoding:%s\n", charset);
									m_search.SetAt(charset, wumanber);
								}
							}
						}
					}
					charset = strtok_s(NULL, sep, &next_token);
				}
				delete c;
			}
			delete unicode_key;
			bRet = true;
		}
		delete buffer;
	}
	//::LeaveCriticalSection(&m_crit);
	return bRet;
}

void* CKeywordsFilter::loadKeywords(IMultiLanguage2* pML, WCHAR* keywords, UINT toCodePage, bool bFirst)
{
	UINT destSize = 0;
	DWORD dwMode = 0;
	void* wu = NULL;
	if (SUCCEEDED(pML->ConvertStringFromUnicode(&dwMode, toCodePage, keywords, NULL, NULL, &destSize)))
	{
		CHAR* dest = new CHAR[destSize + 1];
		if (SUCCEEDED(pML->ConvertStringFromUnicode(&dwMode, toCodePage, keywords, NULL, dest, &destSize)))
		{
			dest[destSize] = '\0';
			wu = mwmNew();
			int i = 0;
			char* next_token = NULL;
			LPCSTR seps = ";,\r\n";
			char* keyword = strtok_s(dest, seps, &next_token);
			while (keyword)
			{
				if (keyword[0] != '\0')
				{
					if (bFirst)
					{
						m_patterns.AddTail(keyword);
					}
					mwmAddPatternEx(wu, (BYTE*)keyword, strlen(keyword), 1, 0, 0, (void*)(i++), 0);
				}
				keyword = strtok_s(NULL, seps, &next_token);
			}
			mwmPrepPatterns(wu);
		}
		delete dest;
	}
	return wu;
}

bool CKeywordsFilter::loadConfig()
{
	TCHAR path[MAX_PATH];
	::PathCombine(path, m_szModulePath, _T("config.ini"));
	CW2A p(path);
	m_config.autoDetectEncoding = GetPrivateProfileIntA("config", "auto_detect_encoding", 1, p);
	m_config.staticRescanInterval = GetPrivateProfileIntA("config", "static_rescan_interval", 60 * 60 , p) * 1000;
	m_config.dynamicRescanInterval = GetPrivateProfileIntA("config", "dynamic_rescan_interval", 10 * 60, p) * 1000;
	GetPrivateProfileStringA("config", "default_encoding", "utf-8", m_config.defaultEncoding, _countof(m_config.defaultEncoding), p);
	return true;
}


bool CKeywordsFilter::loadErrorHtml()
{
	::EnterCriticalSection(&m_crit);
	TCHAR path[MAX_PATH];
	::PathCombine(path, m_szModulePath, _T("error.html"));
	m_errorHtml = ReadFile(path);
	::LeaveCriticalSection(&m_crit);
	return m_errorHtml != NULL;
}

