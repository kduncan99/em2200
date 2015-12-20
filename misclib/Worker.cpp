//  Worker base class implementation
//  Copyright (c) 2015 by Kurt Duncan



#include    "misclib.h"



//  constructors, destructors
Worker::Worker( const std::string& name )
	:m_ActiveFlag( false ),
	m_Name( name ),
	m_TermFlag( false ),
#ifdef WIN32
    m_EventHandle( 0 ),
	m_ThreadHandle( reinterpret_cast<HANDLE>(-1) ),
	m_ThreadId( 0 )
#else
    m_pThreadCondition( 0 ),
    m_ThreadId( 0 ),
    m_pThreadMutex( 0 )
#endif
{
#ifdef WIN32
	// Create event handle
	m_EventHandle = CreateEvent( NULL, FALSE, FALSE, NULL );
#else
    m_pThreadCondition = new pthread_cond_t;
    m_pThreadMutex = new pthread_mutex_t;
    pthread_cond_init( m_pThreadCondition, 0 );
    pthread_mutex_init( m_pThreadMutex, 0 );
#endif
}


Worker::~Worker()
{
#ifdef WIN32
	CloseHandle( m_EventHandle );
    m_EventHandle = 0;
#else
    pthread_cond_destroy( m_pThreadCondition );
    pthread_mutex_destroy( m_pThreadMutex );
    delete m_pThreadCondition;
    delete m_pThreadMutex;
#endif
}


// other functions

//  workerSetTermFlag()
//
//  Sets the term flag, which the derived class's worker thread should be polling.
void
Worker::workerSetTermFlag()
{
    if ( WORKER_LOGGING )
    {
        std::string logStr = "Setting term flag for worker ";
        logStr += getWorkerName();
        SystemLog::getInstance()->write( logStr );
    }

    m_TermFlag = true;
}


//  workerSignal()
//
//  Allows any client to signal the Worker that something is ready, or needs attention.
//  Useful for waking up the worker from some sleeping state.
void
Worker::workerSignal() const
{
#ifdef WIN32
    SetEvent( m_EventHandle );
#else
    pthread_mutex_lock( m_pThreadMutex );
    pthread_cond_signal( m_pThreadCondition );
    pthread_mutex_unlock( m_pThreadMutex );
#endif
}


//  workerStart()
//
//  Starts the thread
bool
Worker::workerStart()
{
    if ( isWorkerActive() )
        return false;

    m_ActiveFlag = true;
    m_TermFlag = false;

    // Create thread
#ifdef WIN32
    m_ThreadHandle = reinterpret_cast<HANDLE>(_beginthread( Worker::workerThreadProc, 0, reinterpret_cast<void*>(this) ));
    if ( m_ThreadHandle == reinterpret_cast<HANDLE>(-1) )
    {
        std::string logStr = "Failed to start thread for ";
        logStr += this->getWorkerName();
        SystemLog::getInstance()->write( logStr );
        m_ActiveFlag = false;
        return false;
    }
#else
    int retn = pthread_create( &m_ThreadId, 0, workerThreadProc, this );
    if ( retn != 0 )
    {
        std::string logStr = "Failed to start thread for ";
        logStr += this->getWorkerName();
        SystemLog::getInstance()->write( logStr );
        m_ActiveFlag = false;
        return false;
    }
#endif

    return true;
}


//  workerStop()
//
//  Stops the thread
//
//  Parameters:
//      WaitFlag:           true if we should hold off returning to caller until worker thread is non-active,
//                              else false for immediate return
bool
Worker::workerStop
    (
    const bool              waitFlag
    )
{
    if ( !isWorkerActive() )
        return false;

    workerSetTermFlag();
    workerSignal();
    if ( waitFlag )
    {
#ifdef WIN32
        while ( isWorkerActive() )
            Sleep( 0 );
#else
        pthread_join( m_ThreadId, 0 );
#endif
    }

    return true;
}


//  workerWait()
//
//  Sleep for the indicated number of milliseconds, or until we are signaled.
void
Worker::workerWait
    (
    const COUNT32       Milliseconds
    ) const
{
#ifdef WIN32
    WaitForSingleObjectEx( m_EventHandle, Milliseconds, TRUE );
#else
    COUNT64 million = 1000000UL;

    struct timeval tvNow;
    gettimeofday( &tvNow, 0 );
    COUNT64 millisNow = (static_cast<COUNT64>(tvNow.tv_sec) * million) + tvNow.tv_usec;
    COUNT64 millisLimit = millisNow + Milliseconds;

    struct timespec timeToWait;
    timeToWait.tv_sec = millisLimit / million;
    timeToWait.tv_nsec = (millisLimit % million) * 1000UL;

    pthread_mutex_lock( m_pThreadMutex );
    pthread_cond_timedwait( m_pThreadCondition, m_pThreadMutex, &timeToWait );
    pthread_mutex_unlock( m_pThreadMutex );
#endif
}



// static functions

//  workerThreadProc()
//
//  This is the procedure which gets spun off by beginthread().
//  We basically just call the derived class's worker() function.
//
//  Static.
Worker::THREADPROCRETURN
Worker::workerThreadProc
    (
    void* const         pWorker
    )
{
    std::string         logStr;
    Worker*             pw;

    pw = static_cast<Worker*>( pWorker );

    if ( WORKER_LOGGING )
    {
        logStr = "Starting worker for ";
        logStr += pw->getWorkerName();
        SystemLog::getInstance()->write( logStr );
    }

    pw->worker();

    if ( WORKER_LOGGING )
    {
        logStr = "Worker exiting for ";
        logStr += pw->getWorkerName();
        SystemLog::getInstance()->write( logStr );
    }

    pw->m_TermFlag = true;
    pw->m_ActiveFlag = false;
#ifdef  WIN32
    _endthread();
#else
    pthread_exit( 0 );
#endif
}

