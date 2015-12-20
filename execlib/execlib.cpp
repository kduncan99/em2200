//  execlib.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//  miscellaneous static functions



#include    "execlib.h"



//  execCompareAbsoluteCycles()
//
//  Compares two absolute cycle values to determine whether the first is less than, equal to, or greater than the second.
//  The comparison actually depends on the cyclical nature of the cycle range.
//  e.g., 1 is greater than 999, since 999 + 1 -> 1.
//  However, 800 is *not* greater than 999.
//  The comparison horizon is essentially half the range - that is, for any given cycle, half of the other cycles sort
//  greater, and the other half sort less than, the given cycle.
//
//  Returns:
//      -1  if cycle1 is less than cycle2
//      0   if cycle1 is equal to cycle2
//      +1  if cycle1 is greater than cycle2
int
execCompareAbsoluteCycles
(
    const UINT16        cycle1,
    const UINT16        cycle2
)
{
    if ( cycle1 == cycle2 )
        return 0;
    if ( cycle1 > 499 )
    {
        if ( (cycle2 < cycle1) && (cycle2 >= cycle1 - 499 ) )
            return 1;
        return -1;
    }
    else
    {
        if ( (cycle2 > cycle1) && (cycle2 < cycle1 + 499) )
            return -1;
        return 1;
    }
}


//  execDecrementAbsoluteCycle()
//
//  Decreases the given cycle by the indicated value.
//  Accounts for the cyclical nature of file cycles, by decrementing from 1 to 999.
UINT16
execDecrementAbsoluteCycle
(
        const UINT16        cycle,
        const UINT16        value
)
{
    int iResult = cycle - value;
    while ( iResult < 1 )
        iResult += 999;
    return static_cast<UINT16>( iResult );
}


//  execGetAbsoluteCycleRange()
//
//  Retrieves the range from the first cycle to the next.
//  Order of cycle specification is not important.
UINT16
execGetAbsoluteCycleRange
(
    const UINT16        cycle1,
    const UINT16        cycle2
)
{
    int comp = execCompareAbsoluteCycles( cycle1, cycle2 );
    if ( comp == 0 )
        return 1;
    else if ( comp < 0 )
    {
        //  cycle1 preceeds cycle2
        if ( cycle1 < cycle2 )
            return cycle2 - cycle1 + 1;
        else
            return cycle2 + 999 - cycle1 + 1;
    }
    else
    {
        //  cycle2 preceeds cycle1
        if ( cycle2 < cycle1 )
            return cycle1 - cycle2 + 1;
        else
            return cycle1 + 999 - cycle2 + 1;
    }
}


//  execGetGranularityString()
const char*
execGetGranularityString
(
    const MassStorageGranularity    granularity
)
{
    switch ( granularity )
    {
    case MSGRAN_POSITION:   return "POS";
    case MSGRAN_TRACK:      return "TRK";
    }

    return "???";
}


//  execGetIoTranslateFormat()
//
//  Converts a tape format to the appropriate translation format
ChannelModule::IoTranslateFormat
execGetIoTranslateFormat
(
const TapeFormat    tapeFormat
)
{
    switch ( tapeFormat )
    {
    case TFMT_QUARTER_WORD:         return ChannelModule::IoTranslateFormat::A;
    case TFMT_SIX_BIT_PACKED:       return ChannelModule::IoTranslateFormat::B;
    case TFMT_EIGHT_BIT_PACKED:     return ChannelModule::IoTranslateFormat::C;
    case TFMT_QUARTER_WORD_IGNORE:  return ChannelModule::IoTranslateFormat::D;
    }

    //  this should never happen
    return ChannelModule::IoTranslateFormat::A;
}


//  execGetOptionsString()
//
//  Converts an options bit mask to a displayable string indicating the options which we set.
//  Bit 6 (from the left, starting at 0) corresponds to option A.
//  Bit 31 (LSB) corresponds to option Z.
std::string
execGetOptionsString
(
    const UINT32        options
)
{
    char ch = 'A';
    std::string str;
    UINT32 mask = options << 6;
    while ( ch <= 'Z' )
    {
        if ( mask & 0x80000000 )
            str += ch;
        mask <<= 1;
        ++ch;
    }

    return str;
}


//  execGetTapeDataCompressionString()
//
//  Converts value to string
const char*
execGetTapeDataCompressionString
(
    const TapeDataCompression   compression
)
{
    switch ( compression )
    {
    case TDC_OFF:       return "Off";
    case TDC_ON:        return "On";
    case TDC_OPTIONAL:  return "Optional";
    }

    return "???";
}


//  execGetTapeDensityString()
//
//  Converts value to string
const char*
execGetTapeDensityString
(
    const TapeDensity   density
)
{
    switch ( density )
    {
    case TDENS_800:     return "800BPI";
    case TDENS_1600:    return "1600BPI";
    case TDENS_3800:    return "3800BPI";//???? what is this?
    case TDENS_6250:    return "6250BPI";
    case TDENS_38000:   return "38000BPI";
    case TDENS_76000:   return "76000BPI";
    case TDENS_85937:   return "85937BPI";
    case TDENS_5090:    return "5090BPMM";
    }

    return "???";
}


//  execGetTapeFormatString()
//
//  Converts value to string
const char*
execGetTapeFormatString
(
    const TapeFormat    format
)
{
    switch ( format )
    {
    case TFMT_QUARTER_WORD:         return "Q";
    case TFMT_QUARTER_WORD_IGNORE:  return "QD"; //  should never happen
    case TFMT_SIX_BIT_PACKED:       return "6";
    case TFMT_EIGHT_BIT_PACKED:     return "8";
    }

    return "???";
}


//  execGetTapeParityString()
//
//  Converts value to string
const char*
execGetTapeParityString
(
    const TapeParity    parity
)
{
    switch ( parity )
    {
    case TPAR_NONE:     return "None";
    case TPAR_EVEN:     return "Even";
    case TPAR_ODD:      return "Odd";
    }

    return "???";
}


//  execGetTapeTypeString()
//
//  Converts value to string
const char*
execGetTapeTypeString
(
    const TapeType      type
)
{
    switch ( type )
    {
    case TTYPE_SEVEN_TRACK:     return "U7";
    case TTYPE_NINE_TRACK:      return "U9";
    case TTYPE_QIC:             return "QIC";
    case TTYPE_HIC:             return "HIC";
    case TTYPE_DLT:             return "DLT";
    case TTYPE_HIS:             return "HIS";
    }

    return "???";
}


//  execIncrementAbsoluteCycle()
//
//  Increases the given cycle by the indicated value.
//  Accounts for the cyclical nature of file cycles, by incrementing from 999 to 1.
UINT16
execIncrementAbsoluteCycle
(
        const UINT16        cycle,
        const UINT16        value
)
{
    int iResult = cycle + value;
    while ( iResult > 999 )
        iResult -= 999;
    return static_cast<UINT16>( iResult );
}


