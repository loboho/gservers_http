/*------------------------------------------------------------------------------
* FileName    : httpquery.h
* Author      : lbh
* Create Time : 2018-09-04
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#ifndef __GBASE__HTTPQUERY_H__
#define __GBASE__HTTPQUERY_H__

#include <string>
#include <functional>
#include <set>
#include <vector>
#include "gbase/threads.h"
#include "gbase/socketbase.h"
#include "gbase/memorys.h"

class GHttpSender
{
private:
	class GHttpConnector;
public:
	typedef std::vector<GRefBuffer::SPtr> tResultVector;
	typedef std::function<bool (tResultVector& vecRecv)> tCheckHttpResult;

	tCheckHttpResult HttpPost(const char* hostname, int nPort, const char* pathname,
		const char* content, uint uContentSize);

	tCheckHttpResult HttpGet(const char* hostname, int nPort, const char* pathname,	const char* param);

	static bool HttpAddressExtract(const char* address, std::string& host, int& port,
		std::string& path, bool& bHttps);

	~GHttpSender();
private:
	//int HttpPost(const char* hostname, const char* parameters, std::string& message);
	bool CheckData(GHttpConnector* pConnector, tResultVector& vecRecv);
	bool ThreadLoop();
	std::set<GHttpConnector*> m_setConnectReceiving;
	std::set<GHttpConnector*> m_setConnectReceived;
	GThread* m_piThread = nullptr;
	GSocketConnector m_cConnector;
	GSpinLock m_spinLock;

	class GHttpConnector
	{
	public:
		enum
		{
			MAX_RECV_TIME = 10
		};
		GHttpConnector();
		~GHttpConnector();
		bool Connect(
			const char* detIPAddress, int dstPort,
			const char* localIPAddress = nullptr, int nlocalPort = 0);
		bool Send(const char* pData, uint uDataSize);
		// check and recv data
		int CheckAndRecv(GRefBuffer** piBuf);
	public:
		GSocketConnector m_cConnector;
		IGSocketStream* m_piSocketStream = nullptr;
		tResultVector m_vecPackets;
		time_t m_tStartTime;
	};
};

#endif // __GBASE__HTTPQUERY_H__
