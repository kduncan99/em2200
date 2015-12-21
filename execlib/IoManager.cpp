//  IoManager.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//  Implementation of IoManager class.
//
//  Accepts async IO request from some Activity, and converts it from the 36-bit buffer /
//  FacilityItem world to byte-oriented controller/device world, then passes the IO
//  to the appropriate IoController object.
//
//  One thing we do here for disk IO, is splitting up requests into device-block IOs.
//  This can result in quite a bit of IO, so we are hoping either the Controller or the Disk
//  are doing some caching.


/* TODO:DEFERRED
Should we do any of the following?
To prevent unnecessary delays while the Exec waits for the B or G response, the
Exec simulates an M response and answers the message internally when either of
the following conditions occurs:
* When the run-id is not EXEC8 and the error message allows a B and M or
a G and M response
* When a tape I/O error message occurs that allows only B and M responses,
regardless of run-id
Tape I/O messages do not allow an A response as an option unless the error occurred
during a rewind or when the Exec was verifying tape identity.

An automatic G response is generated for the I/O errors on TIP duplexed files when
the TIP file was reserved through the FREIPS processor (NMSG=P). This action takes
place only for TIP files where one leg of the duplexed file is still operational.
These automatic responses are generated every 6 seconds. The number of responses
is set at system generation time. After that, the normal read-and-reply for I/O errors is
sent to the system console.
*/


/*TODO:DEFERRED move explanatory comments to appropriate locations in code

TAPE --------------------------------------------------------------------

BUSY AGM
Busy status returned. Do not try to access
a busy unit. Answer A. If trouble persists,
check the unit.

CUERR BM
Sense data indicates an internal error has
occurred in the control unit.

DEVNA AGM
5073 control unit detected that a device is
not available.

NORING B
A write is being attempted on a tape that is
not write-enabled.
If NORING was just displayed for a
mounted tape and a B reply was entered or
forced by the Exec, the LOAD is restarted.
If a label write was tried, the tape rewinds
to interlock (unloaded) and the appropriate
LOAD message is redisplayed at the
console. Label checking is done again
before the label write is tried again.
Normally, the same previously mounted
reel is loaded with a write ring.

NOTRDY ABM
Not Ready.
Indicates one of the following:
* Tape not mounted on tape drive
referenced.
* Referenced tape drive in interlock
position.
* Correct the malfunction.
* Answer A.

DISK --------------------------------------------------------

ABNORM AGM
Abnormal condition detected during
recovery. Normal recovery procedure
aborted.

BLKERR AGM
Block size error

BUSY AGM
DEVICE END status is pending for the
selected device (that is, the device is busy
to either this channel or another channel). A
busy condition should be temporary; thus,
the operation can be retried. If the busy
status persists, check the unit.

CM-REJ AGM
Command reject, defective track, or end of
pack.
Invalid command or parameter in the
SCSI CFB.

DATCHK ABGM
Permanent data check. The response order
to DATCHK messages is as follows:
(1) M
(2) Three or more A responses
(3) B.
(SCSI) Nonrecoverable error due to a flaw in
the data or medium.

DEVERR ABGM
Nonpermanent data check, busy to other
channel, overrun, or seek check.

EQPCHK ABGM
Equipment check.
(SCSI) Nonrecoverable hardware failure.

FIPROT AGM
File protect; the data transfer exceeded the
extent limit.

INT-RQ AGM
Intervention required; the drive may be offline.

LABCHK AGM
Label read required; invalid pack change
may be cause.

NO-REC ABGM
No record found or invalid track format.

PAKCHG AGM
Attention or attention/busy, which is
interpreted by software as an invalid pack
change.
(SCSI) Unit attention presented indicating a
media for change or reset condition.

PERMER ABGM
One of the following errors has occurred:
* DATACHK and operation incomplete:
During read operations, the data check
was in the data field of a block within
the transfer (not the first block).
* DATACHK and permanent error: An
error occurred in the data field of the
first block during read operations, or an
error occurred in the ID field of a block
within the transfer during read or write
operations.

QFULL GM
(SCSI) Command queue full.

R-ONLY AGM
Read-only switch on.

TRKINV AGM
Invalid track format.

See table B-5, B-6 for function mnemonics
*/



#include	"execlib.h"



//	statics



//  Private methods

//  allocateSpace()
//
//  Special code for disk IO to allocate space, or write.
//  For writes, we always trip through here just to make sure our targeted space is allocated.
//  If it is, MFDManager basically NOPs and comes back fairly quickly.
//
//  TODO:ER    We need a special handler for EACQ$ because it has slightly different rules.
void
IoManager::allocateSpace
(
    MassStorageRequestTracker* const    pTracker
)
{
    //  So, we need to get a base file-relative track id and track count that will encompass
    //  the entire requested area.
    TRACK_ID fileTrackId = pTracker->m_StartingWordAddress / 1792;
    WORD_COUNT wordExtent = (pTracker->m_StartingWordAddress % 1792) + pTracker->m_TotalWordCount;
    TRACK_COUNT trackCount = wordExtent / 1792;
    if ( wordExtent % 1792 > 0 )
        ++trackCount;

    //  Do the allocation.
    MFDManager* pMfdMgr = dynamic_cast<MFDManager*>( m_pExec->getManager( Exec::MID_MFD_MANAGER ) );
    MFDManager::Result mfdResult =
        pMfdMgr->allocateFileTracks( m_pIoActivity, pTracker->m_pDiskItem, fileTrackId, trackCount );
    if ( mfdResult.m_Status != MFDManager::MFDST_SUCCESSFUL )
    {
        pTracker->m_pIoPacket->m_Status = convertMFDResult( mfdResult );
        completeTracker( pTracker );
        return;
    }

    //  Done - get ready for the next step (state remains the same, but allocateDone is now true).
    pTracker->m_AllocationDone = true;
    return;
}


//  attachChildBuffer()
//
//  Allocates a child buffer to the request tracker, if one is available
bool
IoManager::attachChildBuffer
(
    MassStorageRequestTracker* const    pTracker
)
{
    bool result = false;

    if ( m_ChildBuffersAvailable.size() > 0 )
    {
        ITCHILDBUFFERS itBuff = m_ChildBuffersAvailable.begin();
        pTracker->m_pChildBuffer = *itBuff;
        m_ChildBuffersInUse.insert( *itBuff );
        m_ChildBuffersAvailable.erase( itBuff );
        result = true;
    }

    return result;
}


//  detachChildBuffer()
//
//  Detaches a child buffer from the request tracker
void
IoManager::detachChildBuffer
(
    MassStorageRequestTracker* const    pTracker
)
{
    if ( pTracker->m_pChildBuffer )
    {
        ITCHILDBUFFERS itBuff = m_ChildBuffersInUse.find( pTracker->m_pChildBuffer );
        m_ChildBuffersAvailable.insert( *itBuff );
        m_ChildBuffersInUse.erase( itBuff );
        pTracker->m_pChildBuffer = 0;
    }
}


//  pollChildIoDone()
//
//  Redirector
//
//  Returns true if the state (or some conceptual sub-state) of the tracker has changed; else false
inline bool
IoManager::pollChildIoDone
(
    RequestTracker* const   pTracker
)
{
    switch ( pTracker->m_Type )
    {
    case RequestTracker::RTTYPE_MASS_STORAGE:
        return pollChildIoDoneMassStorage( dynamic_cast<MassStorageRequestTracker*>( pTracker ) );

    case RequestTracker::RTTYPE_TAPE:
        return pollChildIoDoneTape( dynamic_cast<TapeRequestTracker*>( pTracker ) );
    }

    return false;
}


