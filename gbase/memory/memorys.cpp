/*------------------------------------------------------------------------------
* FileName    : memorys.cpp
* Author      : lbh
* Create Time : 2018-08-24
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#include "pch.h"
#include <vector>
#include <list>
#include <algorithm>
#include "gbase/memorys.h"
#include "memhelper.hpp"
#include "gsafemem.h"

using namespace GBASE_INTERNAL;
using namespace std;

GRefBuffer * GRefBuffer::Create(size_t size)
{
	size_t szBuffer = sizeof(GRefBuffer) + size - 1;
	auto pRefbuffer = reinterpret_cast<GRefBuffer*>(GSafeMem::Inst().MSafeAlloc(szBuffer));
	new (pRefbuffer) GRefBuffer; // placement new 
	pRefbuffer->m_nSize = size;
	return pRefbuffer;
}


/* reserveSize固定存储于data + m_nSize的位置，在data数据发生写入溢出（超出当前m_nSize大小）时会有难料的结果
   若要加强此安全检查，可以再开辟多一个int放在reserveSize后作为校验（比如校验码为reserveSize ^ m_nSize） */
GResizableBuffer* GResizableBuffer::Create(size_t size, size_t reserveSize)
{
	if (reserveSize < size)
	{
		G_ASSERT(false);
		reserveSize = size;
	}
	GResizableBuffer* piBuffer = static_cast<GResizableBuffer*>(
		GRefBuffer::Create(reserveSize + sizeof(size_t)));
	*(size_t*)((char*)piBuffer->GetData() + size) = reserveSize;
	piBuffer->m_nSize = size;
	return piBuffer;
}

namespace GMEM
{

	char* feu_alloc(size_t size)
	{
		return _GMemHelper::Inst().FeuAlloc(size);
	}

	// safe alloc a memory block
	char* safe_alloc(size_t size)
	{
		return (char*)GSafeMem::Inst().MSafeAlloc(size);
	}

	// safe alloc a memory block with special alignment, must use safe_free_align to free
	char* safe_alloc_align(size_t alignment, size_t size)
	{
		return (char*)GSafeMem::Inst().MSafeAlloc(alignment, size);
	}

	// safe free the block
	void safe_free(void* p)
	{
		GSafeMem::Inst().MSafeFree(p);
	}

	// safe free the block allocated by safe_alloc_align
	void safe_free_align(void* p)
	{
		GSafeMem::Inst().MSafeFreeAlign(p);
	}

	// check the safe p is valid or not
	bool safe_check_valid(void* p)
	{
		return GSafeMem::Inst().MCheckValid(p);
	}
}

template<typename T>
class GAutoIncPool<T>::_Implement
{

public:
	G_FROCE_INLINE T *Buy()
	{
		T *p = nullptr;
		return p;
	}
	G_FROCE_INLINE void Recycle(T *p)
	{
	}
};


template<typename T>
T *GAutoIncPool<T>::Buy()
{
	return pImplement->Buy();
}

template<typename T>
void GAutoIncPool<T>::Recycle(T *p)
{
	return pImplement->Recycle(p);
}
