/*------------------------------------------------------------------------------
* FileName    : httpquery.cpp
* Author      : lbh
* Create Time : 2018-09-04
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#include "pch.h"
#include <cctype>
#include <sstream>
#include "gbase/httpquery.h"

using std::string;
using std::stringstream;
//application/octet-stream

GHttpSender::tCheckHttpResult GHttpSender::HttpPost(const char* hostname, int nPort,
	const char* pathname, const char * content, uint uContentSize)
{
	bool bRet;
	stringstream sstream;
	GHttpConnector* piConnector = GMEM::safe_new<GHttpConnector>();
	G_ASSERT_EXIT_0(piConnector);
	bRet = piConnector->Connect(hostname, nPort);
	G_ASSERT_EXIT_0(bRet);
	sstream << "POST " << pathname << " HTTP/1.1\r\n";
	sstream << "Host:" << hostname << ':' << nPort << "\r\n";
	sstream << "Accept: */*\r\n"
		"User-Agent: GServers/1.0\r\n"
		"Content-Type:application/x-www-form-urlencoded\r\n"
		"Content-Length: " << uContentSize << "\r\n"
		"Connection: close\r\n\r\n";
	bRet = piConnector->Send(sstream.str().c_str(), sstream.str().size());
	GLOG_TRACE("[Send Http Post]", sstream.str().c_str());
	G_ASSERT_EXIT_0(bRet)

	bRet = piConnector->Send(content, uContentSize);
	//bRet = piConnector->Send(END_BOUNDARY, sizeof(END_BOUNDARY) -1 );
	GLOG_TRACE("[Send Http Post]", content);//, END_BOUNDARY);
	G_ASSERT_EXIT_0(bRet)
	//G_ASSERT_EXIT_0(bRet)
	//auto piaa = piConnector->CheckAndRecv();
	//G_ASSERT_EXIT_0(piaa);
	if (!m_piThread)
	{
		m_piThread = GThread::Create();
		m_piThread->SetLoopSleep(5);
		m_piThread->PushJob(this, std::bind(&GHttpSender::ThreadLoop, this));
	}
	m_spinLock.Lock();
	m_setConnectReceiving.insert(piConnector);
	if (!m_piThread->IsRunning())
		m_piThread->Start();
	if (m_piThread->IsPaused())
		m_piThread->Resume();
	m_spinLock.Unlock();
	return [this, piConnector](tResultVector& vecRecv) { return CheckData(piConnector, vecRecv); };
G_EXIT_0:
	if (piConnector)
		GMEM::safe_delete(piConnector);
	return nullptr;
}

GHttpSender::tCheckHttpResult GHttpSender::HttpGet(const char * hostname, int nPort,
	const char * pathname, const char *param)
{
	bool bRet;
	stringstream header;
	GHttpConnector* piConnector = GMEM::safe_new<GHttpConnector>();
	G_ASSERT_EXIT_0(piConnector);
	bRet = piConnector->Connect(hostname, nPort);
	G_ASSERT_EXIT_0(bRet);

	header << "GET " << pathname << '?' << param << " HTTP/1.1\r\n";
	header << "Host: " << hostname << "\r\n";
	header << "Accept: */*\r\n";
	header << "User-Agent: GServers-WJ/1.0\r\n";
	header << "Connection: Keep-Alive\r\n";
	header << "\r\n";
	bRet = piConnector->Send(header.str().c_str(), header.str().size());
	GLOG_TRACE("[Send Http Get]", header.str());
	G_ASSERT_EXIT_0(bRet);
	if (!m_piThread)
	{
		m_piThread = GThread::Create();
		m_piThread->SetLoopSleep(5);
		m_piThread->PushJob(this, std::bind(&GHttpSender::ThreadLoop, this));
	}
	m_spinLock.Lock();
	m_setConnectReceiving.insert(piConnector);
	if (!m_piThread->IsRunning())
		m_piThread->Start();
	if (m_piThread->IsPaused())
		m_piThread->Resume();
	m_spinLock.Unlock();
	return [this, piConnector](tResultVector& vecRecv) { return CheckData(piConnector, vecRecv); };
G_EXIT_0:
	if (piConnector)
		GMEM::safe_delete(piConnector);
	return nullptr;
}

bool GHttpSender::CheckData(GHttpConnector* pConnector, tResultVector& vecRecv)
{
	{
		GLockGuard _lg(m_spinLock);
		bool bFound = (m_setConnectReceived.find(pConnector) != m_setConnectReceived.end());
		if (!bFound)
			return false;

		vecRecv = pConnector->m_vecPackets;
		m_setConnectReceived.erase(pConnector);
	}
	GMEM::safe_delete(pConnector);
	return true;
}

bool GHttpSender::ThreadLoop()
{
	std::vector<GHttpConnector*> vecConn;
	{
		GLockGuard _lg(m_spinLock);
		if (m_setConnectReceiving.size() == 0)
		{
			m_piThread->Pause();
			return true;
		}
		vecConn.assign(m_setConnectReceiving.begin(), m_setConnectReceiving.end());
	}
	for (auto conn : vecConn)
	{
		GRefBuffer* piBuffer;
		int nRet = conn->CheckAndRecv(&piBuffer);
		if (nRet != 0)
		{
			GLockGuard _lg(m_spinLock);
			// Only recv once
			m_setConnectReceiving.erase(conn);
			m_setConnectReceived.insert(conn);
			if (nRet > 0)
			{
				conn->m_vecPackets.emplace_back(piBuffer);
			}
			else // disconnect
			{
				GLOG_WARN("Http get response failed, ret code", nRet);
			}
		}
	}
	return true;
}

