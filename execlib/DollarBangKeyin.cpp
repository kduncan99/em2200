//  DollarBangKeyin.cpp
//
//  Displays system date and time on the requesting console.



#include	"execlib.h"



//  private / protected methods

//  isAllowed()
//
//  Returns true if this keyin is allowed
bool
DollarBangKeyin::isAllowed() const
{
    return true;
}


//  handler()
//
//  Called from Keyin base class worker() function
void
DollarBangKeyin::handler()
{
    //  No validity checks - just do it.
    m_pExec->stopExec( Exec::SC_OPERATOR_KEYIN );
}



// constructors / destructors

DollarBangKeyin::DollarBangKeyin
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