//  pollChildIoDoneMassStorage()
//
//  A child IO for mass storage file IO is done - evaulate the status.
//  If it's in error, post a console message.  If it's okay, clean up this IO and move on...
//
//  Canonical stuff:
//      BLKERR  Disk AGM    Block size error
//      BUSY    Disk AGM    Unit busy
//      CM-REJ  Disk AGM    Command reject, defective track, or end of pack
//      CUERR   Disk GM     Internal target error (for SCSI)
//      DATCHK  Disk ABGM   Permanent data check
//      DEVERR  Disk        Nonpermanent data check, busy to other channel, seek error
//      EFTERM  Disk AGM    Command terminated by external function (cause?)
//      EQPCHK  Disk ABGM   Equipment check (AGM for SCSI)
//      INT-RQ  Disk AGM    Intervention required, drive may be offline
//      LABCHK  Disk AGM    Label read required
//      NO-REC  Disk ABGM   No record found or invalid track format
//      PAKCHG  Disk AGM    Unit attention
//      R-ONLY  Disk AGM    Read-only switch is on
//      QFULL   Disk GM     Command queue full (SCSI)
//      SEEKCK  Disk ABGM   Seek error (possibly bad block request)
//      TRKINV  Disk AGM    Invalid track format
//
//  Returns true if the state (or some conceptual sub-state) of the tracker has changed; else false
bool
IoManager::pollChildIoDoneMassStorage
(
    MassStorageRequestTracker* const pTracker
)
{
    //  Success
    if ( pTracker->m_pChannelProgram->m_ChannelStatus == ChannelModule::Status::SUCCESSFUL )
    {
        pTracker->m_RetryFlag = false;

        //  detach the child buffer, if there is one
        if ( pTracker->m_pChildBuffer )
            detachChildBuffer( pTracker );

        //  Update word count
        pTracker->m_pIoPacket->m_FinalWordCount += pTracker->m_pChannelProgram->m_WordsTransferred;

        //  Go back to setup in case there is more to be done for this IO.
        //  Only MS IOs do this (see below, TAPE goes straight to completeTracker())
        pTracker->m_State = RequestTracker::RTST_CHILD_IO_SETUP;
        return true;
    }

    //  Error at the device level
    if ( pTracker->m_pChannelProgram->m_ChannelStatus == ChannelModule::Status::DEVICE_ERROR )
    {
        switch ( pTracker->m_pChannelProgram->m_DeviceStatus )
        {
        case Device::IoStatus::SUCCESSFUL:
            //  We should never get here with Status::DEVICE_ERROR set
            pTracker->m_PendingStatus = EXIOSTAT_INTERNAL_ERROR;
            postConsoleMessage( pTracker, "CUERR", "BM" );
            return true;

        case Device::IoStatus::BUFFER_TOO_SMALL:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "CM_REJ", "AGM" );
            return true;

        case Device::IoStatus::DEVICE_BUSY:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "BUSY", "AGM" );
            return true;

        case Device::IoStatus::INVALID_BLOCK_COUNT:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "CM_REJ", "AGM" );
            return true;

        case Device::IoStatus::INVALID_BLOCK_ID:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "SEEKCK", "ABGM" );
            return true;

        case Device::IoStatus::INVALID_BLOCK_SIZE:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "BLKERR", "AGM" );
            return true;

        case Device::IoStatus::INVALID_DEVICE_ADDRESS:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "DEVNA", "AGM" );
            return true;

        case Device::IoStatus::INVALID_FUNCTION:
            //  Shouldn't get here, because we filter this out before the IO.
            pTracker->m_pIoPacket->m_Status = EXIOSTAT_INVALID_FUNCTION_CODE;
            completeTracker( pTracker );
            return true;

        case Device::IoStatus::IN_PROGRESS:
            //  We should never get here with Status::DEVICE_ERROR set
            pTracker->m_PendingStatus = EXIOSTAT_INTERNAL_ERROR;
            postConsoleMessage( pTracker, "CUERR", "BM" );
            return true;

        case Device::IoStatus::NO_DEVICE:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "DEVNA", "AGM" );
            return true;

        case Device::IoStatus::NOT_PREPPED:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "LABCHK", "AGM" );
            return true;

        case Device::IoStatus::NOT_READY:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "INT-RQ", "AGM" );
            return true;

        case Device::IoStatus::QUEUE_FULL:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "QFULL", "GM" );
            return true;

        case Device::IoStatus::SYSTEM_EXCEPTION:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "EQPCHK", "BM" );
            return true;

        case Device::IoStatus::UNIT_ATTENTION:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "PAKCHG", "AGM" );
            return true;

        case Device::IoStatus::WRITE_PROTECTED:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "R-ONLY", "AGM" );
            return true;

        default:
            //  This should never happen - stop the exec and return bad status
            //????
            return false;
        }
    }

    //  Error at the channel level
    switch ( pTracker->m_pChannelProgram->m_ChannelStatus )
    {
    case ChannelModule::Status::CANCELLED:
        //  Various reasons for this - do the best we can
        pTracker->m_pIoPacket->m_Status = EXIOSTAT_TASK_ABORT;
        completeTracker( pTracker );
        return true;

    case ChannelModule::Status::INSUFFICIENT_BUFFERS:
        //  Silent retry
        pTracker->m_RetryFlag = true;
        pTracker->m_State = RequestTracker::RTST_CHILD_IO_READY;
        return false;

    case ChannelModule::Status::INVALID_CHANNEL_MODULE_ADDRESS:
    case ChannelModule::Status::INVALID_CONTROLLER_ADDRESS:
    case ChannelModule::Status::INVALID_UPI_NUMBER:
        //  Setup for the IO is wrong
        pTracker->m_PendingStatus = EXIOSTAT_INTERNAL_ERROR;
        postConsoleMessage( pTracker, "CHIFRC", "B" );
        return true;

    case ChannelModule::Status::IN_PROGRESS:
        //  This should not happen, but we can deal with it...
        return false;

    default:
        //  This should never happen - stop the exec and return bad status
        //????
        return false;
    }

    //  drop-through should not happen.
    return false;
}


//  pollChildIoDoneTape()
//
//  A child IO for mass storage file IO is done - evaulate the status.
//  If it's in error, post a console message.  If it's okay, clean up this IO and move on...
//
//  Canonical stuff...
//      BUSY    Tape AGM    Unit busy
//      CHIFRC  Tape B      Channel Interface abnormal condition
//      CUERR   Tape BM     Internal error in controller
//      DATCHK  Tape AGM    Parity Error
//      DEVERR  Tape ABM    Physical End of Tape (in what context?)
//      DEVNA   Tape AGM    Device not available
//      EQPCHK  Tape BM     Odd controller or device issue
//      NORING  Tape B      Write attempted on non-write-enabled volume
//      NOTRDY  Tape ABM
//      REWERR  Tape AGM    Error on rewind
//      RPOSER  Tape BM     Loss of position during reposition
//      RUNWAY  Tape ABGM   Read didn't find a valid datablock
//      WDCNT0  Tape AGM    Data not transferred to control unit
//
//  Returns true if the state (or some conceptual sub-state) of the tracker has changed; else false
bool
IoManager::pollChildIoDoneTape
(
    TapeRequestTracker* const pTracker
)
{
    //  Success
    if ( pTracker->m_pChannelProgram->m_ChannelStatus == ChannelModule::Status::SUCCESSFUL )
    {
        pTracker->m_RetryFlag = false;
        pTracker->m_pIoPacket->m_FinalWordCount = pTracker->m_pChannelProgram->m_BytesTransferred / 4;
        pTracker->m_pIoPacket->m_AbnormalFrameCount = pTracker->m_pChannelProgram->m_BytesTransferred % 4;
        if ( pTracker->m_pIoPacket->m_AbnormalFrameCount > 0 )
            pTracker->m_pIoPacket->m_Status = EXIOSTAT_NON_INTEGRAL_BLOCK;
        else
            pTracker->m_pIoPacket->m_Status = EXIOSTAT_SUCCESSFUL;

        completeTracker( pTracker );
        return true;
    }

    //  Error at the device level
    if ( pTracker->m_pChannelProgram->m_ChannelStatus == ChannelModule::Status::DEVICE_ERROR )
    {
        switch ( pTracker->m_pChannelProgram->m_DeviceStatus )
        {
        case Device::IoStatus::SUCCESSFUL:
            //  We should never get here with Status::DEVICE_ERROR set
            pTracker->m_PendingStatus = EXIOSTAT_INTERNAL_ERROR;
            postConsoleMessage( pTracker, "CUERR", "BM" );
            return true;

        case Device::IoStatus::BUFFER_TOO_SMALL:
            //  Tape block read is larger than user buffer - not an error
            pTracker->m_pIoPacket->m_FinalWordCount = pTracker->m_pChannelProgram->m_BytesTransferred / 4;
            pTracker->m_pIoPacket->m_AbnormalFrameCount = pTracker->m_pChannelProgram->m_BytesTransferred % 4;
            pTracker->m_pIoPacket->m_Status = EXIOSTAT_SUCCESSFUL;
            completeTracker( pTracker );
            return true;

        case Device::IoStatus::DEVICE_BUSY:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "BUSY", "AGM" );
            return true;

        case Device::IoStatus::END_OF_TAPE:
            pTracker->m_pIoPacket->m_Status = EXIOSTAT_END_OF_TAPE;
            completeTracker( pTracker );
            return true;

        case Device::IoStatus::FILE_MARK:
            //  End-of-file encountered on read - not an error, but not successful either
            pTracker->m_pIoPacket->m_Status = EXIOSTAT_END_OF_FILE;
            completeTracker( pTracker );
            return true;

        case Device::IoStatus::INVALID_DEVICE_ADDRESS:
            pTracker->m_PendingStatus = EXIOSTAT_NON_RECOVERABLE_ERROR;
            postConsoleMessage( pTracker, "DEVNA", "AGM" );
            return true;

        case Device::IoStatus::INVALID_FUNCTION:
            //  Shouldn't get here, because we filter this out before the IO.
            pTracker->m_pIoPacket->m_Status = EXIOSTAT_INVALID_FUNCTION_CODE;
            completeTracker( pTracker );
            return true;

        case Device::IoStatus::INVALID_MODE:
            pTracker->m_pIoPacket->m_Status = EXIOSTAT_INCOMPATIBLE_MODE_FIELD;
            completeTracker( pTracker );
            return true;

        case Device::IoStatus::INVALID_TRANSFER_SIZE:
            pTracker->m_pIoPacket->m_Status = EXIOSTAT_INVALID_FUNCTION_CODE;
            completeTracker( pTracker );
            return true;

        case Device::IoStatus::IN_PROGRESS:
            //  We should never get here with Status::DEVICE_ERROR set
            pTracker->m_PendingStatus = EXIOSTAT_INTERNAL_ERROR;
            postConsoleMessage( pTracker, "CUERR", "BM" );
            return true;

        case Device::IoStatus::LOST_POSITION:
            pTracker->m_PendingStatus = EXIOSTAT_LOSS_OF_POSITION;
            postConsoleMessage( pTracker, "RPOSER", "BM" );
            return true;

        case Device::IoStatus::NO_DEVICE:
            pTracker->m_PendingStatus = EXIOSTAT_INTERNAL_ERROR;
            postConsoleMessage( pTracker, "DEVNA", "AGM" );
            return true;

        case Device::IoStatus::NOT_READY:
            pTracker->m_PendingStatus = EXIOSTAT_INTERNAL_ERROR;
            postConsoleMessage( pTracker, "NOTRDY", "ABM" );
            return true;

        case Device::IoStatus::SYSTEM_EXCEPTION:
            pTracker->m_PendingStatus = EXIOSTAT_INTERNAL_ERROR;
            postConsoleMessage( pTracker, "EQPCHK", "BM" );
            return true;

        case Device::IoStatus::WRITE_PROTECTED:
            pTracker->m_PendingStatus = EXIOSTAT_INTERNAL_ERROR;
            postConsoleMessage( pTracker, "NORING", "B" );
            return true;

        default:
            //  This should never happen - stop the exec and return bad status
            //????
            return false;
        }
    }

    //  Error at the channel level
    switch ( pTracker->m_pChannelProgram->m_ChannelStatus )
    {
    case ChannelModule::Status::CANCELLED:
        //  Various reasons for this - do the best we can
        pTracker->m_pIoPacket->m_Status = EXIOSTAT_TASK_ABORT;
        completeTracker( pTracker );
        return true;

    case ChannelModule::Status::INSUFFICIENT_BUFFERS:
        //  Silent retry
        pTracker->m_RetryFlag = true;
        pTracker->m_State = RequestTracker::RTST_CHILD_IO_READY;
        return false;

    case ChannelModule::Status::INVALID_CHANNEL_MODULE_ADDRESS:
    case ChannelModule::Status::INVALID_CONTROLLER_ADDRESS:
    case ChannelModule::Status::INVALID_UPI_NUMBER:
        //  Setup for the IO is wrong
        pTracker->m_PendingStatus = EXIOSTAT_INTERNAL_ERROR;
        postConsoleMessage( pTracker, "CHIFRC", "B" );
        return true;

    case ChannelModule::Status::IN_PROGRESS:
        //  This should not happen, but we can deal with it...
        return false;

    default:
        //  This should never happen - stop the exec and return bad status
        //????
        return false;
    }
}


