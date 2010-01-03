/*
* SWMRG.cpp - This file is the part of the IISKeywordsFilter.
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
#include "StdAfx.h"
#include "SWMRG.h"

CSWMRG::CSWMRG(void)
{
	m_nWaitingReaders = m_nWaitingWriters = m_nActive = 0;
	m_hsemReaders = ::CreateSemaphore(NULL, 0, MAXLONG, NULL);
	m_hsemWriters = ::CreateSemaphore(NULL, 0, MAXLONG, NULL);
	::InitializeCriticalSection(&m_cs);
}

CSWMRG::~CSWMRG(void)
{
	m_nWaitingReaders = m_nWaitingWriters = m_nActive = 0;
	DeleteCriticalSection(&m_cs);
	CloseHandle(m_hsemReaders);
	CloseHandle(m_hsemWriters);
}

void CSWMRG::WaitToRead()
{
	EnterCriticalSection(&m_cs);
	bool fResourceWritePending = (m_nWaitingWriters || (m_nActive < 0));
	if (fResourceWritePending)
	{
		m_nWaitingReaders++;
	}
	else
	{
		m_nActive++;
	}
	LeaveCriticalSection(&m_cs);
	if (fResourceWritePending)
	{
		::WaitForSingleObject(m_hsemReaders, INFINITE);
	}
}

void CSWMRG::WaitToWrite()
{
	EnterCriticalSection(&m_cs);
	bool fResourceOwned = (m_nActive != 0);
	if (fResourceOwned)
	{
		m_nWaitingWriters++;
	}
	else
	{
		m_nActive = -1;
	}
	LeaveCriticalSection(&m_cs);
	if (fResourceOwned)
	{
		::WaitForSingleObject(m_hsemWriters, INFINITE);
	}
}

void CSWMRG::Done()
{
	EnterCriticalSection(&m_cs);
	if (m_nActive > 0)
	{
		m_nActive--;
	}
	else
	{
		m_nActive++;
	}
	HANDLE hsem = NULL;
	LONG lCount = 1;
	if (m_nActive == 0)
	{
		if (m_nWaitingWriters > 0)
		{
			m_nActive = -1;
			m_nWaitingWriters--;
			hsem = m_hsemWriters;
		}
		else if (m_nWaitingReaders > 0)
		{
			m_nActive = m_nWaitingReaders;
			m_nWaitingReaders = 0;
			hsem = m_hsemReaders;
			lCount = m_nActive;
		}
		else
		{
			// There are no threads waiting at all : no semaphore gets released
		}
	}
	::LeaveCriticalSection(&m_cs);
	if (hsem != NULL)
	{
		::ReleaseSemaphore(hsem, lCount, NULL);
	}
}
