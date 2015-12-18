//	FacilitiesKeyin.cpp
//
//	Functions common to two or more facilities keyins



#include	"execlib.h"



//  statics

COUNT                   FacilitiesKeyin::m_StaticObjectCount = 0;
std::recursive_mutex    FacilitiesKeyin::m_StaticLock;



//	private / protected methods

//  generateOutput()
//
//  Generates output to the requesting console for the given list of NodeEntry objects.
void
    FacilitiesKeyin::generateOutput
    (
    NODEENTRYSET&       nodes
    ) const
{
    LSTRING output;
    std::string pendingText;

    for ( CITNODEENTRYSET itne = nodes.begin(); itne != nodes.end(); ++itne )
    {
        bool shortFlag;
        std::string text;
        if ( generateOutputForEntry( *itne, &text, &shortFlag ) )
        {
            if ( shortFlag )
            {
                if ( pendingText.empty() )
                {
                    pendingText = text;
                }
                else
                {
                    pendingText.resize( 20, ' ' );
                    pendingText += text;
                    output.push_back( pendingText );
                    pendingText.clear();
                }
            }
            else
            {
                if ( !pendingText.empty() )
                {
                    output.push_back( pendingText );
                    pendingText.clear();
                }

                output.push_back( text );
            }
        }
    }

    if ( !pendingText.empty() )
        output.push_back( pendingText );

    for ( LCITSTRING ito = output.begin(); ito != output.end(); ++ito )
        m_pConsoleManager->postReadOnlyMessage( *ito, m_Routing, m_pExec->getRunInfo() );
}


//  generateOutputForDisk()
//
//  Generates a line (or short line) of output describing the status of the given disk.
//  Format:
//      {name} ' ' {state} [' NA'] [ [' *'] [' R' | ' F'] ' PACKID ' {packId} ]
bool
    FacilitiesKeyin::generateOutputForDisk
    (
    const DeviceManager::NodeEntry* const   pNodeEntry,
    std::string* const                      pOutput,
    bool* const                             pShortFlag
    ) const
{
    *pOutput = pNodeEntry->m_pNode->getName() + " ";
    *pOutput += m_pDeviceManager->getNodeStatusAndAvailabilityString( pNodeEntry->m_NodeId );
    *pShortFlag = true;

    //  Is there a pack mounted?  If so, we're doing the long format output
    const MFDManager::PackInfo* pPackInfo = m_pMFDManager->getPackInfo( pNodeEntry->m_NodeId );
    if ( pPackInfo != 0 )
    {
        if ( pPackInfo->m_AssignCount > 0 )
            *pOutput += " *";
        *pOutput += ( pPackInfo->m_IsFixed ? " F" : " R" );
        *pOutput += " PACKID " + pPackInfo->m_PackName;
        *pShortFlag = false;
    }

    return true;
}


//  generateOutputForEntry()
//
//  Generates a line (or short line) of output describing the status of the given NodeEntry object
bool
    FacilitiesKeyin::generateOutputForEntry
    (
    const DeviceManager::NodeEntry* const   pNodeEntry,
    std::string* const                      pOutput,
    bool* const                             pShortFlag
    ) const
{
    if ( pNodeEntry->isDevice() )
    {
        const Device* pDevice = reinterpret_cast<const Device*>( pNodeEntry->m_pNode);
        switch ( pDevice->getDeviceType() )
        {
        case Device::DeviceType::DISK:
            return generateOutputForDisk( pNodeEntry, pOutput, pShortFlag );

        case Device::DeviceType::SYMBIONT:
            break;//TODO:SYMB

        case Device::DeviceType::TAPE:
            break;//TODO:TAPE

        default:
            //  nonsense cases
            break;
        }
    }

    //  Fall through for everything which is not more-specific.
    //  Show status and availability, and nothing else.
    *pOutput = pNodeEntry->m_pNode->getName()
        + " "
        + m_pDeviceManager->getNodeStatusAndAvailabilityString( pNodeEntry->m_NodeId );
    *pShortFlag = true;
    return true;
}


//  getDownstreamNodes()
//
//  Populates a set of NodeEntry pointers representing devices which are downstream from the given parent node.
//  Designed for recursion and calling for multiple parents, for deriving a set of devices affected by some
//  operation on a collection of parent nodes.
//
//  Note that, per its intended use, this function does NOT clear out the devices set before processing.
//
//  Callable with IOP, CM, or CU parent nodes (in fact, the higher level parents result in recursive calls with
//  the lower level parents).  Do not call with device nodes, although since devices don't have children,
//  this would produce an effective NOP.
//
//  Returns:
//      true generally; false if something is wrong with internal tables
bool
    FacilitiesKeyin::getDownstreamNodes
    (
    const DeviceManager::NodeEntry* const   pParentNode,
    NODEENTRYSET* const                     pNodes
    ) const
{
    const DeviceManager::NODEIDMAP& children = pParentNode->m_ChildNodeIds;
    for ( DeviceManager::CITNODEIDMAP itChild = children.begin(); itChild != children.end(); ++itChild )
    {
        const DeviceManager::NodeEntry* pChild = m_pDeviceManager->getNodeEntry( itChild->second );
        if ( pChild == 0 )
        {
            std::stringstream strm;
            strm << "FacilitiesKeyin::getDownstreamDevices() cannot resolve node id " << itChild->second;
            SystemLog::write( strm.str() );
            return false;
        }

        pNodes->insert( pChild );
        getDownstreamNodes( pChild, pNodes );
    }

    return true;
}