//  pollChildIoInProgress()
//
//  Checks a tracker which has a state indicating that a child IO is in progress,
//  to see if that child IO has completed.
//
//  Returns true if the state (or some conceptual sub-state) of the tracker has changed; else false
bool
IoManager::pollChildIoInProgress
(
    RequestTracker* const pTracker
)
{
    if ( pTracker->m_pChannelProgram->m_ChannelStatus != ChannelModule::Status::IN_PROGRESS )
    {
        pTracker->m_State = RequestTracker::RTST_CHILD_IO_DONE;
        return true;
    }

    return false;
}


//  pollChildIoReady()
//
//  Redirector
//
//  Returns true if the state (or some conceptual sub-state) of the tracker has changed; else false
inline bool
IoManager::pollChildIoReady
(
    RequestTracker* const   pTracker
)
{
    switch ( pTracker->m_Type )
    {
    case RequestTracker::RTTYPE_MASS_STORAGE:
        return pollChildIoReadyMassStorage( dynamic_cast<MassStorageRequestTracker*>( pTracker ) );

    case RequestTracker::RTTYPE_TAPE:
        return pollChildIoReadyTape( dynamic_cast<TapeRequestTracker*>( pTracker ) );
    }

    return false;
}


//  pollChildIoReadyMassStorage()
//
//  Various child IO parameters have been set up, and we are nearly ready to schedule a child IO.
//  We *might* need to allocate a temporary buffer, though, which is why this is a separate state.
//
//  Returns true if the state (or some conceptual sub-state) of the tracker has changed; else false
bool
IoManager::pollChildIoReadyMassStorage
(
    MassStorageRequestTracker* const    pTracker
)
{
    //  Do child buffer stuff only the first time - no need to do it over for a retry...
    if ( !pTracker->m_RetryFlag )
    {
        //  Do we need a temporary buffer for the child IO?
        if ( pTracker->m_ChildBufferNeeded && ( pTracker->m_pChildBuffer == 0 ) )
        {
            //  If we cannot attach a buffer, just return false - the scheduler will wait a little bit
            //  and try again, which is the right thing to do.
            if ( !attachChildBuffer( pTracker ) )
                return false;

            //  Update the child IO ACW to refer to this buffer
            pTracker->m_pChannelProgram->m_AccessControlList.clear();
            pTracker->m_pChannelProgram->m_AccessControlList.push_back( IoAccessControlWord( pTracker->m_pChildBuffer,
                                                                                              pTracker->m_ChildIoPrepFactor,
                                                                                              EXIOBAM_INCREMENT ) );
        }

        //  If we are ready to do a write operation and we have a child buffer, then we are setting up
        //  the second half of a read-before-write thing - we've already read the block from disk,
        //  now we need to update a subset of it from the user's buffer space.
        if ( ( pTracker->m_pChildBuffer != 0 )
              && ( pTracker->m_pChannelProgram->m_Command == ChannelModule::Command::WRITE ) )
        {
            COUNT wordOffset = pTracker->m_NextWordAddress % pTracker->m_ChildIoPrepFactor;
            COUNT overlayWords = pTracker->m_ChildIoPrepFactor - wordOffset;
            INDEX destx = wordOffset;
            while ( (overlayWords > 0) && ( pTracker->m_itUserACW != pTracker->m_pIoPacket->m_AccessControlList.end() ) )
            {
                pTracker->m_pChildBuffer[destx++].setW( (*(pTracker->m_itUserACW))->getW() );
                --overlayWords;
                --pTracker->m_RemainingWordCount;
            }
        }
    }

    //  Get path to the targeted device
    pTracker->m_pChildIoPath = m_pDeviceManager->getNextPath( pTracker->m_ChildIoDeviceId );
    if ( !pTracker->m_pChildIoPath )
    {
        if ( pTracker->m_pChildBuffer )
            detachChildBuffer( pTracker );
        pTracker->m_pIoPacket->m_Status = EXIOSTAT_DEVICE_NOT_AVAILABLE;
        completeTracker( pTracker );
        return true;
    }

    //  Fill path information into channel program
    pTracker->m_pChannelProgram->m_ProcessorUPI = pTracker->m_pChildIoPath->m_IOPUPINumber;
    pTracker->m_pChannelProgram->m_ChannelModuleAddress = pTracker->m_pChildIoPath->m_ChannelModuleAddress;
    pTracker->m_pChannelProgram->m_ControllerAddress = pTracker->m_pChildIoPath->m_ControllerAddress;
    pTracker->m_pChannelProgram->m_DeviceAddress = pTracker->m_pChildIoPath->m_DeviceAddress;

    //  Queue the channel program
    const DeviceManager::ProcessorEntry* pIOPEntry =
        m_pDeviceManager->getProcessorEntry( pTracker->m_pChildIoPath->m_IOPIdentifier );
    IOProcessor* pIOP = dynamic_cast<IOProcessor*>( pIOPEntry->m_pNode );
    pIOP->routeIo( pTracker->m_pChannelProgram );
    pTracker->m_State = RequestTracker::RTST_CHILD_IO_IN_PROGRESS;

    return true;
}


//  pollChildIoReadyTape()
//
//  Set up the IO using the caller's buffer information, and schedule it.
//
//  Returns true if the state (or some conceptual sub-state) of the tracker has changed; else false
bool
IoManager::pollChildIoReadyTape
(
    TapeRequestTracker* const pTracker
)
{
    //  Work with DeviceManager to figure out a path to the indicated device
    DeviceManager::NODE_ID deviceId = pTracker->m_pTapeItem->getAssignedNodes()[pTracker->m_pTapeItem->getCurrentNodeIndex()];
    pTracker->m_pChildIoPath = m_pDeviceManager->getNextPath( deviceId );
    if ( !pTracker->m_pChildIoPath )
    {
        pTracker->m_pIoPacket->m_Status = EXIOSTAT_DEVICE_NOT_AVAILABLE;
        completeTracker( pTracker );
        return true;
    }

    //  Update channel program with path info
    pTracker->m_pChannelProgram->m_ProcessorUPI = pTracker->m_pChildIoPath->m_IOPUPINumber;
    pTracker->m_pChannelProgram->m_ChannelModuleAddress = pTracker->m_pChildIoPath->m_ChannelModuleAddress;
    pTracker->m_pChannelProgram->m_ControllerAddress = pTracker->m_pChildIoPath->m_ControllerAddress;
    pTracker->m_pChannelProgram->m_DeviceAddress = pTracker->m_pChildIoPath->m_DeviceAddress;

    //  Queue the program
    const DeviceManager::ProcessorEntry* pIOPEntry =
        m_pDeviceManager->getProcessorEntry( pTracker->m_pChildIoPath->m_IOPIdentifier );
    IOProcessor* pIOP = dynamic_cast<IOProcessor*>( pIOPEntry->m_pNode );
    pIOP->routeIo( pTracker->m_pChannelProgram );
    pTracker->m_State = RequestTracker::RTST_CHILD_IO_IN_PROGRESS;

    return true;
}


