/*------------------------------------------------------------------------------
* FileName    : socketbase.cpp
* Author      : lbh
* Create Time : 2018-08-26
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#include "pch.h"
#ifndef G_WIN_PLATFORM
#include <signal.h>
#endif // !G_WIN_PLATFORM

#include "gbase/memorys.h"
#include "gbase/socketbase.h"
#include "gbase/socketapi.h"
#include "gbase/threads.h"

using namespace GSOCKET_API;

constexpr auto MAX_SEND_RECV = 65500;

timeval _mk_time_val(const GTimeVal& val)
{
	return timeval{ val.nSeconds, val.nMicroSeconds };
}

// Class GSocketStream begin
class GSocketStream : public GThreadSafeRefRes<IGSocketStream>
{
public:
	static GSocketStream* Create(int nSocket, const sockaddr_in& remoteAddr);	
	virtual int SetTimeout(const GTimeVal& time);
	// return -1: error, 0: timeout, 1: success
	virtual int CheckCanSend(const GTimeVal& time);
	// return -1: error, 0: timeout, 1: success, don't send a package > 65500 bytes
	virtual int Send(GRefBuffer& prBuffer);
	// return -1: error, 0: timeout, 1: success
	virtual int CheckCanRecv(const GTimeVal& time);
	// return -2: again, -1: error, 0: timeout, >0: the received bytes
	virtual int Recv(GResizableBuffer& prBuffer);
	virtual int TestAlive();
	virtual int GetRemoteAddress(struct in_addr *pRemoteIP, ushort *pusRemotePort);
private:
	GSocketStream(int nSocket, const sockaddr_in& remoteAddr);
	~GSocketStream();
	int m_nSocket = -1;
	timeval m_cTimeVal = { -1, -1 };
	struct sockaddr_in  m_RemoteAddress;
	template<typename T, class... Args>
	friend T* GMEM::safe_new(Args... args);
};

GSocketStream::GSocketStream(int nSocket, const sockaddr_in& remoteAddr)
{
	m_RemoteAddress = remoteAddr;
	m_nSocket = nSocket;
}

GSocketStream::~GSocketStream()
{
	if (m_nSocket >= 0)
		gclose_socket(m_nSocket);
}

GSocketStream * GSocketStream::Create(int nSocket, const sockaddr_in& remoteAddr)
{
	return GMEM::safe_new<GSocketStream>(nSocket, remoteAddr);
}

int GSocketStream::SetTimeout(const GTimeVal& time)
{
	m_cTimeVal.tv_sec = time.nSeconds;
	m_cTimeVal.tv_usec = time.nMicroSeconds;
	return 1;
}

int GSocketStream::CheckCanSend(const GTimeVal& time)
{
	timeval val = _mk_time_val(time);
	return gcheck_can_send(m_nSocket, &val);
}

int GSocketStream::Send(GRefBuffer& prBuffer)
{
	int nResult = -1;  // error
	int nRetCode = false;
	unsigned uBufferSize = 0;

	prBuffer.AddRef();

	uBufferSize = prBuffer.GetSize();
	G_ASSERT_EXIT_0(uBufferSize <= MAX_SEND_RECV);

	//pbyBuffer = (unsigned char *)pBuffer->GetData();

	//pbyBuffer -= sizeof(short);
	//uBufferSize += sizeof(short);

	//*((short *)pbyBuffer) = (short)uBufferSize;

	nRetCode = gsend_or_recv_buffer(
		true,   // SendFlag = true
		m_nSocket,
		(char*)prBuffer.GetData(),
		(int)uBufferSize,
		m_cTimeVal
	);

	nResult = nRetCode;
	G_TEST_EXIT_0(nRetCode > 0);
		

	nResult = 1;    // success
G_EXIT_0:
	prBuffer.Release();

	return nResult;
}

int GSocketStream::CheckCanRecv(const GTimeVal& time)
{
	timeval val = _mk_time_val(time);
	return gcheck_can_recv(m_nSocket, &val);
}

int GSocketStream::Recv(GResizableBuffer& prBuffer)
{
	int nResult = -1;   // error

	nResult = gsend_or_recv_buffer(false, m_nSocket,
		(char*)prBuffer.GetData(), prBuffer.GetSize(), m_cTimeVal);
	if (nResult > 0)
		prBuffer.Resize(nResult);
	else
		prBuffer.Resize(0);
G_EXIT_0:
	return nResult;
}

int GSocketStream::TestAlive()
{
	int nRetsult = false;
	int nRetCode = false;
	volatile int nData = false; // no use


	G_ASSERT_EXIT_0(m_nSocket != -1);

	nRetCode = send(m_nSocket, (char *)&nData, 0, 0);
	G_TEST_EXIT_0(nRetCode >= 0);

	nRetsult = true;
G_EXIT_0:
	return nRetsult;
}

int GSocketStream::GetRemoteAddress(in_addr * pRemoteIP, ushort * pusRemotePort)
{
	int nResult = false;

	G_ASSERT_EXIT_0(m_nSocket != -1);

	if (pRemoteIP)
		*pRemoteIP = m_RemoteAddress.sin_addr;

	if (pusRemotePort)
		*pusRemotePort = m_RemoteAddress.sin_port;

	nResult = true;
G_EXIT_0:
	return nResult;
}

// =============================================================================================
// Class GSocketAcceptor begin

class GSocketAcceptor : public GThreadSafeRefRes<IGSocketAcceptor>
{
public:
	GSocketAcceptor();
	~GSocketAcceptor();
	// cszIPAddress: local ip. if cszIPAddress is nullptr(or "") then is bind in INADDR_ANY address
	// when succeed, return true,otherwise return false;
	// ulAddress default is INADDR_ANY
	virtual int Open(const char cszIPAddress[], int nPort); 													  
	virtual int SetTimeout(const GTimeVal& time);
	virtual IGSocketStream *Accept();
	// virtual IGSocketStream *AcceptSecurity(ENCODE_DECODE_MODE EncodeDecodeMode);
	virtual int Close();
private:
	int m_nListenSocket;
	timeval m_cTimeval;
	GSpinLock m_AcceptLock;
};

GSocketAcceptor::GSocketAcceptor(void)
{
	m_nListenSocket = -1;
	m_cTimeval = {-1, -1};
}

GSocketAcceptor::~GSocketAcceptor(void)
{
	if (m_nListenSocket != -1)
	{
		gclose_socket(m_nListenSocket);
		m_nListenSocket = -1;
	}
}

int GSocketAcceptor::Open(const char cszIPAddress[], int nPort)
{
	int nResult = false;
	int nRetCode = false;

#ifdef __GNUC__
	// linux
	{
		typedef void(*SignalHandlerPointer)(int);
		SignalHandlerPointer pSignalHandler = SIG_ERR;

		pSignalHandler = signal(SIGPIPE, SIG_IGN);
		G_ASSERT_EXIT_0(pSignalHandler != SIG_ERR);  // write when connection reset.
	}
#endif

	nRetCode = gcreate_listen_socket(cszIPAddress, nPort, &m_nListenSocket);
	G_ASSERT_EXIT_0(nRetCode);

	nResult = true;

G_EXIT_0:
	if (!nResult)
	{
		if (m_nListenSocket != -1)
		{
			gclose_socket(m_nListenSocket);
			m_nListenSocket = -1;
		}
	}
	return nResult;
}

int GSocketAcceptor::SetTimeout(const GTimeVal& time)
{
	m_cTimeval = _mk_time_val(time);
	return true;
}

int GSocketAcceptor::Close(void)
{
	if (m_nListenSocket != -1)
	{
		gclose_socket(m_nListenSocket);
		m_nListenSocket = -1;
	}
	return true;
}

IGSocketStream *GSocketAcceptor::Accept(void)
{
	int nResult = false;
	int nRetCode = -1;

	IGSocketStream *piResult = nullptr;

	int nClientSocket = -1;
	struct sockaddr_in remoteAddr;
	socklen_t nAddrLen = 0;

	m_AcceptLock.Lock();

	G_ASSERT_EXIT_0(m_nListenSocket != -1);

	while (true)
	{
		nRetCode = gcheck_can_recv(m_nListenSocket, &m_cTimeval);
		if (nRetCode <= 0)
		{
			if (EINTR == G_GET_SOCKET_ERR_CODE())   // if can restore then continue
				continue;

			goto G_EXIT_0;
		}

		nAddrLen = sizeof(sockaddr_in);

		nClientSocket = (int)accept(m_nListenSocket, (struct sockaddr *)&remoteAddr, &nAddrLen);
		if (nClientSocket == -1)
		{
			if (EINTR == G_GET_SOCKET_ERR_CODE())   // if can restore then continue
				continue;

			goto G_EXIT_0;
		}

		break;
	}


	piResult = GSocketStream::Create(nClientSocket, remoteAddr);
	G_ASSERT_EXIT_0(piResult);
	nClientSocket = -1;

	nRetCode = piResult->SetTimeout(GTimeVal{ (int)m_cTimeval.tv_sec, (int)m_cTimeval.tv_usec });
	G_ASSERT_EXIT_0(nRetCode);

	nResult = true;
G_EXIT_0:

	if (!nResult)
	{
		G_SAFE_RELEASE(piResult);

		if (nClientSocket != -1)
		{
			gclose_socket(nClientSocket);
			nClientSocket = -1;
		}
	}

	m_AcceptLock.Unlock();

	return piResult;
}

IGSocketAcceptor * IGSocketAcceptor::CreateSocketAcceptor(ACCEPTOR_TYPE type)
{
	GSocketAcceptor* pAcceptor = GMEM::safe_new<GSocketAcceptor>();
	return pAcceptor;
}
// class GSocketAcceptor end
// ================================================================================================
IGSocketStream * GSocketConnector::Connect(const char * dstIPAddress, int dstPort, const char * localIPAddress, int nlocalPort)
{
	int nResult = false;
	int nRetCode = false;
	IGSocketStream* piResult = nullptr;

	int nSocket = -1;

	sockaddr_in dstAddr, localAddr;

	auto pHost = gethostbyname(dstIPAddress);
	G_ASSERT_EXIT_0(pHost);

	nSocket = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	G_ASSERT_EXIT_0(nSocket != -1);

	gbind_ip_address(&dstAddr, pHost, dstPort);
	gbind_ip_address(&localAddr, localIPAddress, nlocalPort);

	nRetCode = bind(nSocket, (struct sockaddr *)&localAddr, sizeof(sockaddr_in));
	G_ASSERT_EXIT_0(nRetCode != -1);

#ifdef __GNUC__
	// linux
	{
		// 屏蔽SIGPIPE信号。其实不需要每次都做这个操作，但是Connector没有Init函数，所以只能暂时先放这里了。
		typedef void(*SignalHandlerPointer)(int);
		SignalHandlerPointer pSignalHandler = SIG_ERR;

		pSignalHandler = signal(SIGPIPE, SIG_IGN);
		G_ASSERT_EXIT_0(pSignalHandler != SIG_ERR);  // write when connection reset.
	}
#endif

	while (true)
	{
		nRetCode = connect(nSocket, (struct sockaddr *)&dstAddr, sizeof(sockaddr_in));
		if (nRetCode >= 0)
			break;

		nRetCode = (EINTR == G_GET_SOCKET_ERR_CODE());
		G_ASSERT_EXIT_0(nRetCode);
		// if can restore then continue
	}

	piResult = GSocketStream::Create(nSocket, dstAddr);
	G_ASSERT_EXIT_0(piResult);

	nSocket = -1;

	nResult = true;
G_EXIT_0:

	if (!nResult)
	{
		G_SAFE_RELEASE(piResult);

		if (nSocket != -1)
		{
			gclose_socket(nSocket);
			nSocket = -1;
		}
	}

	return piResult;
}
