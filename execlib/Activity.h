//  Activity.h header file representing an Activity



#ifndef EXECLIB_ACTIVITY_H
#define EXECLIB_ACTIVITY_H



//  Need these forward-references to avoid circular header-file includes
class Exec;
class RunInfo;



class Activity : public Worker, public Lockable
{
public:
    enum StopReason
    {
        STOP_NONE,
        STOP_SHUTDOWN,      //  Exec going down
        STOP_DEMAND_TERM,   //  DEMAND run abruptly terminated
        STOP_DEMAND_X,      //  @@X keyin from DEMAND
    };

protected:
    Exec* const                     m_pExec;
    RunInfo* const                  m_pRunInfo;
    StopReason                      m_StopReason;

public:
    Activity( Exec* const           pExec,
              const std::string&    threadName,
              RunInfo* const        pRunInfo );
    virtual ~Activity();

    bool                            start();
    bool                            stop( const StopReason  reason,
                                          const bool        wait );

    inline RunInfo*                 getRunInfo() const          { return m_pRunInfo; }
    inline const std::string&       getThreadName() const       { return Worker::getWorkerName(); }
    inline bool                     isActive() const            { return Worker::isWorkerActive(); }
    inline bool                     isTerminated() const        { return Worker::isWorkerTerminated(); }
    inline bool                     isTerminating() const       { return Worker::isWorkerTerminating(); }
    inline void                     signal()                    { return Worker::workerSignal(); }

    inline StopReason               getStopReason() const       { return m_StopReason; }
    inline void                     wait( const COUNT32 msecs ) { Worker::workerWait( msecs ); }

    virtual void                    dump( std::ostream&         stream,
                                            const std::string&  prefix,
                                            const DUMPBITS      dumpBits ) = 0;

    static std::string              getStopReasonString( const StopReason reason );
};



#endif
