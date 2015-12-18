//  Word36 class declaration



#ifndef     EM2200_WORD36_H
#define     EM2200_WORD36_H



class   Word36
{
private:
    UINT64              m_Value;

protected:
    const static UINT64 MASK            = 0777777777777;
    const static UINT64 NEGATIVE_BIT    = 0400000000000;
    const static UINT64 NEGATIVE_ZERO   = 0777777777777;
    const static UINT64 ZERO            = 0;

    const static UINT64 MASK_B0         = ( (UINT64)1 << 35 );
    const static UINT64 MASK_B1         = ( (UINT64)1 << 34 );
    const static UINT64 MASK_B2         = ( (UINT64)1 << 33 );
    const static UINT64 MASK_B3         = ( (UINT64)1 << 32 );
    const static UINT64 MASK_B4         = ( (UINT64)1 << 31 );
    const static UINT64 MASK_B5         = ( (UINT64)1 << 30 );
    const static UINT64 MASK_B6         = ( (UINT64)1 << 29 );
    const static UINT64 MASK_B7         = ( (UINT64)1 << 28 );
    const static UINT64 MASK_B8         = ( (UINT64)1 << 27 );
    const static UINT64 MASK_B9         = ( (UINT64)1 << 26 );
    const static UINT64 MASK_B10        = ( (UINT64)1 << 25 );
    const static UINT64 MASK_B11        = ( (UINT64)1 << 24 );
    const static UINT64 MASK_B12        = ( (UINT64)1 << 23 );
    const static UINT64 MASK_B13        = ( (UINT64)1 << 22 );
    const static UINT64 MASK_B14        = ( (UINT64)1 << 21 );
    const static UINT64 MASK_B15        = ( (UINT64)1 << 20 );
    const static UINT64 MASK_B16        = ( (UINT64)1 << 19 );
    const static UINT64 MASK_B17        = ( (UINT64)1 << 18 );
    const static UINT64 MASK_B18        = ( (UINT64)1 << 17 );
    const static UINT64 MASK_B19        = ( (UINT64)1 << 16 );
    const static UINT64 MASK_B20        = ( (UINT64)1 << 15 );
    const static UINT64 MASK_B21        = ( (UINT64)1 << 14 );
    const static UINT64 MASK_B22        = ( (UINT64)1 << 13 );
    const static UINT64 MASK_B23        = ( (UINT64)1 << 12 );
    const static UINT64 MASK_B24        = ( (UINT64)1 << 11 );
    const static UINT64 MASK_B25        = ( (UINT64)1 << 10 );
    const static UINT64 MASK_B26        = ( (UINT64)1 << 9 );
    const static UINT64 MASK_B27        = ( (UINT64)1 << 8 );
    const static UINT64 MASK_B28        = ( (UINT64)1 << 7 );
    const static UINT64 MASK_B29        = ( (UINT64)1 << 6 );
    const static UINT64 MASK_B30        = ( (UINT64)1 << 5 );
    const static UINT64 MASK_B31        = ( (UINT64)1 << 4 );
    const static UINT64 MASK_B32        = ( (UINT64)1 << 3 );
    const static UINT64 MASK_B33        = ( (UINT64)1 << 2 );
    const static UINT64 MASK_B34        = ( (UINT64)1 << 1 );
    const static UINT64 MASK_B35        = ( (UINT64)1 << 0 );

