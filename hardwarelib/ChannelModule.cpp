//  ChannelModule implementation
//  Copyright (c) 2015 by Kurt Duncan



#include    "hardwarelib.h"


#define     MAX_BUFFERS         8



//  Private, protected methods

//  assignBuffer()
//
//  Assigns a buffer (if possible) to a Tracker which we presume has not had one assigned yet.
//  Buffer assignation goes like this:
//      Is there an available buffer of the correct size?  Use it.
//      Have we created the maximum allowable number of buffers?
//          If not, create a new one of the correct size and use it.
//      Is there an available buffer larger than necessary?  Use it.
//      Is there an available buffer smaller than necessary?  Expand and use it.
//      Forget it.
//
//  Note that this might be sub-optimal for tape IO.
//  We should think about this; but for now, strongly encourage keeping disks and tapes
//  on separate channel modules (I suppose this holds for anything non-disk/non-tape as well).
//
//  Returns:
//      true if a buffer has been assigned (although the caller could figure that out for itself).
bool
ChannelModule::assignBuffer
(
    ITTRACKERS      itTracker
)
{
    //  Look for a properly-sized buffer
    for ( ITCONVERSIONBUFFERS itcb = m_ConversionBuffers.begin();
            itcb != m_ConversionBuffers.end(); ++itcb )
    {
        if ( !(*itcb)->m_InUse && ( itTracker->m_TransferSizeBytes == (*itcb)->m_BufferSizeBytes ) )
        {
            (*itcb)->m_InUse = true;
            itTracker->m_pConversionBuffer = *itcb;
            return true;
        }
    }

    //  Can we allocate a new one?
    if ( m_ConversionBuffers.size() < MAX_BUFFERS )
    {
        ConversionBuffer* pBuffer = new ConversionBuffer( itTracker->m_TransferSizeBytes );
        pBuffer->m_InUse = true;
        m_ConversionBuffers.push_back( pBuffer );
        itTracker->m_pConversionBuffer = pBuffer;
        return true;
    }

    //  look for a buffer which is larger than required
    for ( ITCONVERSIONBUFFERS itcb = m_ConversionBuffers.begin();
            itcb != m_ConversionBuffers.end(); ++itcb )
    {
        if ( !(*itcb)->m_InUse && ( itTracker->m_TransferSizeBytes <= (*itcb)->m_BufferSizeBytes ) )
        {
            (*itcb)->m_InUse = true;
            itTracker->m_pConversionBuffer = *itcb;
            return true;
        }
    }

    //  look for a buffer which is smaller than required
    for ( ITCONVERSIONBUFFERS itcb = m_ConversionBuffers.begin();
        itcb != m_ConversionBuffers.end(); ++itcb )
    {
        if ( !(*itcb)->m_InUse && ( itTracker->m_TransferSizeBytes > (*itcb)->m_BufferSizeBytes ) )
        {
            (*itcb)->m_InUse = true;
            delete[] (*itcb)->m_pBuffer;
            (*itcb)->m_pBuffer = new BYTE[itTracker->m_TransferSizeBytes];
            itTracker->m_pConversionBuffer = *itcb;
            return true;
        }
    }

    return false;
}


//  assignBuffers()
//
//  Iterate over any Trackers with unassigned buffers, and try to assign a buffer to each one.
//  Don't do this for Trackers which do not (for some reason) require a buffer.
//  If successful (and the operation is sends data to the device in any way),
//  copy/translate data to the newly-assigned buffer from the given ACW's.
//
//  Returns:
//      true if we found anything to do (regardless of whether we actually did something); else false
bool
ChannelModule::assignBuffers()
{
    bool foundSomething = false;
    for ( ITTRACKERS itt = m_Trackers.begin(); itt != m_Trackers.end(); ++itt )
    {
        //  Don't do this if the parent IO has been cancelled
        if ( !itt->m_Cancelled )
        {
            //  If there's not (YET) a conversion buffer and this is a transfer (in or out)... get a buffer.
            if ( (itt->m_pConversionBuffer == 0) && (isTransferCommand( itt->m_pChannelProgram->m_Command )) )
            {
                foundSomething = true;
                if ( assignBuffer( itt ) )
                {
                    //  If this is a write command, convert the data from user space into the conversion buffer.
                    if ( isTransferOutCommand( itt->m_pChannelProgram->m_Command ) )
                    {
                        switch ( itt->m_pChannelProgram->m_Format )
                        {
                        case IoTranslateFormat::A:
                            translateToA( &*itt, false );
                            break;
                        case IoTranslateFormat::B:
                            translateToB( &*itt );
                            break;
                        case IoTranslateFormat::C:
                            translateToC( &*itt );
                            break;
                        case IoTranslateFormat::D:
                            translateToA( &*itt, true );
                        }
                    }
                }
            }
        }
    }

    return foundSomething;
}


