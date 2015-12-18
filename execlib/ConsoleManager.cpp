//	ConsoleManager.cpp
//
//	Implementation of ConsoleManager class



#include	"execlib.h"



//  statics
const char* ConsoleManager::m_GroupSystemString             = "SYSMSG";
const char* ConsoleManager::m_GroupIOActivityString         = "IOMSG";
const char* ConsoleManager::m_GroupCommunicationsString     = "COMMSG";
const char* ConsoleManager::m_GroupHardwareConfidenceString = "HWMSG";

static const Word36 MainConsoleRouting( 01 );



//  private methods

//  pollInterfaces()
//
//  Checks all registered consoles to see whether there is any input to be processed.
//  If we find something, we process it and return true.  We return false if there is nothing to be done.
//  Call under lock().
bool
ConsoleManager::pollInterfaces()
{
    for ( ITCONSOLES itc = m_Consoles.begin(); itc != m_Consoles.end(); ++itc )
    {
        std::string unsolicited;
        if ( itc->second->pollUnsolicitedMessage( &unsolicited ) )
        {
            SystemLog::write( "Cons UM:" + unsolicited );
            m_pExec->createKeyin( itc->first, unsolicited );
            return true;
        }

        std::string reply;
        ConsoleInterface::MESSAGE_ID messageId;
        if ( itc->second->pollReadReplyMessage( &messageId, &reply ) )
        {
            //  Find the associated read-reply request
            for ( ITREADREPLYREQUESTS itrr = m_ReadReplyRequests.begin(); itrr != m_ReadReplyRequests.end(); ++itrr )
            {
                if ( ((*itrr)->getCurrentConsole() == itc->second)
                      && ((*itrr)->getMessageId() == messageId) )
                {
                    (*itrr)->setResponse( reply );
                    std::stringstream strm;
                    strm << "Cons RP (" << (*itrr) << "):" << reply;
                    SystemLog::write( strm.str() );

                    //  Log the reply to the RunInfo console message log.
                    //  It is theoretically possible that there isn't a RunInfo pointer.  Very unlikely, but possible.
                    RunInfo* pRunInfo = (*itrr)->m_pRunInfo;
                    if ( pRunInfo )
                    {
                        std::stringstream strm;
                        strm << (*itrr)->getMessageId() << " " << (*itrr)->getResponse();

                        (*itrr)->m_pRunInfo->attach();
                        postToConsoleLog( pRunInfo, strm.str() );
                        (*itrr)->m_pRunInfo->detach();
                    }

                    //  Set the completed flag, and release the thing
                    (*itrr)->setCompleted( true );
                    m_ReadReplyRequests.erase( itrr );
                    return true;
                }
            }
        }
    }

    return false;
}


//  pollReadOnly()
//
//  Checks the pending read-only request queues to see if there's anything that needs to be sent to any consoles.
//  If there's anything to do, it does one thing, then returns true.  If there's nothing to do, it returns false.
//  Call under lock().
bool
ConsoleManager::pollReadOnly()
{
    //  Check pending read-only messages.  Note that this departs from conventional Exec behavior.
    //  The real Exec holds onto undeliverable messages, retrying them every 30 seconds EVEN IF they succeed
    //  when sent to other devices.  This was due in large part, to pagewriters which could be offline for awhile
    //  when paper ran out.  We have no such worries; if we can send a read-only message to even one console, we're done.
    for ( ITREADONLYREQUESTS itro = m_ReadOnlyRequests.begin(); itro != m_ReadOnlyRequests.end(); ++itro )
    {
        //  Does the message have a specific routing, and does that routing represent a registered console?
        if ( (*itro)->m_Routing.getValue() != 0 )
        {
            CITCONSOLES itc = m_Consoles.find( (*itro)->m_Routing );
            if ( itc != m_Consoles.end() )
            {
                if ( itc->second->postReadOnlyMessage( (*itro)->m_Message ) )
                {
                    //  Message delivered - we're done
                    m_ReadOnlyRequests.erase( itro );
                    return true;
                }
            }
        }

        //  That didn't work - check the group; send to all the consoles registered for that group.
        //  If at least one of them succeeds, we're happy.
        //  The request object guarantees a valid group value.
        ITREADONLYGROUPASG itga = m_ReadOnlyGroupAssignments.find((*itro)->m_Group);
        if ( itga != m_ReadOnlyGroupAssignments.end() )
        {
            bool success = false;
            for ( std::list<ConsoleInterface*>::const_iterator itc = itga->second.begin(); itc != itga->second.end(); ++itc )
            {
                if ( (*itc)->postReadOnlyMessage( (*itro)->m_Message ) )
                    success = true;
            }

            if ( success )
            {
                //  Message delivered - we're done
                m_ReadOnlyRequests.erase( itro );
                return true;
            }
        }

        //  So, that message could not be delivered.  Skip it and move on to the next.
        //  This can happen only if there are no system consoles currently logged in, as we automatically
        //  reset read-only group assignments such that each group is always assigned to at least one console.
        //  Anyway, we'll be back here again soon to retry this.
        ++itro;
    }

    return false;
}