    const static UINT64 MASK_NOT_B0     = ( MASK ^ MASK_B0 );
    const static UINT64 MASK_NOT_B1     = ( MASK ^ MASK_B1 );
    const static UINT64 MASK_NOT_B2     = ( MASK ^ MASK_B2 );
    const static UINT64 MASK_NOT_B3     = ( MASK ^ MASK_B3 );
    const static UINT64 MASK_NOT_B4     = ( MASK ^ MASK_B4 );
    const static UINT64 MASK_NOT_B5     = ( MASK ^ MASK_B5 );
    const static UINT64 MASK_NOT_B6     = ( MASK ^ MASK_B6 );
    const static UINT64 MASK_NOT_B7     = ( MASK ^ MASK_B7 );
    const static UINT64 MASK_NOT_B8     = ( MASK ^ MASK_B8 );
    const static UINT64 MASK_NOT_B9     = ( MASK ^ MASK_B9 );
    const static UINT64 MASK_NOT_B10    = ( MASK ^ MASK_B10 );
    const static UINT64 MASK_NOT_B11    = ( MASK ^ MASK_B11 );
    const static UINT64 MASK_NOT_B12    = ( MASK ^ MASK_B12 );
    const static UINT64 MASK_NOT_B13    = ( MASK ^ MASK_B13 );
    const static UINT64 MASK_NOT_B14    = ( MASK ^ MASK_B14 );
    const static UINT64 MASK_NOT_B15    = ( MASK ^ MASK_B15 );
    const static UINT64 MASK_NOT_B16    = ( MASK ^ MASK_B16 );
    const static UINT64 MASK_NOT_B17    = ( MASK ^ MASK_B17 );
    const static UINT64 MASK_NOT_B18    = ( MASK ^ MASK_B18 );
    const static UINT64 MASK_NOT_B19    = ( MASK ^ MASK_B19 );
    const static UINT64 MASK_NOT_B20    = ( MASK ^ MASK_B20 );
    const static UINT64 MASK_NOT_B21    = ( MASK ^ MASK_B21 );
    const static UINT64 MASK_NOT_B22    = ( MASK ^ MASK_B22 );
    const static UINT64 MASK_NOT_B23    = ( MASK ^ MASK_B23 );
    const static UINT64 MASK_NOT_B24    = ( MASK ^ MASK_B24 );
    const static UINT64 MASK_NOT_B25    = ( MASK ^ MASK_B25 );
    const static UINT64 MASK_NOT_B26    = ( MASK ^ MASK_B26 );
    const static UINT64 MASK_NOT_B27    = ( MASK ^ MASK_B27 );
    const static UINT64 MASK_NOT_B28    = ( MASK ^ MASK_B28 );
    const static UINT64 MASK_NOT_B29    = ( MASK ^ MASK_B29 );
    const static UINT64 MASK_NOT_B30    = ( MASK ^ MASK_B30 );
    const static UINT64 MASK_NOT_B31    = ( MASK ^ MASK_B31 );
    const static UINT64 MASK_NOT_B32    = ( MASK ^ MASK_B32 );
    const static UINT64 MASK_NOT_B33    = ( MASK ^ MASK_B33 );
    const static UINT64 MASK_NOT_B34    = ( MASK ^ MASK_B34 );
    const static UINT64 MASK_NOT_B35    = ( MASK ^ MASK_B35 );

    // general partial-word masks
    const static UINT64 MASK_H1         = 0777777000000;
    const static UINT64 MASK_H2         = 0000000777777;
    const static UINT64 MASK_Q1         = 0777000000000;
    const static UINT64 MASK_Q2         = 0000777000000;
    const static UINT64 MASK_Q3         = 0000000777000;
    const static UINT64 MASK_Q4         = 0000000000777;
    const static UINT64 MASK_S1         = 0770000000000;
    const static UINT64 MASK_S2         = 0007700000000;
    const static UINT64 MASK_S3         = 0000077000000;
    const static UINT64 MASK_S4         = 0000000770000;
    const static UINT64 MASK_S5         = 0000000007700;
    const static UINT64 MASK_S6         = 0000000000077;
    const static UINT64 MASK_T1         = 0777700000000;
    const static UINT64 MASK_T2         = 0000077770000;
    const static UINT64 MASK_T3         = 0000000007777;

    const static UINT64 MASK_NOT_H1     = 0000000777777;
    const static UINT64 MASK_NOT_H2     = 0777777000000;
    const static UINT64 MASK_NOT_Q1     = 0000777777777;
    const static UINT64 MASK_NOT_Q2     = 0777000777777;
    const static UINT64 MASK_NOT_Q3     = 0777777000777;
    const static UINT64 MASK_NOT_Q4     = 0777777777000;
    const static UINT64 MASK_NOT_S1     = 0007777777777;
    const static UINT64 MASK_NOT_S2     = 0770077777777;
    const static UINT64 MASK_NOT_S3     = 0777700777777;
    const static UINT64 MASK_NOT_S4     = 0777777007777;
    const static UINT64 MASK_NOT_S5     = 0777777770077;
    const static UINT64 MASK_NOT_S6     = 0777777777700;
    const static UINT64 MASK_NOT_T1     = 0000077777777;
    const static UINT64 MASK_NOT_T2     = 0777700007777;
    const static UINT64 MASK_NOT_T3     = 0777777770000;

