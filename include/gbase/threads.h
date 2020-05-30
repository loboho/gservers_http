/*------------------------------------------------------------------------------
* FileName    : threads.h
* Author      : lbh
* Create Time : 2018-08-27
* Description : 
* CopyRight   : from gservers_engine @loboho
* ----------------------------------------------------------------------------*/
#ifndef __GBASE__THREADS_H__
#define __GBASE__THREADS_H__

#include <map>
#include <functional>
#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "basedef.h"
#include "memorys.h"

/* ���������������룬����Ƕ�׵��û��������
* ��������������С����ռ��ʱ�䣨���ȣ�С���߳�ͬ�����Լ�����ʹ��TryLock�ĳ���
*/
class GSpinLock
{
public:
	// uSleepMillSecs > 0���߳�ִ��sleepfor���еȴ�����uSleepMillSecsΪ0���߳�ִ��yield
	void Lock(const uint uSleepMillSecs = 0);
	bool TryLock();
	void Unlock();
	GSpinLock() noexcept { m_bIsLocked.store(false, std::memory_order_relaxed); }
protected:
	G_DEF_NONE_COPYABLE(GSpinLock);
	void _LockSleep(uint uSleepMillSecs);
	void _LockYield();
	std::atomic<bool> m_bIsLocked;
};

// �Ŷ������������룬�����ھ�������������̼߳������߳�ͬ��
class GQueueLock
{
public:
	void Lock(const uint uSleepMillSecs = 0);
	void Unlock();
	GQueueLock() noexcept { 
		m_nTicker.store(0, std::memory_order_relaxed);
		m_nChecker.store(0, std::memory_order_relaxed);
	}
	G_DEF_NONE_COPYABLE(GQueueLock);
protected:
	std::atomic<short> m_nTicker;
	std::atomic<short> m_nChecker;
};

// �߳�����ͬһ�߳̿�������
template<typename BaseLockT = GSpinLock>
class GThreadLock
{
public:
	// nCurrentThreadUserId: �û��Զ�����߳�id��uSleepMillSecs��������Ŀ��еȴ�����
	void Lock(int nCurrentThreadUserId, uint uSleepMillSecs = 0);
	void Unlock();
	GThreadLock() = default;
protected:
	G_DEF_NONE_COPYABLE(GThreadLock);
	std::atomic_int m_nCurLockedThreadId = -1; // ��ǰ���Ȩ�޵��߳�id����id�û�����
	int m_nRecursion = 0; // ��ǰ�߳�������
	BaseLockT m_baseLock;
};

// ʹ��GLockGuardҪע�⣬��������
template<typename GLocker>
class GLockGuard
{
public:
	GLockGuard(GLocker& locker, uint uSleepMillSecs = 0) noexcept :
		_locker(locker) {
		_locker.Lock(uSleepMillSecs);
	}
	~GLockGuard() { _locker.Unlock(); }
private:
	G_DEF_NONE_COPYABLE(GLockGuard);
	GLocker& _locker;
};

// safe wrap lock-exec-unlock
struct GLockWrapper final
{
	// the Lock-Exec-Unlock wrapper, CallableT should be a none-param callable type like lambda [...]()->RetType
	template<class GLocker, class CallableT> inline
	static auto Lock_Exec_Unlock(GLocker& locker, CallableT&& doSomthing, uint uSleepMillSecs = 0)
	{
		GLockGuard _lg(locker, uSleepMillSecs); // abide the raii since doDomthing might throw exception
		return doSomthing();
	}
};


/* ˫���л���ķ�ʽ������������дͬ��������1��1�ĵ����д��һ���̶߳���һ���߳�д���߳�ͬ��
	����ڶ�д�����ķ�ʽ�����ᷢ��д�̻߳�ȡ�����ݻ������ȴ�����£����̲߳��ܼ�ʱ��ȡ����д�����ݵ�����
	�����ringbuffer��lock-free queue�ȷ�ʽ�����Ϊ������
*/
class GDualQueueRwSync
{
public:
	enum QueIndex
	{
		FIRST_QUEUE = 0, // ��ʾ��һ�����пɶ� / д
		SECOND_QUEUE = 1, // ��ʾ�ڶ������пɶ� / д
		QUEUE_NODATA = -1, //  ֻ����Reader_Queryʱ���أ���ʾ�����ݵ���
		QUEUE_INCOMING = -2, // ֻ����Reader_Queryʱ���أ���ʾ��������д��
	};
	/* ���̵߳��ã������ȡһ������
		����Reader_Query�ɹ���ȡһ�����к�����һ�ε���Reader_Queryǰ��Writer_Lock���������ö��� */
	QueIndex Reader_Query();
	/* д�̵߳��ã�����һ������д��
		�ú��������������һ��Reader_Query�ҳɹ����صĶ��У�д����ɺ���Ҫ����Writer_Release */
	QueIndex Writer_Lock();
	/* д�̵߳��ã��ͷ�������д���У�������б�ɿɶ� */
	void Writer_Release();
private:
	bool m_readyReadSlot = false;
	bool m_curWriteSlot = false;
	bool m_LastReadSlot = false;
	bool m_LastWriteSlot = false;
	std::atomic_int m_LockedSlot;
};