//  checkChildIOs()
//
//  Check all the trackers with IoInfo objects attached to see if they've completed.
//  If so, take appropriate action.
//
//  Returns:
//      true if we found anything to do (regardless of whether we actually did something); else false
bool
ChannelModule::checkChildIOs()
{
    bool foundSomething = false;
    ITTRACKERS itt = m_Trackers.begin();
    while ( itt != m_Trackers.end() )
    {
        //  Is the parent IO cancelled?  If so, the IO packet is no longer valid.
        //  However, the child IO may still be in flight.
        if ( itt->m_Cancelled )
        {
            if ( itt->m_pChildIo && ( itt->m_pChildIo->getStatus() == Device::IoStatus::IN_PROGRESS ) )
            {
                //  Child IO is still in progress and thus, still using the child IO packet.  Do nothing.
                ++itt;
            }
            else
            {
                foundSomething = true;
                //  No child IO, or child IO is done - lose the tracker silently.
                itt->m_pConversionBuffer->m_InUse = false;
                itt->m_pConversionBuffer = 0;
                delete itt->m_pChildIo;
                itt->m_pChildIo = 0;
                itt = m_Trackers.erase( itt );
            }

            continue;
        }

        //  Parent IO is not cancelled.  If there's not a child IO, bail.
        //  That shouldn't happen, but maybe something thready is going on.
        //  Also, if there IS a child IO but it's still in process... bail.
        if ( (itt->m_pChildIo == 0) || (itt->m_pChildIo->getStatus() == Device::IoStatus::IN_PROGRESS) )
        {
            ++itt;
            continue;
        }

        //  IO is complete (maybe in error, but still it's done) - handle parent IO completion.
        //  Grab pointer to source activity now, because it might go away as soon as we set channel status.
        //  In the process of updating the parent IO packet, set the channel status last.
        //  The calling activity *should* wait for us to signal it before picking up the channel status
        //  and discarding the packet, but it might not.
        //  This opens a race condition - if the calling activity picks up the packet status too soon.
        //  it might terminate before we have a chance to signal it - then the signal will fail on a stale
        //  pointer - we don't have an immediate solution for this...TODO:BUG
        foundSomething = true;
        Worker* pSource = itt->m_pChannelProgram->m_pSource;
        if ( itt->m_pChildIo->getStatus() != Device::IoStatus::SUCCESSFUL )
        {
            //  Bad completion - notify parent IO packet, being careful to set channel status last,
            //  as that releases the calling thread to do as it wishes with the parent IO packet.
            itt->m_pChannelProgram->m_DeviceStatus = itt->m_pChildIo->getStatus();
            itt->m_pChannelProgram->m_SystemErrorCode = itt->m_pChildIo->getSystemError();
            itt->m_pChannelProgram->m_SenseBytes = itt->m_pChildIo->getSenseBytes();
            itt->m_pChannelProgram->m_ChannelStatus = Status::DEVICE_ERROR;
        }

        else
        {
            //  Successful IO completion.
            //  If the successful IO is a read (i.e., *In*) transfer the given data to the caller's buffer.
            if ( isTransferInCommand( itt->m_pChannelProgram->m_Command ) )
            {
#if EXECLIB_LOG_CHANNEL_IO_BUFFERS
                //  Need to dump IO Buffers on read completion
                std::stringstream strm;
                const IoAccessControlList::IOACWS& acList = itt->m_pChannelProgram->m_AccessControlList.getAccessControlWords();
                for ( IoAccessControlList::CITIOACWS itacw = acList.begin(); itacw != acList.end(); ++itacw )
                {
                    strm.str( "" );
                    strm << "    ACW: pBuf=0" << itacw->m_pBuffer
                        << " words=0" << std::oct << itacw->m_BufferSize
                        << " " << miscGetExecIoBufferAddressModifierString( itacw->m_AddressModifier );
                    SystemLog::write( strm.str() );

                    miscDumpWord36BufferToLog( SystemLog::getInstance(), itacw->m_pBuffer, itacw->m_BufferSize );
                }
#endif

                COUNT residue = 0;
                switch ( itt->m_pChannelProgram->m_Format )
                {
                case IoTranslateFormat::A:
                case IoTranslateFormat::D:
                    translateFromA( &*itt, &residue );
                    break;

                case IoTranslateFormat::B:
                    translateFromB( &*itt, &residue );
                    break;

                case IoTranslateFormat::C:
                    translateFromC( &*itt, &residue );
                    break;
                }

                //TODO:TAPE do something about residue and resulting sub-status
            }

            //  Set channel status last (see above comment regarding threadiness)
            itt->m_pChannelProgram->m_ChannelStatus = Status::SUCCESSFUL;
        }

        //  Notify calling activity in case it hasn't picked up the channel status yet.
        if ( pSource )
            pSource->workerSignal();

        //  Release the conversion buffer
        itt->m_pConversionBuffer->m_InUse = false;
        itt->m_pConversionBuffer = 0;

        //  Lose the Device::IoInfo object
        delete itt->m_pChildIo;
        itt->m_pChildIo = 0;

        itt = m_Trackers.erase( itt );
    }

    return foundSomething;
}


