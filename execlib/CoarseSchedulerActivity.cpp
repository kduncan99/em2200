//  CoarseSchedulerActivity
//  Copyright (c) 2015 by Kurt Duncan
//
//  Regularly polls all batch and Demand RunInfo objects, to see if any Control Mode processing needs done.
//  This activity runs under the Exec RunInfo context.
//  It must be started in the boot process before any user runs are opened (i.e., SYS)
//  It can be started probably as early as BootActivity, although it doesn't need to be...
//
//  For each input found from any RunInfo, we create a CSInterpreter object to interpret the statement,
//  and a ControlStatementActivity object to handle it.



#include    "execlib.h"



//  private methods

//  processControlStatement()
//
//  We've read input; check to see if it's a control statement.
//
//  Parameters:
//      pRunInfo:           pointer to RunInfo which generated the control statement
//
//  Returns:
//      true if it's a control statement and we handled it accordingly,
//      false if it isn't
bool
CoarseSchedulerActivity::processControlStatement
(
    ControlModeRunInfo* const       pRunInfo
)
{
    //  Interpret the statement stack as a Control Statement, if possible
    CSInterpreter* pcsi = new CSInterpreter( m_pExec, this, pRunInfo->getSecurityContext(), pRunInfo );
    CSInterpreter::Status csiStat =
        pcsi->interpretStatementStack( pRunInfo->getStatementImageStack(), pRunInfo->isDemand(), true, true );

    //  If we don't have a control statement, just return false.
    if ( csiStat == CSInterpreter::CSIST_NOT_FOUND )
    {
        delete pcsi;
        return false;
    }

    //  Continuation sentinel found:
    if ( csiStat == CSInterpreter::CSIST_CONTINUED )
    {
        delete pcsi;
        return true;
    }

    //  Should we skip this statement due to @TEST or @JUMP?
    bool skipFlag = processSkip( pRunInfo, pcsi->getLabel() );

    //  Echo the statement appropriately.  Maybe.
    echoStatementStack( pRunInfo, false );

    //  Scanning error?
    if ( csiStat != CSInterpreter::CSIST_SUCCESSFUL )
    {
        //  Let CSInterpreter formulate the proper error message
        pcsi->postInterpretStatusToPrint( pRunInfo );
        pRunInfo->clearStatementImageStack();
        pRunInfo->setErrorMode();
        delete pcsi;
        return true;
    }

    //  Good control statement.  Skip if appropriate.
    if ( skipFlag )
    {
        pRunInfo->clearStatementImageStack();
        delete pcsi;
        return true;
    }

    //  Create a control statement activity and pass everything thereto
    pRunInfo->attach();
    ControlStatementActivity* pcsa = new ControlStatementActivity( m_pExec, pRunInfo, pcsi );
    pRunInfo->setControlModeActivity( pcsa );
    pRunInfo->detach();
    pcsa->start();

    return true;
}


//  processRunInfo()
//
//  Do all the control mode input processing (see in-line commentary).
//
//  Parameters:
//      pRunInfo:           pointer to RunInfo of interest
//
//  Returns:
//      true if we did something useful
bool
CoarseSchedulerActivity::processRunInfo
(
    ControlModeRunInfo* const       pRunInfo
)
{
    //  Does run have a control mode activity in process?
    //  If so, do nothing (wait for the activity to finish)
    //  If it has one that's done, get rid of it.
    Activity* pcmAct = pRunInfo->getControlModeActivity();
    if ( pcmAct )
    {
        if ( !pcmAct->isTerminated() )
            return false;
        pRunInfo->deleteControlModeActivity();
    }

    //  Should the run be terminated due to ERROR or ABORT?  If so, do it.
    //TODO:TASK This shouldn't be here, because it rules out @PMD, but what to do?
    if ( processCheckErrorAbort( pRunInfo ) )
        return true;

    //  Look for a DCSE statement (from ER QECL$)
    //TODO:QECL
//      If there is a DCSE statement:
//          Dequeue DCSE statement
//          Echo DCSE statement to PRINT$
//          Process it
//          Done

    //  Read input from READ$ or the RSI session.
    //  If we get input, it will be placed on the RunInfo's statement stack.
    if ( !processReadInput( pRunInfo ) )
        return false;

    //  We're in control mode, so the first character had better be a masterspace
    const std::string& firstImage = pRunInfo->getStatementImageStack().front();
    if ( (firstImage.size() == 0) || (firstImage[0] != '@') )
    {
        //  Assume a data image, and emit a diagnostic accordingly.
        //  Do NOT place the run in error.
        if ( !pRunInfo->getDataIgnoredMsgFlag() )
        {
            pRunInfo->postToPrint( "DATA IGNORED - IN CONTROL MODE" );
            pRunInfo->setDataIgnoredMsgFlag( true );
        }

        pRunInfo->clearStatementImageStack();
        return true;
    }

    pRunInfo->setDataIgnoredMsgFlag( false );

    //  Handle it as either a control statement or processor call statement.
    if ( processControlStatement( pRunInfo ) )
        return true;

    //TODO:PROC Is it a processor call?  If this errors, we display an error presuming it is a pooched processor call.
    //TODO:BATCH honor @TEST/@JUMP logic (already have that covered?)

    //TODO:BATCH echo (non)control statement after page eject
    //          PAGEJECT (default True) unless overridden by SETC,N
    //          PAGEJECT False, no eject unless overridden by SETC,P)
    //  following is temporary
    echoStatementStack( pRunInfo, false );
    pRunInfo->postToPrint( "PROCESSOR NOT FOUND" );
    pRunInfo->clearStatementImageStack();
    pRunInfo->setErrorMode();
    return true;
}