    const static UINT64 MASK_TWELFTH[12];
    const static UINT64 MASK_NOT_TWELFTH[12];


public:
    // Constructors
    Word36()
        :m_Value(0)
    {}

    Word36( const UINT64 value )
        :m_Value( value & MASK )
    {}

    // Getters
    inline INT64    getSignedValue() const  {return isNegative() ? -(INT64)(m_Value ^ MASK) : (INT64)m_Value;}
    inline UINT64   getValue() const        { return m_Value; }
    inline bool     isNegative() const      { return (m_Value & NEGATIVE_BIT) ? true : false; }
    inline bool     isPositive() const      { return !isNegative(); }
    inline bool     isZero() const          { return (m_Value == ZERO) || (m_Value == NEGATIVE_ZERO); }

    // Partial-word getters
    inline UINT32   getH1() const           { return (UINT32)((m_Value & MASK_H1) >> 18); }
    inline UINT32   getH2() const           { return (UINT32)(m_Value & MASK_H2); }
    inline UINT16   getQ1() const           { return (UINT16)((m_Value & MASK_Q1) >> 27); }
    inline UINT16   getQ2() const           { return (UINT16)((m_Value & MASK_Q2) >> 18); }
    inline UINT16   getQ3() const           { return (UINT16)((m_Value & MASK_Q3) >> 9); }
    inline UINT16   getQ4() const           { return (UINT16)(m_Value & MASK_Q4); }
    inline UINT8    getS1() const           { return (UINT8)((m_Value & MASK_S1) >> 30); }
    inline UINT8    getS2() const           { return (UINT8)((m_Value & MASK_S2) >> 24); }
    inline UINT8    getS3() const           { return (UINT8)((m_Value & MASK_S3) >> 18); }
    inline UINT8    getS4() const           { return (UINT8)((m_Value & MASK_S4) >> 12); }
    inline UINT8    getS5() const           { return (UINT8)((m_Value & MASK_S5) >> 6); }
    inline UINT8    getS6() const           { return (UINT8)(m_Value & MASK_S6); }
    inline UINT16   getT1() const           { return (UINT16)((m_Value & MASK_T1) >> 24); }
    inline UINT16   getT2() const           { return (UINT16)((m_Value & MASK_T2) >> 12); }
    inline UINT16   getT3() const           { return (UINT16)(m_Value & MASK_T3); }
    inline UINT64   getW() const            { return m_Value; }

    inline UINT64 getTwelfth( const INDEX twelfth ) const
    {
        INDEX tx = twelfth % 12;
        COUNT shift = (11 - tx) * 3;
        return ( m_Value >> shift) & 07;
    }

    // Sign-extended partial-word getters...
    inline UINT64 getXH1() const
    {
        UINT64 result = getH1();
        if ( result & 0400000 )
            result &= 0777777000000;
        return result;
    }

    inline UINT64 getXH2() const
    {
        UINT64 result = getH2();
        if ( result & 0400000 )
            result &= 0777777000000;
        return result;
    }

    inline UINT64 getXT1() const
    {
        UINT64 result = getT1();
        if (result & 04000)
            result &= 0777777770000;
        return result;
    }

    inline UINT64 getXT2() const
    {
        UINT64 result = getT2();
        if (result & 04000)
            result &= 0777777770000;
        return result;
    }

    inline UINT64 getXT3() const
    {
        UINT64 result = getT3();
        if (result & 04000)
            result &= 0777777770000;
        return result;
    }


    // Setters
    inline void setValue( const UINT64 value )
    {
        m_Value = value & MASK;
    }

    // Conversion from 2's complement to 1's complement
    inline void setValue( INT64 value )
    {
        if ( value >= 0 )
            setValue( (UINT64)value );
        else
            setValue( (UINT64)(-value) );
    }

