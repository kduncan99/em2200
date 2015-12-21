//	CJKeyin.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//	Handles the CJ keyin



#include	"execlib.h"



//  private / protected methods

//  isAllowed()
//
//  Returns true if this keyin is allowed
bool
CJKeyin::isAllowed() const
{
    switch ( m_pExec->getStatus() )
    {
    case Exec::ST_BOOTING_1:
    case Exec::ST_RUNNING:
        return true;
    default:
            return false;
    }
}


//  handler()
//
//  Called from Keyin base class worker() function
void
CJKeyin::handler()
{
    //  We want either 'ALL' as an option, or a list of jumpkeys as an argument.
    std::set<BYTE> jumpKeys;

    if ( m_Option.size() > 0 )
    {
        if ( (m_Option.compareNoCase( "A" ) != 0) && (m_Option.compareNoCase( "ALL" ) != 0) )
        {
            displayInvalidOption();
            return;
        }
        for ( BYTE jk = 1; jk <= 36; ++jk )
            jumpKeys.insert( jk );
    }

    else if ( m_Parameters.size() == 0 )
    {
        displayNoParameters();
        return;
    }

    else
    {
        for ( INDEX px = 0; px < m_Parameters.size(); ++px )
        {
            BYTE jk = atoi( m_Parameters[px].c_str() );
            if ( (jk < 1) || (jk > 36) )
            {
                displayInvalidParameter();
                return;
            }
            jumpKeys.insert( jk );
        }
    }

    for ( std::set<BYTE>::const_iterator itjk = jumpKeys.begin(); itjk != jumpKeys.end(); ++itjk )
        m_pExec->setJumpKey( *itjk, false );
    displayJumpKeys();
}



//  constructors / destructors

CJKeyin::CJKeyin
(
    Exec* const                     pExec,
    const SuperString&              KeyinId,
    const SuperString&              Option,
    const std::vector<SuperString>& Parameters,
    const Word36&                   Routing
)
:JumpKeyKeyin( pExec, KeyinId, Option, Parameters, Routing )
{
}



//	public methods

