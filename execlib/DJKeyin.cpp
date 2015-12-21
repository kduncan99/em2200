//	DJKeyin.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//	Handles the DJ keyin



#include	"execlib.h"



//	private / protected methods

//	isAllowed()
//
//	Returns true if this keyin is allowed
bool
DJKeyin::isAllowed() const
{
    //  Always allowed
	return true;
}


//	handler()
//
//	Called from Keyin base class worker() function
void
DJKeyin::handler()
{
	if ( m_Option.size() > 0 )
		displayOptionNotAllowed();
	else if ( m_Parameters.size() > 0 )
		displayParametersNotAllowed();
	else
		displayJumpKeys();
}



// constructors / destructors

DJKeyin::DJKeyin
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
