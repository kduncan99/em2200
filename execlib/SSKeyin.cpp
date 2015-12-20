//	SSKeyin.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//	Handles the SS keyin



#include	"execlib.h"



//	private / protected methods

//	isAllowed()
//
//	Returns true if this keyin is allowed
bool
SSKeyin::isAllowed() const
{
	switch ( m_pExec->getStatus() )
	{
    case Exec::ST_SYS:
	case Exec::ST_RUNNING:
		return true;
    default:
        return false;
	}
}


//	handler()
//
//	Called from Keyin base class worker() function
//
//  STATUS: hhmmss OMSG nnnn, STRT nnn, DDLN nnn
//  HELD: OPER nnnn, TAPE nnnn, MASS nnnn, SOPT nnn
//  ACTV: OPEN nnnn, ITLD nnnn SITE: PART-A HOST: A
//  IP: BAT xx% DEM xx% R/T xx% TIP xx% idle
//  OPEN: BAT nnnn DEM nnnn UNOPND: nnnn HOLDS holds
//  PAGE IO/M: nnnn MASS: nnnnnnnn KWMEM: nnnnnnnn TIME: hh:mm:ss zone
//  EXP: xx%-nnnnnn E11:nnn ID: level TIP: fnnnnnn/r Q: nnnn A: nnnn
void
SSKeyin::handler()
{
	if ( m_Option.size() > 0 )
		displayOptionNotAllowed();
	else if ( m_Parameters.size() > 0 )
		displayParametersNotAllowed();
	else
    {
        std::stringstream strm;
        SystemTime* pExecTime = SystemTime::createFromMicroseconds( m_pExec->getExecTime() );

        COUNT readReplyCount = m_pConsoleManager->getReadReplyCount();
        COUNT runsWaitingOnStartTime = 0;
        COUNT unopenedDeadlineRuns = 0;
        COUNT operationsHoldRuns = 0;
        COUNT tapeHoldRuns = 0;     //  Can we get this (and the next one) from Facilities?
        COUNT diskHoldRuns = 0;
        COUNT sOptionHoldRuns = 0;
        COUNT activeRuns = 0;
        COUNT initialLoadRuns = 0;

        m_pExec->getStatusCounters( &runsWaitingOnStartTime,
                                    &unopenedDeadlineRuns,
                                    &operationsHoldRuns,
                                    &sOptionHoldRuns,
                                    &activeRuns,
                                    &initialLoadRuns );
        strm << "STATUS: "
            << std::dec << std::setw( 2 ) << std::setfill( '0' ) << pExecTime->getHour()
            << std::dec << std::setw( 2 ) << std::setfill( '0' ) << pExecTime->getMinute()
            << std::dec << std::setw( 2 ) << std::setfill( '0' ) << pExecTime->getSecond()
            << " OMSG " << std::dec << std::setw( 4 ) << std::setfill( '0' ) << readReplyCount
            << ", STRT " << std::dec << std::setw( 4 ) << std::setfill( '0' ) << runsWaitingOnStartTime
            << ", DDLN " << std::dec << std::setw( 4 ) << std::setfill( '0' ) << unopenedDeadlineRuns;
        m_pConsoleManager->postReadOnlyMessage( strm.str(), m_Routing, m_pExec->getRunInfo() );

        strm.str( "" );
        strm << "HELD: OPER " << std::dec << std::setw( 4 ) << std::setfill( '0' ) << operationsHoldRuns
            << ", TAPE " << std::dec << std::setw( 4 ) << std::setfill( '0' ) << tapeHoldRuns
            << ", MASS " << std::dec << std::setw( 4 ) << std::setfill( '0' ) << diskHoldRuns
            << ", SOPT " << std::dec << std::setw( 4 ) << std::setfill( '0' ) << sOptionHoldRuns;
        m_pConsoleManager->postReadOnlyMessage( strm.str(), m_Routing, m_pExec->getRunInfo() );

        const std::string& siteId = m_pExec->getConfiguration().getStringValue( "IDS" );
        strm.str( "" );
        strm << "ACTV: OPEN " << std::dec << std::setw( 4 ) << std::setfill( '0' ) << activeRuns
            << ", ITLD " << std::dec << std::setw( 4 ) << std::setfill( '0' ) << initialLoadRuns
            << " SITE: " << siteId;
        m_pConsoleManager->postReadOnlyMessage( strm.str(), m_Routing, m_pExec->getRunInfo() );

        m_pConsoleManager->postReadOnlyMessage( getStatusLine1(), m_Routing, m_pExec->getRunInfo() );
        m_pConsoleManager->postReadOnlyMessage( getStatusLine2( m_pExec, m_pConsoleManager ),
                                                m_Routing,
                                                m_pExec->getRunInfo() );
        m_pConsoleManager->postReadOnlyMessage( getStatusLine3( m_pExec ),
                                                m_Routing,
                                                m_pExec->getRunInfo() );
        m_pConsoleManager->postReadOnlyMessage( getStatusLine4(), m_Routing, m_pExec->getRunInfo() );

        delete pExecTime;
        pExecTime = 0;
    }
}



