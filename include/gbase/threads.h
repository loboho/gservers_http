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

/* 自旋锁，不可重入，所有嵌套调用会造成死锁
* 适用于锁竞争较小，锁占用时间（粒度）小的线程同步，以及可以使用TryLock的场合
*/
class GSpinLock
{
public:
	// uSleepMillSecs > 0则线程执行sleepfor空闲等待，若uSleepMillSecs为0，线程执行yield
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

// 排队锁，不可重入，适用于竞争激烈需避免线程饥饿的线程同步
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

// 线程锁，同一线程可以重入
template<typename BaseLockT = GSpinLock>
class GThreadLock
{
public:
	// nCurrentThreadUserId: 用户自定义的线程id，uSleepMillSecs竞锁间隔的空闲等待毫秒
	void Lock(int nCurrentThreadUserId, uint uSleepMillSecs = 0);
	void Unlock();
	GThreadLock() = default;
protected:
	G_DEF_NONE_COPYABLE(GThreadLock);
	std::atomic_int m_nCurLockedThreadId = -1; // 当前获得权限的线程id，该id用户定义
	int m_nRecursion = 0; // 当前线程重入数
	BaseLockT m_baseLock;
};

// 使用GLockGuard要注意，锁的粒度
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


/* 双队列缓冲的方式进行无阻塞读写同步，用于1对1的单向读写（一个线程读，一个线程写）线程同步
	相比于读写计数的方式，不会发生写线程获取新数据会阻塞等待情况下，读线程不能及时读取到已写入数据的问题
	相比于ringbuffer，lock-free queue等方式，则更为简便灵活
*/
class GDualQueueRwSync
{
public:
	enum QueIndex
	{
		FIRST_QUEUE = 0, // 表示第一个队列可读 / 写
		SECOND_QUEUE = 1, // 表示第二个队列可读 / 写
		QUEUE_NODATA = -1, //  只会在Reader_Query时返回，表示无数据到达
		QUEUE_INCOMING = -2, // 只会在Reader_Query时返回，表示数据正在写入
	};
	/* 读线程调用，申请读取一个队列
		调用Reader_Query成功获取一个队列后，在下一次调用Reader_Query前，Writer_Lock不会锁定该队列 */
	QueIndex Reader_Query();
	/* 写线程调用，锁定一个队列写入
		该函数不会锁定最近一次Reader_Query且成功返回的队列，写入完成后需要调用Writer_Release */
	QueIndex Writer_Lock();
	/* 写线程调用，释放锁定的写队列，且令队列变成可读 */
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
		若Release触发对象销毁时线程未结束，则Release会发生阻塞等待线程结束 */
	void Release() noexcept { super::Release(); }
	// 阻塞等待到线程中止（需要先调用Stop或Terminate中止线程）
	int WaitForExit();
	// 暴力中止线程执行，若暴力中止，正在执行的job可能会发生资源泄漏等异常
	int Terminate(uint dwExitCode = 0);
	// Add a job, job must be a bool() callable, if a job callback return false, the job will remove from thread
	int PushJob(void* jobKey, std::function<bool()> job);
	// Remove job, the key for each job must be unique
	int RemoveJob(void* jobKey);
	// 设置每工作循环Sleep毫秒数，默认为0
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

/* 读线程调用，申请读取一个队列
		调用Reader_Query成功获取一个队列后，在下一次调用Reader_Query前，Writer_Lock不会锁定该队列 */
inline GDualQueueRwSync::QueIndex GDualQueueRwSync::Reader_Query()
{
	if (m_LastReadSlot == m_LastWriteSlot)
		return QUEUE_NODATA; // no data
	// 切换读slot，写线程下一次写时会自动切换写slot（避免写线程收包频繁时反复抢占同一个slot导致读线程读取不到）
	m_readyReadSlot = m_LastWriteSlot;
	if (m_LockedSlot.load(std::memory_order_relaxed) == (int)m_readyReadSlot ||
		m_LockedSlot.exchange(m_readyReadSlot, std::memory_order_relaxed) == (int)m_readyReadSlot)
		return QUEUE_INCOMING; // 写线程还在占用此slot
	m_LastReadSlot = m_readyReadSlot;
	std::atomic_thread_fence(std::memory_order_acquire);
	return (QueIndex)m_readyReadSlot;
}

/* 写线程调用，锁定一个队列写入
		该函数不会锁定最近一次Reader_Query且成功返回的队列，写入完成后需要调用Writer_Release */
inline GDualQueueRwSync::QueIndex GDualQueueRwSync::Writer_Lock()
{
	m_curWriteSlot = !m_readyReadSlot;
	while (m_LockedSlot.exchange(m_curWriteSlot, std::memory_order_relaxed) == (int)m_curWriteSlot)
		m_curWriteSlot = !m_curWriteSlot; // switch slot，这里不会与读线程造成循环竞争，一般最多切换一次
	return (QueIndex)m_curWriteSlot;
}

/* 写线程调用，释放锁定的写队列，且令队列变成可读 */
inline void GDualQueueRwSync::Writer_Release()
{
	m_LockedSlot.store(-1, std::memory_order_release);
	m_LastWriteSlot = m_curWriteSlot;
}

#endif // __GBASE__THREADS_H__
