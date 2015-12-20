//  SSPConsole implementation
//  Copyright (c) 2015 by Kurt Duncan



#include    "emssp.h"



//  Public methods

bool
SSPConsole::injectConsoleMessage
(
    const std::string&      message
)
{
    lock();

    if ( message.size() == 0 )
    {
        unlock();
        return false;
    }

    if ( isdigit( message[0] ) )
    {
        INDEX mx = message[0] - '0';
        SuperString reply;

        if ( message.size() > 1 )
        {
            if ( message[1] != ' ' )
            {
                unlock();
                std::cout << "Invalid Console Message ID" << std::endl;
                return false;
            }
            else
            {
                reply = message.substr( 2 );
                reply.trimLeadingSpaces();
                reply.trimTrailingSpaces();
            }
        }

        if ( m_ReadReplyMessages[mx] == 0 )
        {
            unlock();
            std::cout << "Message ID Not Assigned" << std::endl;
            return false;
        }

        if ( m_ReadReplyMessages[mx]->m_ReplyReady )
        {
            unlock();
            std::cout << "Message Already Replied To" << std::endl;
            return false;
        }

        if ( reply.size() > m_ReadReplyMessages[mx]->m_MaxReplyLength )
        {
            unlock();
            std::cout << "Reply Too Long" << std::endl;
            return false;
        }

        m_ReadReplyMessages[mx]->m_Reply = reply;
        m_ReadReplyMessages[mx]->m_ReplyReady = true;
        unlock();
        return true;
    }
    else
    {
        SuperString temp = message;
        temp.trimLeadingSpaces();
        temp.trimTrailingSpaces();

        m_UnsolicitedKeyins.push_back( temp );
        unlock();
        return true;
    }
}



//  ConsoleInterface overrides

bool
SSPConsole::cancelReadReplyMessage
(
    const MESSAGE_ID    identifier
)
{
    lock();
    if ( m_ReadReplyMessages[identifier] != 0 )
    {
        m_ReadReplyMessages[identifier] = 0;
        unlock();
        std::cout << "CONS:" << identifier << " <CANCELED>" << std::endl;
        return true;
    }

    unlock();
    return false;
}


const std::string&
SSPConsole::getName() const
{
    return m_ConsoleName;
}


COUNT
SSPConsole::getOutstandingMessageCount() const
{
    COUNT count = 0;
    lock();
    for ( INDEX mx = 0; mx < m_ReadReplyMessages.size(); ++mx )
    {
        if ( m_ReadReplyMessages[mx] != 0 )
            ++count;
    }

    unlock();
    return count;
}


COUNT
SSPConsole::getOutstandingMessageLimit() const
{
    return m_ReadReplyMessages.size();
}


bool
SSPConsole::isRSIConsole() const
{
    return false;
}


bool
SSPConsole::isSystemConsole() const
{
    return true;
}


bool
SSPConsole::pollReadReplyMessage
(
    MESSAGE_ID* const     pIdentifier,
    std::string* const    pReply
)
{
    lock();
    for ( INDEX mx = 0; mx < m_ReadReplyMessages.size(); ++mx )
    {
        if ( (m_ReadReplyMessages[mx] != 0) && (m_ReadReplyMessages[mx]->m_ReplyReady) )
        {
            *pIdentifier = mx;
            *pReply = m_ReadReplyMessages[mx]->m_Reply;
            delete m_ReadReplyMessages[mx];
            m_ReadReplyMessages[mx] = 0;
            unlock();
            return true;
        }
    }

    unlock();
    return false;
}


bool
SSPConsole::pollUnsolicitedMessage
(
    std::string* const pMessage
)
{
    lock();
    if ( m_UnsolicitedKeyins.size() == 0 )
    {
        unlock();
        return false;
    }

    *pMessage = m_UnsolicitedKeyins.front();
    m_UnsolicitedKeyins.pop_front();
    unlock();

    return true;
}


bool
SSPConsole::postReadOnlyMessage
(
    const std::string&  message
)
{
    std::cout << " " << message << std::endl;
    return true;
}


bool
SSPConsole::postReadReplyMessage
(
    const std::string&    message,
    const COUNT           maxReplyLength,
    MESSAGE_ID* const     pIdentifier
)
{
    lock();
    INDEX mx = 0;
    while ( m_ReadReplyMessages[mx] != 0 )
    {
        ++mx;
        if ( mx == m_ReadReplyMessages.size() )
        {
            unlock();
            return false;
        }
    }

    m_ReadReplyMessages[mx] = new ReadReplyMsg( message, maxReplyLength );
    unlock();

    *pIdentifier = mx;
    std::cout << mx << " " << message << std::endl;
    return true;
}


bool
SSPConsole::postSystemMessages
(
    const std::string& message1,
    const std::string& message2
)
{
    //  Throw them away - we don't want to see them
    return true;
}


void
SSPConsole::reset()
{
    std::cout << "CONS:<RESET>" << std::endl;

    lock();

    for ( INDEX mx = 0; mx < m_ReadReplyMessages.size(); ++mx )
    {
        delete m_ReadReplyMessages[mx];
        m_ReadReplyMessages[mx] = 0;
    }

    m_UnsolicitedKeyins.clear();
    unlock();
}