//  private, protected static methods

//  echoStatementStack()
//
//  Echos the statement stack to PRINT$ if we are batch,
//  or if we have processed an @ADD,L (nested @ADD's inherit the setting)
void
CoarseSchedulerActivity::echoStatementStack
(
    ControlModeRunInfo* const   pRunInfo,
    const bool                  processorCallFlag
)
{
    //  Do we echo? TODO:ADD account for @ADD,L when we can
    if ( pRunInfo->isBatch() )
    {
        COUNT lineSpace = 3;
        bool ejectFlag = processorCallFlag;
        //TODO:BATCH override this with PAGEJECT and SETC,P, SETC,N

        const LSTRING& stack = pRunInfo->getStatementImageStack();
        for ( LCITSTRING its = stack.begin(); its != stack.end(); ++its )
        {
            pRunInfo->postToPrint( *its, lineSpace, ejectFlag );
            lineSpace = 1;
            ejectFlag = false;
        }
    }
}


//  processCheckErrorAbort()
//
//  Batch runs for which the most recent task ended in ERROR or ABORT status will
//  be terminated IF:
//      The operator issued an E or X keyin, *or*
//      @SETC,I is not in play (i.e., RCW bit 5 is clear)
//
//  Returns:
//      true if we shut down the run, false for caller to continue processing.
bool
CoarseSchedulerActivity::processCheckErrorAbort
(
    ControlModeRunInfo* const       pRunInfo
)
{
#if 0 //TODO:TASK pending reconsideration - we don't want to do this immediately, only after no @PMD is found, right?

    //  If we're not batch, forget it.
    if ( !pRunInfo->isBatch() )
        return false;

    //  If a recent task is not in error, forget it.
    RunConditionWord& rcw = pRunInfo->getRunConditionWord();
    if ( !rcw.anyPreviousTaskInError() )
        return false;

    //  So some recent task is in error.
    //  So, we're in ERROR/ABORT mode.  If the operator put us here, or RCW bit 5 is clear,
    //  terminate the run.  (Also dispense with iteration delay, as we may be able to dispense
    //  with this run on the next poll iteration).
    if ( pRunInfo->getOperatorEKeyin() || pRunInfo->getOperatorXKeyin() || ( (rcwT1 & 0100) == 0 ) )
    {
        pRunInfo->setStatus( RunInfo::ST_FIN );
        setDelay( 0 );
        return true;
    }
#endif

    return false;
}


//  processReadInput()
//
//  Reads an input image, and does manipulations and checks as appropriate
//
//  Returns:
//      true if a complete image is in the RunInfo's control image stack, ready to be processed.
//      false otherwise (more input is needed)
bool
CoarseSchedulerActivity::processReadInput
(
    ControlModeRunInfo* const       pRunInfo
)
{
    //  Read input from the current read symbiont buffer.
    std::string image;
    UserRunInfo::SymbiontStatus readStatus = pRunInfo->pollFromRead( &image );

    switch ( readStatus )
    {
    case UserRunInfo::SST_BAD_FORMAT:
        //  For READ$ or @ADD, we discovered non-SDF data.
        //  Complain to PRINT$ (and maybe to console?) and take run to error mode.
        //  For RSI, it's (probably) a programming error... ?
        //TODO:BATCH
        return false;

    case UserRunInfo::SST_CONTROL_IMAGE:
        //  Read and processed control image from file buffer,
        //  but no data is available.  Try again.
        //TODO:BATCH
        return false;

    case UserRunInfo::SST_END_OF_FILE:
        //  077 control image read from READ$ or @ADD
        //  Make sure we don't have a partial image stacked.
        //  For @ADD, close the file and pop the SymbiontBuffer, then return false.
        //  If this is READ$, do implied @FIN processing.
        //TODO:BATCH
        //TODO:ADD
        return false;

    case UserRunInfo::SST_IO_ERROR:
        //  IO error encountered reading from READ$ or @ADD, or session closed for RSI.
        //  Complain to PRINT$ (and maybe to console?) and take run to error mode.
        //  For @ADD close the file and pop the SymbiontBuffer, then return false.
        //  For READ$, do implied @FIN processing.
        //  For RSI, do implied @FIN processing.
        //TODO:BATCH
        //TODO:ADD
        if ( pRunInfo->isDemand() )
        {
        }
        return false;

    case UserRunInfo::SST_NOT_READY:
        //  No input available, or output buffer full for RSI (the latter is not relevant here)
        return false;

    case UserRunInfo::SST_PARTIAL_IMAGE:
        //  A partial image is available - do not process it, pend it and come back here
        //  for the next (possibly last) part of the image.
        //  This can (maybe) happen on any input buffer
        //TODO:BATCH
        return false;

    case UserRunInfo::SST_SUCCESSFUL:
        //  Image read successfully
        break;
    }

    //  Stack new input on the statement image stack, and kick the image counter.
    pRunInfo->attach();
    pRunInfo->appendStatementImage( image );
    pRunInfo->incrementImagesRead();
    pRunInfo->detach();

    return true;
}


