//	FFKeyin.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//	Handles the FF keyin



/*
Format
FF [dir-id#]qualifier*filename[(F-cycle)] [.]
where:
dir-id
is the directory of the file (STD or SHARED). If you do not specify the directory-id on
the keyin, all directories are assumed. The STD directory is displayed, followed by
the SHARED directory. If MHFS is down on the host, then the SHARED directory is
ignored.
qualifier
is the file qualifier, from 1 to 12 characters in length. This field may include the
characters A to Z, 0 to 9, -, and $.
filename
is the file name, from 1 to 12 characters in length. This field may include the
characters A to Z, 0 to 9, -, and $.
F-cycle
is the absolute F-cycle number, expressed as an octal integer (indicated by a leading
zero) with a range of 0 to 01747, or as a decimal integer with a range of 0 to 999. An
F-cycle of 0 is equivalent to no cycle. Relative F-cycle numbers are neither
recognized nor accepted.
If you do not specify an F-cycle, all cycles of the requested file are displayed (up to
32). The keyin displays the information ordered by F-cycle. All name sections with
the first file of the F-cycle set assigned are displayed first; then all name sections
with the second set, and so on. (In most cases, however, only one or two cycles
exist.)
You must specify at least the qualifier and file name when using the FF keyin.
*/


#include	"execlib.h"



//	private / protected methods

//  atEnd()
bool
FFKeyin::atEnd()
{
    return (m_Index >= m_Input.size());
}


//	isAllowed()
//
//	Returns true if this keyin is allowed
bool
FFKeyin::isAllowed() const
{
	switch ( m_pExec->getStatus() )
	{
	case Exec::ST_RUNNING:  return true;
    default:                return false;
	}
}


//	handler()
//
//	Called from Keyin base class worker() function
void
FFKeyin::handler()
{
	if ( m_Option.size() > 0 )
    {
        displayOptionNotAllowed();
        return;
    }

    if ( m_Parameters.size() != 1 )
    {
        displayGeneralError();
        return;
    }

    m_Input = m_Parameters[0];
    m_Input.foldToUpperCase();
    m_Index = 0;
    m_AbsoluteCycle = 0;

    if ( parseQualifier() != SUCCESSFUL )
    {
        displayGeneralError();
        return;
    }

    if ( parseAsterisk() != SUCCESSFUL )
    {
        displayGeneralError();
        return;
    }

    if ( parseFilename() != SUCCESSFUL )
    {
        displayGeneralError();
        return;
    }

    if (parseCycle() == SYNTAX_ERROR )
    {
        displayGeneralError();
        return;
    }

    parsePeriod();
    if ( !atEnd() )
    {
        displayGeneralError();
        return;
    }

    //  TODO:CNS Display any such files assigned to common-name sections
    bool foundEntry = false;

    //  Get list of RunIds from EXEC, then check the RunInfo objects for file assignments
    LSTRING runIdList;
    m_pExec->getRunids( &runIdList );
    for ( LCITSTRING its = runIdList.begin(); its != runIdList.end(); ++its )
    {
        //  Due to synchronicity, the Run might no longer be accessible...
        //  Try to get the pointer, and attach, then check whether we actually got it.
        RunInfo* pRunInfo = m_pExec->getRunInfo( *its, true );
        if ( pRunInfo )
        {
            //  Get container of FacilityItems, then iterate over each
            const FACITEMS facItems = pRunInfo->getFacilityItems();
            for ( CITFACITEMS itfi = facItems.begin(); itfi != facItems.end(); ++itfi )
            {
                //  Must be standard file, non-temporary, and Q*F match.
                const StandardFacilityItem* pFacItem = dynamic_cast<StandardFacilityItem*>( itfi->second );
                if ( pFacItem
                    && !pFacItem->getTemporaryFileFlag()
                    && ( m_Qualifier.compareNoCase( pFacItem->getQualifier() ) == 0 )
                    && ( m_Filename.compareNoCase( pFacItem->getFileName() ) == 0 ) )
                {
                    //  Possibly match absolute file cycle
                    if ( (m_AbsoluteCycle > 0) && (m_AbsoluteCycle != pFacItem->getAbsoluteFileCycle()) )
                        continue;

                    //  We have a match - display the result
                    foundEntry = true;
                    std::stringstream consStrm;
                    consStrm << "RUN:  " << pRunInfo->getActualRunId() << " HAS "
                            << m_Qualifier << "*" << m_Filename
                            << "(" << pFacItem->getAbsoluteFileCycle() << ")";
                    m_pConsoleManager->postReadOnlyMessage( consStrm.str(), m_Routing, m_pExec->getRunInfo() );
                }
            }

            pRunInfo->detach();
        }
    }

    //  Make sure user isn't left with no results whatsoever...
    if ( !foundEntry )
        m_pConsoleManager->postReadOnlyMessage( "NO ENTRIES FOUND FOR FF KEYIN", m_Routing, m_pExec->getRunInfo() );
}


//  parseAsterisk
FFKeyin::ParseResult
FFKeyin::parseAsterisk()
{
    if ( !atEnd() && (m_Input[m_Index] == '*') )
    {
        ++m_Index;
        return SUCCESSFUL;
    }

    return NOT_FOUND;
}


//  parseCycle
FFKeyin::ParseResult
FFKeyin::parseCycle()
{
    if ( atEnd() || (m_Input[m_Index] != '(') )
        return NOT_FOUND;
    m_AbsoluteCycle = 0;
    ++m_Index;

    COUNT digits = 0;
    UINT32 radix = 10;
    while (!atEnd())
    {
        char ch = m_Input[m_Index++];
        if ( ch == ')' )
            return SUCCESSFUL;
        if ( !isdigit( ch ) )
            return SYNTAX_ERROR;
        if ( (ch == '0') && (digits == 0) )
            radix = 8;
        m_AbsoluteCycle *= radix;
        m_AbsoluteCycle += ch - '0';
        if ( m_AbsoluteCycle > 999 )
            return SYNTAX_ERROR;
    }

    return SYNTAX_ERROR;
}


//  parseFilename
FFKeyin::ParseResult
FFKeyin::parseFilename()
{
    m_Filename.clear();

    while (!atEnd())
    {
        char ch = m_Input[m_Index];
        if ( !isalpha(ch) && !isdigit(ch) && (ch != '-') && (ch != '$') )
            break;
        ++m_Index;
        m_Filename += ch;
    }

    return ( m_Filename.size() > 0 ) ? SUCCESSFUL : NOT_FOUND;
}


//  parsePeriod
FFKeyin::ParseResult
FFKeyin::parsePeriod()
{
    if ( !atEnd() && (m_Input[m_Index] == '.') )
    {
        ++m_Index;
        return SUCCESSFUL;
    }

    return NOT_FOUND;
}


//  parseQualifier
FFKeyin::ParseResult
FFKeyin::parseQualifier()
{
    m_Qualifier.clear();

    while (!atEnd())
    {
        char ch = m_Input[m_Index];
        if ( !isalpha(ch) && !isdigit(ch) && (ch != '-') && (ch != '$') )
            break;
        ++m_Index;
        m_Qualifier += ch;
    }

    return ( m_Qualifier.size() > 0 ) ? SUCCESSFUL : NOT_FOUND;
}



// constructors / destructors

FFKeyin::FFKeyin
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