//  pollChildIoSetup()
//
//  Redirector
//
//  Returns true if the state (or some conceptual sub-state) of the tracker has changed; else false
inline bool
IoManager::pollChildIoSetup
(
    RequestTracker* const   pTracker
)
{
    switch ( pTracker->m_Type )
    {
    case RequestTracker::RTTYPE_MASS_STORAGE:
        return pollChildIoSetupMassStorage( dynamic_cast<MassStorageRequestTracker*>( pTracker ) );

    case RequestTracker::RTTYPE_TAPE:
        return pollChildIoSetupTape( dynamic_cast<TapeRequestTracker*>( pTracker ) );
    }

    return false;
}


//  pollChildIoSetupMassStorage()
//
//  Either we got here from NEW state or from CHILD_IO_DONE state.
//  If there's more to do, do it.  If not, we've completed the entire request.
//
//  Returns true if the state (or some conceptual sub-state) of the tracker has changed; else false
bool
IoManager::pollChildIoSetupMassStorage
(
    MassStorageRequestTracker* const    pTracker
)
{
    //  If we have reached the end of the user's buffer space, we're done.
    if ( pTracker->m_itUserACW == pTracker->m_pIoPacket->m_AccessControlList.end() )
    {
        pTracker->m_pIoPacket->m_Status = EXIOSTAT_SUCCESSFUL;
        completeTracker( pTracker );
        return true;
    }

    //  Do we need to allocate space?
    if ( isAllocationCandidate( pTracker->m_pIoPacket->m_Function ) && !pTracker->m_AllocationDone )
        allocateSpace( pTracker );

    //  We're going to set up a read or a write operation.
    //  Find file-relative track-id containing the first word in the indicated transfer.
    //  This is necessary for converting file-relative track ID to LDATINDEX and device-relative track id.
    TRACK_ID fileTrackId = pTracker->m_NextWordAddress / 1792;
    bool found = pTracker->m_pDiskItem->getFileAllocationTable()->convertTrackId( fileTrackId,
                                                                                  &pTracker->m_PendingLDATIndex,
                                                                                  &pTracker->m_PendingDiskTrackId );
    if ( !found )
    {
        //  Conversion failed - for reads, probably the track is unallocated.  for writes, this is bad news.
        //  Either way, the user IO fails - we just need to figure out which status to send back to the caller.
        if ( isWriteFunction( pTracker->m_pIoPacket->m_Function ) )
            pTracker->m_pIoPacket->m_Status = EXIOSTAT_ADDRESS_TRANSLATION;
        else
            pTracker->m_pIoPacket->m_Status = EXIOSTAT_REGION_NOT_ALLOCATED;
        completeTracker( pTracker );
        return true;
    }

    //  Get the DeviceManager DEVICE_ID and the PREPFACTOR for the physical disk.
    if ( !m_pMFDManager->getDeviceId( pTracker->m_PendingLDATIndex, &pTracker->m_ChildIoDeviceId ) )
    {
        pTracker->m_pIoPacket->m_Status = EXIOSTAT_PACK_UNAVAILABLE;
        completeTracker( pTracker );
        return true;
    }

    if ( !m_pMFDManager->getPrepFactor( pTracker->m_PendingLDATIndex, &pTracker->m_ChildIoPrepFactor ) )
    {
        pTracker->m_pIoPacket->m_Status = EXIOSTAT_PACK_UNAVAILABLE;
        completeTracker( pTracker );
        return true;
    }

    //  Is the first word we want aligned on a block boundary?  How about the word count?
    //  If either the beginning or the end of what we want is non-aligned, we need a child IO buffer.
    COUNT wordOffset = pTracker->m_NextWordAddress % pTracker->m_ChildIoPrepFactor;
    pTracker->m_ChildBufferNeeded = ( (wordOffset > 0) || (pTracker->m_RemainingWordCount < pTracker->m_ChildIoPrepFactor) );

    //  If we didn't need a child buffer (i.e., we're transferring directly to user space)
    //  go ahead and set up the channelprogram ACW(s) based on the user's IO packet.
    if ( !pTracker->m_ChildBufferNeeded )
    {
        pTracker->m_pIoPacket->m_AccessControlList.getSubList( &pTracker->m_pChannelProgram->m_AccessControlList,
                                                               pTracker->m_itUserACW,
                                                               pTracker->m_ChildIoPrepFactor );
        if ( pTracker->m_pChannelProgram->m_AccessControlList.getExtent() != pTracker->m_ChildIoPrepFactor )
        {
            pTracker->m_pIoPacket->m_Status = EXIOSTAT_INTERNAL_ERROR;
            completeTracker( pTracker );
            return true;
        }

        pTracker->m_itUserACW += pTracker->m_ChildIoPrepFactor;
    }

    //  Calculate device-relative block ID for the IO
    BLOCK_ID fileBlockId = pTracker->m_NextWordAddress - wordOffset;
    COUNT blockOffset = static_cast<COUNT>( fileBlockId / 1792 );
    COUNT blocksPerTrack = 1792 / pTracker->m_ChildIoPrepFactor;
    pTracker->m_pChannelProgram->m_Address = pTracker->m_PendingDiskTrackId * blocksPerTrack + blockOffset;

    //  For read operations, the channel command will be a read.
    //  For write operations which do not consume an entire block, again the command will be a read.
    //  Otherwise, it will be a write.
    if ( isWriteFunction( pTracker->m_pIoPacket->m_Function ) && ( !pTracker->m_ChildBufferNeeded ) )
        pTracker->m_pChannelProgram->m_Command = ChannelModule::Command::WRITE;
    else
        pTracker->m_pChannelProgram->m_Command = ChannelModule::Command::READ;

    //  Ready for the IO (well, as ready as we can get without a potentially necessary temp buffer
    pTracker->m_State = RequestTracker::RTST_CHILD_IO_READY;
    return true;
}


//  pollChildIoSetupTape()
//
//  Pull together any information we need in order to prepare for setting up an IO.
//  For tape IO, there's very little to be done.
//  There's actually no good reason for splitting tape IO up among the *Setup* and *Ready*
//  routines -- but we have to do that for mass storage file IO, so we do it for tape also,
//  just for consistency.
//
//  Returns true if the state (or some conceptual sub-state) of the tracker has changed; else false
bool
IoManager::pollChildIoSetupTape
(
    TapeRequestTracker* const   pTracker
)
{
    //  Determine channel program command
    switch ( pTracker->m_pIoPacket->m_Function )
    {
    case EXIOFUNC_GATHER_WRITE:
    case EXIOFUNC_WRITE:
    case EXIOFUNC_WRITE_BY_BDI:
    case EXIOFUNC_WRITE_BY_BDI_EXTENDED:
        //  All of these are consolidated by higher-level code into the same stuff.
        //  Basically, it's just a matter of converting user-space buffer information into ACWS.
        pTracker->m_pChannelProgram->m_Command = ChannelModule::Command::WRITE;
        break;

    case EXIOFUNC_READ:
    case EXIOFUNC_READ_BY_BDI:
    case EXIOFUNC_READ_BY_BDI_EXTENDED:
    case EXIOFUNC_SCATTER_READ:
        //  All of these are consolidated by higher-level code into the same stuff.
        //  Basically, it's just a matter of converting user-space buffer information into ACWS.
        pTracker->m_pChannelProgram->m_Command = ChannelModule::Command::READ;
        break;

        //TODO:TAPE more commands to be handled here

    default:
        //  bad function... what to do????
        break;
    }

    //  Update channel program for the IO (we won't do the path bit yet - we'll leave that for later).
    pTracker->m_pChannelProgram->m_Format = execGetIoTranslateFormat( pTracker->m_pTapeItem->getTapeFormat() );
    pTracker->m_pChannelProgram->m_AccessControlList = pTracker->m_pIoPacket->m_AccessControlList;
    pTracker->m_State = RequestTracker::RTST_CHILD_IO_READY;
    return true;
}


