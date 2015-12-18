//  JumpKeyKeyin Intermediate class implementation



#include    "execlib.h"



//  private, protected methods

//	displayJumpKeys()
//
//	Displays jump keys which are set - used by several jump-key-related keyins
//
void
    JumpKeyKeyin::displayJumpKeys() const
{
    std::vector<bool> jumpKeys;
    m_pExec->getJumpKeys( &jumpKeys );

    std::stringstream strm;
    strm << "Jump keys set: ";
    bool foundAny = false;
    bool comma = false;
    for ( INDEX jkx = 0; jkx < 35; ++jkx )
    {
        if ( jumpKeys[jkx] )
        {
            if ( strm.str().size() > 72 )
            {
                m_pConsoleManager->postReadOnlyMessage( strm.str(), m_Routing, m_pExec->getRunInfo() );
                strm.str() = "";
                strm << "               ";
                comma = false;
            }

            if ( comma )
                strm << ", ";

            strm << std::setw( 2 ) << std::setfill( '0' ) << (jkx + 1);
            comma = true;
            foundAny = true;
        }
    }

    if ( !foundAny )
        strm << "NONE";

    m_pConsoleManager->postReadOnlyMessage( strm.str(), m_Routing, m_pExec->getRunInfo() );
}



//  constructors, destructors

JumpKeyKeyin::JumpKeyKeyin
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