//  pollReadReply()
//
//  Checks the pending read-reply request queues to see if there's anything that needs to be sent to any consoles.
//  If there's anything to do, it does one thing, then returns true.  If there's nothing to do, it returns false.
bool
ConsoleManager::pollReadReply()
{
    //  Check pending read-reply messages
    for ( ITREADREPLYREQUESTS itrr = m_ReadReplyRequests.begin(); itrr != m_ReadReplyRequests.end(); ++itrr )
    {
        //  Is this assigned to a console?  If not, find one and send it.
        if ( (*itrr)->getCurrentConsole() == 0 )
        {
            ConsoleInterface* pChosenConsole = 0;

            //  Does the message have a specific routing, and does that routing represent a registered console?
            if ( (*itrr)->m_Routing.getValue() != 0 )
            {
                CITCONSOLES itc = m_Consoles.find( (*itrr)->m_Routing );
                if ( itc != m_Consoles.end() )
                    pChosenConsole = itc->second;
            }

            //  We don't have a console yet - so go find the read-reply console for the indicated group.
            //  The request object guarantees a valid group value.
            if ( !pChosenConsole )
                pChosenConsole = m_ReadReplyGroupAssignments[(*itrr)->m_Group];

            //  Route the message to the console, but only if it is not 'full'
            if ( pChosenConsole->getOutstandingMessageCount() < pChosenConsole->getOutstandingMessageLimit() )
            {
                ConsoleInterface::MESSAGE_ID messageId;
                if ( pChosenConsole->postReadReplyMessage( (*itrr)->m_Message, (*itrr)->m_MaxResponseLength, &messageId ) )
                {
                    //  If it was accepted, update the request to reflect the assignment,
                    //  then post an entry to the RunInfo console log.
                    //  It is theoretically possible that there isn't a RunInfo pointer.  Very unlikely, but possible.
                    (*itrr)->setCurrentConsole( pChosenConsole );
                    (*itrr)->setMessageId( messageId );

                    RunInfo* pRunInfo = (*itrr)->m_pRunInfo;
                    if ( pRunInfo )
                    {
                        std::stringstream strm;
                        strm << (*itrr)->getMessageId() << "-" << (*itrr)->m_Message;
                        (*itrr)->m_pRunInfo->attach();
                        postToConsoleLog( pRunInfo, strm.str() );
                        (*itrr)->m_pRunInfo->detach();
                    }
                }
            }
        }
    }

    return false;
}


//  postToConsoleLog()
//
//  We get messages from all RunInfo's, including Exec's.
//  We need to post them to only NonExecRunInfo console logs (because Exec doesn't have one)
inline void
ConsoleManager::postToConsoleLog
(
    RunInfo* const          pRunInfo,
    const std::string&      message
) const
{
    //  Post to RunInfo
    UserRunInfo* puri = dynamic_cast<UserRunInfo*>( pRunInfo );
    if ( puri )
    {
        puri->attach();
        puri->postToConsoleLog( message );
        puri->detach();
    }
}



