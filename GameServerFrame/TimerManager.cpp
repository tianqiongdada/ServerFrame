#include "TimerManager.h"
std::map<TimeEnum, std::vector<stTimingTaskInfo>> g_mapTimingTasks;

TimerManager::TimerManager(IEpoller* pEpoller)
{
    m_pEpoller = pEpoller;
	g_mapTimingTasks[Time_100_MS] = std::vector<stTimingTaskInfo>{};
	g_mapTimingTasks[Time_1_Sec] = std::vector<stTimingTaskInfo>{};
	g_mapTimingTasks[Time_5_Sec] = std::vector<stTimingTaskInfo>{};
	g_mapTimingTasks[Time_30_Sec] = std::vector<stTimingTaskInfo>{};
	g_mapTimingTasks[Time_1_Min] = std::vector<stTimingTaskInfo>{};
}

TimerManager::~TimerManager()
{

}

int TimerManager::_CreateTimerObj(uint uiSec, uint uiNsec)
{
    if (uiSec <= 0)
    {
        if (uiNsec <= 0)
            return -1;
    }

    int iRet = 0;
    int tfd = 0;
    struct itimerspec timeValue;

    //初始化定时器
    /*
     When the file descriptor is no longer required it should be closed.
     When all file descriptors associated with the same timer object have been closed,
     the timer is disarmed and its resources are freed by the kernel.

     意思是该文件句柄还是需要调用close函数关闭的
    */
    tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (tfd < 0)
    {
        LOG_ERROR << "定时器对象创建失败";
        return -1;
    }

    //设置开启定时器
    /*
    Setting either field of new_value.it_value to a nonzero value arms the timer.
    Setting both fields of new_value.it_value to zero disarms the timer.
    意思是如果不设置it_interval的值非零，那么即关闭定时器
    */
    //第一次调用的延时时间
    timeValue.it_value.tv_sec = (time_t)uiSec;
    timeValue.it_value.tv_nsec = (long)uiNsec;

    //设置定时器周期
    timeValue.it_interval.tv_sec = (time_t)uiSec;
    timeValue.it_interval.tv_nsec = (long)uiNsec;

    iRet = timerfd_settime(tfd, 0, &timeValue, NULL);
    if (iRet < 0)
    {
		LOG_ERROR << "定时器对象设置失败";
        return -1;
    }

    return tfd;
}

int TimerManager::_AddTimerObjToEpoll(int tFd, stTimerParam* pParam)
{
    if (!m_pEpoller)
        return -1;

    return m_pEpoller->add(tFd, pParam, EPOLLIN | EPOLLERR | EPOLLHUP);
}

void TimerManager::Timer100mSec(int tFd)
{
	uint64_t exp = 0;
	int ret = read(tFd, &exp, sizeof(uint64_t));
	if (ret >= 0)
	{
        std::vector<stTimingTaskInfo>& info = g_mapTimingTasks[Time_100_MS];
        for (auto t_task : info)
        {
            t_task.task();
		}
	}
}

void TimerManager::Timer1Sec(int tFd)
{
	uint64_t exp = 0;
	int ret = read(tFd, &exp, sizeof(uint64_t));
    if (ret >= 0)
    {
		std::vector<stTimingTaskInfo>& info = g_mapTimingTasks[Time_1_Sec];
		for (auto t_task : info)
		{
			t_task.task();
		}
    }

}

void TimerManager::Timer5Sec(int tFd)
{
	uint64_t exp = 0;
	int ret = read(tFd, &exp, sizeof(uint64_t));
	if (ret >= 0)
	{
		std::vector<stTimingTaskInfo>& info = g_mapTimingTasks[Time_5_Sec];
		for (auto t_task : info)
		{
			t_task.task();
		}
        //pUserManager->KeepLife();
	}
}

void TimerManager::Timer30Sec(int tFd)
{
	uint64_t exp = 0;
	int ret = read(tFd, &exp, sizeof(uint64_t));
	if (ret >= 0)
	{
		std::vector<stTimingTaskInfo>& info = g_mapTimingTasks[Time_30_Sec];
		for (auto t_task : info)
		{
			t_task.task();
		}
	}
}

void TimerManager::Timer60Sec(int tFd)
{
	uint64_t exp = 0;
	int ret = read(tFd, &exp, sizeof(uint64_t));
	if (ret >= 0)
	{
		std::vector<stTimingTaskInfo>& info = g_mapTimingTasks[Time_1_Min];
		for (auto t_task : info)
		{
			t_task.task();
		}
	}
}

int TimerManager::CreateTimerTask(uint uiSec, uint uiNsec, TimingFunc func)
{
    int tfd = _CreateTimerObj(uiSec, uiNsec);
    if (tfd < 0)
    {
        LOG_ERROR << "_CreateTimerObj Err!";
        return tfd;
    }
     
    stTimerParam* pParam = new stTimerParam();
    pParam->setParams(TYPE_TIMER, tfd);
    int nRet = _AddTimerObjToEpoll(tfd, pParam);
    if (nRet < 0)
    {
		LOG_ERROR << "_AddTimerObjToEpoll Err!";
        return nRet;
    }

   m_mapTimer[tfd] = func;
   return 0;
}

bool TimerManager::CreateTimerTasks()
{
	TimingFunc t001 = Timer100mSec;
	TimingFunc t1 = Timer1Sec;
	TimingFunc t5 = Timer5Sec;
    TimingFunc t30 = Timer30Sec;
    //TimingFunc t60 = Timer60Sec;
    int nRet = -1;
	nRet = CreateTimerTask(0, 100, t001);
	if (nRet < 0)
		return false;

	nRet = CreateTimerTask(1, 0, t1);
    if (nRet < 0)
        return false;

    nRet = CreateTimerTask(5, 0, t5);
	if (nRet < 0)
		return false;

	nRet = CreateTimerTask(30, 0, t30);
	if (nRet < 0)
		return false;

	//CreateTimerTask(60, 0, t60, nullptr);
    return true;
}

bool TimerManager::IsTimerTask(int tFd)
{
    return (m_mapTimer.find(tFd) != m_mapTimer.end());
}

bool TimerManager::doTimerTask(int tFd)
{
    if (IsTimerTask(tFd))
    {
		m_mapTimer[tFd](tFd); //执行定时任务
        return true;
    }

    return false;
}

void TimerManager::CloseTimerObj(int tFd, stTimerParam* pParam)
{
	if (m_pEpoller)
		m_pEpoller->del(tFd, pParam, EPOLLOUT);

	auto it = m_mapTimer.find(tFd);
	if (it != m_mapTimer.end())
		m_mapTimer.erase(it);

	close(tFd);

    LOG_INFO << "closeTimerObj";
}