class GThread : public GThreadSafeRefRes<IGRefRes>
{
public:
	using super = GThreadSafeRefRes<IGRefRes>;
	static GThread* Create();
	// Start Thread
	int Start();
	/* Pause the thread, this action will push an pause-job,
		the thread will not immediately pause until the pause-job being excuted 
		The thread will be blocked until Resume is called
	*/
	void Pause();
	// resume the thread, it's free to be called, not necessary to match Pause Count and Resume Count
	void Resume();
	// Notify the thread to exit, if will not immediately stop until current thread loop is finished
	int Stop();
	/* Release the thread object, while destroy the object it will call Stop() then
		WaitForExit() which will block until the thread finished,
		��Release������������ʱ�߳�δ��������Release�ᷢ�������ȴ��߳̽��� */
	void Release() noexcept { super::Release(); }
	// �����ȴ����߳���ֹ����Ҫ�ȵ���Stop��Terminate��ֹ�̣߳�
	int WaitForExit();
	// ������ֹ�߳�ִ�У���������ֹ������ִ�е�job���ܻᷢ����Դй©���쳣
	int Terminate(uint dwExitCode = 0);
	// Add a job, job must be a bool() callable, if a job callback return false, the job will remove from thread
	int PushJob(void* jobKey, std::function<bool()> job);
	// Remove job, the key for each job must be unique
	int RemoveJob(void* jobKey);
	// ����ÿ����ѭ��Sleep��������Ĭ��Ϊ0
	void SetLoopSleep(uint uMilliseconds) { m_uLoopSleepMs = std::chrono::milliseconds(uMilliseconds); }
	// Check if running
	bool IsRunning() { return m_bContinuous.load(std::memory_order_relaxed); }
	// Check if paused
	bool IsPaused() { return m_bIsPaused.load(std::memory_order_relaxed); }
private:
	G_DEF_NONE_COPYABLE(GThread);
	bool _DoPauseJob();
	GThread() = default;
	~GThread();
	int _ThreadLoop();
#ifdef G_WIN_PLATFORM
	std::atomic<HANDLE>	m_ThreadHandle = 0;
#else
	std::atomic<pthread_t> m_ThreadHandle = 0;
#endif
	std::map<void*, std::function<int()> > m_mpJobs;
	std::map<void*, std::function<int()> > m_mpJobsReady;
	std::atomic_bool m_bContinuous = false;
	std::atomic_bool m_bJobsReady = false;
	std::atomic_bool m_bIsPaused = false;
	GSpinLock m_spJobsReadyLocked;
	std::condition_variable m_conPause;
	std::mutex m_mutexPause;
	std::chrono::milliseconds m_uLoopSleepMs{ 0 };
	friend GThread* GMEM::safe_new<GThread>();
};

inline bool GSpinLock::TryLock()
{
	return !m_bIsLocked.load(std::memory_order_relaxed) && !m_bIsLocked.exchange(true, std::memory_order_acquire);
}

inline void GSpinLock::Unlock()
{
	m_bIsLocked.store(false, std::memory_order_release);
}

inline void GQueueLock::Unlock()
{
	m_nChecker.fetch_add(1, std::memory_order_release);
}

template<typename BaseLockT>
void GThreadLock<BaseLockT>::Lock(int nCurrentThreadUserId, uint uSleepMillSecs)
{
	if (m_nCurLockedThreadId.load(std::memory_order_consume) != nCurrentThreadUserId)
	{
		m_baseLock.Lock(uSleepMillSecs);
		m_nRecursion = 1;
		m_nCurLockedThreadId.store(nCurrentThreadUserId, std::memory_order_relaxed);
	}
	else
	{
		G_ASSERT(-1 != nCurrentThreadUserId); // cannot use -1
		++m_nRecursion;
	}
}

template<typename BaseLockT>
void GThreadLock<BaseLockT>::Unlock()
{
	G_ASSERT_THEN(m_nRecursion > 0)
	{
		if (0 == --m_nRecursion)
		{
			m_nCurLockedThreadId.store(-1, std::memory_order_relaxed);
			m_baseLock.Unlock();
		}
	}
}

/* ���̵߳��ã������ȡһ������
		����Reader_Query�ɹ���ȡһ�����к�����һ�ε���Reader_Queryǰ��Writer_Lock���������ö��� */
inline GDualQueueRwSync::QueIndex GDualQueueRwSync::Reader_Query()
{
	if (m_LastReadSlot == m_LastWriteSlot)
		return QUEUE_NODATA; // no data
	// �л���slot��д�߳���һ��дʱ���Զ��л�дslot������д�߳��հ�Ƶ��ʱ������ռͬһ��slot���¶��̶߳�ȡ������
	m_readyReadSlot = m_LastWriteSlot;
	if (m_LockedSlot.load(std::memory_order_relaxed) == (int)m_readyReadSlot ||
		m_LockedSlot.exchange(m_readyReadSlot, std::memory_order_relaxed) == (int)m_readyReadSlot)
		return QUEUE_INCOMING; // д�̻߳���ռ�ô�slot
	m_LastReadSlot = m_readyReadSlot;
	std::atomic_thread_fence(std::memory_order_acquire);
	return (QueIndex)m_readyReadSlot;
}

/* д�̵߳��ã�����һ������д��
		�ú��������������һ��Reader_Query�ҳɹ����صĶ��У�д����ɺ���Ҫ����Writer_Release */
inline GDualQueueRwSync::QueIndex GDualQueueRwSync::Writer_Lock()
{
	m_curWriteSlot = !m_readyReadSlot;
	while (m_LockedSlot.exchange(m_curWriteSlot, std::memory_order_relaxed) == (int)m_curWriteSlot)
		m_curWriteSlot = !m_curWriteSlot; // switch slot�����ﲻ������߳����ѭ��������һ������л�һ��
	return (QueIndex)m_curWriteSlot;
}

/* д�̵߳��ã��ͷ�������д���У�������б�ɿɶ� */
inline void GDualQueueRwSync::Writer_Release()
{
	m_LockedSlot.store(-1, std::memory_order_release);
	m_LastWriteSlot = m_curWriteSlot;
}

#endif // __GBASE__THREADS_H__
