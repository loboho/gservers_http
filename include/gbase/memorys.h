/*------------------------------------------------------------------------------
* FileName    : memorys.h
* Author      : lbh
* Create Time : 2018-08-23
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#ifndef __GBASE__MEMORYS_H__

#include "basedef.h"
#include "ginterfaces.h"
#include "typeext.h"
#include <functional>
#include <memory>
#include <atomic>
#include <type_traits>

#define G_AUTO_RELEASE(piRes) GMEM::_GAutoRelease piRes##_autorelease(piRes)

#define GFillMemory(p, len, c)	memset(p, c, len)
#define GZeroMemory(p, len)		memset(p, 0, len)

#define GZeroStruct(buf)		GZeroMemory(&(buf), sizeof(buf))

class GRefBuffer;
namespace GMEM
{
	// 申请一个永久对象，外部不需负责对象释放，T需要是可平凡析构类型
	template <typename T, class... Args>
	T* feu_new(Args... args);
	// 申请一块永久内存块
	char* feu_alloc(size_t size);
	// 申请一块引用计数内存
	GRefBuffer* alloc_ref_buffer(size_t size);
	// safe alloc a memory block
	char* safe_alloc(size_t size);
	// safe alloc a memory block with special alignment, must use safe_free_align to free
	char* safe_alloc_align(size_t alignment, size_t size);
	// safe free the block
	void safe_free(void* p);
	// safe free the block allocated by safe_alloc_align
	void safe_free_align(void* p);
	// check the safe p is valid or not
	bool safe_check_valid(void* p);
	// safe new an object
	template<typename T, class... Args>
	T* safe_new(Args... args);
	// safe delete the block
	template<typename T>
	void safe_delete(T* p);
}

// Bind void (*)(void*) to void (*)(FINAL_T*)
template<typename FINAL_T, void(*_real_release)(FINAL_T*)> inline
static void BindReleaser(void* p) { _real_release(reinterpret_cast<FINAL_T*>(p)); }
// Note: the default releaser which use GMEM::safe_delete requires GMEM::safe_new or GMEM::alloc to gen the res object
template<typename TInterface, void (*_releaser)(void*) = nullptr >
struct GThreadSafeRefRes : TInterface
{
	typedef GThreadSafeRefRes<TInterface, _releaser> THIS_TYPE;
	constexpr static auto releaser = _releaser ? _releaser : BindReleaser<THIS_TYPE, GMEM::safe_delete>;
	static_assert(std::is_base_of<IGRefRes, TInterface>::value, "TInterface must be (derived from) IGRefRes");
	void AddRef() noexcept { m_nRef.fetch_add(1, std::memory_order_relaxed); }
	void Release() noexcept { if (m_nRef.fetch_sub(1, std::memory_order_relaxed) == 1) releaser(this); }
	GThreadSafeRefRes() { m_nRef.store(1, std::memory_order_relaxed); }
	virtual ~GThreadSafeRefRes() {}
	std::atomic<int> m_nRef;
};

class GResizableBuffer;
class GRefBuffer : public GThreadSafeRefRes<IGRefRes>
{
public:
	using UPtr = std::unique_ptr<GRefBuffer, GReleaseDeleter<GRefBuffer>>; // unique ptr
	using SPtr = GSharedRefObjectPtr<GRefBuffer>; // shared ptr
	// Create a RefBuffer
	static GRefBuffer* Create(size_t size);
	static void ForceSetSize(GRefBuffer& rbuf, int nSize) noexcept;
	static void ForceSetSize(GResizableBuffer& rbuf, int nSize) = delete;
	// 效率考虑，GetData,GetSize不使用虚函数
	void*		GetData() noexcept;
	const void*	GetData()const noexcept;
	size_t		GetSize() const noexcept;

protected:
	GRefBuffer() = default;
	virtual ~GRefBuffer() = default;
	size_t m_nSize;
	char data[1];
};

class GResizableBuffer : public GRefBuffer
{
public:
	using UPtr = std::unique_ptr<GResizableBuffer, GReleaseDeleter<GResizableBuffer>>;
	using SPtr = GSharedRefObjectPtr<GResizableBuffer>;
	/*              size:	current available size
	   maxExtendibleSize:	the maximum extendible size*/
	static GResizableBuffer* Create(size_t size, size_t maxExtendibleSize);
	/* reset the buffer size，注意若扩充buffer一定要调用ReSize扩充好才能往buffer写入数据！
	   when decrease the buffer size, the old data from 0 ~ size is safe
	   when increase the buffer size, the old data is safe, the available buffer is extended*/
	bool Resize(size_t size) noexcept;
	// get the max extendible buffer size
	size_t GetMaxExtendibleSize() noexcept;
protected:
	GResizableBuffer() = default;
	virtual ~GResizableBuffer() = default;
};

/******** an auto-increasable objects pool **********
* the type T must have T() constructor
****************************************************/
#define G_POOL_USE_INIT _Pool_Use_Init
#define G_POOL_USE_UNINIT _Pool_Use_UnInit
template<typename T> class GAutoIncPool
{
public:
	// obtain an object, the T::_Pool_Use_Init() will be called (if exist) rather than the constructor
	T* Buy();
	// give it back to pool, the T::_Pool_Use_UnInit() will be called (if exist) rather than the destructor
	void Recycle(T *p);
	~GAutoIncPool();
protected:
	class _Implement;
	_Implement* pImplement;
};

#include "hpp/memorys.hpp"

#define __GBASE__MEMORYS_H__
#endif // __GBASE__MEMORYS_H__
