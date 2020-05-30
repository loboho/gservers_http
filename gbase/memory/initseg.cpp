/*------------------------------------------------------------------------------
* FileName    : initseg.cpp
* Author      : lbh
* Create Time : 2018-08-23
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#include "pch.h"
#include "memhelper.hpp"
#include "gsafemem.h"
#include "logger.h"
//#include "logwritter.h"

using namespace GBASE_INTERNAL;

#pragma init_seg(lib)

G_SINGLETON_SAFE_DEF(GSafeMem)
//G_SINGLETON_SAFE_DEF(GScreenLogWritter)
G_SINGLETON_SAFE_DEF(_GMemHelper)


