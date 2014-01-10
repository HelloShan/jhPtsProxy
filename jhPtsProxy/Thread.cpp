#include "Thread.h"


#define  MAX_THREAD_STACK	1024*1024*2 // 2M

Thread::Thread()
{
	m_Thread = 0;
	m_bRuning = false ;	
	
}

Thread::~Thread()
{

}

int	Thread::start()
{

#ifdef __WIN32__
	m_bRuning = false ;
	DWORD ThreadID;
	m_Thread = ::CreateThread(0, MAX_THREAD_STACK ,startThread,this,0,&ThreadID);

	if( NULL != m_Thread )
	{
		m_bRuning = false ;
	}
	else
	{
		m_bRuning = false ;
		return 0;
	}
#else
	//const 	type_info& tData = typeid( *this ) ;

	pthread_attr_t attr;

	pthread_attr_init(&attr); /* initialize attr with default attributes */
	
	m_bRuning = false ;
	if ( pthread_create ( &m_Thread, &attr, startThread, (void *)this ) == 0 ) 
	{

	}
	else
	{
		return 0;	
	}
#endif

	return true ;

}

THREAD_FUN Thread::startThread (void * Argument) 
{
	Thread *pThread = (Thread *)Argument ;
	
	pThread->m_bRuning = true;
	pThread->main() ;

    return 0;
}

// main thread loop
THREAD_FUN Thread::main()
{
	m_bRuning = true;
	m_Thread = 0;
	return 0;
}

void Thread::join() 
{
#ifdef __WIN32__

	if( 0 != m_Thread )
		WaitForSingleObject( m_Thread , INFINITE );
#else
	// wait for termination
	if( 0 != m_Thread )
		pthread_join ( m_Thread, 0 );

#endif

}

void Thread::kill_thread() 
{
#ifdef __WIN32__
	TerminateThread(m_Thread,0 );
#else
	pthread_kill(m_Thread, SIGQUIT);
#endif
}