//  constructors, destructors

ConsoleManager::ConsoleManager
(
    Exec* const         pExec
)
:ExecManager( pExec )
{
    //  No main console yet
    m_pMainConsole = 0;

    //  Create empty lists for all R/O group assignment groups
    m_ReadOnlyGroupAssignments[Group::SYSTEM];
    m_ReadOnlyGroupAssignments[Group::IO_ACTIVITY];
    m_ReadOnlyGroupAssignments[Group::COMMUNICATIONS];
    m_ReadOnlyGroupAssignments[Group::HARDWARE_CONFIDENCE];

    //  Create empty maps for all R/R group assignment groups
    m_ReadReplyGroupAssignments[Group::SYSTEM] = 0;
    m_ReadReplyGroupAssignments[Group::IO_ACTIVITY] = 0;
    m_ReadReplyGroupAssignments[Group::COMMUNICATIONS] = 0;
    m_ReadReplyGroupAssignments[Group::HARDWARE_CONFIDENCE] = 0;
}


ConsoleManager::~ConsoleManager()
{
    //  Clear out requests which might be left over from a previous session
    while ( !m_ReadOnlyRequests.empty() )
    {
        delete m_ReadOnlyRequests.back();
        m_ReadOnlyRequests.pop_back();
    }

    //  We don't delete these, since we didn't create them.
    m_ReadReplyRequests.clear();
    m_Consoles.clear();
}



//  public methods

//  dump()
//
//  For debugging
void
ConsoleManager::dump
(
    std::ostream&       stream,
    const DUMPBITS      dumpBits
)
{
    lock();

    stream << "ConsoleManager ----------" << std::endl;
    stream << "  Registered Consoles:" << std::endl;
    for ( CITCONSOLES itc = m_Consoles.begin(); itc != m_Consoles.end(); ++itc )
    {
        const ConsoleInterface* const pci = itc->second;
        stream << "    " << pci->getName() << "  Routing=" << itc->first.toOctal() << std::endl;
    }

    stream << "  Read-Only Group Assignments:" << std::endl;
    for ( CITREADONLYGROUPASG ita = m_ReadOnlyGroupAssignments.begin(); ita != m_ReadOnlyGroupAssignments.end(); ++ita )
    {
        std::string list = "";
        for ( std::list<ConsoleInterface*>::const_iterator itc = ita->second.begin(); itc != ita->second.end(); ++itc )
            list += (*itc)->getName() + " ";

        if ( list.size() == 0 )
            list = "<none>";
        stream << "    Group " << getGroupName(ita->first) << ": " << list << std::endl;
    }

    stream << "  Read-Reply Group Assignments:" << std::endl;
    for ( CITREADREPLYGROUPASG ita = m_ReadReplyGroupAssignments.begin(); ita != m_ReadReplyGroupAssignments.end(); ++ita )
    {
        std::string name = ( ita->second ) ? ita->second->getName() : "<none>";
        stream << "    Group " << getGroupName(ita->first) << ": " << name << std::endl;
    }

    stream << "  (Undelivered) ReadOnly table:" << std::endl;
    for ( CITREADONLYREQUESTS itReq = m_ReadOnlyRequests.begin(); itReq != m_ReadOnlyRequests.end(); ++itReq )
    {
        const ReadOnlyRequest* pReq = *itReq;
        stream << "    runid=" << (pReq->m_pRunInfo ? pReq->m_pRunInfo->getActualRunId() : "<?>")
            << " route=" << pReq->m_Routing.toFieldata()
            << " group=" << getGroupName( pReq->m_Group )
            << " msg=" << pReq->m_Message << std::endl;
    }

    stream << "  ReadReply table:" << std::endl;
    for ( CITREADREPLYREQUESTS itReq = m_ReadReplyRequests.begin(); itReq != m_ReadReplyRequests.end(); ++itReq )
    {
        const ReadReplyRequest* pReq = *itReq;
        stream << "    runid=" << (pReq->m_pRunInfo ? pReq->m_pRunInfo->getActualRunId() : "<?>")
            << " canceled=" << pReq->isCancelled()
            << " completed=" << pReq->isCompleted() << std::endl;
        stream << "      route=" << pReq->m_Routing.toFieldata()
            << " group=" << getGroupName( pReq->m_Group )
            << " maxReply=" << (COUNT)pReq->m_MaxResponseLength << std::endl;
        stream << "      msg=" << pReq->m_Message << std::endl;
        stream << "      reply=" << pReq->getResponse() << std::endl;
    }

    unlock();
}


