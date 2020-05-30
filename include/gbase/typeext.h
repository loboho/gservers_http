/*------------------------------------------------------------------------------
* FileName    : typeext.h
* Author      : lbh
* Create Time : 2018-10-21
* Description : 
* 
* ----------------------------------------------------------------------------*/
#ifndef __GBASE__TYPEEXT_H__
#define __GBASE__TYPEEXT_H__

#include <type_traits>
#include <functional>
#include <optional>

// optional return value supporting void
template <typename R>
struct GRetOptional : public std::optional<R>
{
	template<class F, class... ArgTypes> inline
	void InvokeAndGetRet(F&& fn, ArgTypes&&... args)
	{
		emplace(fn(std::forward<ArgTypes>(args)...));
	}
	template<class C, class F, class... ArgTypes> inline
	void InvokeAndGetRet(C&& c, F fn, ArgTypes&&... args)
	{
		emplace((c.*fn)(std::forward<ArgTypes>(args)...));
	}
};

template <>
struct GRetOptional<void> : public std::optional<std::is_void<void>>
{
	template<class F, class... ArgTypes> inline
	void InvokeAndGetRet(F&& fn, ArgTypes&&... args)
	{
		fn(std::forward<ArgTypes>(args)...);
	}
	template<class C, class F, class... ArgTypes> inline
	void InvokeAndGetRet(C&& c, F fn, ArgTypes&&... args)
	{
		(c.*fn)(std::forward<ArgTypes>(args)...);
	}
};

class GReleaseCall
{
	std::function<void()> m_fn;
public:
	// CallableT must be a void() callable
	template<class CallableT>
	GReleaseCall(CallableT&& fn) : m_fn(fn) {};
	~GReleaseCall() { m_fn(); }
	G_DEF_NONE_COPYABLE(GReleaseCall)
};


template <class T>
struct GReleaseDeleter {
	constexpr GReleaseDeleter() noexcept = default;
	void operator()(T* p)const noexcept { p->Release(); }
};

// the shared_ptr for internal reference counting object
template <class T>
class GSharedRefObjectPtr {
public:
	constexpr GSharedRefObjectPtr() noexcept : m_p(nullptr) {}
	constexpr GSharedRefObjectPtr(std::nullptr_t) noexcept : m_p(nullptr) {}
	explicit GSharedRefObjectPtr(T* p) noexcept : m_p(p) { }
	// copy constructor
	GSharedRefObjectPtr(const GSharedRefObjectPtr& other) noexcept : m_p(other.m_p) {
		if (m_p) m_p->AddRef();
	}
	// move constructor
	GSharedRefObjectPtr(GSharedRefObjectPtr&& other) noexcept : m_p(other.m_p) {
		other.m_p = nullptr;
	}
	~GSharedRefObjectPtr() { if (m_p) m_p->Release(); }
	GSharedRefObjectPtr& operator=(const GSharedRefObjectPtr& other) noexcept {
		GSharedRefObjectPtr(other).swap(*this);
		return *this;
	}
	GSharedRefObjectPtr& operator=(GSharedRefObjectPtr&& other) noexcept {
		GSharedRefObjectPtr(std::move(other)).swap(*this);
		return *this;
	}
	operator bool() const noexcept { return m_p != nullptr; }
	void swap(GSharedRefObjectPtr<T>& other) noexcept {
		std::swap(m_p, other.m_p);
	}
	void reset(T* p = nullptr) noexcept {
		GSharedRefObjectPtr(p).swap(*this);
	}
	T* get() const noexcept { return m_p; }
	T& operator*() const noexcept { return *m_p; }
	T* operator->() const noexcept { return m_p; }
private:
	T* m_p;
};

#endif // __GBASE__TYPEEXT_H__
