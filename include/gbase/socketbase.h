/*------------------------------------------------------------------------------
* FileName    : socketbase.h
* Author      : lbh
* Create Time : 2018-08-26
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#ifndef __GBASE__SOCKETBASE_H__
#define __GBASE__SOCKETBASE_H__

#include <functional>

struct GTimeVal
{
	int nSeconds;
	int nMicroSeconds;
};

enum ACCEPTOR_TYPE
{
	ACCEPTOR_TYPE_NORMAL = 0,
};

class GRefBuffer;
class GResizableBuffer;
class IGSocketStream : public IGRefRes
{
public:
	virtual int SetTimeout(const GTimeVal& time) = 0;

	// return -1: error, 0: timeout, 1: success
	virtual int CheckCanSend(const GTimeVal& time) = 0;

	// return -1: error, -2: disconnected, 0: timeout, 1: success, don't send a package > 65500 bytes
	virtual int Send(GRefBuffer& prBuffer) = 0;

	// return -1: error, 0: timeout, 1: success
	virtual int CheckCanRecv(const GTimeVal& time) = 0;

	/* return -2: disconnected, -1: error, 0: timeout, >0: received size
	   the max recv size is piBuffer->GetSize
	   then the piBuffer will be Resized to the true received size (0 if no data received)
	   */
	virtual int Recv(GResizableBuffer& prBuffer) = 0;

	virtual int TestAlive() = 0;

	virtual int GetRemoteAddress(struct in_addr *pRemoteIP, ushort *pusRemotePort) = 0;
};

class IGSocketAcceptor : public IGRefRes
{
public:
	// cszIPAddress: local ip. if cszIPAddress is nullptr(or "") then is bind in INADDR_ANY address
	virtual int Open(const char cszIPAddress[], int nPort) = 0;    //when succeed, return true,otherwise return false;
													  //ulAddress default is INADDR_ANY
	virtual int SetTimeout(const GTimeVal& time) = 0;

	virtual IGSocketStream* Accept() = 0;

	//virtual IGSocketStream *AcceptSecurity(ENCODE_DECODE_MODE EncodeDecodeMode);

	virtual int Close() = 0;

	static IGSocketAcceptor* CreateSocketAcceptor(ACCEPTOR_TYPE type);
};

struct GSocketConnector
{
	static IGSocketStream* Connect(
		const char* detIPAddress, int dstPort,
		const char* localIPAddress = nullptr, int nlocalPort = 0);
};

// SOCKET逻辑事件定义
enum  GSOCKET_EVENT
{
	SOCKET_PACKAGE, // 数据包到达, pvData: package address unpacked by IGImmutableUnpacker，nData: package len
	SOCKET_CREATE, // 连接创建, pvData: in_addr* of remote address，nData: remote port
	SOCKET_CLOSE,	// 连接丢失
	SOCKET_BLOCK, //  连接阻塞，可能发送缓冲区满
};

struct GHOST_ADDRESS
{
	char szIntranetIp[16]; // 内网Ip
	char szInternetIp[16]; // 外网Ip
	int nListenPort;
};

enum GE_SOCKET_MODE
{
	eG_SOCKET_MODE_NORMAL = 0
};

#endif // __GBASE__SOCKETBASE_H__