//  getReadReplyCount()
//
//  Retrieves number of outstanding read-reply messages
COUNT
ConsoleManager::getReadReplyCount()
{
    lock();
    COUNT messages = m_ReadReplyRequests.size();
    unlock();
    return messages;
}


//  poll()
//
//  Called periodically (at fairly frequent intervals, if you please) by ConsoleActivity
//  Checks the pending request queues to see if there's anything that needs to be sent to any consoles.
//  If there's anything to do, it does one thing, then returns true.  If there's nothing to do, it returns false.
bool
ConsoleManager::poll()
{
    lock();

    bool result = pollReadOnly();
    if ( !result )
        result = pollReadReply();
    if ( !result )
        result = pollInterfaces();
    unlock();
    return result;
}


//  postReadOnlyMessage()
//
//  Posts a read-only message
void
ConsoleManager::postReadOnlyMessage
(
    RunInfo* const      pRunInfo,
    const Word36&       routing,
    const Group         group,
    const std::string&  message
)
{
    SystemLog::write( "Cons RO:" + message );

    //  Post it to the pending list, and let poll() pick it up
    lock();
    m_ReadOnlyRequests.push_back( new ReadOnlyRequest( pRunInfo, routing, group, message ));
    unlock();

    //  Post to the Run's console log
    if ( pRunInfo )
    {
        std::string logstr = "  ";
        logstr += message;
        pRunInfo->attach();
        postToConsoleLog( pRunInfo, logstr );
        pRunInfo->detach();
    }
}


//  postReadReplyMessage()
//
//  Posts a read-reply request, and routes it appropriately
void
ConsoleManager::postReadReplyMessage
(
    ReadReplyRequest* const pRequest,
    const bool              waitForResponse
)
{
    std::stringstream strm;
    strm << "Cons RR (" << pRequest << "):" << pRequest->m_Message;
    SystemLog::write( strm.str() );

    //  Post it to the pending list, and let the next poll() pick it up.
    lock();
    pRequest->setCurrentConsole( 0 );
    m_ReadReplyRequests.push_back( pRequest );
    unlock();

    if ( waitForResponse )
    {
        while ( !pRequest->isCompleted() && !pRequest->isCancelled() )
            miscSleep( 100 );
    }
}


//  postReadReplyMessage()
//
//  Posts a read-reply message and waits for the user to respond with one of a limited set of
//  acceptable responses.  Assumes system message group and system console routing.
//
//  Parameters:
//      prompt:				prompt to be displayed
//      responses:			container of acceptable responses
//      pIndex:				where we store an index into Responses, indicating which message was accepted
//      pRunInfo:			pointer to RunInfo which posted this request, else NULL
//
//  Returns:
//      true if we got a good response; false if message was cancelled
//          (indicating we are closing, or have already closed)
bool
ConsoleManager::postReadReplyMessage
(
    const std::string&      prompt,
    const VSTRING&          responses,
    INDEX* const            pIndex,
    RunInfo* const          pRunInfo
)
{
    //  Find max response length, and build a request object
    BYTE max = 0;
    for ( INDEX rx = 0; rx < responses.size(); ++rx )
    {
        if ( max < responses[rx].size() )
            max = static_cast<BYTE>(responses[rx].size());
    }

    ReadReplyRequest request( pRunInfo, 0, Group::SYSTEM, prompt, max );

    //  Loop until we get a good answer
    while ( !request.isCancelled() && !request.isCompleted() )
    {
        postReadReplyMessage( &request, true );
        if ( request.isCompleted() )
        {
            bool found = false;
            for ( INDEX rx = 0; rx < responses.size(); ++rx )
            {
                if ( request.getResponse().compareNoCase( responses[rx] ) == 0 )
                {
                    *pIndex = rx;
                    found = true;
                    break;
                }
            }

            if ( !found )
                request.setCompleted( false );
        }
    }

	return !request.isCancelled();
}


