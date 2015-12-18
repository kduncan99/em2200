//	MSKeyin.cpp
//
//	Handles the DJ keyin



#include	"execlib.h"



//	private / protected methods

//	isAllowed()
//
//	Returns true if this keyin is allowed
bool
MSKeyin::isAllowed() const
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
void
MSKeyin::handler()
{
	if ( m_Option.size() > 0 )
		displayOptionNotAllowed();
	else if ( m_Parameters.size() > 0 )
		displayParametersNotAllowed();
	else
    {
        TRACK_COUNT accessible = 0;
        TRACK_COUNT available = 0;
        MFDManager* pmfdmgr = dynamic_cast<MFDManager*>(m_pExec->getManager( Exec::MID_MFD_MANAGER ));
        MFDManager::Result mfdResult = pmfdmgr->getFixedPoolTrackCounts( &accessible, &available );

        if ( mfdResult.m_Status == MFDManager::MFDST_SUCCESSFUL )
        {
            float roloutStart = m_pExec->getConfiguration().getFloatValue( "STDMSTRT" );
            float roloutGoal = m_pExec->getConfiguration().getFloatValue( "STDMSAVL" );
            TRACK_COUNT roloutStartTracks = static_cast<TRACK_COUNT>(accessible * (roloutStart / 100.0));
            TRACK_COUNT roloutGoalTracks = static_cast<TRACK_COUNT>(accessible * (roloutGoal / 100.0));

            std::stringstream strm;
            strm << "SUMMARY: STD FIXED TRACKS ACCESSIBLE   = " << accessible;
            m_pConsoleManager->postReadOnlyMessage( strm.str(), m_Routing, m_pExec->getRunInfo() );

            strm.str( "" );
            strm << "         STD FIXED TRACKS AVAILABLE    = " << available;
            m_pConsoleManager->postReadOnlyMessage( strm.str(), m_Routing, m_pExec->getRunInfo() );

            strm.str( "" );
            strm << "         STD ROLOUT START THRESHOLD    = " << roloutStartTracks;
            m_pConsoleManager->postReadOnlyMessage( strm.str(), m_Routing, m_pExec->getRunInfo() );

            strm.str( "" );
            strm << "         STD ROLOUT AVAILABILITY GOAL  = " << roloutGoalTracks;
            m_pConsoleManager->postReadOnlyMessage( strm.str(), m_Routing, m_pExec->getRunInfo() );
        }
    }
}



// constructors / destructors

MSKeyin::MSKeyin
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