//  notifyDoesNotExist()
//
//  Sends canned message when something the operator asked for doesn't exist in the MCT
void
    FacilitiesKeyin::notifyDoesNotExist
    (
    const std::string&      componentName
    ) const
{
    std::string msg = m_KeyinId + " KEYIN - " + componentName + " DOES NOT EXIST, INPUT IGNORED";
    m_pConsoleManager->postReadOnlyMessage( msg, m_Routing, m_pExec->getRunInfo() );
}


//  notifyInternalError()
//
//  Sends canned message when an internal error aborts the keyin
void
    FacilitiesKeyin::notifyInternalError
    (
    const DeviceManager::NodeEntry* const   pNodeEntry
    ) const
{
    std::string msg = m_KeyinId;
    msg += " OF ";
    msg += pNodeEntry->m_pNode->getName();
    msg += " FAILED DUE TO INTERNAL DATA CORRUPTION";
    m_pConsoleManager->postReadOnlyMessage( msg, m_Routing, m_pExec->getRunInfo() );
}


//  notifyKeyinAborted()
//
//  Sends canned message when operator decides not to proceed with the keyin
void
    FacilitiesKeyin::notifyKeyinAborted
    (
    const DeviceManager::NodeEntry* const   pNodeEntry
    ) const
{
    std::string msg = m_KeyinId;
    msg += " OF ";
    msg += pNodeEntry->m_pNode->getName();
    msg += " NOT PERFORMED - KEYIN ABORTED";
    m_pConsoleManager->postReadOnlyMessage( msg, m_Routing, m_pExec->getRunInfo() );
}


//  notifyKeyinAlreadyPerformed()
//
//  Sends canned message when operator tries to set a node state and the node is already in that state.
void
    FacilitiesKeyin::notifyKeyinAlreadyPerformed
    (
    const DeviceManager::NodeEntry* const   pNodeEntry
    ) const
{
    std::string msg = m_KeyinId;
    msg += " KEYIN - ALREADY PERFORMED FOR ";
    msg += pNodeEntry->m_pNode->getName();
    m_pConsoleManager->postReadOnlyMessage( msg, m_Routing, m_pExec->getRunInfo() );
}


//  notifyKeyinNotAllowed()
//
//  Sends canned message indicating that the requested keyin is not allowed
void
    FacilitiesKeyin::notifyKeyinNotAllowed
    (
    const DeviceManager::NodeEntry* const   pNodeEntry
    ) const
{
    std::string msg = m_KeyinId;
    msg += " OF ";
    msg += pNodeEntry->m_pNode->getName();
    msg += " IS NOT ALLOWED";
    m_pConsoleManager->postReadOnlyMessage( msg, m_Routing, m_pExec->getRunInfo() );
}


//  pollOperator()
//
//  Asks the operator a yes/no question, and returns the result to the caller
FacilitiesKeyin::PollResult
    FacilitiesKeyin::pollOperator
    (
    const std::string&  prompt
    ) const
{
    while ( true )
    {
        ConsoleManager::ReadReplyRequest request( m_pExec->getRunInfo(), m_Routing, ConsoleManager::Group::SYSTEM, prompt, 1 );
        m_pConsoleManager->postReadReplyMessage( &request, true );
        if ( request.isCancelled() )
            return PR_CANCELED;
        else if ( request.getResponse().compareNoCase( "Y" ) == 0 )
            return PR_YES;
        else if ( request.getResponse().compareNoCase( "N" ) == 0 )
            return PR_NO;
    }
}



// constructors / destructors

FacilitiesKeyin::FacilitiesKeyin
(
    Exec* const                     pExec,
    const SuperString&              KeyinId,
    const SuperString&              Option,
    const std::vector<SuperString>& Parameters,
    const Word36&                   Routing
)
:KeyinActivity( pExec, KeyinId, Option, Parameters, Routing )
{
    m_pDeviceManager = dynamic_cast<DeviceManager*>(m_pExec->getManager( Exec::MID_DEVICE_MANAGER ));
    m_pMFDManager = dynamic_cast<MFDManager*>(m_pExec->getManager( Exec::MID_MFD_MANAGER ));

    lock();
    ++m_StaticObjectCount;
    unlock();
}

