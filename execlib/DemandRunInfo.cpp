//  DemandRunInfo implementation
//  Copyright (c) 2015 by Kurt Duncan



#include    "execlib.h"



//  private, protected methods



//  constructors, destructors

DemandRunInfo::DemandRunInfo
(
    Exec* const         pExec,
    const std::string&  originalRunId,
    const std::string&  actualRunId,
    const std::string&  accountId,
    const std::string&  projectId,
    const std::string&  userId,
    const UINT32        options,
    const char          schedulingPriority,
    const char          processorDispatchingPriority,
    const COUNT         rsiSessionNumber
)
:ControlModeRunInfo( pExec,
                     STATE_ACTIVE,
                     originalRunId,
                     actualRunId,
                     accountId,
                     projectId,
                     userId,
                     options,
                     schedulingPriority,
                     processorDispatchingPriority ),
m_RSISessionNumber( rsiSessionNumber )
{
    //  Create RSI symbiont buffers for READ$ and PRINT$
    SymbiontBuffer* pRead = new RSISymbiontBuffer( rsiSessionNumber );
    SymbiontBuffer* pPrint = new RSISymbiontBuffer( rsiSessionNumber );

    //  Preload SDF label image in both RSI buffers
    Word36 sdfLabel[2];
    sdfLabel[0].setS1( 050 );
    sdfLabel[0].setS2( 01 );
    sdfLabel[0].setS3( SymbiontBuffer::SDFT_UNSPECIFIED );
    sdfLabel[0].setS6( SymbiontBuffer::CSET_ASCII );
    miscStringToWord36Fieldata( "*SDFF*", &sdfLabel[1], 1 );

    pRead->writeWords( sdfLabel, 2 );
    pRead->setCurrentCharacterSet( SymbiontBuffer::CSET_ASCII );
    pPrint->writeWords( sdfLabel, 2 );
    pPrint->setCurrentCharacterSet( SymbiontBuffer::CSET_ASCII );

    //  Set up the READ$ and PRINT$ buffer stacks
    m_SymbiontBufferRead.push_back( pRead );
    m_SymbiontBufferPrint.push_back( pPrint );
}


DemandRunInfo::~DemandRunInfo()
{
}



//  public methods

//  dump()
//
//  For debugging
void
    DemandRunInfo::dump
    (
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
    )
{
    stream << prefix << "DemandRunInfo: RSISessionNumber=" << m_RSISessionNumber << std::endl;
    RunInfo::dump( stream, prefix, dumpBits );
}


