//	RVKeyin.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//	Handles the RV keyin


/*  MESSAGES

    RV OF component IS NOT ALLOWED
    (Exec) The equipment type is incompatible with the keyin.

    RV OF component NOT PERFORMED - KEYIN ABORTED
    (Exec) You respond N to a previous message to perform an RV keyin.

    RV OF device NOT PERFORMED - PACK VERIFICATION ACTIVE
    (Exec) You tried to reserve (RV) a disk device while pack verification is occurring.
    Wait until pack verification is completed and try the keyin again.bool

    RV of component requested by requestor was received.
    (Exec) An internal request for component partitioning is being processed. The
    variable requestor is the partitioning requestor (fault recovery, SCF, or the operator).    lock();

*/



#include	"execlib.h"



//	private / protected methods

//	handler()
//
//	Called from Keyin base class worker() function
void
    RVKeyin::handler()
{
}


//  isAllowed()
bool
    RVKeyin::isAllowed() const
{
    Exec::Status execStatus = m_pExec->getStatus();
    return (execStatus == Exec::ST_BOOTING_1) || (execStatus == Exec::ST_RUNNING);
}



// constructors / destructors

RVKeyin::RVKeyin
(
    Exec* const                     pExec,
    const SuperString&              KeyinId,
    const SuperString&              Option,
    const std::vector<SuperString>& Parameters,
    const Word36&                   Routing
)
:FacilitiesKeyin( pExec, KeyinId, Option, Parameters, Routing )
{
}



//	public methods