//  postSystemMessages()
//
//  Posts the system messages to the system consoles - we don't care if any of them succeed.
void
ConsoleManager::postSystemMessages
(
    const std::string&      message1,
    const std::string&      message2
)
{
    lock();
    for ( ITCONSOLES itc = m_Consoles.begin(); itc != m_Consoles.end(); ++itc )
        itc->second->postSystemMessages( message1, message2 );
    unlock();
}


//  registerConsole()
//
//  A console notifies us that it is ready to be used as such
//
//  Parameters:
//      pConsole:           pointer to the ConsoleInterface being registered
//      mainConsole:        true if this is the main console, false for secondary system or RSI consoles
//
//  Returns:
//      true generally; false if there is a console routing conflict, or if the console is already registered
bool
ConsoleManager::registerConsole
(
    ConsoleInterface* const pConsole,
    const bool              mainConsole
)
{
    std::stringstream logStrm;
    logStrm << "ConsoleManager::Register console " << pConsole->getName()
            << " mainConsole=" << (mainConsole ? "Yes" : "No");
    SystemLog::write( logStrm.str() );

    lock();
    std::string msg = "ConsoleManager:Attempt to register console " + pConsole->getName();
    for ( CITCONSOLES itc = m_Consoles.begin(); itc != m_Consoles.end(); ++itc )
    {
        if ( itc->second == pConsole )
        {
            unlock();
            msg += "ConsoleManager:Already registered";
            SystemLog::write( msg );
            return false;
        }

        if ( itc->second->getName().compare( pConsole->getName() ) == 0 )
        {
            unlock();
            msg += "ConsoleManager::Name conflict";
            SystemLog::write( msg );
            return false;
        }
    }

    //  Generate a unique routing value for this console.
    //  For System consoles, we use binary value starting with MainConsoleRouting (01) for the first registrant.
    //  For RSI consoles, we convert the name to fieldata LJSF, and use that.
    Word36 routing;
    if ( pConsole->isSystemConsole() )
    {
        if ( mainConsole )
            routing = MainConsoleRouting;
        else
        {
            routing = MainConsoleRouting.getValue() + 1;
            while ( m_Consoles.find( routing ) != m_Consoles.end() )
                routing.setValue( routing.getValue() + 1 );
        }
    }
    else if ( pConsole->isRSIConsole() )
    {
        std::string fixString = pConsole->getName();
        fixString.resize( 6, ' ' );
        miscStringToWord36Fieldata( fixString, &routing, 1 );
    }

    m_Consoles[routing] = pConsole;
    if ( mainConsole )
        m_pMainConsole = pConsole;

    logStrm.str( "" );
    logStrm << "ConsoleManager::Registered with routing "
            << std::oct << std::setw( 12 ) << std::setfill( '0' ) << routing.toOctal();
    SystemLog::write( logStrm.str() );

    //  Is this is a system console?  If so, it gets all readonly groups
    if ( pConsole->isSystemConsole() )
    {
        m_ReadOnlyGroupAssignments[Group::COMMUNICATIONS].push_back( pConsole );
        m_ReadOnlyGroupAssignments[Group::HARDWARE_CONFIDENCE].push_back( pConsole );
        m_ReadOnlyGroupAssignments[Group::IO_ACTIVITY].push_back( pConsole );
        m_ReadOnlyGroupAssignments[Group::SYSTEM].push_back( pConsole );

        //  If this is the first console, give it all groups for readreply.
        if ( m_Consoles.size() == 1 )
        {
            m_ReadReplyGroupAssignments[Group::COMMUNICATIONS] = pConsole;
            m_ReadReplyGroupAssignments[Group::HARDWARE_CONFIDENCE] = pConsole;
            m_ReadReplyGroupAssignments[Group::IO_ACTIVITY] = pConsole;
            m_ReadReplyGroupAssignments[Group::SYSTEM] = pConsole;
        }
    }

    //  RSI consoles don't get anything yet - if they are supposed to have any group assignments,
    //  RSI must set that up subsequent to registration.

    unlock();
    return true;
}


