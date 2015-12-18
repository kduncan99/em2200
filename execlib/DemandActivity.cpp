//  DemandActivity
//
//  Temporary development version.
//  Only for testing basic ECL.
//  Cannot start other intrinsic or extrinsic stuff (@start is okay, though)
//  Also, no @@ stuff (that is for RSI, and we are not RSI)
//  Userid is always 'TEST', runid is from the @RUN card.
//
//  Scope:
//      Exec instantiates this with a pointer to the relevant RunInfo struct.
//      Adds a pointer to us, to the RunInfo container of activity pointers.
//      Starts us, and then leaves us to do the rest.
//
//      We open a SmartConsole for user communication,
//      and go to control mode (waiting for @RUN).
//
//      Wait for ECL, and interpret and execute as appropriate.
//      Upon FAC error, display {runid} FAC ERROR message, set error mode, do not FIN
//      Upon other errors, simply display {runid} ERROR, set error mode do not FIN
//
//      Upon @FIN from user (or closing the SmartConsole window), we 'clean up'
//          Release all facilities
//          Notify Exec we are done
//          Stop ourselves (any worry about race condition with previous step?)
//
//      Exec checks to see if the RunInfo can be released
//          (if anything is queued to an output symbiont, it has to stay)



#if 0 // i think this is obsolete...????
#include    "execlib.h"



//  private, protected methods

/*
static void                 handleECLImage( IntrinsicActivity* const pObject );
static void                 handleRunImage( IntrinsicActivity* const pObject );
*/

//  handleShutdown()
//
//  SHUTDOWN state handler
//  Next state is m_TerminalState.
//  Closes the SmartConsole, and notifies the Exec that we are done.
static void
    DemandActivity::handleShutdown
    (
    IntrinsicActivity* const    pObject
    )
{
}


//  handleStartup()
//
//  STARTUP state handler.
//  Next state (by default) is READ_RUN_IMAGE.
//  Opens a SmartConsole and displays ENTER @RUN IMAGE message.
//  If anything goes wrong, we set the abort(????) flag in the run condition word
//  and transition to SHUTDOWN state with no delay.
static void
    DemandActivity::handleStartup
    (
    IntrinsicActivity* const pObject
    )
{
    pObject->m_pConsole = new SmartConsole( "Demand:TEST" );
    pObject->m_pConsole.open( Exec::getInstance()->getOwnerWindow(), Exec::getInstance()->getTerminalIcon() );
}



//  constructors, destructors

DemandActivity::DemandActivity
    (
    RunInfo* const      pRunInfo
    )
    :IntrinsicActivity( "Demand:TEST", pRunInfo )
{
    m_pConsole = 0;

    establishStateEntry( STARTUP, handleStartup, READ_RUN_IMAGE, 0 );
}


DemandActivity::~DemandActivity()
{
    assert( m_pConsole == 0 );
    assert( isTerminated() );
}



//  public methods


#endif