//  pollConsoleMessage()
//
//  The given tracker is (or was) waiting on a read-reply console message.
//  Check whether we've received a response; if so, do something about it.
//
//  Returns true if the state (or some conceptual sub-state) of the tracker has changed; else false
//
//  Operator can respond with some subset of the following responses...
//  the responses accepted are limited by the code which initiated the console message.
//
//  A   Continue processing or retry the failing command
//  B   Unrecoverable error - Requesting program is notified.
//      For Tape, tape position is lost.  For Mass Storage, BadSpot the track
//  G   Unrecoverable error - Requesting program is notified.
//      For tape, tape position is not lost.
//  M   Display diagnostic information and repeat the console message
bool
IoManager::pollConsoleMessage
(
    RequestTracker* const   pTracker
)
{
    //  Console message is canceled indicates EXEC is shutting down
    //  If this is the case, release any child buffer and complete the tracker.
    if ( pTracker->m_pConsoleMessageInfo->m_pConsolePacket->isCancelled() )
    {
        MassStorageRequestTracker* pmsTracker = dynamic_cast<MassStorageRequestTracker*>( pTracker );
        if ( pmsTracker && pmsTracker->m_pChildBuffer )
            detachChildBuffer( pmsTracker );

        completeTracker( pTracker );
        pTracker->m_pIoPacket->m_Status = EXIOSTAT_TASK_ABORT;
        return true;
    }

    //  Console message is completed - means we do something about it.
    if ( pTracker->m_pConsoleMessageInfo->m_pConsolePacket->isCompleted() )
    {
        //  Is the response among the set of accepted responses?
        //  If not, repeat the console message.
        const SuperString& response = pTracker->m_pConsoleMessageInfo->m_pConsolePacket->getResponse();
        if ( pTracker->m_pConsoleMessageInfo->m_AcceptedResponses.find( response[0] ) == std::string::npos )
        {
            repostConsoleMessage( pTracker, true );
            return true;
        }

        //  We have an acceptable response - handle it
        if ( response.compareNoCase( "A" ) == 0 )
        {
            //  Retry the IO
            delete pTracker->m_pConsoleMessageInfo;
            pTracker->m_pConsoleMessageInfo = 0;
            pTracker->m_RetryFlag = true;
            pTracker->m_State = RequestTracker::RTST_CHILD_IO_READY;
            return true;
        }

        if ( response.compareNoCase( "B" ) == 0 )
        {
            //  Unrecoverable error - for tapes, set position lost; for disks, badtrack the track.
            if ( pTracker->m_pIoPacket->m_pFacItem->isTape() )
                pTracker->m_PendingStatus = EXIOSTAT_LOSS_OF_POSITION;
            else if ( pTracker->m_pIoPacket->m_pFacItem->isSectorMassStorage()
                    || pTracker->m_pIoPacket->m_pFacItem->isWordMassStorage() )
            {
                MassStorageRequestTracker* pmsTracker = dynamic_cast<MassStorageRequestTracker*>( pTracker );
                m_pMFDManager->setBadTrack( pTracker->m_pIoPacket->m_pOwnerActivity,
                                            pmsTracker->m_PendingLDATIndex,
                                            pmsTracker->m_PendingDiskTrackId );
            }

            //  Return an IO error to the caller.
            pTracker->m_pIoPacket->m_Status = pTracker->m_PendingStatus;
            delete pTracker->m_pConsoleMessageInfo;
            pTracker->m_pConsoleMessageInfo = 0;
            return true;
        }

        if ( response.compareNoCase( "G" ) == 0 )
        {
            //  Unrecoverable error - return an IO error to the caller.
            pTracker->m_pIoPacket->m_Status = pTracker->m_PendingStatus;
            delete pTracker->m_pConsoleMessageInfo;
            pTracker->m_pConsoleMessageInfo = 0;
            return true;
        }

        if ( response.compareNoCase( "M" ) == 0 )
        {
            postConsoleMessageDetail( pTracker );
            repostConsoleMessage( pTracker, false );
            return true;
        }
    }

    return false;
}


//  pollNew()
//
//  Redirector
//
//  Returns true if the state (or some conceptual sub-state) of the tracker has changed; else false
inline bool
IoManager::pollNew
(
    RequestTracker* const   pTracker
)
{
    pTracker->m_pIoPacket->m_AbnormalFrameCount = 0;
    pTracker->m_pIoPacket->m_FinalWordCount = 0;

    switch ( pTracker->m_Type )
    {
    case RequestTracker::RTTYPE_MASS_STORAGE:
        return pollNewMassStorage( dynamic_cast<MassStorageRequestTracker*>( pTracker ) );

    case RequestTracker::RTTYPE_TAPE:
        return pollNewTape( dynamic_cast<TapeRequestTracker*>( pTracker ) );
    }

    pTracker->m_pIoPacket->m_Status = EXIOSTAT_INTERNAL_ERROR;
    completeTracker( pTracker );
    return true;
}


//  pollNewMassStorage()
//
//  Handling a new request tracker for mass storage file IO.
//  Filter out obvious wrongness...
//
//  Returns true if the state (or some conceptual sub-state) of the tracker has changed; else false
bool
IoManager::pollNewMassStorage
(
    MassStorageRequestTracker* const    pTracker
)
{
    //  Filter out bad functions
    switch ( pTracker->m_pIoPacket->m_Function )
    {
    case EXIOFUNC_WRITE_BY_BDI:
    case EXIOFUNC_WRITE_BY_BDI_EXTENDED:
    case EXIOFUNC_WRITE:
    case EXIOFUNC_GATHER_WRITE:
    case EXIOFUNC_ACQUIRE:
    case EXIOFUNC_EXTENDED_ACQUIRE:
    case EXIOFUNC_READ:
    case EXIOFUNC_READ_AND_RELEASE:
    case EXIOFUNC_RELEASE:
    case EXIOFUNC_READ_AND_LOCK:
    case EXIOFUNC_UNLOCK:
    case EXIOFUNC_EXTENDED_RELEASE:
    case EXIOFUNC_SCATTER_READ:
    case EXIOFUNC_READ_BY_BDI:
    case EXIOFUNC_READ_BY_BDI_EXTENDED:
    case EXIOFUNC_FILE_UPDATE_WAIT:
        break;

    default:
        pTracker->m_pIoPacket->m_Status = EXIOSTAT_INVALID_FUNCTION_CODE;
        completeTracker( pTracker );
        return true;
    }

    pTracker->m_StartingWordAddress = pTracker->m_pIoPacket->m_Address;
    if ( pTracker->m_pIoPacket->m_pFacItem->isSectorMassStorage() )
    {
        pTracker->m_StartingWordAddress *= 28;
    }

    pTracker->m_TotalWordCount = pTracker->m_pIoPacket->m_AccessControlList.getExtent();
    pTracker->m_NextWordAddress = pTracker->m_StartingWordAddress;
    pTracker->m_RemainingWordCount = pTracker->m_TotalWordCount;

    pTracker->m_State = RequestTracker::RTST_CHILD_IO_SETUP;
    return true;
}


//  pollNewTape()
//
//  Handling a new request tracker for tape IO.
//  Filter out obvious wrongness, and move on to the next state if we're okay so far.
//
//  Returns true if the state (or some conceptual sub-state) of the tracker has changed; else false
bool
IoManager::pollNewTape
(
    TapeRequestTracker* const pTracker
)
{
    //  Filter out unaccepted function codes
    switch ( pTracker->m_pIoPacket->m_Function )
    {
    case EXIOFUNC_GATHER_WRITE:
    case EXIOFUNC_WRITE:
    case EXIOFUNC_WRITE_BY_BDI:
    case EXIOFUNC_WRITE_BY_BDI_EXTENDED:
    case EXIOFUNC_READ:
    case EXIOFUNC_READ_BY_BDI:
    case EXIOFUNC_READ_BY_BDI_EXTENDED:
    case EXIOFUNC_SCATTER_READ:
        break;

    case EXIOFUNC_BACK_SPACE_FILE:
    case EXIOFUNC_FORWARD_SPACE_FILE:
    case EXIOFUNC_MODE_SET:
    case EXIOFUNC_MOVE_BACKWARD:
    case EXIOFUNC_MOVE_FORWARD:
    case EXIOFUNC_MULTIREQUEST:
    case EXIOFUNC_READ_BACKWARD:
    case EXIOFUNC_READ_BACKWARD_BY_BDI:
    case EXIOFUNC_REWIND:
    case EXIOFUNC_REWIND_WITH_INTERLOCK:
    case EXIOFUNC_SCATTER_READ_BACKWARD:
    case EXIOFUNC_SET_MODE:
    case EXIOFUNC_WRITE_END_OF_FILE:
        //TODO:TAPE  Not implemented yet - fall-through to default
    default:
        pTracker->m_pIoPacket->m_Status = EXIOSTAT_INVALID_FUNCTION_CODE;
        completeTracker( pTracker );
        return true;
    }

    //  Do some checking which only applies to writes
    if ( (pTracker->m_pIoPacket->m_Function == EXIOFUNC_WRITE)
        || (pTracker->m_pIoPacket->m_Function == EXIOFUNC_WRITE_BY_BDI)
        || (pTracker->m_pIoPacket->m_Function == EXIOFUNC_WRITE_BY_BDI_EXTENDED) )
    {
        //  Filter out writes-too-small...
        COUNT byteCount = 0;
        COUNT wordCount = pTracker->m_pIoPacket->m_AccessControlList.getExtent();
        switch ( pTracker->m_pTapeItem->getTapeFormat() )
        {
        case TFMT_QUARTER_WORD:
        case TFMT_QUARTER_WORD_IGNORE:
            byteCount = 4 * wordCount;
            break;
        case TFMT_SIX_BIT_PACKED:
            byteCount = 6 * wordCount;
            break;
        case TFMT_EIGHT_BIT_PACKED:
            byteCount = ((wordCount >> 1) * 4) + ((wordCount >> 1) * 5);
            if ( wordCount % 2 )
                ++byteCount;
            break;
        }

        if ( byteCount <= pTracker->m_pTapeItem->getNoiseConstant() )
        {
            pTracker->m_pIoPacket->m_Status = EXIOSTAT_WRITE_LESS_THAN_NOISE;
            completeTracker( pTracker );
            return true;
        }

        //  Filter out writes-too-big...
        wordCount = pTracker->m_pIoPacket->m_AccessControlList.getExtent();
        if ( wordCount > (131 * 1024) )
        {
            pTracker->m_pIoPacket->m_Status = EXIOSTAT_WRITE_TOO_LARGE;
            completeTracker( pTracker );
            return true;
        }
    }

    pTracker->m_State = RequestTracker::RTST_CHILD_IO_SETUP;
    return true;
}