//  processSkip()
//
//  Indicates whether we should skip the statement currently under consideration.
//  We may update skip states in pRunInfo.
//
//  Returns:
//      true if it should be skipped, else false
bool
CoarseSchedulerActivity::processSkip
(
    ControlModeRunInfo* const       pRunInfo,
    const SuperString&              currentLabel
)
{
    //  Are we skipping {n} lines?
    if ( pRunInfo->getSkipCount() > 0 )
    {
        //  For control statements, any statement other than @EOF, @FILE, and @ENDF is
        //  countable.  We never see @FILE and @ENDF here, and @EOF will be filtered out
        //  prior to calling here, if it has an illegal label.
        //  Processor calls are always countable.
        pRunInfo->decrementSkipCount();
        return true;
    }

    //  Are we skipping to a particular label?
    else if ( pRunInfo->getSkipLabel().size() > 0 )
    {
        //  If the label matches, end skip mode and let the current statement execute.
        if ( currentLabel.compareNoCase( pRunInfo->getSkipLabel() ) == 0 )
        {
            pRunInfo->setSkipLabel( "" );
            return false;
        }

        //  Label doesn't match, skip, and keep skipping.
        return true;
    }

    return false;
}


//  worker()
//
//  We go through this over and over, until the world ends.
void
CoarseSchedulerActivity::worker()
{
    while ( !isWorkerTerminating() )
    {
        bool delayFlag = true;

        //  Odd loop, due to the fact that we need to iterate over a container which may
        //  change its content during our noodling around.  The idea is that we're going
        //  to process all of the RunInfo objects which the Exec knows about... as best we can.
        LSTRING runids;
        m_pExec->getRunids( &runids );
        for ( LITSTRING its = runids.begin(); its != runids.end(); ++its )
        {
            //  Retrieve the indicated RunInfo, if we can.
            //  If we cannot, then something has updated the Exec's list of RunInfos,
            //  and we should just move on to the next thing.
            RunInfo* pRunInfo = m_pExec->getRunInfo( *its, false );
            if ( pRunInfo )
            {
                ControlModeRunInfo* pCMRunInfo = dynamic_cast<ControlModeRunInfo*>( pRunInfo );
                if ( pCMRunInfo )
                {
                    //  Only do batch and demand RunInfo objects which are active AND in control mode
                    RunInfo::State runState = pRunInfo->getState();
                    if ( ((pRunInfo->isBatch()) || (pRunInfo->isDemand()))
                        && ( runState == RunInfo::STATE_ACTIVE )
                        && pRunInfo->inControlMode() )
                    {
                        //  Process the RunInfo object.  If we get true for a result, something useful
                        //  was done, and we should forego our normal wait at the end of our poll cycle,
                        //  since there might be something *else* we can immediately do.
                        pCMRunInfo->attach();
                        bool result = processRunInfo( pCMRunInfo );
                        pCMRunInfo->detach();
                        if ( result )
                            delayFlag = false;
                    }
                }
            }
        }

        if ( delayFlag )
            miscSleep( 100 );
    }
}



//  constructors, destructors

CoarseSchedulerActivity::CoarseSchedulerActivity
(
    Exec* const             pExec
)
:IntrinsicActivity( pExec, "CoarseSchedulerActivity", pExec->getRunInfo() )
{
}



//  public methods

//  dump()
//
//  IntrinsicActivity interface
//  For debugging
void
CoarseSchedulerActivity::dump
(
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
)
{
    stream << prefix << "CoarseSchedulerActivity" << std::endl;
    IntrinsicActivity::dump( stream, prefix + "  ", dumpBits );
}

