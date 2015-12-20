//  IOProcessor implementation
//  Copyright (c) 2015 by Kurt Duncan
//
//  Not much here -- IOP's don't do  much, the CMod's do it all.



#include    "hardwarelib.h"



//  protected and private methods



//  constructors, destructors



//  public methods

//  cancelIo()
//
//  Informs IOProcessor (and by extension, the channel program) that we are taking back the
//  channel program object and its buffer, and it should no longer be updated.
//
//  Used in situations where the exec is shutting down, or the caller got tired of waiting.
bool
IOProcessor::cancelIo
(
    const ChannelModule::ChannelProgram* const  pChannelProgram
)
{
    ITCHILDNODES itn = m_ChildNodes.find( pChannelProgram->m_ChannelModuleAddress );
    if ( itn == m_ChildNodes.end() )
        return false;

    ChannelModule* pcm = dynamic_cast<ChannelModule*>( itn->second );
    return pcm->cancelIo( pChannelProgram );
}


//  dump()
void
IOProcessor::dump
(
std::ostream&       stream
) const
{
    Processor::dump( stream );
}


//  routeIo()
//
//  Routes a channel program to the appropriate channel module
void
IOProcessor::routeIo
(
    ChannelModule::ChannelProgram* const    pChannelProgram
)
{
    ITCHILDNODES itn = m_ChildNodes.find( pChannelProgram->m_ChannelModuleAddress );
    if ( itn == m_ChildNodes.end() )
    {
        pChannelProgram->m_ChannelStatus = ChannelModule::Status::INVALID_CHANNEL_MODULE_ADDRESS;
        if ( pChannelProgram->m_pSource )
            pChannelProgram->m_pSource->workerSignal();
    }
    else
    {
        pChannelProgram->m_ChannelStatus = ChannelModule::Status::IN_PROGRESS;
        ChannelModule* pcm = dynamic_cast<ChannelModule*>( itn->second );
        pcm->handleIo( pChannelProgram );
    }
}

