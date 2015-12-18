//  BatchRunInfo class declaration
//
//  Some specifics which apply only to batch runs



#ifndef     EXECLIB_BATCH_RUN_INFO_H
#define     EXECLIB_BATCH_RUN_INFO_H



#include    "ControlModeRunInfo.h"



//  annoying forward-references
class   Exec;



class   BatchRunInfo : public ControlModeRunInfo
{
private:
    bool                    m_OperatorHold;

public:
    BatchRunInfo( Exec* const           pExec,
                  const std::string&    originalRunId,
                  const std::string&    actualRunId,
                  const std::string&    accountId,
                  const std::string&    projectId,
                  const std::string&    userId,
                  const UINT32          options,
                  const char            schedulingPriority,
                  const char            processorDispatchingPriority,
                  const bool            privilegedUserId )
    :ControlModeRunInfo( pExec,
                         STATE_IN_BACKLOG,
                         originalRunId,
                         actualRunId,
                         accountId,
                         projectId,
                         userId,
                         options,
                         schedulingPriority,
                         processorDispatchingPriority ),
    m_OperatorHold( false )
    {}

    void            dump( std::ostream&         stream,
                            const std::string&  prefix,
                            const DUMPBITS      dumpBits );

    inline bool inControlMode() const                           { return m_pTask == 0; }
    inline bool isBatch() const                                 { return true; }
    inline bool isHeld() const                                  { return m_OperatorHold; }
    inline void setHold( const bool flag )                      { m_OperatorHold = flag; }
};



#endif
