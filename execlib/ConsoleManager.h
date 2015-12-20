//	ConsoleManager.h
//  Copyright (c) 2015 by Kurt Duncan
//
//	Console manager - interface between console sources and console destinations.
//  Sources include:
//      Direct calls (from the Exec and its subsidiaries)
//      ER COM$ (for Extrinsic Activities)
//  Destinations include:
//      System console (via Emitter/Listener interface)
//      RSI
//
//  We handle only output to the destinations.  Input is routed by those destinations back through the Exec object.



#ifndef		EXECLIB_CONSOLE_MANAGER_H
#define		EXECLIB_CONSOLE_MANAGER_H

#include    "ConsoleInterface.h"
#include    "DeviceManager.h"



class ConsoleManager : public ExecManager
{
public:
    enum class Group
    {
        SYSTEM                  = 0,
        IO_ACTIVITY             = 1,
        COMMUNICATIONS          = 2,
        HARDWARE_CONFIDENCE     = 3,
    };

    class ReadReplyRequest
    {
    private:
        bool                            m_Cancelled;            //  We have abandoned this; owner can delete it at any time.
                                                                //      We will NOT set this until the current console (if any)
                                                                //      has been notified of cancelation
        bool                            m_Completed;            //  We are done with this; owner can delete it at any time
        ConsoleInterface*               m_pCurrentConsole;      //  If a console is working on this, else null
        ConsoleInterface::MESSAGE_ID    m_MessageId;            //  Set by the ConsoleInterface when we post this
        SuperString                     m_Response;

    public:
        const Group                     m_Group;
        const UINT8                     m_MaxResponseLength;
        const std::string               m_Message;
        const Word36                    m_Routing;
        RunInfo* const                  m_pRunInfo;

        ReadReplyRequest( RunInfo* const        pRunInfo,
                          const Word36&         routing,
                          const Group           group,
                          const std::string&    message,
                          const UINT8           maxResponseLength )
                          :m_Group( ConsoleManager::fixGroup( group ) ),
                          m_MaxResponseLength( maxResponseLength ),
                          m_Message( message ),
                          m_Routing( routing ),
                          m_pRunInfo( pRunInfo )
        {
            m_Cancelled = false;
            m_Completed = false;
            m_pCurrentConsole = 0;
            m_MessageId = 0;
        }

        ConsoleInterface*   getCurrentConsole() const                                   { return m_pCurrentConsole; }
        ConsoleInterface::MESSAGE_ID
                            getMessageId() const                                        { return m_MessageId; }
        const SuperString&  getResponse() const                                         { return m_Response; }
        bool                isCancelled() const                                         { return m_Cancelled; }
        bool                isCompleted() const                                         { return m_Completed; }
        void                setCancelled()                                              { m_Cancelled = true; }
        void                setCompleted( const bool flag = true )                      { m_Completed = flag; }
        void                setCurrentConsole( ConsoleInterface* const pConsole )       { m_pCurrentConsole = pConsole; }
        void                setMessageId( const ConsoleInterface::MESSAGE_ID messageId ){ m_MessageId = messageId; }
        void                setResponse( const std::string& response )                  { m_Response = response; }
    };

private:
    //  Represents a read-only message request.
    //  External callers never deal with this, and we only use particular instantiations for the briefest of timespans.
    class ReadOnlyRequest
    {
    public:
        const Group                     m_Group;
        const std::string               m_Message;
        const Word36                    m_Routing;
        RunInfo* const                  m_pRunInfo;

        ReadOnlyRequest( RunInfo* const     pRunInfo,
                         const Word36       routing,
                         const Group        group,
                         const std::string& message )
                         :m_Group( ConsoleManager::fixGroup( group ) ),
                         m_Message( message ),
                         m_Routing( routing ),
                         m_pRunInfo( pRunInfo )
        {}                        
    };

    //  An implementation of the ConsoleInterface, specifically for driving local console devices via the IO stack.
    class SystemConsole : public ConsoleInterface
    {
    private:
        const DeviceManager::NODE_ID    m_ConsoleNodeId;

    public:
        SystemConsole( const DeviceManager::NODE_ID nodeId )
            :m_ConsoleNodeId( nodeId )
        {}

        bool                cancelReadReplyMessage( ReadReplyRequest* const pRequest );
        const std::string&  getName() const;
        const Word36&       getRouting() const;
        bool                isRSIConsole() const;
        bool                isSystemConsole() const;
        bool                postReadOnlyMessage( const std::string& message );
        bool                postReadReplyMessage( ReadReplyRequest* const pRequest );
        bool                postSystemMessages( const std::string&   message1,
                                                const std::string&   message2 );
    };

