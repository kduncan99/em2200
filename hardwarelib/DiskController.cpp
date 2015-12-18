//  DiskController implementation
//
//  Not much here



#include    "hardwarelib.h"



//  private, protected methods



//  constructors, destructors



//  public methods

//  getContainingBufferSize()
//
//  ChannelModule wants to send {n} bytes; we tell it how many bytes it should really send
//  so that it can allocate a proper buffer size and send an IO which the disk will not reject.
COUNT
DiskController::getContainingBufferSize
(
    const COUNT requestedBufferSize
) const
{
    //  classic bit-wise round-up-to-power-of-2 algorithm, modified for 64-bits
    COUNT value = requestedBufferSize - 1;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return value + 1;
}

