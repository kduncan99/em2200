//	CSKeyin.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//	Handles the CS keyin



#include	"execlib.h"



//	private / protected methods

//	isAllowed()
//
//	Returns true if this keyin is allowed
bool
CSKeyin::isAllowed() const
{
    return ( m_pExec->getStatus() == Exec::ST_RUNNING );
}


//	handler()
//
//	Called from Keyin base class worker() function
//
//  CS A
//  CS A {runid}
//  CS AD
//  CS ALL          removes master and individual holds set by CS H
//  CS AT
//  CS H
//  CS H {runid}
//  CS HD
//  CS HT
//  CS {run-id}*[PxDhhmmShhmmLyF]
void
CSKeyin::handler()
{
	if ( m_Option.size() > 0 )
    {
		displayOptionNotAllowed();
        return;
    }

    if ( m_Parameters.size() == 0 )
    {
        displayGeneralError();
        return;
    }

    if ( m_Parameters[0].compareNoCase( "A" ) == 0 )
        handleA();
    else if ( m_Parameters[0].compareNoCase( "AD" ) == 0 )
        handleAD();
    else if ( m_Parameters[0].compareNoCase( "ALL" ) == 0 )
        handleALL();
    else if ( m_Parameters[0].compareNoCase( "AT" ) == 0 )
        handleAT();
    else if ( m_Parameters[0].compareNoCase( "H" ) == 0 )
        handleH();
    else if ( m_Parameters[0].compareNoCase( "HD" ) == 0 )
        handleHD();
    else if ( m_Parameters[0].compareNoCase( "HT" ) == 0 )
        handleHT();
    else
        handleRunid();
}


//  handleA()
//
//  Handles CS A
void
CSKeyin::handleA()
{
    if ( m_Parameters.size() == 1 )
    {
        m_pExec->setHoldBatchRuns( false );
    }
    else if ( m_Parameters.size() == 2 )
    {
        RunInfo* pRunInfo = m_pExec->getRunInfo( m_Parameters[1], true );
        if ( pRunInfo == 0 )
        {
            std::string msg = m_Parameters[1] + " NOT FOUND";
            m_pConsoleManager->postReadOnlyMessage( msg, m_Routing, m_pExec->getRunInfo() );
            return;
        }

        if ( pRunInfo->getState() != RunInfo::STATE_IN_BACKLOG )
        {
            std::string msg = m_Parameters[1] + " NOT IN BACKLOG";
            m_pConsoleManager->postReadOnlyMessage( msg, m_Routing, m_pExec->getRunInfo() );
        }
        else
        {
            BatchRunInfo* pbri = dynamic_cast<BatchRunInfo*>( pRunInfo );
            pbri->setHold( false );
        }

        pRunInfo->detach();
    }
    else
    {
        displayGeneralError();
    }
}


//  handleAD()
//
//  Handles CS AD
void
CSKeyin::handleAD()
{
    if ( m_Parameters.size() != 1 )
        displayGeneralError();
    else
        m_pExec->setHoldDemandRuns( false );
}


//  handleALL()
//
//  Handles CS ALL
void
CSKeyin::handleALL()
{
    if ( m_Parameters.size() != 1 )
        displayGeneralError();
    else
    {
        m_pExec->setHoldBatchRuns( false );
        LSTRING runIds;
        m_pExec->getRunids( &runIds );
        for ( LCITSTRING itRunId = runIds.begin(); itRunId != runIds.end(); ++itRunId )
        {
            RunInfo* pRunInfo = m_pExec->getRunInfo( *itRunId, true );
            if ( pRunInfo )
            {
                BatchRunInfo* pbri = dynamic_cast<BatchRunInfo*>( pRunInfo );
                if ( pbri )
                    pbri->setHold( false );
                pRunInfo->detach();
            }
        }
    }
}


//  handleAT()
//
//  Handles CS AT
void
CSKeyin::handleAT()
{
    if ( m_Parameters.size() != 1 )
        displayGeneralError();
    else
        m_pExec->setHoldDemandTerminals( false );
}


//  handleH()
//
//  Handles CS H
void
CSKeyin::handleH()
{
    if ( m_Parameters.size() == 1 )
    {
        m_pExec->setHoldBatchRuns( true );
    }
    else if ( m_Parameters.size() == 2 )
    {
        RunInfo* pRunInfo = m_pExec->getRunInfo( m_Parameters[1], true );
        if ( pRunInfo == 0 )
        {
            std::string msg = m_Parameters[1] + " NOT FOUND";
            m_pConsoleManager->postReadOnlyMessage( msg, m_Routing, m_pExec->getRunInfo() );
            return;
        }

        if ( pRunInfo->getState() != RunInfo::STATE_IN_BACKLOG )
        {
            std::string msg = m_Parameters[1] + " NOT IN BACKLOG";
            m_pConsoleManager->postReadOnlyMessage( msg, m_Routing, m_pExec->getRunInfo() );
        }
        else
        {
            BatchRunInfo* pbri = dynamic_cast<BatchRunInfo*>( pRunInfo );
            pbri->setHold( true );
        }

        pRunInfo->detach();
    }
    else
    {
        displayGeneralError();
    }
}


//  handleHD()
//
//  Handles CS HD
void
CSKeyin::handleHD()
{
    if ( m_Parameters.size() != 1 )
        displayGeneralError();
    else
        m_pExec->setHoldDemandRuns( true );
}


//  handleHT()
//
//  Handles CS HT
void
CSKeyin::handleHT()
{
    if ( m_Parameters.size() != 1 )
        displayGeneralError();
    else
        m_pExec->setHoldDemandTerminals( true );
}


//  handleRunid()
//
//  Handles CS {run-id}*[PxDhhmmShhmmLyF]
void
CSKeyin::handleRunid()
{
    //TODO:BATCH implement this some day
}



// constructors / destructors

CSKeyin::CSKeyin
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

