//  ControlStatementActivity
//  Copyright (c) 2015 by Kurt Duncan
//
//  Manages the execution of a control statement for a particular batch or demand run.



#include    "execlib.h"



//  private, protected static methods

//  worker()
//
//  This is where the work gets done
void
ControlStatementActivity::worker()
{
    m_pCSInterpreter->executeStatement( this );

    //  If execution of the control statement caused an error, go to ERROR mode.
    ControlModeRunInfo* pri = dynamic_cast<ControlModeRunInfo*>( getRunInfo() );
    pri->attach();
    if ( m_pCSInterpreter->isErrorStatus() )
        pri->setErrorMode();

    m_pCSInterpreter->postExecuteStatusToPrint( pri );
    pri->clearStatementImageStack();
    pri->detach();
}



//  constructors, destructors

ControlStatementActivity::ControlStatementActivity
(
    Exec* const                 pExec,
    ControlModeRunInfo* const   pRunInfo,
    CSInterpreter* const        pCSInterpreter
)
:IntrinsicActivity( pExec, "ControlStatementActivity", pRunInfo ),
m_pCSInterpreter( pCSInterpreter )
{
}


ControlStatementActivity::~ControlStatementActivity()
{
    delete m_pCSInterpreter;
}



//  public methods

//  dump()
//
//  IntrinsicActivity interface
//  For debugging
void
    ControlStatementActivity::dump
    (
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
    )
{
    stream << prefix << "ControlStatementActivity" << std::endl;

    stream << prefix << "  Control Statement Interpreter Dump ------------------" << std::endl;
    if ( m_pCSInterpreter )
        stream << m_pCSInterpreter;
    stream << prefix << "  End Control Statement Interpreter Dump ------------------" << std::endl;

    IntrinsicActivity::dump( stream, prefix + "  ", dumpBits );
}

