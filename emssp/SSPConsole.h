//  SSPConsole
//  Copyright (c) 2015 by Kurt Duncan
//
//  Implements a very basic system console for the SSP



#ifndef     SSPCONSOLE_H
#define     SSPCONSOLE_H



class   SSPConsole : public ConsoleInterface, public Lockable
{
private:
    class   ReadReplyMsg
    {
    public:
        const std::string   m_Message;
        const COUNT         m_MaxReplyLength;
        std::string         m_Reply;
        bool                m_ReplyReady;

        ReadReplyMsg( const std::string&    message,
                      const COUNT           maxReplyLength )
                :m_Message( message ),
                m_MaxReplyLength( maxReplyLength )
        {
            m_ReplyReady = false;
        }
    };

    const std::string           m_ConsoleName;
    std::vector<ReadReplyMsg*>  m_ReadReplyMessages;
    std::list<std::string>      m_UnsolicitedKeyins;

public:
    SSPConsole( const std::string& consoleName )
            :m_ConsoleName( consoleName )
    {
        m_ReadReplyMessages.resize( 10, 0 );
    }

    virtual ~SSPConsole(){}

    bool                injectConsoleMessage( const std::string& message );

    //  ConsoleInterface overrides    
    bool                cancelReadReplyMessage( const MESSAGE_ID identifier );
    const std::string&  getName() const;
    COUNT               getOutstandingMessageCount() const;
    COUNT               getOutstandingMessageLimit() const;
    bool                isRSIConsole() const;
    bool                isSystemConsole() const;
    bool                pollReadReplyMessage( MESSAGE_ID* const     pIdentifier,
                                              std::string* const    pReply );
    bool                postReadOnlyMessage( const std::string& message );
    bool                pollUnsolicitedMessage( std::string* const pMessage );
    bool                postReadReplyMessage( const std::string&    message,
                                              const COUNT           maxReplyLength,
                                              MESSAGE_ID* const     pIdentifier );
    bool                postSystemMessages( const std::string&   message1,
                                            const std::string&   message2 );
    void                reset();
};



#endif	/* SSPCONSOLE_H */

