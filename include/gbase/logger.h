/*------------------------------------------------------------------------------
* FileName    : logger.h
* Author      : lbh
* Create Time : 2018-08-26
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#ifndef __GBASE__LOGGER_H__
#define __GBASE__LOGGER_H__

#include<iostream>
#include<string>

class Logger
{
public:
	template<class... Args>
	static void LogPrint(Args&& ... args)
	{
		LogPrintImpl(std::forward<Args>(args)...);
		std::cout << std::endl;
	}
	static std::ostream& LogStream()
	{
		return std::cout;
	}
protected:
	template <typename T> inline 
	static void Unpack(T&& t)
	{
		std::cout << std::forward<T>(t) << ' ';
	}
	template <typename ... Args> inline
	static void LogPrintImpl(Args&& ... args)
	{
		//int dummy[] = { 0, ((void)Unpack(std::forward<Args>(args)), 0) ... };
		((void)Unpack(std::forward<Args>(args)), ...); //// for c++ 17: 
	}
};

template <> inline
void Logger::Unpack<const std::string>(const std::string&& t)
{
	std::cout << std::forward<const std::string>(t).c_str() << ' ';
}

#endif // __GBASE__LOGGER_H__