//  startChildIOs()
//
//  Called when the worker detects a tracker with no DeviceIoInfo object attached.
//  One is created, attached, and routed to the appropriate Controller for further handling.
//
//  Returns:
//      true if we found anything to do (regardless of whether we actually did something); else false
bool
ChannelModule::startChildIOs()
{
    bool foundSomething = false;

    ITTRACKERS itt = m_Trackers.begin();
    while ( itt != m_Trackers.end() )
    {
        foundSomething = true;

        //  If this tracker isn't cancelled, and we don't yet have a child IO, create one.
        if ( !itt->m_Cancelled && ( itt->m_pChildIo == 0 ) )
        {
            Device::IoFunction ioFunction = Device::IoFunction::NONE;
            switch ( itt->m_pChannelProgram->m_Command )
            {
            case ChannelModule::Command::INQUIRY:
                ioFunction = Device::IoFunction::GET_INFO;
                break;

            case ChannelModule::Command::READ:
                ioFunction = Device::IoFunction::READ;
                break;

            case ChannelModule::Command::WRITE:
                ioFunction = Device::IoFunction::WRITE;
                break;

            case ChannelModule::Command::NOP:
                //???? what to do here?
                break;
            }

            itt->m_pChildIo = new Device::IoInfo( this,
                                                  ioFunction,
                                                  itt->m_pConversionBuffer->m_pBuffer,
                                                  static_cast<BLOCK_ID>(itt->m_pChannelProgram->m_Address),
                                                  itt->m_TransferSizeBytes );
            Controller* pController = dynamic_cast<Controller*>( m_ChildNodes[itt->m_pChannelProgram->m_ControllerAddress] );
            pController->routeIo(itt->m_pChannelProgram->m_DeviceAddress, itt->m_pChildIo);
            ++itt;
        }
        else
        {
            //  Caller cancelled, so just get rid of this one.
            if ( itt->m_pConversionBuffer )
                itt->m_pConversionBuffer->m_InUse = false;
            itt = m_Trackers.erase( itt );
        }
    }

    return foundSomething;
}


//  translateFromA()
//
//  Transfer in (read)
//  Translates A-format frames to Word36 buffers.
//  Each 8-bit frame is copied into successive quarter-words, with the BSBit set to zero.
void
ChannelModule::translateFromA
(
    Tracker* const      pTracker,
    COUNT* const        pResidue
)
{
    const COUNT bytesAvailable = pTracker->m_pChildIo->getBytesTransferred();
    const BYTE* const pByteBuffer = pTracker->m_pChildIo->getBuffer();
    *pResidue = 0;
    INDEX bx = 0;
    IoAccessControlList& acList = pTracker->m_pChannelProgram->m_AccessControlList;

    pTracker->m_pChannelProgram->m_BytesTransferred = 0;
    pTracker->m_pChannelProgram->m_WordsTransferred = 0;

    //  Iterate over the destination buffer(s)
    UINT64 value;
    for (IoAccessControlList::Iterator itacList = acList.begin(); !itacList.atEnd() && (bx < bytesAvailable); ++itacList )
    {
        value = 0;
        for ( INDEX qx = 0; qx < 4; ++qx )
        {
            value <<= 9;
            if ( bx < bytesAvailable )
            {
                value |= pByteBuffer[bx++];
                ++pTracker->m_pChannelProgram->m_BytesTransferred;
            }
        }

        //  If we're not skipping, save the value to the buffer
        if ( (itacList.bufferAddressModifier() != EXIOBAM_SKIP_DATA) || !m_SkipDataFlag )
            (*itacList)->setW( value );
        ++pTracker->m_pChannelProgram->m_WordsTransferred;
    }

    *pResidue = pTracker->m_pChannelProgram->m_BytesTransferred % 4;
}


