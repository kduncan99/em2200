//  CoarseSchedulerActivity.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Object which handles control mode for demand and batch modes



#ifndef     EXECLIB_COARSE_SCHEDULER_ACTIVITY_H
#define     EXECLIB_COARSE_SCHEDULER_ACTIVITY_H



#include    "IntrinsicActivity.h"
#include    "ControlModeRunInfo.h"
#include    "CSInterpreter.h"



class CoarseSchedulerActivity : public IntrinsicActivity
{
private:
    enum LocalState
    {
        STARTUP             = m_InitialState,   //  Anything necessary prior to first user interation
        POLL,                                   //  Check on Runs
        DONE                = m_TerminalState,
    };

    bool                        processControlStatement( ControlModeRunInfo* const pRunInfo );
    bool                        processRunInfo( ControlModeRunInfo* const pRunInfo );
    void                        worker();

    static void                 echoStatementStack( ControlModeRunInfo* const   pRunInfo,
                                                    const bool                  processorCallFlag );
    static void                 handlePoll( IntrinsicActivity* const pObject );
    static bool                 processCheckErrorAbort( ControlModeRunInfo* const pRunInfo );
    static bool                 processReadInput( ControlModeRunInfo* const pRunInfo );
    static bool                 processSkip( ControlModeRunInfo* const  pRunInfo,
                                                const SuperString&      currentLabel );


public:
    CoarseSchedulerActivity( Exec* const pExec );

    //  IntrinsicActivity interface
    void                        dump( std::ostream&         stream,
                                        const std::string&  prefix,
                                        const DUMPBITS      dumpBits );
};



#endif