//  shutdown()
//
//  Exec is preparing to shut down
void
ConsoleManager::shutdown()
{
    SystemLog::write("ConsoleManager::shutdown()");

    //  Cancel any pending read-reply requests.
    //  We MUST clear the container because the owner of the read-reply request is likely to delete it
    //  from under us.
    lock();

    for ( ITREADREPLYREQUESTS itr = m_ReadReplyRequests.begin(); itr != m_ReadReplyRequests.end(); ++itr )
    {
        if ( !(*itr)->isCompleted() && !(*itr)->isCancelled() )
        {
            ConsoleInterface* pConsole = (*itr)->getCurrentConsole();
            if ( pConsole != 0 )
                pConsole->cancelReadReplyMessage( (*itr)->getMessageId() );
            (*itr)->setCancelled();
        }
    }
    m_ReadReplyRequests.clear();

    unlock();
}


//  startup()
//
//  Exec is starting up
bool
ConsoleManager::startup()
{
    SystemLog::write("ConsoleManager::startup()");

    //  Clear out requests which might be left over from a previous session
    while ( !m_ReadOnlyRequests.empty() )
    {
        delete m_ReadOnlyRequests.back();
        m_ReadOnlyRequests.pop_back();
    }

    //  We don't delete these, since we didn't create them - we just clear the container
    m_ReadReplyRequests.clear();

    //  Lose all the RSI console interfaces - RSIManager has, or will delete them (thus, we don't).
    //  Do NOT lose the system interfaces - they were registered by an external entity and should remain.
    CITCONSOLES itc = m_Consoles.begin();
    while ( itc != m_Consoles.end() )
    {
        if ( itc->second->isRSIConsole() )
            itc = m_Consoles.erase( itc );
        else
            ++itc;
    }

    //  Make sure we have at least the main console
    if ( m_pMainConsole == 0 )
    {
        SystemLog::write( "ConsoleManager::Aborting boot - no system console" );
        return false;
    }

    //  Reset all consoles
    for ( itc = m_Consoles.begin(); itc != m_Consoles.end(); ++itc )
        itc->second->reset();

    return true;
}


//  terminate()
//
//  Final termination of exec
void
ConsoleManager::terminate()
{
    SystemLog::write("ConsoleManager::terminate()");
}


