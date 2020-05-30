/*------------------------------------------------------------------------------
* FileName    : pch.h
* Author      : lbh
* Create Time : 2018-08-23
* Description : precompile header
* 
* ----------------------------------------------------------------------------*/
#ifndef __GBASE__PCH_H__
#define __GBASE__PCH_H__

#include <vector>
#include <list>
#include <map>
#include <ostream>
#include <cstring>
#include "gbase/basedef.h"
// -------------------------------------------------------------------------
#ifdef G_WIN_PLATFORM
#define WIN32_LEAN_AND_MEAN // 防止winsock等头文件冲突
#include <Windows.h>
//#define _WINSOCKAPI_

#endif




#endif // __GBASE__PCH_H__
