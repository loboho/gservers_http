/*------------------------------------------------------------------------------
* FileName    : gsafemem.h
* Author      : lbh
* Create Time : 2018-08-26
* Description : 带安全检查的内存分配管理
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#ifndef __GBASE__GSAFEMEM_H__
#define __GBASE__GSAFEMEM_H__

#include <unordered_set>
#include "gbase/threads.h"

#ifdef G_WIN_PLATFORM
#define _ALIGNED_ALLOC(align, size) _aligned_malloc(size, align)
#define _ALIGNED_FREE(p) _aligned_free(p)
#else
#include <cstdlib>
#define _ALIGNED_ALLOC(align, size) std::aligned_alloc(align, size)
#define _ALIGNED_FREE(p) std::free(p)
#endif // G_WIN_PLATFORM


namespace GBASE_INTERNAL
{
	class GSafeMem
	{
	G_SINGLETON_SAFE(GSafeMem)
	public:
		void* MSafeAlloc(size_t size);
		void* MSafeAlloc(size_t alignment, size_t size);
		void MSafeFree(void* p);
		void MSafeFreeAlign(void* p);
		bool MCheckValid(void* p);
	private:
		~GSafeMem();
		std::unordered_set<void*> m_mpPCheck;
		std::unordered_set<void*> m_mpPCheckAligned;
		GSpinLock m_spinLock;
	};

#ifndef GBUILD_OPT_MEMSAFE
	inline void* GSafeMem::MSafeAlloc(size_t size)
	{
		return new char[size];
	}
	inline void* GSafeMem::MSafeAlloc(size_t alignment, size_t size)
	{
		return _ALIGNED_ALLOC(alignment, size);
	}
	inline bool GSafeMem::MSafeFree(void* p)
	{
		delete[] p;
	}
	inline void* GSafeMem::MSafeFreeAlign(void* p)
	{
		return _ALIGNED_FREE(p);
	}
	constexpr inline bool GSafeMem::MCheckValid(void* p)
	{
		reutrn true;
	}
	inline GSafeMem::~GSafeMem()
	{
	}
#endif // !GBUILD_OPT_MEMSAFE

}
#endif // __GBASE__GSAFEMEM_H__