    //  List of all consoles registered with us - keyed by console routing value
    typedef std::map<Word36, ConsoleInterface*>             CONSOLES;
    typedef CONSOLES::iterator                              ITCONSOLES;
    typedef CONSOLES::const_iterator                        CITCONSOLES;

    //  Indicates all the consoles to which read-only messages for a particular group should be sent
    typedef std::map<Group, std::list<ConsoleInterface*>>   READONLYGROUPASG;
    typedef READONLYGROUPASG::iterator                      ITREADONLYGROUPASG;
    typedef READONLYGROUPASG::const_iterator                CITREADONLYGROUPASG;

    //  Indicates to which console the read-reply messages for a particular group should be sent
    typedef std::map<Group, ConsoleInterface*>              READREPLYGROUPASG;
    typedef READREPLYGROUPASG::iterator                     ITREADREPLYGROUPASG;
    typedef READREPLYGROUPASG::const_iterator               CITREADREPLYGROUPASG;

    //  List of ReadOnlyRequests which have not yet been cancelled or completed
    typedef std::list<ReadOnlyRequest *>                    READONLYREQUESTS;
    typedef READONLYREQUESTS::iterator                      ITREADONLYREQUESTS;
    typedef READONLYREQUESTS::const_iterator                CITREADONLYREQUESTS;

    //  List of ReadReplyRequests which have not yet been posted anywhere
    typedef std::list<ReadReplyRequest*>                    READREPLYREQUESTS;
    typedef READREPLYREQUESTS::iterator                     ITREADREPLYREQUESTS;
    typedef READREPLYREQUESTS::const_iterator               CITREADREPLYREQUESTS;

    CONSOLES                m_Consoles;
    ConsoleInterface*       m_pMainConsole;
    READONLYGROUPASG        m_ReadOnlyGroupAssignments;
    READREPLYGROUPASG       m_ReadReplyGroupAssignments;
    READONLYREQUESTS        m_ReadOnlyRequests;
    READREPLYREQUESTS       m_ReadReplyRequests;

    static const char*      m_GroupSystemString;
    static const char*      m_GroupIOActivityString;
    static const char*      m_GroupCommunicationsString;
    static const char*      m_GroupHardwareConfidenceString;

    bool                            pollInterfaces();
    bool                            pollReadOnly();
    bool                            pollReadReply();
    void                            postToConsoleLog( RunInfo* const        pRunInfo,
                                                      const std::string&    message ) const;

    static inline Group             fixGroup( const Group group )
    {
        switch ( group )
        {
        case Group::HARDWARE_CONFIDENCE:
        case Group::IO_ACTIVITY:
        case Group::SYSTEM:
            return group;
        default:
            return Group::SYSTEM;
        }
    }

public:
    ConsoleManager( Exec* const pExec );
    ~ConsoleManager();

    COUNT                           getReadReplyCount();
    bool                            notifyReadReplyMessageDisplayed( const COUNT64  serialNumber,
                                                                     const BYTE     messageId ) const;
    bool                            notifyReadReplyMessageReply( const COUNT64      serialNumber,
                                                                 const BYTE         messageId,
                                                                 const std::string& reply );
    bool                            poll();
    void                            postReadOnlyMessage( RunInfo* const     pRunInfo,
                                                         const Word36&      routing,
                                                         const Group        group,
                                                         const std::string& message );
    void                            postReadReplyMessage( ReadReplyRequest* const   pRequest,
                                                          const bool                waitForResponse );
    bool                            postReadReplyMessage( const std::string&        text,
                                                          const VSTRING&            validResponses,
                                                          INDEX* const              pResponseIndex,
                                                          RunInfo* const            pRunInfo );
    void                            postSystemMessages( const std::string&  message1,
                                                        const std::string&  message2 );

    bool                            registerConsole( ConsoleInterface* const    pConsole,
                                                     const bool                 mainConsole );
    bool                            unregisterConsole( ConsoleInterface* const pConsole );

    //  ExecManager interface
    void                            dump( std::ostream&     stream,
                                          const DUMPBITS    dumpBits );
    void							shutdown();
    bool							startup();
    void                            terminate();

    //  Convenient wrappers
    inline void                     postReadOnlyMessage( const std::string& message,
                                                         RunInfo* const     pRunInfo )
    {
        postReadOnlyMessage( pRunInfo, 0, Group::SYSTEM, message );
    }

    inline void                     postReadOnlyMessage( const std::string& message,
                                                         const Word36&      routing,
                                                         RunInfo* const     pRunInfo )
    {
        postReadOnlyMessage( pRunInfo, routing, Group::SYSTEM, message );
    }

    //  Statics
    static bool                     getGroup( const SuperString&    groupName,
                                              Group* const          pGroup );
    static const char*              getGroupName( const Group group );
};



#endif
