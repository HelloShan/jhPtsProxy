#ifndef __THREAD_QUEUE_H__
#define __THREAD_QUEUE_H__

#include "global.h"

//only for pointer data
template<typename T>
class ThreadQueue
{
	queue<T> m_queue;
	CRITICAL_SECTION m_mutex;

public:
	ThreadQueue()
	{
		InitializeCriticalSection(&m_mutex);
	}
	virtual ~ThreadQueue()
	{
		clear();
	}

	void clear()
	{
		EnterCriticalSection(&m_mutex);
		while(!m_queue.empty())
		{
			T item = m_queue.front();
			delete item;
			m_queue.pop();
		}
		LeaveCriticalSection(&m_mutex);
	}

	void push(T &item)
	{
		EnterCriticalSection(&m_mutex);
		m_queue.push(item);
		LeaveCriticalSection(&m_mutex);
	}

	void pop(T &item)
	{
		EnterCriticalSection(&m_mutex);
		if(!m_queue.empty())
		{
			item = m_queue.front();
			m_queue.pop();
		}
		LeaveCriticalSection(&m_mutex);
	}

	bool empty()
	{
		return m_queue.empty();
	}
};

#endif//__THREAD_QUEUE_H__