//  postConsoleMessage()
//
//  Given appropriate inputs, we create a ConsoleMessageInfo object, post the message,
//  link it to the tracker, and update the tracker state.
void
IoManager::postConsoleMessage
(
    RequestTracker* const   pTracker,
    const std::string&      errorMnemonic,
    const std::string&      acceptedResponses
) const
{
    ConsoleMessageInfo* pConsInfo = new ConsoleMessageInfo();
    pConsInfo->m_AcceptedResponses = acceptedResponses;

    pConsInfo->m_Message = m_pDeviceManager->getNodeName( pTracker->m_pChildIoPath->m_DeviceIdentifier );
    pConsInfo->m_Message += " ";
    pConsInfo->m_Message += "/" + m_pDeviceManager->getNodeName( pTracker->m_pChildIoPath->m_IOPIdentifier );
    pConsInfo->m_Message += "/" + m_pDeviceManager->getNodeName( pTracker->m_pChildIoPath->m_ChannelModuleIdentifier );
    pConsInfo->m_Message += "/" + m_pDeviceManager->getNodeName( pTracker->m_pChildIoPath->m_ControllerIdentifier );
    pConsInfo->m_Message += " " + errorMnemonic;
    pConsInfo->m_Message += " ";
    pConsInfo->m_Message += getFunctionMnemonic( pTracker->m_pIoPacket->m_Function );
    pConsInfo->m_Message += " " + pTracker->m_pIoPacket->m_pOwnerActivity->getRunInfo()->getActualRunId();
    pConsInfo->m_Message += " " + pConsInfo->m_AcceptedResponses;

    pConsInfo->m_pConsolePacket = new ConsoleManager::ReadReplyRequest( pTracker->m_pIoPacket->m_pOwnerActivity->getRunInfo(),
                                                                        0,
                                                                        ConsoleManager::Group::IO_ACTIVITY,
                                                                        pConsInfo->m_Message,
                                                                        1 );
    m_pConsoleManager->postReadReplyMessage( pConsInfo->m_pConsolePacket, false );
    pTracker->m_pConsoleMessageInfo = pConsInfo;
    pTracker->m_State = RequestTracker::RTST_CONSOLE_MESSAGE_PENDING;
}


//  postConsoleMessageDetail()
//
//  Posts error details - invoked via the M response to a console message.
void
IoManager::postConsoleMessageDetail
(
    RequestTracker* const   pTracker
) const
{
    RunInfo* prinfo = pTracker->m_pIoPacket->m_pActivity->getRunInfo();
    std::string deviceName = m_pDeviceManager->getNodeName( pTracker->m_pChildIoPath->m_DeviceIdentifier );
    StandardFacilityItem* psfac = dynamic_cast<StandardFacilityItem*>( pTracker->m_pIoPacket->m_pFacItem );

    std::stringstream msgStream;

    //  The following is not currently used, because we don't go through here for DIR$ functions
    //  B.4.1. Subfunction Line
    //      The subfunction line for the M response is an optional line that is not dependent on
    //      the device type. This line appears for DIR$ functions only, such as READ VOL1, READ
    //      SECTOR1, and WRITE SECTOR1.
    //  Format
    //      targ-dev DEVICE = device SUBFUNCTION = subfunction
    //  where:
    //  targ-dev    is the name of the target device that had the error.
    //  device      is the name of the device that was used to issue the I/O that resulted in the error.
    //  subfunction is one of the list DIR$ subfunctions: RDVOL1, RDSEC1, DEVCAP, WRVOL1, or WRSEC1

    //  The following is not currently used, because we don't do UTIL functions (at the mooment, anyway)
    //  B.4.2 Hardware Function Line
    //      The hardware function line of the M response is an optional line that appears for UTIL
    //      functions only. The information provided depends on the type of function and the
    //      device on which the error appeared.
    //  Format
    //      device DEVICE = ctrl-unit H/W CMD = h/w-func
    //  where:
    //      device      is the name of the device in error.
    //      ctrl-unit   is the name of the control device in error.
    //      h/w-func    is the hardware command that found the error.
    //  Table B�7 lists and describes the word mnemonics that appear on the hardware function line for UTIL functions.
    //  Table B�7. Word Interface Hardware Function Mnemonics (UTIL Function Only)
    //      Mnemonic    Function
    //      CLRICD      Clear internal cache down indicator
    //      CLRTME      Clear segment timestamp
    //      DELSEG      Delete segment
    //      DRAIN       Drain
    //      INIT        Initialize
    //      PARAM       Parameterize
    //      PRPTRK      Prep track
    //      RDFSI       Read file status indicator
    //      RDHID       Read Hardware ID attempt
    //      RDICD       Read ICD data
    //      RDICDX      Read ICD data and microcode trace
    //      RDMLEV      Read microcode revision lLevel
    //      RDPAGE      Read mode page attempt
    //      RDTABL      Read tables

    //  Table B�8 describes mnemonics for half-inch cartridge tape devices.
    //  Table B�8. Half-Inch Cartridge Tape Device Mnemonics
    //      Mnemonic    Function
    //      ASSIGN      Assign
    //      CLRPG       The LOG-SENSE or the LOG-SELECT command could not be performed to the device
    //      CONACC      Control access
    //      INQRY       Inquiry
    //      LOADCR      Load cartridge
    //      LOADSP      Load display
    //      MODSNS      The MODE-SENSE command could not be performed to the device
    //      PROCLN
    //      REPDEN
    //      SPGID       Set path group-id
    //      UNASGN      Unassign

    //  Emit the Run/File Line
    //      device RUNID = program Q*F = qual*file(cycle)
    msgStream.str() = deviceName;
    msgStream << " RUNID=" << prinfo->getActualRunId()
                << " Q*F=" << psfac->getQualifier()
                << "*" << psfac->getFileName()
                << "(" << (psfac->getAbsoluteFileCycleExists() ? psfac->getAbsoluteFileCycle() : 0)
                << ").";
    m_pConsoleManager->postReadOnlyMessage( msgStream.str(), prinfo );

    //  We do have a channel status, so we'll use that in lieu of the CSW's
    msgStream.str() = deviceName;
    msgStream << "CSW0-3="
        << std::oct << std::setw(12) << std::setfill( '0' )
        << static_cast<UINT32>(pTracker->m_pChannelProgram->m_ChannelStatus)
        << " " << 0 << " " << 0 << " " << 0;
    m_pConsoleManager->postReadOnlyMessage( msgStream.str(), prinfo );

    //  Emit sense byte lines if there are any sense bytes
    COUNT senseBytes = pTracker->m_pChannelProgram->m_SenseBytes.size();
    if ( senseBytes > 0 )
    {
        const VBYTE& senseData = pTracker->m_pChannelProgram->m_SenseBytes;
        INDEX sdx = 0;
        while ( sdx < senseBytes )
        {
            msgStream.str() = deviceName;
            msgStream << " SB " << sdx << "-" << (sdx + 15) << " =";
            for ( INDEX sdy = 0; sdy < 15; ++sdy )
            {
                if ( (sdx + sdy) < senseBytes )
                {
                    msgStream << " " << std::setw( 2 ) << std::setfill( '0' ) << std::hex << senseData[sdx + sdy];
                }
            }

            m_pConsoleManager->postReadOnlyMessage( msgStream.str(), prinfo );
            sdx += 16;
        }
    }

    //  Emit channel and device status values (this is non-standard, but useful)
    msgStream.str() = deviceName;
    msgStream << " CHSTAT=" << ChannelModule::getStatusString( pTracker->m_pChannelProgram->m_ChannelStatus );
    msgStream << " DEVSTAT=" << Device::getIoStatusString( pTracker->m_pChannelProgram->m_DeviceStatus,
                                                            pTracker->m_pChannelProgram->m_SystemErrorCode );
    m_pConsoleManager->postReadOnlyMessage( msgStream.str(), prinfo );

    //  For disk I/O, display device-relative word address
    if ( pTracker->m_pIoPacket->m_pFacItem->isSectorMassStorage()
        || pTracker->m_pIoPacket->m_pFacItem->isWordMassStorage() )
    {
        MassStorageRequestTracker* pmsTracker = dynamic_cast<MassStorageRequestTracker*>( pTracker );
        msgStream.str() = deviceName;
        msgStream << " DRA=0" << std::oct << (pmsTracker->m_PendingDiskTrackId * 1792);
        m_pConsoleManager->postReadOnlyMessage( msgStream.str(), prinfo );
    }

    //  For tape I/O, display number of files and blocks extended
    else if ( pTracker->m_pIoPacket->m_pFacItem->isTape() )
    {
        TapeFacilityItem* pfi = dynamic_cast<TapeFacilityItem*>( pTracker->m_pIoPacket->m_pFacItem );
        msgStream.str() = deviceName;
        msgStream << " NFE =  " << pfi->getFilesExtended() << " NBE = " << pfi->getBlocksExtended();
        m_pConsoleManager->postReadOnlyMessage( msgStream.str(), prinfo );
    }
}


//  repostConsoleMessage()
//
//  Reposts the current console message, with optional '?' prepended to the original message
void
IoManager::repostConsoleMessage
(
    RequestTracker* const   pTracker,
    const bool              prependQuery
) const
{
    delete pTracker->m_pConsoleMessageInfo->m_pConsolePacket;

    std::string message;
    if ( prependQuery )
        message = "?";
    message += pTracker->m_pConsoleMessageInfo->m_Message;

    pTracker->m_pConsoleMessageInfo->m_pConsolePacket =
            new ConsoleManager::ReadReplyRequest( pTracker->m_pIoPacket->m_pOwnerActivity->getRunInfo(),
                                                  0,
                                                  ConsoleManager::Group::IO_ACTIVITY,
                                                  message,
                                                  1 );
    m_pConsoleManager->postReadReplyMessage( pTracker->m_pConsoleMessageInfo->m_pConsolePacket, false );
    pTracker->m_State = RequestTracker::RTST_CONSOLE_MESSAGE_PENDING;
}