namespace {
	constexpr char cszHttpAdrHead[] = "http://";
	constexpr char cszHttpsAdrHead[] = "https://";
};

bool GHttpSender::HttpAddressExtract(const char * address, std::string & host, int & port, std::string & path, bool & bHttps)
{
	G_ASSERT_RET_CODE(address, false);
	char szAddressBuf[1024];
	uint i;
	for (i = 0; i < sizeof(szAddressBuf) - 2; i++)
	{
		char c = *address;
		if (!c)
			break;
		while (isspace(c))
			address++;
		szAddressBuf[i] = c;
	}
	szAddressBuf[i] = '\0';
	int copyStart = 0, copyEnd = 0;
	if (memcmp(szAddressBuf, cszHttpAdrHead, sizeof(cszHttpAdrHead) - 1) == 0)
	{
		bHttps = false;
		copyStart = sizeof(cszHttpAdrHead) - 1;
	}
	else if (memcmp(szAddressBuf, cszHttpsAdrHead, sizeof(cszHttpsAdrHead) - 1) == 0)
	{
		bHttps = true;
		copyStart = sizeof(cszHttpsAdrHead) - 1;
	}
	else
		bHttps = false;

	auto findEnd = [szAddressBuf](int nFindStart) {
		while (szAddressBuf[nFindStart] != '\0' && szAddressBuf[nFindStart] != ':'&&
			szAddressBuf[nFindStart] != '/' && szAddressBuf[nFindStart] != '\\') {
			++nFindStart;
		}
		return nFindStart;
	};
	auto copy2Str = [szAddressBuf](int nCopyStart, int nCopyEnd) mutable {
		char tmp = szAddressBuf[nCopyEnd];
		szAddressBuf[nCopyEnd] = '\0';
		string str = &szAddressBuf[nCopyStart];
		szAddressBuf[nCopyEnd] = tmp;
		return str;
	};
	copyEnd = findEnd(copyStart);
	host = copy2Str(copyStart, copyEnd);
	copyStart = copyEnd + 1;
	if (szAddressBuf[copyEnd] == ':')
	{
		copyEnd = findEnd(copyStart);
		string strPort = copy2Str(copyStart, copyEnd);
		port = atoi(strPort.c_str());
		copyStart = copyEnd + 1;
	}
	path = szAddressBuf[copyStart];
	return false;
}


bool GHttpSender::GHttpConnector::Send(const char * pData, uint uDataSize)
{
	G_ASSERT_RET_CODE(m_piSocketStream, false);
	GRefBuffer::UPtr ptr(GRefBuffer::Create(uDataSize));
	memcpy(ptr->GetData(), pData, uDataSize);
	return m_piSocketStream->Send(*ptr) > 0;
}

GHttpSender::~GHttpSender()
{
	G_SAFE_RELEASE(m_piThread);
}

constexpr auto HTTP_RECV_BUF_SIZE = 1024;
int GHttpSender::GHttpConnector::CheckAndRecv(GRefBuffer** piBuf)
{
	time_t tCurTime = time(nullptr);
	if (tCurTime > m_tStartTime + MAX_RECV_TIME) // timeout
	{
		GLOG_WARN("Get http reponse timeout!");
		return -2;
	}
	GResizableBuffer* piBuffer = GResizableBuffer::Create(HTTP_RECV_BUF_SIZE, HTTP_RECV_BUF_SIZE);
	int nBytes = m_piSocketStream->Recv(*piBuffer);
	if (nBytes <= 0)
	{
		piBuffer->Release();
		return nBytes;
	}
	while (nBytes == HTTP_RECV_BUF_SIZE)
	{
		GResizableBuffer* piCirRead = GResizableBuffer::Create(HTTP_RECV_BUF_SIZE, HTTP_RECV_BUF_SIZE);
		nBytes = m_piSocketStream->Recv(*piCirRead);
		if (nBytes > 0)
		{
			auto newSize = nBytes + piBuffer->GetSize();
			auto piNewBuf = GResizableBuffer::Create(newSize, newSize);
			memcpy(piNewBuf->GetData(), piBuffer->GetData(), piBuffer->GetSize());
			memcpy((char*)piNewBuf->GetData() + piBuffer->GetSize(), piCirRead->GetData(), piCirRead->GetSize());
			piBuffer->Release();
			piCirRead->Release();
			piBuffer = piNewBuf;
		}
		else if (nBytes < 0)
		{
			piBuffer->Release();
			return nBytes;
		}
	};
	*piBuf = piBuffer;
	return piBuffer->GetSize();
}

bool GHttpSender::GHttpConnector::Connect(
	const char* detIPAddress, int dstPort,
	const char* localIPAddress, int nlocalPort)
{
	m_piSocketStream = m_cConnector.Connect(detIPAddress, dstPort, localIPAddress, nlocalPort);
	if (m_piSocketStream)
		m_piSocketStream->SetTimeout({0, 10000});
	return m_piSocketStream != nullptr;
}

GHttpSender::GHttpConnector::GHttpConnector()
{
	m_tStartTime = time(nullptr);
}

GHttpSender::GHttpConnector::~GHttpConnector()
{
	G_SAFE_RELEASE(m_piSocketStream);
}