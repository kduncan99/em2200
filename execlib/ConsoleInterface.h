//  ConsoleInterface.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Represents some entity that implements a console.
//  ConsoleManager deals with these things...
//  It is up to the implementation to provide instantiations of this... at least one anyway.



#ifndef     EXECLIB_CONSOLE_INTERFACE_H
#define     EXECLIB_CONSOLE_INTERFACE_H


class ConsoleInterface
{
protected:
    ConsoleInterface(){}

public:
    typedef     INDEX           MESSAGE_ID;

    virtual ~ConsoleInterface(){};

    virtual bool                cancelReadReplyMessage( const MESSAGE_ID identifier ) = 0;
    virtual const std::string&  getName() const = 0;
    virtual COUNT               getOutstandingMessageCount() const = 0;
    virtual COUNT               getOutstandingMessageLimit() const = 0;
    virtual bool                isRSIConsole() const = 0;
    virtual bool                isSystemConsole() const = 0;
    virtual bool                pollReadReplyMessage( MESSAGE_ID* const     pIdentifier,
                                                      std::string* const    pReply ) = 0;
    virtual bool                postReadOnlyMessage( const std::string& message ) = 0;
    virtual bool                pollUnsolicitedMessage( std::string* const pMessage ) = 0;
    virtual bool                postReadReplyMessage( const std::string&    message,
                                                      const COUNT           maxReplyLength,
                                                      MESSAGE_ID* const     pIdentifier ) = 0;
    virtual bool                postSystemMessages( const std::string&   message1,
                                                    const std::string&   message2 ) = 0;
    virtual void                reset() = 0;
};


#endif
