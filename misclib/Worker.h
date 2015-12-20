//	Worker.h
//  Copyright (c) 2015 by Kurt Duncan
//
//	Base class for anything that wants to have a basic worker thread



#ifndef		EM2200_WORKER_H
#define		EM2200_WORKER_H



class Worker
{
private:
    bool                        m_ActiveFlag;
    const std::string           m_Name;
    bool                        m_TermFlag;
#ifdef WIN32
    HANDLE                      m_EventHandle;
    HANDLE                      m_ThreadHandle;
    DWORD                       m_ThreadId;
#else
    pthread_cond_t *            m_pThreadCondition;
    pthread_t                   m_ThreadId;
    pthread_mutex_t *           m_pThreadMutex;     // private mutex; others must use Lockable implementation
#endif

#ifdef WIN32
    typedef void                THREADPROCRETURN;
#else
    typedef void *              THREADPROCRETURN;
#endif

    static const bool           WORKER_LOGGING = true;
    static THREADPROCRETURN     workerThreadProc( void* pArg );
    virtual void                worker() = 0;

public:
    Worker( const std::string& Name );
    virtual ~Worker();

    void                        workerSignal() const;
    void                        workerSetTermFlag();
    void                        workerWait( const COUNT32 Milliseconds ) const;

    inline const std::string&   getWorkerName() const           { return m_Name; }
    inline bool                 isWorkerActive() const          { return m_ActiveFlag; }
    inline bool                 isWorkerTerminated() const      { return m_TermFlag && !m_ActiveFlag; }
    inline bool                 isWorkerTerminating() const     { return m_TermFlag; }

    virtual bool				workerStart();
    virtual bool				workerStop( const bool WaitFlag );
};



#endif