//  translateFromB()
//
//  Transfer in (read)
//  Translates B-format frames to Word36 buffers.
//  Each 6-bit frame (wrapped in the lower 6 bits of each byte) is copied into successive sixth-words.
void
ChannelModule::translateFromB
(
    Tracker* const      pTracker,
    COUNT* const        pResidue
)
{
    const COUNT bytesAvailable = pTracker->m_pChildIo->getBytesTransferred();
    const BYTE* const pByteBuffer = pTracker->m_pChildIo->getBuffer();
    INDEX bx = 0;
    IoAccessControlList& acList = pTracker->m_pChannelProgram->m_AccessControlList;

    pTracker->m_pChannelProgram->m_BytesTransferred = 0;
    pTracker->m_pChannelProgram->m_WordsTransferred = 0;

    //  Iterate over the destination buffer(s)
    UINT64 value;
    for (IoAccessControlList::Iterator itacList = acList.begin(); !itacList.atEnd(), bx < bytesAvailable; ++itacList )
    {
        value = 0;
        for ( INDEX sx = 0; sx < 6; ++sx )
        {
            value <<= 6;
            if ( bx < bytesAvailable )
            {
                value |= (pByteBuffer[bx++] && 077);
                ++pTracker->m_pChannelProgram->m_BytesTransferred;
            }
        }

        //  If we're not skipping, save the value to the buffer
        if ( (itacList.bufferAddressModifier() != EXIOBAM_SKIP_DATA) || !m_SkipDataFlag )
            (*itacList)->setW( value );
        ++pTracker->m_pChannelProgram->m_WordsTransferred;
    }

    *pResidue = pTracker->m_pChannelProgram->m_BytesTransferred % 6;
}


//  translateFromC()
//
//  Transfer in (read)
//  Translates C-format frames to Word36 buffers.
//  Each 8-bit frame is wrapped into successive bits of successive 36-bit words.
//  Every 4.5 bytes create another Word36.
void
ChannelModule::translateFromC
(
    Tracker* const      pTracker,
    COUNT* const        pResidue
)
{
    const COUNT bytesAvailable = pTracker->m_pChildIo->getBytesTransferred();
    const BYTE* const pByteBuffer = pTracker->m_pChildIo->getBuffer();
    INDEX bx = 0;
    IoAccessControlList& acList = pTracker->m_pChannelProgram->m_AccessControlList;

    pTracker->m_pChannelProgram->m_BytesTransferred = 0;
    pTracker->m_pChannelProgram->m_WordsTransferred = 0;

    //  Iterate over the destination buffer(s)
    UINT64 value;
    IoAccessControlList::Iterator itacList = acList.begin();
    while ( !itacList.atEnd() && (bx < bytesAvailable) )
    {
        //  Get 32 bits of the odd-word...
        value = 0;
        for ( INDEX x = 0; x < 4; ++x )
        {
            value <<= 8;
            if ( bx < bytesAvailable )
            {
                value |= pByteBuffer[bx++];
                ++pTracker->m_pChannelProgram->m_BytesTransferred;
            }
        }

        //  Get remaining 4 bits of the odd-word
        value <<= 4;
        BYTE splitByte = 0;
        if ( bx < bytesAvailable )
        {
            splitByte = pByteBuffer[bx++];
            ++pTracker->m_pChannelProgram->m_BytesTransferred;
            value |= splitByte >> 4;
        }

        //  If we're not skipping, save the value to the buffer
        if ( (itacList.bufferAddressModifier() != EXIOBAM_SKIP_DATA) || !m_SkipDataFlag )
            (*itacList)->setW( value );
        ++itacList;
        ++pTracker->m_pChannelProgram->m_WordsTransferred;

        //  Is this the end of the ACL?
        if ( itacList == acList.end() )
        {
            *pResidue = pTracker->m_pChannelProgram->m_BytesTransferred % 9;
            if ( *pResidue == 5 )
                *pResidue = 0;
            break;
        }

        //  Get first 4 bits of the even-word
        value = splitByte & 0x0f;

        //  Continue with the rest of the even-word
        for ( INDEX x = 0; x < 4; ++x )
        {
            value <<= 8;
            if ( bx < bytesAvailable )
            {
                value |= pByteBuffer[bx++];
                ++pTracker->m_pChannelProgram->m_BytesTransferred;
            }
        }

        //  If we're not skipping, save the value to the buffer
        if ( (itacList.bufferAddressModifier() != EXIOBAM_SKIP_DATA) || !m_SkipDataFlag )
            (*itacList)->setW( value );
        ++itacList;
        ++pTracker->m_pChannelProgram->m_WordsTransferred;
    }

    *pResidue = pTracker->m_pChannelProgram->m_BytesTransferred % 9;
}


