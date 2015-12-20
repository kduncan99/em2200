//  ControlStatementActivity.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Handles the actual execution of a control statement for a batch or demand run



#ifndef     EXECLIB_CONTROL_STATEMENT_ACTIVITY_H
#define     EXECLIB_CONTROL_STATEMENT_ACTIVITY_H



#include    "CSInterpreter.h"
#include    "IntrinsicActivity.h"



class ControlStatementActivity : public IntrinsicActivity
{
private:
    CSInterpreter* const        m_pCSInterpreter;

    void                        worker();

public:
    ControlStatementActivity( Exec* const           pExec,
                              ControlModeRunInfo*   pRunInfo,
                              CSInterpreter*        pCSInterpreter );
    ~ControlStatementActivity();

    //  IntrinsicActivity interface
    void                        dump( std::ostream&         stream,
                                        const std::string&  prefix,
                                        const DUMPBITS      dumpBits );
};



#endif

