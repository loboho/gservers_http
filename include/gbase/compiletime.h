/*------------------------------------------------------------------------------
* FileName    : compiletime.h
* Author      : lbh
* Create Time : 2018-09-13
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#ifndef __GBASE__COMPILETIME_H__
#define __GBASE__COMPILETIME_H__

namespace CPLT
{
	// compare the constant string s1 and s2
	template<class S1, class S2>
	constexpr int strcmp_c(S1 s1, S2 s2)
	{
		int i = 0;
		for (; s1[i]; i++)
		{
			if ((unsigned char)(s1[i]) > (unsigned char)(s2[i]))
				return 1;
			else if (s1[i] != s2[i])
				return -1;
		}
		return s2[i] == '\0' ? 0 : -1;
	}
};

// string join in compile time
class CStrJoin
{
public:
	enum { BUF_SIZE = 10240 };
	char buf[BUF_SIZE] = { };
	template<class... ArgCalcSize>
	constexpr CStrJoin(ArgCalcSize... args)
	{
		buf[0] = '\0';
		DoJoin(args...);
	}
private:
	constexpr void DoJoin(const char sz[])
	{
		int nTailPos = 0;
		while (buf[nTailPos])
			++nTailPos;
		do
		{
			buf[nTailPos++] = *sz++;
		} while (*sz);
	}
	template<class... ArgCalcSize>
	constexpr void DoJoin(const char sz[], ArgCalcSize... args)
	{
		int nTailPos = 0;
		while (buf[nTailPos])
			++nTailPos;
		do
		{
			buf[nTailPos++] = *sz++;
		} while (*sz);
		DoJoin(args...);
	}
};

// use CNumToCStr in compile time or run time
template<int radix = 10>
struct CNumToCStr
{
	constexpr const char* str() { return &_str[start]; }

	template<typename NumT>
	constexpr CNumToCStr(NumT num)
	{
		static_assert(radix > 1 && radix < 36, "radix error!");
		char index[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		NumT unum = num < 0 ? - num : num;
		_str[start] = '\0';
		do
		{
			_str[--start] = index[unum % radix];
			unum /= radix;
		} while (unum);
		if (num < 0)
			_str[--start] = '-';
	}

private:
	constexpr static auto BUF_SIZE = radix >= 24 ? 16 : (radix >= 5 ? 32 : 65);
	char _str[BUF_SIZE] = { };
	int start = BUF_SIZE - 1;
};

template<>
template<typename NumT>
constexpr CNumToCStr<2>::CNumToCStr(NumT num) // 二进制不输出符号
{
	_str[start] = '\0';
	if (num >= 0)
	{
		do
		{
			_str[--start] = (num & 1) ? '1' : '0';
			num >>= 1;
		} while (num);
	}
	else
	{
		NumT nBit = 1;
		do
		{
			_str[--start] = (num & nBit) ? '1' : '0';
			nBit <<= 1;
		} while (nBit);
	}
}

#endif // __GBASE__COMPILETIME_H__
