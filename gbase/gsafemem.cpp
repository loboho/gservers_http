/*------------------------------------------------------------------------------
* FileName    : gsafemem.cpp
* Author      : lbh
* Create Time : 2018-08-26
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#include "pch.h"
#include <cstdlib>
#include "gsafemem.h"

using namespace GBASE_INTERNAL;

static_assert(__STDCPP_DEFAULT_NEW_ALIGNMENT__ >= sizeof(void*), "Not Expected Default Alignment!");

#ifdef GBUILD_OPT_MEMSAFE

void* GSafeMem::MSafeAlloc(size_t size)
{
	void* p = new char[size];
	m_spinLock.Lock();
	bool succeed = m_mpPCheck.insert(p).second;
	m_spinLock.Unlock();
	G_ASSERT(succeed);
	return p;
}

void* GSafeMem::MSafeAlloc(size_t alignment, size_t size)
{
	void* p = _ALIGNED_ALLOC(alignment, size);
	m_spinLock.Lock();
	bool succeed = m_mpPCheckAligned.insert(p).second;
	m_spinLock.Unlock();
	G_ASSERT(succeed);
	return p;
}

void GSafeMem::MSafeFree(void* p)
{
	m_spinLock.Lock();
	bool succeed = (m_mpPCheck.erase(p) == 1);
	m_spinLock.Unlock();
	G_ASSERT_THEN(succeed)
	{
		delete[] (char*)p;
	}
}

void GSafeMem::MSafeFreeAlign(void* p)
{
	m_spinLock.Lock();
	bool succeed = (m_mpPCheckAligned.erase(p) == 1);
	m_spinLock.Unlock();
	G_ASSERT_THEN(succeed)
	{
		_ALIGNED_FREE(p);
	}
}

bool GSafeMem::MCheckValid(void * p)
{
	return m_mpPCheck.find(p) != m_mpPCheck.end() ||
		m_mpPCheckAligned.find(p) != m_mpPCheckAligned.end();
}

GSafeMem::~GSafeMem()
{
	if (m_mpPCheck.size() > 0)
	{
		GLOG_ERROR("memory leak!");
	}
	if (m_mpPCheckAligned.size() > 0)
	{
		GLOG_ERROR("(Aligned) memory leak!");
	}
}

#endif // GBUILD_OPT_MEMSAFE