//  private statics

//  convertMFDResult()
//
//  Converts an MFDResult value to something approximating usefullness in terms of an ExecIoStatus value
ExecIoStatus
IoManager::convertMFDResult
(
    const MFDManager::Result&   result
)
{
    //  Many MFD status values just don't apply - we don't call any MFD routines which return them.
    switch ( result.m_Status )
    {
    case MFDManager::MFDST_IO_ERROR:                return EXIOSTAT_UNIT_EXCEPTION;
    case MFDManager::MFDST_MAX_GRANULES_EXCEEDED:   return EXIOSTAT_ADDRESS_BEYOND_MAXIMUM;
    case MFDManager::MFDST_OUT_OF_SPACE:            return EXIOSTAT_MASS_STORAGE_OVERFLOW;
    case MFDManager::MFDST_SUCCESSFUL:              return EXIOSTAT_SUCCESSFUL;
    case MFDManager::MFDST_TERMINATING:             return EXIOSTAT_TASK_ABORT;
    default:                                        return EXIOSTAT_INTERNAL_ERROR;
    }
}


//  getFunctionMnemonic()
//
//  Converts a child IO function code to a displayable function mnemonic
const char*
IoManager::getFunctionMnemonic
(
    const ExecIoFunction    function
)
{
    switch ( function )
    {
    case EXIOFUNC_ACQUIRE:
    case EXIOFUNC_EXTENDED_ACQUIRE:
    case EXIOFUNC_EXTENDED_RELEASE:
    case EXIOFUNC_UNLOCK:
        //  Should never get here for these IOFUNC's
        return "???";

    case EXIOFUNC_BACK_SPACE_FILE:          return "BSF";   //  BSF Back space file
    case EXIOFUNC_BLOCK_ID_SAFE:            return "BSAFE"; //  BSAFE Block ID safe
    case EXIOFUNC_BLOCK_READ_DRUM:          return "na";    //  The heck is this?
    case EXIOFUNC_BLOCK_SEARCH_DRUM:        return "na";    //  ditto?
    case EXIOFUNC_BLOCK_SEARCH_READ_DRUM:   return "na";    //  ditto?
    case EXIOFUNC_END:                      return "ENDH";  //  ENDH HYPERchannel Clear adapter activity on subchannel
        //  DIR Disk only read VOL1, read/write sector 1
        //  DSCLS Data set closed
        //  DSOPN Data set open
        //  FEEDX Feed
    case EXIOFUNC_FEP_INITIALIZATION:       return "UTIL";
    case EXIOFUNC_FEP_TERMINATION:          return "UTIL";
    case EXIOFUNC_FILE_UPDATE_WAIT:         return "FSAFE"; //  FSAFE File update wait
    case EXIOFUNC_FORWARD_SPACE_FILE:       return "FSF";   //  FSF Forward space file
    case EXIOFUNC_GATHER_WRITE:             return "GW";    //  GW Gather write
    case EXIOFUNC_INPUT_DATA:               return "ID";    //  ID Input data HYPERchannel
    case EXIOFUNC_LOAD_CODE_CONV_BANK:      return "LCCB";  //  LCCB Load Code Conversion Bank HYPERchannel
    case EXIOFUNC_LOCATE_BLOCK:             return "RPOSF"; //  RPOSF Reposition forward... ?
    case EXIOFUNC_MODE_SET:                 return "SM";
    case EXIOFUNC_MOVE_BACKWARD:            return "MB";    //  MB Move backward
    case EXIOFUNC_MOVE_FORWARD:             return "MF";    //  MF Move forward
        //  MS Mode sense
    case EXIOFUNC_MULTIREQUEST:             return "MR";    //  MR Multiple request
    case EXIOFUNC_POSITION_SEARCH_ALL:      return "???";
    case EXIOFUNC_POSITION_SEARCH_FIRST:    return "???";
        //  PHYCAP Read physical characteristics
        //  PROB Probe
    case EXIOFUNC_READ:                     return "R";     //  R Read
    case EXIOFUNC_READ_AND_LOCK:            return "R";     //  R Read
    case EXIOFUNC_READ_AND_RELEASE:         return "R";     //  R Read
    case EXIOFUNC_READ_BACKWARD:            return "RB";    //  RB Read backward
    case EXIOFUNC_READ_BY_BDI:              return "BDR";   //  BDR Read by BDI
    case EXIOFUNC_READ_BACKWARD_BY_BDI:     return "BDRB";  //  BDRB Read backward by BDI
    case EXIOFUNC_READ_BLOCK_IDENTIFIER:    return "???";
    case EXIOFUNC_READ_BLOCK_ID_BEFORE_WRITE:return "???";
        //  RCTL Read control
        //  RDRA Read using device relative address
    case EXIOFUNC_READ_BY_BDI_EXTENDED:     return "BDR";
    case EXIOFUNC_REWIND:                   return "REW";   //  REW Rewind
    case EXIOFUNC_REWIND_WITH_INTERLOCK:    return "REWI";  //  REWI Rewind with interlock
    case EXIOFUNC_SCATTER_READ:             return "SCR";   //  SCR Scatter read
    case EXIOFUNC_SCATTER_READ_BACKWARD:    return "SCRB";  //  SCRB Scatter read backward
        //  RPOSB Reposition backward
        //  RPOSF Reposition forward
    case EXIOFUNC_SEARCH_DRUM:
    case EXIOFUNC_SEARCH_READ_DRUM:
    case EXIOFUNC_SENSE_STATISTICS:         return "SS";    //  SS Sense statistics HYPERchannel
    case EXIOFUNC_SET_MODE:                 return "SM";    //  SM Set mode
    case EXIOFUNC_SET_TEST:                 return "ST";    //  ST Set test?HYPERchannel
    case EXIOFUNC_TRACK_SEARCH_ALL:         return "???";
    case EXIOFUNC_TRACK_SEARCH_FIRST:       return "???";
        //  SW Skip write
        //  TR Test and Read
        //  TW Test and Write
        //  UNLD Unload
        //  UTIL Utility
        //  VOL1 Read volume label
    case EXIOFUNC_WRITE:                    return "W";     //  W Write
    case EXIOFUNC_WRITE_ADDRESS_AND_LENGTH: return "WALL";  //  WALL Write address and lock HYPERchannel
    case EXIOFUNC_WRITE_BY_BDI:             return "BDW";   //  BDW Write by BDI
    case EXIOFUNC_WRITE_BY_BDI_EXTENDED:    return "BDW";   //  BDW Write by BDI
        //  WCTL Write control
        //  WDRA Write using device relative address
    case EXIOFUNC_WRITE_END_OF_FILE:        return "WEF";   //  WEF Write end of file
    default:                                return "???";
    }
}


//  isAllocationCandidate()
//
//  Does the indicated function imply or explicitly involve allocating disk space?
bool
IoManager::isAllocationCandidate
(
    const ExecIoFunction    function
)
{
    switch ( function )
    {
    case EXIOFUNC_WRITE_BY_BDI:
    case EXIOFUNC_WRITE_BY_BDI_EXTENDED:
    case EXIOFUNC_WRITE:
    case EXIOFUNC_GATHER_WRITE:
    case EXIOFUNC_ACQUIRE:
    case EXIOFUNC_EXTENDED_ACQUIRE:
        return true;
    default:
        return false;
    }
}


//  isWriteFunction()
//
//  Does the indicated function involve writing data?
bool
IoManager::isWriteFunction
(
    const ExecIoFunction    function
)
{
    switch ( function )
    {
    case EXIOFUNC_WRITE_BY_BDI:
    case EXIOFUNC_WRITE_BY_BDI_EXTENDED:
    case EXIOFUNC_WRITE:
    case EXIOFUNC_GATHER_WRITE:
        return true;
    default:
        return false;
    }
}



//  Constructors, destructors

IoManager::IoManager
(
    Exec* const         pExec
)
:ExecManager( pExec ),
m_pConsoleManager( dynamic_cast<ConsoleManager*>( m_pExec->getManager( Exec::MID_CONSOLE_MANAGER ) ) ),
m_pDeviceManager( dynamic_cast<DeviceManager*>( m_pExec->getManager( Exec::MID_DEVICE_MANAGER ) ) ),
m_pMFDManager( dynamic_cast<MFDManager*>( m_pExec->getManager( Exec::MID_MFD_MANAGER ) ) )
{
    m_pIoActivity = 0;
}



//  Public methods

//  cleanup()
void
IoManager::cleanup()
{
    //  Release all the child iO buffers
    for ( ITCHILDBUFFERS itcb = m_ChildBuffersAvailable.begin(); itcb != m_ChildBuffersAvailable.end(); ++itcb )
        delete *itcb;
    m_ChildBuffersAvailable.clear();

    for ( ITCHILDBUFFERS itcb = m_ChildBuffersInUse.begin(); itcb != m_ChildBuffersInUse.end(); ++itcb )
        delete *itcb;
    m_ChildBuffersInUse.clear();
}


