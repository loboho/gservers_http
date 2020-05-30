/*------------------------------------------------------------------------------
* FileName    : socketapi.cpp
* Author      : lbh
* Create Time : 2018-08-26
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#include "pch.h"
#include "gbase/socketapi.h"

namespace GSOCKET_API
{
	int gclose_socket(int nSocket)
	{
		struct linger lingerStruct;

		lingerStruct.l_onoff = 1;
		lingerStruct.l_linger = 0;

		setsockopt(
			nSocket,
			SOL_SOCKET, SO_LINGER,
			(char *)&lingerStruct, sizeof(lingerStruct)
		);
#ifdef G_WIN_PLATFORM
		return closesocket(nSocket);
#elif defined(__GNUC__) //linux
		return close(nSocket);
#endif
	}

	int gcreate_listen_socket(cchar cszIPAddress[], int nPort, int *pnRetListenSocket, int nBacklog)
	{
		int nResult = false;
		int nRetCode = false;
		int nOne = 1;
		unsigned long ulAddress = INADDR_ANY;
		int nListenSocket = -1;

		sockaddr_in LocalAddr;

		G_ASSERT_EXIT_0(pnRetListenSocket);

		nListenSocket = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		G_ASSERT_EXIT_0(nListenSocket != -1);

		if (
			(cszIPAddress) &&
			(cszIPAddress[0] != '\0')
			)
		{
			ulAddress = inet_addr(cszIPAddress);
			if (ulAddress == INADDR_NONE)
				ulAddress = INADDR_ANY;
		}

		nRetCode = setsockopt(nListenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&nOne, sizeof(int));
		G_ASSERT_EXIT_0(nRetCode >= 0);


		LocalAddr.sin_family = AF_INET;
		LocalAddr.sin_addr.s_addr = ulAddress;    //not need to htonl
		LocalAddr.sin_port = htons(nPort);

		nRetCode = bind(nListenSocket, (struct sockaddr *)&LocalAddr, sizeof(LocalAddr));
		G_ASSERT_EXIT_0(nRetCode != -1);

		nRetCode = listen(nListenSocket, nBacklog);
		G_ASSERT_EXIT_0(nRetCode >= 0);

		nResult = true;

	G_EXIT_0:
		if (!nResult)
		{
			if (nListenSocket != -1)
			{
				gclose_socket(nListenSocket);
				nListenSocket = -1;
			}
			nRetCode = G_GET_SOCKET_ERR_CODE();
			GLOG_ERROR("Listen Error: ", nRetCode, cszIPAddress, nPort);
		}

		if (pnRetListenSocket)
		{
			*pnRetListenSocket = nListenSocket;
		}
		return nResult;
	}

	// return -1: error, 0: timeout, 1: success
	int gcheck_can_recv(int nSocket, const timeval *pcTimeout)
	{
		if (nSocket < 0)
			return -1;

		fd_set FDSet;
		FD_ZERO(&FDSet);
		FD_SET(nSocket, &FDSet);

		timeval  TempTimeout;
		timeval *pTempTimeout = nullptr;

		if (pcTimeout)
		{
			TempTimeout = *pcTimeout;
			pTempTimeout = &TempTimeout;
		}

		int nRetCode = select(nSocket + 1, &FDSet, nullptr, nullptr, pTempTimeout);

		if (nRetCode == 0)
			return 0;

		if (nRetCode > 0)
			return 1;

		nRetCode = G_GET_SOCKET_ERR_CODE();
		GLOG_ERROR("Socket Error", nSocket, nRetCode);

		return -1;
	}

	// return -1: error, 0: timeout, 1: success
	int gcheck_can_send(int nSocket, const timeval *pcTimeout)
	{
		if (nSocket < 0)
			return -1;

		fd_set FDSet;
		FD_ZERO(&FDSet);
		FD_SET(nSocket, &FDSet);

		timeval  TempTimeout;
		timeval *pTempTimeout = nullptr;

		if (pcTimeout)
		{
			TempTimeout = *pcTimeout;
			pTempTimeout = &TempTimeout;
		}

		int nRetCode = select(nSocket + 1, nullptr, &FDSet, nullptr, pTempTimeout);

		if (nRetCode == 0)
			return 0;

		if (nRetCode > 0)
			return 1;

		nRetCode = G_GET_SOCKET_ERR_CODE();
		GLOG_ERROR("Socket Error", nSocket, nRetCode);

		return -1;
	}

	// if nSendFlag == false, then RecvBuffer
	// if nSendFlag == true,  then SendBuffer   00002746 
	// return -1: error, 0: timeout, >0: data size
	int gsend_or_recv_buffer(
		const int nSendFlag,
		int nSocket,
		char *pbyBuffer, int nSize,
		const timeval &ProcessTimeout
	)
	{
		int nResult = -1;      // error
		int nCopySize = 0;

		const timeval *pcTimeout = nullptr;
		if (ProcessTimeout.tv_sec != -1)
			pcTimeout = &ProcessTimeout;

		G_ASSERT_EXIT_0(nSocket != -1);
		G_ASSERT_EXIT_0(pbyBuffer);
		G_ASSERT_EXIT_0(nSize > 0);

		while (nCopySize < nSize)
		{
			if (nSendFlag)
			{
				//when timeout disabled,pTimeout was set to nullptr.
				nResult = gcheck_can_send(nSocket, pcTimeout);
				G_TEST_EXIT_0(nResult != 0);
				if (nResult < 0)  //error
				{
					if (EINTR == G_GET_SOCKET_ERR_CODE())   // if can restore then continue
						continue;

					goto G_EXIT_0;
				}
				nResult = send(nSocket, pbyBuffer, nSize, 0);
			}
			else    // recv
			{
				//when timeout disabled,pTimeout was set to nullptr.
				nResult = gcheck_can_recv(nSocket, pcTimeout);
				G_TEST_EXIT_0(nResult != 0)
				if (nResult < 0)   // error
				{
					if (EINTR == G_GET_SOCKET_ERR_CODE())   // if can restore then continue
						continue;

					goto G_EXIT_0;
				}
				nResult = recv(nSocket, (char *)pbyBuffer, nSize, 0);
			}

			if (nResult == 0)  // Disconnected
			{
				nResult = -2;
				goto G_EXIT_0;
			}

			if (nResult < 0)   // Error!
			{
				if (EINTR == G_GET_SOCKET_ERR_CODE())   // if can restore then continue
					continue;

				int nErrCode = G_GET_SOCKET_ERR_CODE();
				GLOG_WARN("Socket Send or Recive Error", nSocket, nSendFlag, nErrCode);
				goto G_EXIT_0;
			}

			pbyBuffer += nResult;
			nCopySize += nResult;
			if (!nSendFlag) // recv once
				break;
		}

		nResult = nCopySize;    // success
	G_EXIT_0:
		return nResult;
	}

	void gbind_ip_address(sockaddr_in* addr, const char * cszIPAddr, int nPort)
	{
		*addr = { 0 };
		ulong ulAddress = INADDR_ANY;
		if ((cszIPAddr) && (cszIPAddr[0] != '\0'))
		{
			ulAddress = inet_addr(cszIPAddr);
			if (ulAddress == INADDR_NONE)
				ulAddress = INADDR_ANY;
		}
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr = ulAddress;
		addr->sin_port = htons(nPort);
	}

	void gbind_ip_address(sockaddr_in * addr, hostent* pAddr, int nPort)
	{
		*addr = { 0 };
		ulong ulAddress = INADDR_ANY;
		if (pAddr)
		{
			ulAddress = *(uint*)pAddr->h_addr_list[0];
			if (ulAddress == INADDR_NONE)
				ulAddress = INADDR_ANY;
		}
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr = ulAddress;
		addr->sin_port = htons(nPort);
	}

	int gset_nonblock(int nSocket)
	{
		int nRetCode = -1;
#ifdef G_WIN_PLATFORM
		unsigned long ul = 1;
		nRetCode = ioctlsocket(nSocket, FIONBIO, &ul);
#else		
		int opts = fcntl(nSocket, F_GETFL);
		if (opts != -1)
			nRetCode = fcntl(nSocket, F_SETFL, opts | O_NONBLOCK);
#endif // G_WIN_PLATFORM

		if (nRetCode < 0)
			GLOG_ERROR("gset_nonblock error:", G_GET_SOCKET_ERR_CODE());
		return nRetCode;
	}
}