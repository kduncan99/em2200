//  TransparentActivity.cpp



#include    "execlib.h"



//  private, protected methods

//  handleCM()
//
//  @@CM handler
void
TransparentActivity::handleCM()
{
    std::string consMsg = m_RSISessionName + "*MSG: " + m_pInterpreter->getText().substr( 0, 57 );
    ConsoleManager* pConsMgr = dynamic_cast<ConsoleManager*>( m_pExec->getManager( Exec::MID_CONSOLE_MANAGER ) );
    pConsMgr->postReadOnlyMessage( consMsg, m_pExec->getRunInfo() );
    m_pRSIManager->sendOutput( m_RSISessionNumber, "-@@COMPLETE" );
}


//  handleMSG()
//
//  @@MSG handler
void
TransparentActivity::handleMSG()
{
    if ( m_pRunInfo )
    {
        m_pRSIManager->sendOutput( m_RSISessionNumber, "@@MSG NOT ALLOWED" );
        return;
    }

    std::string consMsg = m_pRunInfo->getActualRunId() + "*MSG: " + m_pInterpreter->getText().substr( 0, 57 );
    ConsoleManager* pConsMgr = dynamic_cast<ConsoleManager*>( m_pExec->getManager( Exec::MID_CONSOLE_MANAGER ) );
    pConsMgr->postReadOnlyMessage( consMsg, m_pExec->getRunInfo() );
    m_pRSIManager->sendOutput( m_RSISessionNumber, "-@@COMPLETE" );
}


//  handleTERM()
//
//  @@TERM handler
void
TransparentActivity::handleTERM()
{
    m_pRSIManager->terminateSession( m_RSISessionNumber );
}


//  handleX()
//
//  @@X handler
void
TransparentActivity::handleX()
{
    if ( !m_pRunInfo )
    {
        m_pRSIManager->sendOutput( m_RSISessionNumber, "@@X NOT ALLOWED" );
        return;
    }

    const UINT32 options = m_pInterpreter->getOptions();
    const SuperString& text = m_pInterpreter->getText();

    const bool cOpt = ( options & OPTB_C ) ? true : false;
    const bool iOpt = ( options & OPTB_I ) ? true : false;
    const bool oOpt = ( options & OPTB_O ) ? true : false;
    const bool rOpt = ( options & OPTB_R ) ? true : false;
    const bool tOpt = ( options & OPTB_T ) ? true : false;

    if ( text.size() > 6 )
    {
        m_pRSIManager->sendOutput( m_RSISessionNumber, "@@X ERROR - TOO MANY DATA CHARACTERS" );
        return;
    }

    //  Maybe tell the Exec to kill whatever's running (if anything)
    if ( cOpt || rOpt || tOpt )
    {
        m_pExec->abortRunDemand( getRunInfo(), cOpt, rOpt, tOpt );
    }

    if ( iOpt )
    {
        //  Get rid of any queued input (such as from @@INQ or whatever)
        //TODO:RSI
    }

    if ( oOpt )
    {
        //  Get rid of any queued output.
        //  For now, RSI doesn't cache anything.  If it does, we'll delete it here.
        //  TODO:RSI
    }

    m_pRSIManager->sendOutput( m_RSISessionNumber, "*EXECUTION TERMINATED*" );
}


//  worker()
void
TransparentActivity::worker()
{
    switch ( m_pInterpreter->getCommand() )
    {
    case TransparentCSInterpreter::TCMD_CM:     return handleCM();
    case TransparentCSInterpreter::TCMD_MSG:    return handleMSG();
    case TransparentCSInterpreter::TCMD_TERM:   return handleTERM();
    case TransparentCSInterpreter::TCMD_X:      return handleX();

    case TransparentCSInterpreter::TCMD_ASG:
    case TransparentCSInterpreter::TCMD_BRKPT:
    case TransparentCSInterpreter::TCMD_CAT:
    case TransparentCSInterpreter::TCMD_FREE:
    case TransparentCSInterpreter::TCMD_HDG:
    case TransparentCSInterpreter::TCMD_LOG:
    case TransparentCSInterpreter::TCMD_MODE:
    case TransparentCSInterpreter::TCMD_PASSWD:
    case TransparentCSInterpreter::TCMD_QUAL:
    case TransparentCSInterpreter::TCMD_START:
    case TransparentCSInterpreter::TCMD_SYM:
    case TransparentCSInterpreter::TCMD_USE:
    case TransparentCSInterpreter::TCMD_CONS:
    case TransparentCSInterpreter::TCMD_CONT:
    case TransparentCSInterpreter::TCMD_CQUE:
    case TransparentCSInterpreter::TCMD_DCT:
    case TransparentCSInterpreter::TCMD_END:
    case TransparentCSInterpreter::TCMD_ESC:
    case TransparentCSInterpreter::TCMD_FUL:
    case TransparentCSInterpreter::TCMD_HOLD:
    case TransparentCSInterpreter::TCMD_INQ:
    case TransparentCSInterpreter::TCMD_INS:
    case TransparentCSInterpreter::TCMD_NOPR:
    case TransparentCSInterpreter::TCMD_PMOD:
    case TransparentCSInterpreter::TCMD_POC:
    case TransparentCSInterpreter::TCMD_PRNT:
    case TransparentCSInterpreter::TCMD_RLD:
    case TransparentCSInterpreter::TCMD_RLU:
    case TransparentCSInterpreter::TCMD_RQUE:
    case TransparentCSInterpreter::TCMD_SEND:
    case TransparentCSInterpreter::TCMD_SKIP:
    case TransparentCSInterpreter::TCMD_TM:
    case TransparentCSInterpreter::TCMD_TOUT:
    case TransparentCSInterpreter::TCMD_TTY:
        //???? send **NOT YET IMPLEMENTED**
        break;

    case TransparentCSInterpreter::TCMD_INIT:
        //???? debug stop
        break;
    }
}



//  constructors / destructors

TransparentActivity::TransparentActivity
(
    Exec* const                             pExec,
    RSIManager* const                       pRSIManager,
    const TransparentCSInterpreter* const   pInterpreter,
    RunInfo* const                          pRunInfo,           //  This will be null, if no RUN is active
    const COUNT                             rsiSessionNumber,
    const std::string&                      rsiSessionName
)
:IntrinsicActivity( pExec, "TransparentActivity", pRunInfo ),
m_pInterpreter( pInterpreter ),
m_pRSIManager( pRSIManager ),
m_RSISessionName( rsiSessionName ),
m_RSISessionNumber( rsiSessionNumber )
{
}


TransparentActivity::~TransparentActivity()
{
    //  Delete the interpreter, which became our responsibility upon instantiation
    delete m_pInterpreter;
}



//  public methods

//  dump()
//
//  IntrinsicActivity interface
//  For debugging
void
TransparentActivity::dump
(
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
)
{
    stream << prefix << "TransparentActivity" << std::endl;
    IntrinsicActivity::dump( stream, prefix + "  ", dumpBits );
}

