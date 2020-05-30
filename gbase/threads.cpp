/*------------------------------------------------------------------------------
* FileName    : threads.cpp
* Author      : lbh
* Create Time : 2018-08-27
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#include "pch.h"
#include "threads.h"

inline void GSpinLock::_LockSleep(uint uSleepMillSecs)
{
	auto sleepMs = std::chrono::milliseconds(uSleepMillSecs);
	while (m_bIsLocked.exchange(true, std::memory_order_acquire)) // RMW
	{
		do // inner loop to reduce RMW opt
		{
			_mm_pause();
			if (!m_bIsLocked.load(std::memory_order_relaxed))
				break;
			std::this_thread::sleep_for(sleepMs);
		} while (m_bIsLocked.load(std::memory_order_relaxed));
	}
}

inline void GSpinLock::_LockYield()
{
	while (m_bIsLocked.exchange(true, std::memory_order_acquire)) // RMW
	{
		do // inner loop to reduce RMW opt
		{
			_mm_pause();
			if (!m_bIsLocked.load(std::memory_order_relaxed))
				break;
			std::this_thread::yield();
		} while (m_bIsLocked.load(std::memory_order_relaxed));
	}
}

void GSpinLock::Lock(const uint uSleepMillSecs)
{
	if (uSleepMillSecs == 0)
		_LockYield();
	else
		_LockSleep(uSleepMillSecs);
}

void GQueueLock::Lock(const uint uSleepMillSecs)
{
	auto sleepMs = std::chrono::milliseconds(uSleepMillSecs);
	auto myTick = m_nTicker.fetch_add(1, std::memory_order_acquire);
	while (myTick != m_nChecker.load(std::memory_order_relaxed))
	{
		_mm_pause();
		if (myTick == m_nChecker.load(std::memory_order_relaxed))
			break;
		if (0 == uSleepMillSecs)
			std::this_thread::yield();
		else
			std::this_thread::sleep_for(sleepMs);
	}
}

GThread* GThread::Create()
{
	GThread* pThread = GMEM::safe_new<GThread>();
	return pThread;
}

int GThread::Start()
{
	int nResult = 0;
	int nRetCode = -1;

	G_TEST_EXIT_0(!m_bContinuous);
	while (m_ThreadHandle)
		WaitForExit();

	m_bContinuous = true;
#ifdef G_WIN_PLATFORM
	unsigned uThreadID;
	m_ThreadHandle = (HANDLE)_beginthreadex(
		nullptr,			        // SD
		0,				        // initial stack size
		[](void* p) {
			reinterpret_cast<GThread*>(p)->_ThreadLoop();
			return 0u; },		// thread function
		(void *)this,	        // thread argument
		0,				        // creation option
		(unsigned *)&uThreadID   // thread identifier
		);
	G_ASSERT_EXIT_0(m_ThreadHandle != 0);
#else
	pthread_attr_t   ThreadAttr;
	nRetCode = pthread_attr_init(&ThreadAttr);
	G_ASSERT_EXIT_0(nRetCode == 0);

	nRetCode = pthread_attr_setstacksize(&ThreadAttr, 256 * 1024);
	G_ASSERT_EXIT_0(nRetCode == 0);

	nRetCode = pthread_create(
		(pthread_t*)&m_ThreadHandle,
		&ThreadAttr,
		[](void* p) {
			reinterpret_cast<GThread*>(p)->_ThreadLoop();
			return (void*)(0); },
		this
			);

	if (nRetCode != 0) // if fail
	{
		m_ThreadHandle = 0;
	}

	pthread_attr_destroy(&ThreadAttr);
	G_ASSERT_EXIT_0(nRetCode == 0);
#endif
	nResult = 1;
G_EXIT_0:
	if (nResult != 1)
		m_bContinuous = false;
	return nResult;
}

void GThread::Pause()
{
	if (!m_bIsPaused.exchange(true, std::memory_order_relaxed))
		PushJob(&m_mutexPause, std::bind(&GThread::_DoPauseJob, this));
}

void GThread::Resume()
{
	// remove the pause job now in ready list if exist
	m_spJobsReadyLocked.Lock();
	m_mpJobsReady.erase(&m_mutexPause);
	m_spJobsReadyLocked.Unlock();
	m_conPause.notify_one();
}

bool GThread::_DoPauseJob()
{
	std::unique_lock<std::mutex> ml(m_mutexPause);
	m_conPause.wait(ml);

	m_bIsPaused.store(false, std::memory_order_relaxed);
	return 0;
}

int GThread::_ThreadLoop()
{
	while (m_bContinuous.load(std::memory_order_relaxed))
	{
		for (auto it = m_mpJobs.begin(); it != m_mpJobs.end();)
		{
			if (!it->second())
				it = m_mpJobs.erase(it);
			else
				++it;
		}
		if (m_bJobsReady.load(std::memory_order_relaxed))
		{
			m_spJobsReadyLocked.Lock();

			// handle the pushing or removing jobs
			for (auto job : m_mpJobsReady)
			{
				if (job.second != nullptr)
					m_mpJobs.insert(job);
				else
					m_mpJobs.erase(job.first);
			}

			m_bJobsReady.store(false, std::memory_order_relaxed);
			m_spJobsReadyLocked.Unlock();
		}

		std::this_thread::sleep_for(m_uLoopSleepMs);
	}
	G_ASSERT(m_ThreadHandle);
#ifdef G_WIN_PLATFORM
	CloseHandle(m_ThreadHandle);
#endif
	m_ThreadHandle = 0;
	return 1;
}

int GThread::PushJob(void * jobKey, std::function<bool()> job)
{
	if (m_ThreadHandle == 0) // not running
	{
		bool succeed = m_mpJobs.insert({ jobKey, job }).second;
		G_ASSERT(succeed);
		return succeed;
	}
	m_spJobsReadyLocked.Lock();
	m_mpJobsReady[jobKey] = job;
	m_bJobsReady.store(true, std::memory_order_relaxed);
	m_spJobsReadyLocked.Unlock();
	return 1;
}

int GThread::RemoveJob(void * jobKey)
{
	if (m_ThreadHandle == 0) // not running
	{
		bool succeed = (m_mpJobs.erase(jobKey) == 1);
		G_ASSERT(succeed);
		return succeed;
	}
	m_spJobsReadyLocked.Lock();
	m_mpJobsReady[jobKey] = nullptr;
	m_bJobsReady.store(true, std::memory_order_relaxed);
	m_spJobsReadyLocked.Unlock();
	return 1;
}

int GThread::Stop()
{
	m_bContinuous = false;
	return 1;
}

int GThread::WaitForExit()
{
	int nResult = 0;
	m_bContinuous = false;
#ifdef G_WIN_PLATFORM
	if (m_ThreadHandle)
		WaitForSingleObject(m_ThreadHandle, INFINITE);	
#else
	if (m_ThreadHandle)
		pthread_join(m_ThreadHandle, nullptr);
#endif
	nResult = 1;
	return nResult;
}

int GThread::Terminate(uint dwExitCode)
{
	int nResult = 0;

	G_ASSERT_EXIT_0(m_ThreadHandle);

#ifdef G_WIN_PLATFORM
	if (m_ThreadHandle)
	{
		TerminateThread(m_ThreadHandle, dwExitCode);
		m_ThreadHandle = 0;
	}
#else
	if (m_ThreadHandle)
	{
		pthread_cancel(m_ThreadHandle);
		m_ThreadHandle = 0;
	}
#endif

	nResult = 1;
G_EXIT_0:
	return nResult;
}

GThread::~GThread()
{
	Stop();
	WaitForExit(); // 阻塞等待线程退出
}

