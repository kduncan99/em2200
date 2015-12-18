//  ExecRunInfo implementation



#include    "execlib.h"



//  private, protected methods



//  constructors, destructors

ExecRunInfo::ExecRunInfo
(
    Exec* const             pExec,
    const std::string&      overheadAccountId,
    const std::string&      overheadUserId
)
:RunInfo( pExec, STATE_ACTIVE, "EXEC-8", "EXEC-8", overheadAccountId, "SYS$", overheadUserId, 0, ' ', ' ' )
{
    //  Build a dummy Task object so we can attach activities to the Exec
    m_pTask = new Task( "EXEC-8", "EXEC KERNEL" );
}


ExecRunInfo::~ExecRunInfo()
{
}



//  public methods

//  dump()
//
//  For debugging
void
    ExecRunInfo::dump
    (
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
    )
{
    stream << prefix << "ExecRunInfo" << std::endl;
    RunInfo::dump( stream, prefix, dumpBits );
}


