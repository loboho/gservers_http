/*------------------------------------------------------------------------------
* FileName    : memorys.hpp
* Author      : lbh
* Create Time : 2018-08-24
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/

namespace GMEM
{
	template <typename T, class... Args>
	T* feu_new(Args... args)
	{
		static_assert(std::is_trivially_destructible<T>::value,
			"the feu_new type should not have explicit destrcutor");
		void * p = feu_alloc(sizeof(T));
		new(p) T(args...);
		return reinterpret_cast<T*>(p);
	}

	// safe alloc a memory block
	template<typename T, class... Args>
	inline T* safe_new(Args... args)
	{
		void* p = alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__ ?
			safe_alloc(sizeof(T)) : safe_alloc_align(alignof(T), sizeof(T));
		new(p) T(args...);
		return reinterpret_cast<T*>(p);
	}

	// safe free the block
	template<typename T>
	inline void safe_delete(T* p)
	{
		if (safe_check_valid(p))
			p->~T();
		if (alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__)
			safe_free(p);
		else
			safe_free_align(p);
	}

	inline GRefBuffer* alloc_ref_buffer(size_t size)
	{
		return GRefBuffer::Create(size);
	}

	class _GAutoRelease
	{
		IGRefRes* m_piRes;
	public:
		_GAutoRelease(IGRefRes* piRes) : m_piRes(piRes) {}
		~_GAutoRelease() { m_piRes->Release(); }
	};
}
inline void * GRefBuffer::GetData() noexcept
{
	return data;
}

inline const void * GRefBuffer::GetData() const noexcept
{
	return data;
}

inline size_t GRefBuffer::GetSize() const noexcept
{
	return m_nSize;
}

inline void GRefBuffer::ForceSetSize(GRefBuffer& rbuf, int nSize) noexcept
{
	rbuf.m_nSize = nSize;
}

// get the max extendible buffer size
inline size_t GResizableBuffer::GetMaxExtendibleSize() noexcept
{
	return *(size_t*)(data + m_nSize);
}

/* reset the buffer size，注意若扩充buffer一定要调用ReSize扩充好才能往buffer写入数据！
	   when decrease the buffer size, the old data from 0 ~ size is safe
	   when increase the buffer size, the old data is safe, the available buffer is extended*/
inline bool GResizableBuffer::Resize(size_t size) noexcept
{
	size_t nReserveSize = *(size_t*)(data + m_nSize); // read the reservesize
	G_ASSERT_RET_CODE(int(nReserveSize) >= int(size), false);
	m_nSize = size;
	*(size_t*)(data + size) = nReserveSize; // replace the reservesize
	return true;
}

//template<typename T>
//GAutoIncPool<T>::~GAutoIncPool<T>()
//{
//	for (auto p : m_vecFreePObject)
//		if (p) delete p;
//}
