/*------------------------------------------------------------------------------
* FileName    : socketapi.h
* Author      : lbh
* Create Time : 2018-08-26
* Description : the socket api wrapper
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#ifndef __GBASE__SOCKETAPI_H__
#define __GBASE__SOCKETAPI_H__

#ifndef G_WIN_PLATFORM
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <arpa/inet.h>

#define G_GET_SOCKET_ERR_CODE() errno
#define G_SOCKET_ERR_WBLOCK EWOULDBLOCK
#define G_SOCKET_ERR_AGAIN EAGAIN
#else

#include <winsock2.h>
#include <WS2tcpip.h>

#define G_GET_SOCKET_ERR_CODE() WSAGetLastError()
#define G_SOCKET_ERR_WBLOCK WSAEWOULDBLOCK
#define G_SOCKET_ERR_AGAIN WSA_IO_PENDING
#define SHUT_RDWR SD_BOTH
typedef int socklen_t;
#endif

inline bool G_SOCKET_IS_BLOCK(int errCode) {
	return errCode == G_SOCKET_ERR_WBLOCK || errCode == G_SOCKET_ERR_AGAIN;
};

struct GNetN2P
{
	GNetN2P(const void* pAddr, int family = AF_INET) {
		inet_ntop(family, pAddr, ipBuf, sizeof(ipBuf));
	}
	const char* str() { return ipBuf; }
private:
	char ipBuf[20];
};

namespace GSOCKET_API
{
	int gclose_socket(int nSocket);
	// return 1:success, 0:error
	int gcreate_listen_socket(cchar cszIPAddress[], int nPort, int *pnRetListenSocket, int nBacklog = SOMAXCONN);
	// return -1: error, 0: timeout, 1: success
	int gcheck_can_recv(int nSocket, const timeval *pcTimeout);
	// return -1: error, 0: timeout, 1: success
	int gcheck_can_send(int nSocket, const timeval *pcTimeout);
	// if nSendFlag == false, then RecvBuffer
	// if nSendFlag == true,  then SendBuffer   00002746 
	// return -1: error, -2:disconnected, 0: timeout, >0: datasize
	int gsend_or_recv_buffer(
		const int nSendFlag,
		int nSocket,
		char *pbyBuffer, int nSize,
		const timeval &ProcessTimeout
	);
	// bind and ipaddress, if cazIPAddr set to nullptr, INADDR_ANY will be binded
	void gbind_ip_address(sockaddr_in* addr, const char* cszIPAddr, int nPort);
	void gbind_ip_address(sockaddr_in* addr, hostent* pAddr, int nPort);
	// set the socket to non blocking
	int gset_nonblock(int nSocket);
}

#endif // __GBASE__SOCKETAPI_H__