    // General partial-word setters
    inline void setH1( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_H1) | ((newValue << 18) & MASK_H1); }
    inline void setH2( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_H2) | (newValue & MASK_H2); }
    inline void setQ1( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_Q1) | ((newValue << 27) & MASK_Q1); }
    inline void setQ2( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_Q2) | ((newValue << 18) & MASK_Q2); }
    inline void setQ3( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_Q3) | ((newValue << 9) & MASK_Q3); }
    inline void setQ4( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_Q4) | (newValue & MASK_Q4); }
    inline void setS1( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_S1) | ((newValue << 30) & MASK_S1); }
    inline void setS2( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_S2) | ((newValue << 24) & MASK_S2); }
    inline void setS3( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_S3) | ((newValue << 18) & MASK_S3); }
    inline void setS4( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_S4) | ((newValue << 12) & MASK_S4); }
    inline void setS5( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_S5) | ((newValue << 6) & MASK_S5); }
    inline void setS6( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_S6) | (newValue & MASK_S6); }
    inline void setT1( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_T1) | ((newValue << 24) & MASK_T1); }
    inline void setT2( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_T2) | ((newValue << 12) & MASK_T2); }
    inline void setT3( const UINT64 newValue )     { m_Value = (m_Value & MASK_NOT_T3) | (newValue & MASK_T3); }
    inline void setW( const UINT64 newValue )      { m_Value = newValue & MASK; }

    //  Masks a 3-bit value into a particular 12th word of the current value.
    //  Parameters:
    //      twelfthIndex:		0 is MSB 0-2, 11 is LSB 33-35.
    //      newValue:			new value to be masked into the original value
    inline void setTwelfth
        (
        const INDEX     twelfthIndex,
        const UINT64    newValue
        )
    {
        COUNT shift = (11 - twelfthIndex) * 3;
        INDEX tx = twelfthIndex % 12;
        setValue( (m_Value & MASK_NOT_TWELFTH[tx]) | (((UINT64)newValue << shift) & MASK_TWELFTH[tx]) );
    }


    // Modifiers
    void        leftShiftCircular( COUNT count );
    void        rightShiftAlgebraic( COUNT count );
    void        rightShiftCircular( COUNT count );

    // inline modifiers
    inline void add
        (
        const UINT64    operand,
        bool* const     pCarry,
        bool* const     pOverflow
        )
    {
        m_Value = miscAdd36( m_Value, operand, pCarry, pOverflow );
    }

    inline void add
        (
        const Word36&   operand,
        bool* const     pCarry,
        bool* const     pOverflow
        )
    {
        m_Value = miscAdd36( m_Value, operand.m_Value, pCarry, pOverflow );
    }

    inline void logicalAnd
        (
        const UINT64        operand
        )
    {
        setValue( m_Value & operand );
    }

    inline void logicalAnd
        (
        const Word36&       operand
        )
    {
        setValue( m_Value & operand.m_Value );
    }

    inline void leftShiftLogical
        (
        const COUNT         count
        )
    {
        setValue( count > 35 ? 0 : m_Value << count );
    }

    inline void logicalNot()
    {
        setValue( m_Value ^ MASK );
    }

    inline void logicalOr
        (
        const UINT64    operand
        )
    {
        setValue( (m_Value | operand) & MASK );
    }

    inline void logicalOr
        (
        const Word36& operand
        )
    {
        setValue( m_Value | operand.m_Value );
    }

    inline void rightShiftLogical
        (
        const COUNT count
        )
    {
        setValue( count > 35 ? 0 : m_Value >> count );
    }

    inline void subtract
        (
        const UINT64    subtrahend,
        bool* const     pCarry,
        bool* const     pOverflow
        )
    {
        m_Value = miscSubtract36( m_Value, subtrahend, pCarry, pOverflow );
    }

    inline void subtract
        (
        const Word36&   subtrahend,
        bool* const     pCarry,
        bool* const     pOverflow
        )
    {
        m_Value = miscAdd36( m_Value, subtrahend.m_Value, pCarry, pOverflow );
    }

    // Other useful methods
    inline void clear()
    {
        m_Value = 0;
    }

    bool                operator<( const Word36& operand ) const;
    std::string         toAscii() const;
    std::string         toFieldata() const;
    std::string         toOctal() const;

    static void         pack( Word36* const pSource, const COUNT count, BYTE* const pDestination );
    static void         unpack( BYTE* const pSource, const COUNT count, Word36* const pDestination );
};


inline std::ostream& operator<< ( std::ostream& stream, const Word36& value )
{
    stream << value.getValue();
    return stream;
}



#endif