//  translateToA()
//
//  For transfer out (write).
//  Translate from Word36 buffers to A-format byte buffer.
//  Copies 8-bit bytes from successive quarter-words into frames.
//  The MSBit from each q-word is discarded so long as it is zero;
//  however, any MSBit of 1 stops the translation, and we transfer the truncated buffer only.
//
//  Parameters:
//      pTracker:           pointer to Tracker object
//      ignoreMSBits:       skips check for MSBIT (for D format)
void
ChannelModule::translateToA
(
    Tracker* const      pTracker,
    const bool          ignoreMSBits
)
{
    pTracker->m_pChannelProgram->m_BytesTransferred = 0;
    pTracker->m_pChannelProgram->m_WordsTransferred = 0;

    bool stopFlag = false;
    INDEX bx = 0;
    for ( IoAccessControlList::Iterator itacList = pTracker->m_pChannelProgram->m_AccessControlList.begin();
         ( !itacList.atEnd() ) && !stopFlag; ++itacList )
    {
        UINT64 value = (*itacList)->getW();
        ++pTracker->m_pChannelProgram->m_WordsTransferred;

        for ( INDEX qx = 0; qx < 4; ++qx )
        {
            if ( !ignoreMSBits && ( (value & 0400000000000ll) != 0 ) )
            {
                stopFlag = true;
                break;
            }

            pTracker->m_pConversionBuffer[bx++] = static_cast<BYTE>((value & 0377000000000ll) >> 27);
            value <<= 9;
            ++pTracker->m_pChannelProgram->m_BytesTransferred;
        }
    }
}


//  translateToB()
//
//  For transfer out (write).
//  Translate from Word36 buffers to B-format byte buffer.
//  Copies 6-bit bytes from successive sixth-words into 6-bit frames, encoded into the
//  lower 6 bits of each 8-bit byte/frame.  Used for 7-track tape drives only.
void
ChannelModule::translateToB
(
    Tracker* const      pTracker
)
{
    pTracker->m_pChannelProgram->m_BytesTransferred = 0;
    pTracker->m_pChannelProgram->m_WordsTransferred = 0;

    INDEX bx = 0;
    for ( IoAccessControlList::Iterator itacList = pTracker->m_pChannelProgram->m_AccessControlList.begin(); !itacList.atEnd(); ++itacList )
    {
        pTracker->m_pConversionBuffer[bx++] = (*itacList)->getS1();
        pTracker->m_pConversionBuffer[bx++] = (*itacList)->getS2();
        pTracker->m_pConversionBuffer[bx++] = (*itacList)->getS3();
        pTracker->m_pConversionBuffer[bx++] = (*itacList)->getS4();
        pTracker->m_pConversionBuffer[bx++] = (*itacList)->getS5();
        pTracker->m_pConversionBuffer[bx++] = (*itacList)->getS6();
        pTracker->m_pChannelProgram->m_BytesTransferred += 6;
        ++pTracker->m_pChannelProgram->m_WordsTransferred;
    }
}


