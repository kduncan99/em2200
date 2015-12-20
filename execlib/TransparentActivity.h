//  TransparentActivity.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Header file for activity which executes a transparent statement for RSI



#ifndef     EXECLIB_TRANSPARENT_ACTIVITY_H
#define     EXECLIB_TRANSPARENT_ACTIVITY_H



#include    "IntrinsicActivity.h"
#include    "RunInfo.h"
#include    "TransparentCSInterpreter.h"



class TransparentActivity : public IntrinsicActivity
{
private:
    enum state
    {
        STARTUP         = m_InitialState,
        TS_CM,                                  //  @@CM
        TS_MSG,                                 //  @@MSG
        TS_TERM,                                //  @@TERM
        TS_X,                                   //  @@X
        DONE            = m_TerminalState,
    };

    const TransparentCSInterpreter* const   m_pInterpreter;
    RSIManager* const                       m_pRSIManager;
    const std::string                       m_RSISessionName;
    const COUNT                             m_RSISessionNumber;

    void                    handleCM();
    void                    handleMSG();
    void                    handleTERM();
    void                    handleX();
    void                    worker();

public:
    TransparentActivity( Exec* const                            pExec,
                         RSIManager* const                      pRSIManager,
                         const TransparentCSInterpreter* const  pInterpreter,
                         RunInfo* const                         pRunInfo,
                         const COUNT                            rsiSessionNumber,
                         const std::string&                     rsiSessionName );
    ~TransparentActivity();

    //  IntrinsicActivity interface
    void                        dump( std::ostream&         stream,
                                        const std::string&  prefix,
                                        const DUMPBITS      dumpBits );
};



#endif
