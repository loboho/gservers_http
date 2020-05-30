/*------------------------------------------------------------------------------
* FileName    : memhelper.hpp
* Author      : lbh
* Create Time : 2018-08-23
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#include<vector>
#include "gbase/basedef.h"

namespace GBASE_INTERNAL
{
	class _GMemHelper
	{
		G_SINGLETON_SAFE(_GMemHelper)
	private:
		std::vector<void*> m_vecFeuNewPointors;
	public:
		G_FROCE_INLINE char* FeuAlloc(size_t size)
		{
			char* tp = new char[size];
			m_vecFeuNewPointors.push_back(tp);
			return tp;
		}

		~_GMemHelper()
		{
			for (auto p : m_vecFeuNewPointors)
				delete (char*)p;
		}
	};

}