//  translateToC()
//
//  For transfer out (write).
//  Translate from Word36 buffers to C-format byte buffer.
//  Packs all 36 bits into 4.5 frames per word.
void
ChannelModule::translateToC
(
    Tracker* const      pTracker
)
{
    pTracker->m_pChannelProgram->m_BytesTransferred = 0;
    pTracker->m_pChannelProgram->m_WordsTransferred = 0;

    INDEX bx = 0;
    IoAccessControlList::Iterator itacList = pTracker->m_pChannelProgram->m_AccessControlList.begin();
    while ( !itacList.atEnd() )
    {
        UINT64 value = (*itacList)->getW();

        ++itacList;
        ++pTracker->m_pChannelProgram->m_WordsTransferred;

        pTracker->m_pConversionBuffer->m_pBuffer[bx++] = static_cast<BYTE>(value >> 28);
        pTracker->m_pConversionBuffer->m_pBuffer[bx++] = static_cast<BYTE>(value >> 20);
        pTracker->m_pConversionBuffer->m_pBuffer[bx++] = static_cast<BYTE>(value >> 12);
        pTracker->m_pConversionBuffer->m_pBuffer[bx++] = static_cast<BYTE>(value >> 4);
        pTracker->m_pConversionBuffer->m_pBuffer[bx++] = static_cast<BYTE>(value << 4);
        pTracker->m_pChannelProgram->m_BytesTransferred += 5;

        if ( !itacList.atEnd() )
        {
            UINT64 value = (*itacList)->getW();

            ++itacList;
            ++pTracker->m_pChannelProgram->m_WordsTransferred;

            pTracker->m_pConversionBuffer->m_pBuffer[bx - 1] |= static_cast<BYTE>(value >> 32);
            pTracker->m_pConversionBuffer->m_pBuffer[bx++] = static_cast<BYTE>(value >> 24);
            pTracker->m_pConversionBuffer->m_pBuffer[bx++] = static_cast<BYTE>(value >> 16);
            pTracker->m_pConversionBuffer->m_pBuffer[bx++] = static_cast<BYTE>(value >> 8);
            pTracker->m_pConversionBuffer->m_pBuffer[bx++] = static_cast<BYTE>(value);
            pTracker->m_pChannelProgram->m_BytesTransferred += 4;
        }
    }
}


//  worker()
//
//  async thread code
void
ChannelModule::worker()
{
    while ( !isWorkerTerminating() )
    {
        bool didSomething = false;

        lock();
        if ( assignBuffers() )
            didSomething = true;
        if ( startChildIOs() )
            didSomething = true;
        if ( checkChildIOs() )
            didSomething = true;
        unlock();

        if ( !didSomething )
            workerWait( 1000 );
    }

    //  Don't worry about IOs still hanging around...
    //  We only get called when the emulator is shutting down (or maybe reconfiguring...?)
    //  In any case, this will NEVER happen with the Exec running, and if the EXEC is not
    //  running, there are no outstanding IOs.
}



//  Constructors, destructors

ChannelModule::~ChannelModule()
{
    workerStop( true );

    //  Worker is supposed to hang until all outstanding IOs are done,
    //  so we should be able to rely on m_Trackers being empty.
    //  Lose the conversion buffers.
    while ( !m_ConversionBuffers.empty() )
    {
        delete m_ConversionBuffers.back();
        m_ConversionBuffers.pop_back();
    }
}



//  Public methods

//  cancelIo()
//
//  Cancels an IO on our buffer
bool
ChannelModule::cancelIo
(
const ChannelProgram* const pChannelProgram
)
{
    bool result = false;

    lock();
    ITTRACKERS itt = m_Trackers.begin();
    while ( itt != m_Trackers.end() )
    {
        if ( itt->m_pChannelProgram == pChannelProgram )
        {
            itt->m_Cancelled = true;
            itt->m_pChannelProgram->m_ChannelStatus = Status::CANCELLED;
            itt->m_pChannelProgram = 0;
            result = true;
            break;
        }
        ++itt;
    }
    unlock();

    return result;
}


