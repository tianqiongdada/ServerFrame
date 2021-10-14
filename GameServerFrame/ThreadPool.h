#pragma once
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <atomic>
#include <future>
//#include <condition_variable>
//#include <thread>
//#include <functional>
#include <stdexcept>

namespace whx
{
	//线程池最大容量,应尽量设小一点
#define  THREADPOOL_MAX_NUM 16

	//线程池,可以提交变参函数或拉姆达表达式的匿名函数执行,可以获取执行返回值
	//不直接支持类成员函数, 支持类静态成员函数或全局函数,Opteron()函数等
	class CThreadPool
	{
	public:
		CThreadPool(unsigned short usSize = 4) { AddThread(usSize); }
		~CThreadPool()
		{
			m_bRun = false;
			m_task_cv.notify_all();
			for (thread& thr : m_vcPool)
			{
				if (thr.joinable())
					thr.join();
			}
		}

	private:
		using Task = function < void() >; //功能函数
		vector<thread> m_vcPool;		   //线程池
		queue<Task>	m_qTask;			   //任务队列
		mutex m_lock;					   //同步
		condition_variable	m_task_cv;	   //条件变量

		//atomic 原子级操作，多线程同步
		atomic<bool> m_bRun{ true };
		atomic<int>	m_nIdlThreadNum{ 0 };

	public:
		int GetIdlThreadNum() { return m_nIdlThreadNum; }
		int GetThreadNum() { return m_vcPool.size(); }

		//可变参数模板
		//提交一个任务函数
		template<class T, class... Args>
		auto TaskCommit(T&& t, Args&&... args)->future < decltype(t(args...)) >
		{
			if (!m_bRun)
				throw runtime_error("TaskCommit on ThreadPool is stopped.");

			using ResType = decltype(t(args...));
			auto task = make_shared<packaged_task<ResType()>>(bind(forward<T>(t), forward<Args>(args)...)); // 把函数入口及参数,打包(绑定)
			future<ResType> fTask = task->get_future();

			//添加到任务队列
			{
				lock_guard<mutex> lock{ m_lock }; //对当前块的语句加锁  lock_guard 是 mutex 的 stack 封装类，构造的时候 lock()，析构的时候 unlock()

				//任务添加
				m_qTask.emplace([task] {
					(*task)();
					});
			}

#ifdef THREADPOOL_AUTO_GROW //定义线程自动添加
			if (m_nIdlThreadNum < 1 && m_vcPool.size() < THREADPOOL_MAX_NUM)
				AddThread(1);
#endif

			m_task_cv.notify_one(); //唤醒一个执行线程
			return fTask;
		}

		void AddThread(unsigned short usSize)
		{
			if (usSize > THREADPOOL_MAX_NUM)
				return;

			for (; usSize > 0; --usSize)
			{
				m_vcPool.emplace_back([this] {    //工作线程函数 lambda
					while (true)
					{
						Task task; //任务函数
						{
							// unique_lock 相比 lock_guard 的好处是：可以随时 unlock() 和 lock()
							unique_lock<mutex> lock{ m_lock };
							m_task_cv.wait(lock, [this] { return !m_bRun || !m_qTask.empty(); });
							if (!m_bRun && m_qTask.empty())
								return;

							task = move(m_qTask.front());
							m_qTask.pop();
						}


						--m_nIdlThreadNum;
						task();  //执行任务
						++m_nIdlThreadNum;
					}

					});

				++m_nIdlThreadNum;
			}
		} //end AddTask
	};
}


#endif 