//  unregisterConsole()
//
//  Unregisters a console, and adjusts the group assignment tables as necessary.
//  We expect this to be used for removing
//      Remote (RSI) consoles
//      All additional system consoles except the first
//
//  Returns:
//      true if the console is registered, false otherwise
bool
ConsoleManager::unregisterConsole
(
    ConsoleInterface* const pConsole
)
{
    SystemLog::write( "ConsoleManager::Unregister console " + pConsole->getName() );
    lock();

    ITCONSOLES itc = m_Consoles.begin();
    while ( itc->second != pConsole )
    {
        if ( itc == m_Consoles.end() )
        {
            unlock();
            SystemLog::write( "ConsoleManager::Console not registered" );
            return false;
        }
        ++itc;
    }

    //  There's quite a bit to do if the EXEC is running
    if ( m_pExec->isRunning() )
    {
        //  Cannot unregister main console while exec is running
        if ( pConsole == m_pMainConsole )
        {
            unlock();
            m_pExec->stopExec( Exec::SC_INTERNAL_ERROR );
            SystemLog::write( "ConsoleManager::Attempt to unregister main console with EXEC running" );
            return false;
        }

        //  Remove this console from read-only group assignments (if any).
        //  If any particular affected group has no other consoles, assign it to the scf console.
        for ( ITREADONLYGROUPASG itga = m_ReadOnlyGroupAssignments.begin(); itga != m_ReadOnlyGroupAssignments.end(); ++itga )
        {
            for ( std::list<ConsoleInterface*>::iterator itc = itga->second.begin(); itc != itga->second.end(); ++itga )
            {
                if ( *itc == pConsole )
                {
                    itga->second.erase( itc );
                    if ( itga->second.empty() )
                    {
                        itga->second.push_back( m_Consoles[MainConsoleRouting] );
                        std::stringstream consMsg;
                        consMsg << getGroupName( itga->first )
                                << " MESSAGES REDIRECTED TO "
                                << m_pMainConsole->getName();
                        m_pMainConsole->postReadOnlyMessage( consMsg.str() );
                    }

                    break;
                }
            }
        }

        //  Remove this console from read-reply group assignments (if any),
        //  and assign the affected groups back to the scf.
        for ( ITREADREPLYGROUPASG itga = m_ReadReplyGroupAssignments.begin();
              itga != m_ReadReplyGroupAssignments.end(); ++itga )
        {
            if ( itga->second == pConsole )
            {
                itga->second = m_pMainConsole;
                std::stringstream consMsg;
                consMsg << getGroupName( itga->first ) << " RESPONSE CONSOLE IS " << m_pMainConsole->getName();
                m_pMainConsole->postReadOnlyMessage( consMsg.str() );
            }
        }

        //  Check pending read/reply messages - any which are assigned to this console get unassigned.
        //  The next poll() will pick it up and reassign it somewhere else.
        for ( ITREADREPLYREQUESTS itr = m_ReadReplyRequests.begin(); itr != m_ReadReplyRequests.end(); ++itr )
        {
            if ( (*itr)->getCurrentConsole() == pConsole )
                (*itr)->setCurrentConsole( 0 );
        }

        //  We don't care about read-only messages - if it's pending, it hasn't been sent anywhwere yet
        //  so this call doesn't matter.  If it's not pending, it's already been sent and gone for good.
    }

    //  Remove the console from our list
    m_Consoles.erase( itc );
    if ( pConsole == m_pMainConsole )
        m_pMainConsole = 0;

    unlock();
    return true;
}



//  statics

//  getGroup()
//
//  Retrieves the Group enum value corresponding to the given case-insensitive group name
bool
ConsoleManager::getGroup
(
    const SuperString&  groupName,
    Group* const        pGroup
)
{
    if ( groupName.compareNoCase( m_GroupSystemString ) == 0 )
    {
        *pGroup = Group::SYSTEM;
        return true;
    }
    else if ( groupName.compareNoCase( m_GroupIOActivityString ) == 0 )
    {
        *pGroup = Group::IO_ACTIVITY;
        return true;
    }
    else if ( groupName.compareNoCase( m_GroupCommunicationsString ) == 0 )
    {
        *pGroup = Group::COMMUNICATIONS;
        return true;
    }
    else if ( groupName.compareNoCase( m_GroupHardwareConfidenceString ) == 0 )
    {
        *pGroup = Group::HARDWARE_CONFIDENCE;
        return true;
    }

    return false;
}


//  getGroupName()
//
//  Retrieves the group name corresponding to the given group enum value
const char*
ConsoleManager::getGroupName
(
    const Group         group
)
{
    switch ( group )
    {
    case Group::SYSTEM:               return m_GroupSystemString;
    case Group::IO_ACTIVITY:          return m_GroupIOActivityString;
    case Group::COMMUNICATIONS:       return m_GroupCommunicationsString;
    case Group::HARDWARE_CONFIDENCE:  return m_GroupHardwareConfidenceString;
    }

    return "???";
}