//  dump()
//
//  For debugging
void
ChannelModule::dump
(
std::ostream&       stream
) const
{
    Node::dump( stream );

    stream << "  Trackers:" << std::endl;
    for ( CITTRACKERS itt = m_Trackers.begin(); itt != m_Trackers.end(); ++itt )
    {
        ChannelProgram* pChannelProgram = itt->m_pChannelProgram;
        stream << "    ConvBuffer=0x" << std::setw(8) << std::setfill('0') << itt->m_pConversionBuffer
            << "  CalcXferSz=0B" << std::oct << itt->m_TransferSizeBytes << std::endl;

        stream << "    ChannelProgram:" << (itt->m_Cancelled ? " CANCELLED" : "") << std::endl;
        if ( !itt->m_Cancelled )
        {
            stream << "      Source:           "
                << (pChannelProgram->m_pSource ? pChannelProgram->m_pSource->getWorkerName() : "<none>") << std::endl;
            stream << "      Path:             "
                << pChannelProgram->m_ProcessorUPI << "/"
                << pChannelProgram->m_ChannelModuleAddress << "/"
                << pChannelProgram->m_ControllerAddress << "/"
                << pChannelProgram->m_DeviceAddress << std::endl;
            stream << "      Command:          " << getCommandString(pChannelProgram->m_Command) << std::endl;
            stream << "      Address:          0" << std::oct << pChannelProgram->m_Address << std::endl;

            //  dump access control words
            stream << "      Access Control Words:" << std::endl;
            const IoAccessControlList::IOACWS& acws = pChannelProgram->m_AccessControlList.getAccessControlWords();
            for ( IoAccessControlList::CITIOACWS itacw = acws.begin(); itacw != acws.end(); ++itacw )
            {
                stream << "        " << miscGetExecIoBufferAddressModifierString( itacw->m_AddressModifier )
                    << " size=0" << std::oct << itacw->m_BufferSize << std::endl;
            }

            stream << "      Format:           " << getIoTranslateFormat(pChannelProgram->m_Format) << std::endl;
            stream << "      BytesTransferred: " << pChannelProgram->m_BytesTransferred << std::endl;
            stream << "      WordsTransferred: " << pChannelProgram->m_WordsTransferred << std::endl;
            stream << "      Channel Status:   " << getStatusString(pChannelProgram->m_ChannelStatus) << std::endl;
            stream << "      Device Status:    "
                << Device::getIoStatusString(pChannelProgram->m_DeviceStatus, pChannelProgram->m_SystemErrorCode) << std::endl;
        }

        stream << "    ChildIo: "
            << (itt->m_pChildIo == 0 ? "Not Allocated" : Device::getIoInfoString( itt->m_pChildIo )) << std::endl;
    }

    stream << "  Conversion Buffers:" << std::endl;
    for ( CITCONVERSIONBUFFERS itcb = m_ConversionBuffers.begin(); itcb != m_ConversionBuffers.end(); ++itcb )
    {
        stream << "    0x" << std::setw(8) << std::setfill('0') << (*itcb)
            << "  Size=" << (*itcb)->m_BufferSizeBytes << "B  InUse=" << ((*itcb)->m_InUse ? "Yes" : "No")
            << "  Buffer=0x" << std::setw(8) << std::setfill('0') << (*itcb)->m_pBuffer << std::endl;
    }
}


