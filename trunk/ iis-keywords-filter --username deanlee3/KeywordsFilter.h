/*
* KeywordsFilter.h - This file is the part of the IISKeywordsFilter.
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
#include "utils.h"
class UrlItem
{
public:
	UrlItem()
	{
		timestamp = time(NULL);
		encoding[0] = '\0';
		keywordIndex = -1;
	}
	time_t timestamp;
	char encoding[50];
	int keywordIndex;
	DECLARE_OBJECT_POOL(UrlItem);
};

class CReqContext
{
public:
	CReqContext(void);
	void Init();
	~CReqContext(void);
	static void destroy(PHTTP_FILTER_CONTEXT pfc);

	static CReqContext* get(HTTP_FILTER_CONTEXT* pfc);
	static CReqContext* create(HTTP_FILTER_CONTEXT* pfc);
public:
	enum FLAG{IN_NONE, IN_ERROR, IN_HEAD, IN_BODY, IN_END};
	HTTP_FILTER_RAW_DATA rawHeader;
	CBodyBuffer* m_body;
	int m_bodyPos;
	bool doFilter;
	int m_index;
	char m_szUrl[1024];
	FLAG m_flag;
	char m_encoding[50];
	UrlItem* m_item;
	DECLARE_OBJECT_POOL(CReqContext);

};

class CKeywordsFilter
{
public:
	CKeywordsFilter();
	~CKeywordsFilter();
	BOOL GetFilterVersion(HTTP_FILTER_VERSION *pVer);
	BOOL TerminateFilter(DWORD dwFlags);
	DWORD HttpFilterProc(HTTP_FILTER_CONTEXT *pfc, DWORD NotificationType, VOID *pvData);

	static HMODULE s_module;
	static DWORD s_tlslot;
	bool loadKeywords(LPCSTR charsets);
	TCHAR m_szModulePath[MAX_PATH];

protected:
	struct Config
	{
		bool autoDetectEncoding;
		int staticRescanInterval;
		int dynamicRescanInterval;
		char defaultEncoding[512];
	};
protected:
	DWORD onPreprocHeaders(HTTP_FILTER_CONTEXT *pfc, PHTTP_FILTER_PREPROC_HEADERS pvData);
	DWORD onSendResponse(HTTP_FILTER_CONTEXT *pfc, PHTTP_FILTER_PREPROC_HEADERS pHeader);
	DWORD onSendRawData(HTTP_FILTER_CONTEXT* pfc, PHTTP_FILTER_RAW_DATA pRawData);
	DWORD onEndOfRequest(HTTP_FILTER_CONTEXT* pfc);
	DWORD onEndOfNetSession(HTTP_FILTER_CONTEXT *pfc);
	void* loadKeywords(IMultiLanguage2* pML, WCHAR* keywords, UINT toCodePage, bool First);
	bool loadErrorHtml();
	bool loadConfig();
	void writeErrorToClient(HTTP_FILTER_CONTEXT *pfc, int keywordIndex);
	UINT detectBestCodePage(IMultiLanguage2* pML2, const char* src, int len, MIMECPINFO& minfo);
protected:
	CAtlMap<CStringA, UrlItem*> m_urlMap;
	CAtlMap<CStringA, void*> m_search;
	char* m_errorHtml;
	CRITICAL_SECTION m_crit;
	CSWMRG m_swm;
	CAtlList<CStringA> m_patterns;
	struct Config m_config;
};