/*
* SWMRG.h - This file is the part of the IISKeywordsFilter.
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

class CSWMRG
{
public:
	class CAutoReadLock
	{
	public:
		CAutoReadLock(CSWMRG& swmrg) : m_swmrg(swmrg)
		{
			m_swmrg.WaitToRead();
		}
		~CAutoReadLock()
		{
			m_swmrg.Done();
		}
	private:
		CSWMRG& m_swmrg;
	};

	class CAutoWriteLock
	{
	public:
		CAutoWriteLock(CSWMRG& swmrg) : m_swmrg(swmrg)
		{
			m_swmrg.WaitToWrite();
		}
		~CAutoWriteLock()
		{
			m_swmrg.Done();
		}
	private:
		CSWMRG& m_swmrg;
	};
public:
	CSWMRG(void);
	~CSWMRG(void);
	void WaitToRead();
	void WaitToWrite();
	void Done();

private:
	CRITICAL_SECTION m_cs;
	HANDLE m_hsemReaders;
	HANDLE m_hsemWriters;
	int m_nWaitingReaders;
	int m_nWaitingWriters;
	int m_nActive;

};
