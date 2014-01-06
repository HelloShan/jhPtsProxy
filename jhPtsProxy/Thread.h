#ifndef __THREAD_H__
#define __THREAD_H__


#ifndef THREAD_FUN
#ifdef __WIN32__
#include <windows.h>
#define THREAD_FUN DWORD WINAPI
#else
#include <pthread.h>
#define THREAD_FUN void*
#endif//__WIN32__
#endif//THREAD_FUN

class Thread
{
  
public:

	Thread();
	
	virtual				~Thread();

	int					start( );

	virtual	THREAD_FUN  main();
	void				join();
	void				kill_thread();

private:
	
	// static method for firing off the thread
	static THREAD_FUN   startThread(void * Argument);
	
	// the actual thread
#ifndef __WIN32__
	pthread_t			m_Thread;
#else
	HANDLE				m_Thread;
#endif

	volatile bool		m_bRuning;
};



#endif //__THREAD_H__
