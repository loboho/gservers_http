/*------------------------------------------------------------------------------
* FileName    : ginterfaces.h
* Author      : lbh
* Create Time : 2018-08-27
* Description : base interfaces define
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#ifndef __GBASE__GINTERFACES_H__
#define __GBASE__GINTERFACES_H__

struct IGRefRes
{
	virtual void AddRef() = 0;
	virtual void Release() = 0;
};


#endif // __GBASE__GINTERFACES_H__
