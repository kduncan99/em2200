//  IOProcessor.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Virtual IOP - Accepts a ChannelProgram which is a description of an IO to be done
//  from the Exec's point of view, and queues it onto a particular ChannelModule.
//  So, it's a fairly thin wrapper, with no async worker.


#ifndef     HARDWARELIB_IO_PROCESSOR
#define     HARDWARELIB_IO_PROCESSOR



#include    "ChannelModule.h"
#include    "Processor.h"



class   IOProcessor : public Processor
{
private:

public:
    IOProcessor( const std::string& name )
        :Processor( ProcessorType::IOP, name )
    {}

    ~IOProcessor(){};

    //  For debugging
    virtual void    dump( std::ostream& stream ) const;

    bool            cancelIo( const ChannelModule::ChannelProgram* const pChannelProgram );                                  
    void            routeIo( ChannelModule::ChannelProgram* const pChannelProgram );
};



#endif
