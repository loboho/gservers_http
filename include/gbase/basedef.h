/*------------------------------------------------------------------------------
* FileName    : basedef.h
* Author      : lbh
* Create Time : 2018-08-23
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#ifndef __GBASE__BASEDEF_H__
#define __GBASE__BASEDEF_H__

#include "logger.h"
#include "compiletime.h"

#define  NOMINMAX	// 禁掉Windows min、max宏
#define GBUILD_OPT_MEMSAFE // open safe memory check
#define _GCOMMENT(statement) #statement
#define GCOMMENT(macro) _GCOMMENT(macro) // 展开宏为字符串

#if defined(_WIN32) || defined(_WIN64)
#pragma warning(disable:4267)
#pragma warning(disable:4996)

#define G_ASSERT_NOTIFY_ON_WIN_DEBUG	_ASSERT(false)
#define G_FROCE_INLINE __forceinline
#define G_WIN_PLATFORM 1

#ifdef _DEBUG
#define G_WIN_DEBUG 1
#endif // _DEBUG

#else // #ifdef G_WIN_PLATFORM

#define G_ASSERT_NOTIFY_ON_WIN_DEBUG
#define G_FROCE_INLINE __attribute__((always_inline))
inline char* strcpy_s(char* dst, const char* src) {
	return strcpy(dst, src); }
inline char* strcpy_s(char* dst, size_t size, const char* src) {
	return strcpy(dst, src); }
inline char* strncpy_s(char* dst, const char* src, size_t maxCount) {
	return strncpy(dst, src, maxCount); }
inline char* strncpy_s(char* dst, size_t size, const char* src, size_t maxCount) {
	return strncpy(dst, src, maxCount); }
inline char* strcat_s(char* dst, const char* src) {
	return strcat(dst, src); }
inline char* strcat_s(char* dst, size_t size, const char* src) {
	return strcat(dst, src); }
template<class... Args>
inline int sprintf_s(char *str, const char *format, Args...args) {
	return sprintf(str, format, args...); }
template<class... Args>
inline int sprintf_s(char *str, size_t size, const char *format, Args...args) {
	return sprintf(str, format, args...); }

#endif

#define G_LOG_STREAM Logger::LogStream()
/*************** NOTE************************************************************
it's better to use stream to print messages due to any formatting print is unsafe
********************************************************************************/
// log the message
#define GLOG(...)	Logger::LogPrint(__VA_ARGS__)
// print an error message (with function and line specified)
#define GLOG_ERROR(...)	{ GLOG(CStrJoin("[@Ecpp] ", __FUNCTION__, ":", CNumToCStr<>(__LINE__).str()).buf,\
							__VA_ARGS__); G_ASSERT_NOTIFY_ON_WIN_DEBUG; }
#define GLUA_ERROR(...)	{ GLOG("[@ELua] ", __VA_ARGS__); G_ASSERT_NOTIFY_ON_WIN_DEBUG; }
// print an info message
#define GLOG_INFO(...)	{ GLOG("[@Icpp] ", __VA_ARGS__); }
// print an trace message
#define GLOG_TRACE(...)	{ GLOG("[@Tcpp] ", __VA_ARGS__); }
// print an warn message
#define GLOG_WARN(...)	{ GLOG("[@Wcpp] ", __VA_ARGS__); }
// print an indicate message
#define GLOG_INDICATE(indicate, ...) { GLOG(indicate" ", __VA_ARGS__); }
// the exit point 0 of the function, referred by G_ASSERT_EXIT_0
#define G_EXIT_0 _g_exit_0
// the exit point 1 of the function, referred by G_ASSERT_EXIT_1
#define G_EXIT_1 _g_exit_1
// test the condition and print the error message if fail
#define G_ASSERT(condition)  if (!(condition)) GLOG_ERROR(#condition); 
// test the condition and do fllowing block/statement if succeed
#define G_ASSERT_THEN(condition)  if (!(condition)) GLOG_ERROR(#condition) else
// do a G_ASSERT and goto exit point 0 if fail
#define G_ASSERT_EXIT_0(condition) if (!(condition)) { GLOG_ERROR(#condition); goto G_EXIT_0; }
// do a G_ASSERT and goto exit point 1 if fail
#define G_ASSERT_EXIT_1(condition) if (!(condition)) { GLOG_ERROR(#condition); goto G_EXIT_1; }
// do simple test and goto exit point 0 if fail
#define G_TEST_EXIT_0(condition) if (!(condition)) goto G_EXIT_0;
// do simple test and goto exit point 1 if fail
#define G_TEST_EXIT_1(condition) if (!(condition)) goto G_EXIT_1;
// do a G_ASSERT and return code if fail
#define G_ASSERT_RET_CODE(condition, code) if (!(condition)) { GLOG_ERROR(#condition); return code; }
#define G_ASSERT_RET(condition) if (!(condition)) { GLOG_ERROR(#condition); return; }
// do a G_ASSERT and break if fail
#define G_ASSERT_BREAK(condition) if (!(condition)) { GLOG_ERROR(#condition); break; }
// do a G_ASSERT and continue if fail
#define G_ASSERT_CONTINUE(condition) if (!(condition)) { GLOG_ERROR(#condition); continue; }

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char byte;
typedef const char cchar;
#if defined(_WIN32) || defined(_WIN64)
typedef __int64 int64;
typedef unsigned __int64 uint64;
#else
typedef long long int int64;
typedef unsigned long long int uint64;
#endif

#define G_DEF_NONE_COPYABLE(cls) cls(const cls&) = delete;\
							cls& operator=(const cls&) = delete;
// safe singlethon, used with G_SINGLETON_SAFE_DEF
#define G_SINGLETON_SAFE2(cls, init_fun) private: cls(){ init_fun(); }\
										private: static cls _Inst; \
										G_DEF_NONE_COPYABLE(cls)\
										public: inline static cls& Inst() { return _Inst; }
										
#define G_SINGLETON_SAFE(cls) private: cls() = default;\
										private: static cls _Inst; \
										G_DEF_NONE_COPYABLE(cls)\
										public: inline static cls& Inst() { return _Inst; }
// safe singlethon inst define
#define G_SINGLETON_SAFE_DEF(cls) cls cls::_Inst;

// not thread safe及有轻微效率损失，但可实现lazy loading，以及lazy coding...
#define G_SINGLETON_LAZY(cls) private: cls() = default; \
								public: static cls& Inst() { static cls _Inst; return _Inst; }

#define G_SAFE_RELEASE(pi) if(pi) { (pi) ->Release(); (pi) = nullptr; }
#define G_SAFE_DELETE(p) if(p) { delete (p); (p) = nullptr; }
#define G_SAFE_DELETE_ARY(p) if(p) { delete[] (p); (p) = nullptr; }

#ifdef G_WIN_PLATFORM
#define G_LAST_ERRCODE() GetLastError()
#else
#define G_LAST_ERRCODE() errno
#endif

#ifndef MAX_PATH
#define MAX_PATH		260
#endif

typedef struct _G_IID {
	unsigned int  Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char  Data4[8];
	bool operator == (const _G_IID& other)
	{
		return 0 == memcmp(this, &other, sizeof(_G_IID));
	}
} G_IID;

#define itoa(...) static_assert(false, "Use CNumToCStr instead!")
#define _itoa(...) static_assert(false, "Use CNumToCStr instead!")

#endif // __GBASE__BASEDEF_H__
