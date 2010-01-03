/*
* objectpool.h - This file is the part of the IISKeywordsFilter.
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

#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H
#include "link.h"
//struct _SEGMENT;
typedef struct _SEGMENT *PSEGMENT;

class CObjectPool  
{
public:
	CObjectPool(IN const size_t   nBufferSize,
		IN const unsigned int	nNumberOfBuffersInSegment,
		IN const unsigned int	nNumberOfSegmentsStrat,
		IN const unsigned int	nNumberOfSegmentsLowMark,
		IN const unsigned int	nNumberOfSegmentsHighMark = -1,
		IN const double			lfRatioForSegmentDeletion = 0.333);
	virtual ~CObjectPool();
	void* Allocate();
	void  Free(const void* pBuffer);

	void Destroy();
	inline size_t GetAllocSize() const
	{
		return (size_t)m_nBufferSize;
	}

	/*bool IsCreated()const
	{
	return m_bCreated;
	}*/
private:
	bool AllocateSegment();
	void FreeSegment(IN PSEGMENT p_segment);

	//member variables
	DLINK			m_SegmentsListHead;

	unsigned int	m_nNumberOfBuffersInSegment;
	unsigned int	m_nNumberOfSegmentsStrat;
	unsigned int	m_nNumberOfSegmentsLowMark;
	unsigned int	m_nNumberOfSegmentsHighMark;
	unsigned int	m_nBufferSize;

	UINT			m_nNumberOfSegments;
	UINT			m_nFreeBuffers;

	PSEGMENT		m_pSegmentCandidateForDeletion;
	unsigned int	m_nCandidateDeletionLimit;

	CRITICAL_SECTION			m_CriticalSection;	
	DWORD m_dwPageSize;
	//bool m_bCreated;
};
/*------------------------------------------------------------------------
operator new()

new "placement" using the buffer pool instade of the real allocation


size_t size                  size of the object a must parameter
CObjectPool * BufferPool     buffer pool that allocates buffers of the object size

------------------------------------------------------------------------*/
//inline void * operator new(size_t size , CObjectPool * BufferPool)
//{
//   //allocate the buffer
//   return BufferPool->Allocate();
//}
///*------------------------------------------------------------------------
//operator delete()
//
//  delete placement using the buffer pool instade of the real allocation
//  
// DoomedObject - pointer to the buffer holding the object
// CObjectPool * BufferPool - buffer pool that free buffers of the object size
//				
//------------------------------------------------------------------------*/
//inline void   operator delete(void * DoomedObject , CObjectPool * BufferPool)
//{   
//   //free the buffer
//   BufferPool->Free(DoomedObject);
//}



template <class T>
class  CObjectPoolT
{
public:
	CObjectPoolT(IN const unsigned int nNumberOfObjectsInSegment,
		IN const unsigned int nNumberOfSegmentsStrat,
		IN const unsigned int nNumberOfSegmentsLowMark,
		IN const unsigned int nNumberOfSegmentsHighMark = -1);

	inline void*   Allocate();

	inline void Free(IN void * obj);

	void        Destroy();

private:
	CObjectPool BufferPool;
};


/*------------------------------------------------------------------------
Create()

Call the create of the buffer pool with the object size

IN UINT nNumberOfObjectsInSegment - Number of Objects in a segment
IN UINT nNumberOfSegmentsStart    - Start Number of Segments
IN UINT nNumberOfSegmentsLowMark  - Minimum Number of Segments
IN UINT nNumberOfSegmentsHighMark - Maximum Number of Segments

------------------------------------------------------------------------*/

template <class T>
CObjectPoolT<T>::CObjectPoolT(IN const unsigned int nNumberOfObjectsInSegment,
							  IN const unsigned int nNumberOfSegmentsStart,
							  IN const unsigned int nNumberOfSegmentsLowMark,
							  IN const unsigned int nNumberOfSegmentsHighMark = -1) :
BufferPool(sizeof(T), nNumberOfObjectsInSegment,
		   nNumberOfSegmentsStart,
		   nNumberOfSegmentsLowMark,
		   nNumberOfSegmentsHighMark)
{
	//create the buffer pool with a buffer size sizeof(T)
}

/*------------------------------------------------------------------------
Allocate()

returnes a pointer to the object allocated using the buffer pool

------------------------------------------------------------------------*/

template <class T>
inline void * CObjectPoolT<T>::Allocate()
{ 
	//call the placement new operator
	return BufferPool.Allocate();
}

/*------------------------------------------------------------------------
Free()

returns the object to the pool allocated using the buffer pool

T * obj - the pointer to the object allocated using the object pool

------------------------------------------------------------------------*/


template <class T>
inline void CObjectPoolT<T>::Free(IN void * obj)
{
	//call the placement delete operator
	BufferPool.Free(obj);
}


/*------------------------------------------------------------------------
Destroy()

Destroys the pool

------------------------------------------------------------------------*/

template <class T>
void CObjectPoolT<T>::Destroy()
{
	//Destroy buffer pool
	BufferPool.Destroy();
}

#define DECLARE_OBJECT_POOL(type)		\
public:	\
	void *operator new(size_t size) {	\
	ATLASSERT(size == s_alloc.GetAllocSize()); \
	return s_alloc.Allocate();	\
}\
	void* operator new(size_t, void* p) \
{ return p; } \
	void operator delete(void *p) {	\
	s_alloc.Free((type*)p);	\
}\
	void* operator new(size_t , LPCSTR, int) \
{ \
	return s_alloc.Allocate(); \
} \
protected:	\
	static CObjectPool s_alloc;\

#define IMPLEMENT_OBJECT_POOL(type, nNumberOfObjectsInSegment, nNumberOfSegmentsStart, nNumberOfSegmentsLowMark)	\
	CObjectPool type::s_alloc(sizeof(type), nNumberOfObjectsInSegment, nNumberOfSegmentsStart, nNumberOfSegmentsLowMark);

#endif // OBJECTPOOL_H