//  handleIo()
//
//  Called by InputOutputProcessor to hand us a ChannelProgram.
//  We do some early grooming on the thing, then queue it up for the worker to deal with.
void
ChannelModule::handleIo
(
    ChannelProgram* const   pChannelProgram
)
{
    //  Check to ensure the controller address is correct
    ITCHILDNODES itnCtl = m_ChildNodes.find( pChannelProgram->m_ControllerAddress );
    if ( itnCtl == m_ChildNodes.end() )
    {
        pChannelProgram->m_ChannelStatus = Status::INVALID_CONTROLLER_ADDRESS;
        if (pChannelProgram->m_pSource)
            pChannelProgram->m_pSource->workerSignal();
        return;
    }

    //  Figure out how many bytes the user is requesting for transfer (only for data transfers)
    COUNT byteCount = 0;
    if ( isTransferCommand( pChannelProgram->m_Command) )
    {
        //  Convert word count to byte count.  This depends on the transfer format...
        WORD_COUNT aclWordCountSum = pChannelProgram->m_AccessControlList.getExtent();
        if ( pChannelProgram->m_TransferSizeWords > aclWordCountSum )
        {
            pChannelProgram->m_ChannelStatus = Status::INSUFFICIENT_BUFFERS;
            if (pChannelProgram->m_pSource)
                pChannelProgram->m_pSource->workerSignal();
            return;
        }

        byteCount = getByteCountFromWordCount( aclWordCountSum, pChannelProgram->m_Format );
        Controller* pController = dynamic_cast<Controller*>( itnCtl->second );
        byteCount = pController->getContainingBufferSize( byteCount );
    }

    Tracker tracker( pChannelProgram );
    tracker.m_TransferSizeBytes = byteCount;

#if EXECLIB_LOG_CHANNEL_IOS
    std::stringstream strm;
    strm << "IOProcessor::routeIo path="
        << pChannelProgram->m_ProcessorUPI << "/"
        << pChannelProgram->m_ChannelModuleAddress << "/"
        << pChannelProgram->m_ControllerAddress << "/"
        << pChannelProgram->m_DeviceAddress
        << " Cmd=" << ChannelModule::getChannelCommandString( pChannelProgram->m_Command )
        << " Addr=" << pChannelProgram->m_Address
        << " Words=" << pChannelProgram->m_TransferSizeWords;
    SystemLog::write( strm.str() );
    const IoAccessControlList::IOACWS& acList = pChannelProgram->m_AccessControlList.getAccessControlWords();
    for ( IoAccessControlList::CITIOACWS itacw = acList.begin(); itacw != acList.end(); ++itacw )
    {
        strm.str( "" );
        strm << "    ACW: pBuf=0" << itacw->m_pBuffer
            << " words=0" << std::oct << itacw->m_BufferSize
            << " " << miscGetExecIoBufferAddressModifierString( itacw->m_AddressModifier );
        SystemLog::write( strm.str() );

#if EXECLIB_LOG_CHANNEL_IO_BUFFERS
        if ( ChannelModule::isTransferOutCommand( pChannelProgram->m_Command ) )
            miscDumpWord36BufferToLog( SystemLog::getInstance(), itacw->m_pBuffer, itacw->m_BufferSize );
#endif
    }
#endif

    //  Queue the tracker and wake up the worker
    lock();
    m_Trackers.push_back( tracker );
    unlock();
    workerSignal();
}


//  initialize()
//
//  Called soon after the object gets built
void
ChannelModule::initialize()
{
    workerStart();
}


//  signal()
//
//  Device has completed an IO for us.
void
ChannelModule::signal
(
Node* const             pSource
)
{
    //  We don't care who it comes from, we just wake up the worker.
    workerSignal();
}


//  terminate()
//
//  Called just before this object gets destroyed
void
ChannelModule::terminate()
{
    Worker::workerStop( true );
}



//  statics

//  getByteCountFromWordCount()
//
//  Calculates the number of bytes required to represent the given word count,
//  in the context of the given translation format
COUNT
ChannelModule::getByteCountFromWordCount
(
    const WORD_COUNT        wordCount,
    const IoTranslateFormat translateFormat
)
{
    switch (translateFormat)
    {
    case IoTranslateFormat::A:
    case IoTranslateFormat::D:
        return wordCount * 4;
    case IoTranslateFormat::B:
        return wordCount * 6;
    case IoTranslateFormat::C:
        return (wordCount * 9 / 2) + (wordCount % 2);
    }

    return 0;
}


const char*
ChannelModule::getCommandString
(
const ChannelModule::Command    command
)
{
    switch ( command )
    {
    case Command::INQUIRY:     return "Inquiry";
    case Command::NOP:         return "NOP";
    case Command::READ:        return "Read";
    case Command::WRITE:       return "Write";
    }

    return "???";
}


const char*
ChannelModule::getStatusString
(
const ChannelModule::Status  status
)
{
    switch ( status )
    {
    case Status::CANCELLED:                          return "Cancelled";
    case Status::SUCCESSFUL:                         return "Successful";
    case Status::DEVICE_ERROR:                       return "Device Error";
    case Status::INVALID_CHANNEL_MODULE_ADDRESS:     return "Invalid Channel Module Address";
    case Status::INVALID_CONTROLLER_ADDRESS:         return "Invalid Controller Address";
    case Status::INVALID_UPI_NUMBER:                 return "Invalid UPI Number";
    case Status::INSUFFICIENT_BUFFERS:               return "Insufficient Buffers";
    case Status::IN_PROGRESS:                        return "In Progress";
    }

    return "???";
}


const char*
ChannelModule::getIoTranslateFormat
(
    const IoTranslateFormat format
)
{
    switch ( format )
    {
    case IoTranslateFormat::A:      return "A";
    case IoTranslateFormat::B:      return "B";
    case IoTranslateFormat::C:      return "C";
    case IoTranslateFormat::D:      return "D";
    }

    return "???";
}