//  dump()
//
//  For debugging
void
IoManager::dump
(
    std::ostream&       stream,
    const DUMPBITS      dumpBits
)
{
    stream << "IoManager ----------" << std::endl;
    ExecManager::dump( stream, dumpBits );

    stream << "  Available buffers:" << std::endl;
    for ( ITCHILDBUFFERS itcb = m_ChildBuffersAvailable.begin(); itcb != m_ChildBuffersAvailable.end(); ++itcb )
        stream << "    0x" << std::hex << reinterpret_cast<void *>( *itcb ) << std::endl;
    stream << "  Attached buffers:" << std::endl;
    for ( ITCHILDBUFFERS itcb = m_ChildBuffersInUse.begin(); itcb != m_ChildBuffersInUse.end(); ++itcb )
        stream << "    0x" << std::hex << reinterpret_cast<void *>( *itcb ) << std::endl;

    stream << "  Pending Requests:" << std::endl;
    for ( CITREQUESTS itr = m_PendingRequests.begin(); itr != m_PendingRequests.end(); ++itr )
    {
        stream << "    Type:" << getRequestTrackerTypeString( (*itr)->m_Type )
                << " State:" << getRequestTrackerStateString( (*itr)->m_State )
                << " Retry:" << ((*itr)->m_RetryFlag ? "YES" : "NO")
                << std::endl;

        const IoPacket* pPkt = (*itr)->m_pIoPacket;
        if ( pPkt )
        {
            stream << "      IoPkt Run:" << pPkt->m_pActivity->getRunInfo()->getActualRunId()
                    << " Owner:" << pPkt->m_pOwnerActivity->getRunInfo()->getActualRunId()
                    << " File:" << pPkt->m_pFacItem->getQualifier() << "*" << pPkt->m_pFacItem->getFileName()
                    << " Fnc:" << miscGetExecIoFunctionString( pPkt->m_Function )
                    << " Addr:0" << std::oct << pPkt->m_Address
                    << " Stat:0" << std::oct << pPkt->m_Status
                    << " AFCnt:0" << std::oct << pPkt->m_AbnormalFrameCount
                    << " FWCnt:0" << std::oct << pPkt->m_FinalWordCount
                    << std::endl;
            pPkt->m_AccessControlList.dump( stream, "    " );
        }

        MassStorageRequestTracker* pDiskTracker = dynamic_cast<MassStorageRequestTracker*>(*itr);
        if ( pDiskTracker != 0 )
        {
            stream << "      Allocation Done:       " << (pDiskTracker->m_AllocationDone ? "YES" : "NO") << std::endl;
            stream << "      Child Buffer Needed:   " << (pDiskTracker->m_ChildBufferNeeded ? "YES" : "NO") << std::endl;
            stream << "      Child IO Device Id:    " << std::dec << pDiskTracker->m_ChildIoDeviceId << std::endl;
            stream << "      Child IO Prep Factor:  " << std::dec << pDiskTracker->m_ChildIoPrepFactor << std::endl;
            stream << "      Attached Child Buffer: 0x" << std::hex << reinterpret_cast<void *>(pDiskTracker->m_pChildBuffer) << std::endl;
            stream << "      Next Word Addr:        0" << std::oct << pDiskTracker->m_NextWordAddress << std::endl;
            stream << "      Remaining Word Count:  0" << std::oct << pDiskTracker->m_RemainingWordCount << std::endl;
            stream << "      Starting Word Address: 0" << std::oct << pDiskTracker->m_StartingWordAddress << std::endl;
            stream << "      Total Word Count:      0" << std::oct << pDiskTracker->m_TotalWordCount << std::endl;
            stream << "      Current User Buffer:   0x" << std::dec << reinterpret_cast<void *>(*(pDiskTracker->m_itUserACW))
                    << "  Words Remaining:0" << std::oct << pDiskTracker->m_itUserACW.acwRemaining()
                    << std::endl;
        }

        TapeRequestTracker* pTapeTracker = dynamic_cast<TapeRequestTracker*>(*itr);
        if ( pTapeTracker != 0 )
        {
            //TODO:TAPE
        }

        const ChannelModule::ChannelProgram* pProg = (*itr)->m_pChannelProgram;
        if ( pProg )
        {
            stream << "      ChProg IoPath:"
                    << pProg->m_ProcessorUPI << "/"
                    << pProg->m_ChannelModuleAddress << "/"
                    << pProg->m_ControllerAddress << "/"
                    << pProg->m_DeviceAddress
                    << " Cmd:" << ChannelModule::getCommandString( pProg->m_Command )
                    << " Addr:0" << std::oct << pProg->m_Address
                    << " Fmt:" << static_cast<UINT32>(pProg->m_Format)
                    << " XferSz:0" << std::oct << pProg->m_TransferSizeWords
                    << std::endl;
            pProg->m_AccessControlList.dump( stream, "      " );
            stream << "      ChanStat:" << ChannelModule::getStatusString( pProg->m_ChannelStatus )
                    << " DevSt:" << Device::getIoStatusString( pProg->m_DeviceStatus, pProg->m_SystemErrorCode )
                    << " BytesXferd:0" << std::oct << pProg->m_BytesTransferred
                    << " WordsXferd:0" << std::oct << pProg->m_WordsTransferred
                    << std::endl;

        }

        ConsoleMessageInfo* pcmInfo = (*itr)->m_pConsoleMessageInfo;
        if ( pcmInfo )
        {
            stream << "      ConsMsg:" << pcmInfo->m_Message;
        }
    }
}


//  pollPendingRequests()
//
//  Checks the list of pending requests to see if any of the need attention.
//  Returns true if we did something useful.
bool
IoManager::pollPendingRequests()
{
    bool didSomething = false;
    lock();

    ITREQUESTS itr = m_PendingRequests.begin();
    while ( itr != m_PendingRequests.end() )
    {
        switch ( (*itr)->m_State )
        {
        case RequestTracker::RTST_CHILD_IO_DONE:
            if ( pollChildIoDone( *itr ) )
                didSomething = true;
            ++itr;
            break;

        case RequestTracker::RTST_CHILD_IO_IN_PROGRESS:
            if ( pollChildIoInProgress( *itr ) )
                didSomething = true;
            ++itr;
            break;

        case RequestTracker::RTST_CHILD_IO_READY:
            if ( pollChildIoReady( *itr ) )
                didSomething = true;
            ++itr;
            break;

        case RequestTracker::RTST_CHILD_IO_SETUP:
            if ( pollChildIoSetup( *itr ) )
                didSomething = true;
            ++itr;
            break;

        case RequestTracker::RTST_COMPLETED:
            //  We are done with this tracker, and the caller has already been notified of
            //  the resulting status - all we do here is release the RequestTracker packet.
            delete *itr;
            itr = m_PendingRequests.erase( itr );
            didSomething = true;
            break;

        case RequestTracker::RTST_CONSOLE_MESSAGE_PENDING:
            if ( pollConsoleMessage( *itr ) )
                didSomething = true;
            ++itr;
            break;

        case RequestTracker::RTST_NEW:
            if ( pollNew( *itr ) )
                didSomething = true;
            ++itr;
            break;
        }
    }

    unlock();
    return didSomething;
}


//  shutdown()
//
//  Exec would like to shut down
void
IoManager::shutdown()
{
    SystemLog::write("IoManager::shutdown()");
}


//  startIo()
//
//  Verifies a couple of very minor things, and then creates a new RequestTracker for the io packet.
void
IoManager::startIo
(
    IoPacket* const         pIoPacket
)
{
    if ( m_pIoActivity->isTerminating() )
    {
        pIoPacket->m_Status = EXIOSTAT_INTERNAL_ERROR;
        pIoPacket->m_pActivity->signal();
    }
    else
    {
        lock();
        if ( pIoPacket->m_pFacItem->isTape() )
            m_PendingRequests.push_back( new TapeRequestTracker( pIoPacket, m_pIoActivity ) );
        else if ( pIoPacket->m_pFacItem->isSectorMassStorage() || pIoPacket->m_pFacItem->isWordMassStorage() )
            m_PendingRequests.push_back( new MassStorageRequestTracker( pIoPacket, m_pIoActivity ) );
        unlock();
    }
}


//  startup()
//
//  Initializes the manager - Exec is booting
bool
IoManager::startup()
{
    SystemLog::write("IoManager::startup()");

    //  Allocate buffers
    while ( m_ChildBuffersAvailable.size() < m_ConcurrentDiskIos )
        m_ChildBuffersAvailable.insert( new Word36[1792] );
    return true;
}


//  terminate()
//
//  Final termination of exec
void
IoManager::terminate()
{
    SystemLog::write("IoManager::terminate()");
}



//  public statics

std::string
IoManager::getRequestTrackerStateString
(
    const RequestTracker::State     state
)
{
    switch ( state )
    {
    case RequestTracker::RTST_CHILD_IO_DONE:            return "ChildIoDone";
    case RequestTracker::RTST_CHILD_IO_IN_PROGRESS:     return "ChildIoInProgress";
    case RequestTracker::RTST_CHILD_IO_READY:           return "ChildIoReady";
    case RequestTracker::RTST_CHILD_IO_SETUP:           return "ChildIoSetup";
    case RequestTracker::RTST_COMPLETED:                return "Completed";
    case RequestTracker::RTST_CONSOLE_MESSAGE_PENDING:  return "ConsMsgPending";
    case RequestTracker::RTST_NEW:                      return "New";
    }

    return "???";
}


std::string
IoManager::getRequestTrackerTypeString
(
    const RequestTracker::Type      type
)
{
    switch ( type )
    {
    case RequestTracker::RTTYPE_MASS_STORAGE:   return "MassStorage";
    case RequestTracker::RTTYPE_TAPE:           return "Tape";
    }

    return "???";
}