// constructors / destructors

SSKeyin::SSKeyin
(
    Exec* const                     pExec,
    const SuperString&              KeyinId,
    const SuperString&              Option,
    const std::vector<SuperString>& Parameters,
    const Word36&                   Routing
)
:KeyinActivity( pExec, KeyinId, Option, Parameters, Routing )
{
}



//	public methods



//  public statics

//  getStatusLine1()
//
//  Generates text for status line 1 (line 4 of SS keyin, or first line of first iteration of console status messages)
//      IP: BAT xx% DEM xx% R/T xx% TIP xx% idle
std::string
SSKeyin::getStatusLine1()
{
    //TODO:TASK revisit this when the IPs can report this
    return "IP: BAT 0% DEM 0% R/T 0% TIP 0% IDLE";
}


//  getStatusLine2()
//
//  Generates text for status line 2 (line 5 of SS keyin, or second line of first iteration of console status messages)
//      OPEN: BAT nnnn DEM nnnn UNOPND: nnnn HOLDS holds
std::string
SSKeyin::getStatusLine2
(
    Exec* const             pExec,
    ConsoleManager* const   pConsoleManager
)
{
    COUNT batchRunCount = 0;
    COUNT demandRunCount = 0;
    COUNT backlogRunCount = 0;
    pExec->getRunCounters( &batchRunCount, &demandRunCount, &backlogRunCount );

    bool aFlag = pExec->areBatchRunsHeld();
    bool dFlag = pExec->areDemandRunsHeld();
    bool iFlag = false;//at least one run is suspended
    bool lFlag = pExec->areDemandTerminalsHeld();
    bool mFlag = false;// mass storage or cataloged file not available
    bool oFlag = pConsoleManager->getReadReplyCount() > 0;
    bool rFlag = false;// operator hold on a run
    bool sFlag = false;// s-option hold
    bool tFlag = false;// tape or other peripheral not available

    std::stringstream strm;
    strm << "OPEN: BAT " << batchRunCount
        << " DEM " << demandRunCount
        << " UNOPND: " << backlogRunCount
        << " HOLDS ";
    if ( aFlag )        strm << "A";
    if ( dFlag )        strm << "D";
    if ( iFlag )        strm << "I";
    if ( lFlag )        strm << "L";
    if ( mFlag )        strm << "M";
    if ( oFlag )        strm << "O";
    if ( rFlag )        strm << "R";
    if ( sFlag )        strm << "S";
    if ( tFlag )        strm << "T";

    return strm.str();
}


//  getStatusLine3()
//
//  Generates text for status line 3 (line 6 of SS keyin, or first line of second iteration of console status messages)
//      PAGE IO/M: nnnn MASS: nnnnnnnn KWMEM: nnnnnnnn TIME: hh:mm:ss zone
std::string
SSKeyin::getStatusLine3
(
    Exec* const         pExec
)
{
    COUNT64 kwAvailable = miscGetAvailableMemory() / 8192;
    TRACK_COUNT accessibleTracks;
    TRACK_COUNT availableTracks;
    MFDManager* pmfdmgr = dynamic_cast<MFDManager*>( pExec->getManager( Exec::MID_MFD_MANAGER ) );
    pmfdmgr->getFixedPoolTrackCounts( &accessibleTracks, &availableTracks );

    SystemTime* pLocalTime = SystemTime::createFromMicroseconds( pExec->getExecTime() );
    std::stringstream strm;
    strm << "PAGE IO/M: n/a MASS: " << availableTracks << " KWMEM: " << kwAvailable
        << " TIME: " << std::dec << std::setw( 2 ) << std::setfill( '0' ) << pLocalTime->getHour()
        << ":" << std::dec << std::setw( 2 ) << std::setfill( '0' ) << pLocalTime->getMinute()
        << ":" << std::dec << std::setw( 2 ) << std::setfill( '0' ) << pLocalTime->getSecond();
    delete pLocalTime;
    pLocalTime = 0;

    return strm.str();
}


//  getStatusLine4()
//
//  Generates text for status line 4 (line 7 of SS keyin, or second line of second iteration of console status messages)
//      EXP: xx%-nnnnnn E11:nnn ID: level TIP: fnnnnnn/r Q: nnnn A: nnnn
std::string
SSKeyin::getStatusLine4()
{
    COUNT64 available = miscGetAvailableMemory();
    COUNT64 total = miscGetTotalMemory();
    COUNT64 availPercent = available * 100 / total;

    std::stringstream strm;
    strm << "EXP: " << availPercent << "%-" << (available / 8)
        << " E11:n/a ID: " << VERSION << "-PreRelease"
        << " TIP: 0/M Q: 0 A: 0";

    return strm.str();
}


