//  Word36 class implementation



#include    "misclib.h"



//  statics

//  leftShiftCircular()
//
//  This object's value is shifted left, with bit1 rotating to bit36 at each iteration.
//  Actual implementation does not necessarily involve iterative shifting.
//
//  Parameters:
//      count:          number of bits to be shifted
void
Word36::leftShiftCircular
    (
    const COUNT         count
    )
{
    COUNT actualCount = count % 36;
    UINT64 residue = m_Value >> (36 - actualCount);         // end-around shifted portion
    setValue( ((m_Value << actualCount) & MASK) | residue );
}


//  operator<()
//
//  Needed for whenever this class is the key for an STL set or map
bool
    Word36::operator<
    (
    const Word36&   operand
    ) const
{
    bool carryFlag;
    bool overflowFlag;
    Word36 result = *this;
    result.subtract( operand, &carryFlag, &overflowFlag );
    if ( result.isZero() )
        return false;

    //  I... *think* the following is correct
    return overflowFlag ? !result.isNegative() : result.isNegative();
}


//  rightShiftAlgebraic()
//
//  Shifts the value right {n} bits, with bit1's value persisting to itself.
//  Actual implementation does not necessarily involve iterative shifting.
//
//  Parameters:
//      count:          number of bits to be shifted
void
Word36::rightShiftAlgebraic
    (
    const COUNT         count
    )
{
    if ( count >= 35 )
    {
        if ( isNegative() )
            setValue( NEGATIVE_ZERO );
        else
            setValue( ZERO );
    }
    else
    {
        const bool wasNegative = isNegative();
        setValue( m_Value >> count );
        if ( wasNegative )
            setValue( m_Value | (~(MASK >> count)) );
    }
}


//  rightShiftCircular()
//
//  Shifts the value right (n) bits, with bit36 rotating to bit1 at each iteration.
//  Actual implementation does not necessarily involve iterative shifting.
//
//  Parameters:
//      count:          number of bits to be shifted
void
Word36::rightShiftCircular
    (
    const COUNT         count
    )
{
    COUNT actualCount = (count % 36);
    UINT64 mask = MASK >> (36 - actualCount);
    UINT64 residue = (m_Value & mask) << (36 - actualCount);
    setValue( (m_Value >> actualCount) | residue );
}


//  toAscii()
//
//  Converts this word to displayable characters, interpreting the data as 4 ASCII values.
//
//  Returns:
//      displayable string of 4 characters
std::string
Word36::toAscii() const
{
    std::string result;
    result += miscAsciiToDisplay( (char)getQ1(), '.' );
    result += miscAsciiToDisplay( (char)getQ2(), '.' );
    result += miscAsciiToDisplay( (char)getQ3(), '.' );
    result += miscAsciiToDisplay( (char)getQ4(), '.' );
    return result;
}


//  toFieldata()
//
//  Converts this word to displayable characters, interpreting the data as 6 Fieldata characters
//
//  Returns:
//      displayable string of 6 characters
std::string
Word36::toFieldata() const
{
    std::string result;
    result += miscFieldataToAscii( (char)getS1() );
    result += miscFieldataToAscii( (char)getS2() );
    result += miscFieldataToAscii( (char)getS3() );
    result += miscFieldataToAscii( (char)getS4() );
    result += miscFieldataToAscii( (char)getS5() );
    result += miscFieldataToAscii( (char)getS6() );
    return result;
}


//  toOctal()
//
//  Converts this word to a displayable string representing its value as 12 octal digits.
//  Does NOT include a leading 0 if the left-most digit is non-zero.
std::string
Word36::toOctal() const
{
    std::stringstream strm;
    strm << std::oct << std::setfill( '0' ) << std::setw( 12 ) << m_Value;
    return strm.str();
}



//  static public functions

//  pack()
//
//  Static function.
//  Packs a series of Word36 objects into an array of bytes.
//
//  Parameters:
//      pSource:            pointer to array of Word36 objects - must be an even number of objects
//      Count:              number of Word36 objects
//      pDestination:       where we store resulting output
void
Word36::pack
    (
    Word36* const       pSource,
    const COUNT         count,
    BYTE* const         pDestination
    )
{
    assert( count % 2 == 0 );

    INDEX bx = 0;
    INDEX wx = 0;
    while ( wx < count )
    {
        pDestination[bx++] = static_cast<BYTE>((pSource[wx].m_Value >> 28));
        pDestination[bx++] = static_cast<BYTE>((pSource[wx].m_Value >> 20));
        pDestination[bx++] = static_cast<BYTE>((pSource[wx].m_Value >> 12));
        pDestination[bx++] = static_cast<BYTE>((pSource[wx].m_Value >> 4));
        pDestination[bx++] = static_cast<BYTE>(((pSource[wx].m_Value << 4) | (pSource[wx+1].m_Value >> 32)));
        wx++;
        if ( wx < count )
        {
            pDestination[bx++] = static_cast<BYTE>((pSource[wx].m_Value >> 24));
            pDestination[bx++] = static_cast<BYTE>((pSource[wx].m_Value >> 16));
            pDestination[bx++] = static_cast<BYTE>((pSource[wx].m_Value >> 8));
            pDestination[bx++] = static_cast<BYTE>((pSource[wx].m_Value));
            wx++;
        }
    }
}


//  unpack()
//
//  Unpacks byte streams into an array of Word36 objects
//
//  Parameters:
//      pSource:            pointer to source array of BYTE values - number of BYTE objects must e divisible by 9
//      count:              number of BYTE values (must be a multiple of 9)
//      pDestination:       pointer to destination array of Word36 objects
void
Word36::unpack
    (
    BYTE* const         pSource,
    const COUNT         count,
    Word36* const       pDestination
    )
{
    assert( count % 9 == 0 );

    INDEX bx = 0;
    INDEX wx = 0;
    while ( bx < count )
    {
        UINT64 val = static_cast<UINT64>(pSource[bx++]) << 28;
        val |= static_cast<UINT64>(pSource[bx++]) << 20;
        val |= static_cast<UINT64>(pSource[bx++]) << 12;
        val |= static_cast<UINT64>(pSource[bx++]) << 4;
        val |= static_cast<UINT64>(pSource[bx]) >> 4;
        pDestination[wx++].setValue( val );

        val = static_cast<UINT64>(pSource[bx++] & 0x0F) << 32;
        val |= static_cast<UINT64>(pSource[bx++]) << 24;
        val |= static_cast<UINT64>(pSource[bx++]) << 16;
        val |= static_cast<UINT64>(pSource[bx++]) << 8;
        val |= static_cast<UINT64>(pSource[bx++]);
        pDestination[wx++].setValue( val );
    }
}

