//  GeneralRegister class declaration
//  Copyright (c) 2015 by Kurt Duncan



#ifndef     EM2200_GENERAL_REGISTER_H
#define     EM2200_GENERAL_REGISTER_H



#include    "Word36.h"



class   GeneralRegister : public Word36
{
public:
    // GRS Register Indices
    enum Indices
    {
        X0 = 0,
        X1 = 1,
        X2 = 2,
        X3 = 3,
        X4 = 4,
        X5 = 5,
        X6 = 6,
        X7 = 7,
        X8 = 8,
        X9 = 9,
        X10 = 10,
        X11 = 11,
        X12 = 12,
        X13 = 13,
        X14 = 14,
        X15 = 15,
        A0 = 12,
        A1 = 13,
        A2 = 14,
        A3 = 15,
        A4 = 16,
        A5 = 17,
        A6 = 18,
        A7 = 19,
        A8 = 20,
        A9 = 21,
        A10 = 22,
        A11 = 23,
        A12 = 24,
        A13 = 25,
        A14 = 26,
        A15 = 27,
        R0 = 64,
        R1 = 65,
        R2 = 66,
        R3 = 67,
        R4 = 68,
        R5 = 69,
        R6 = 70,
        R7 = 71,
        R8 = 72,
        R9 = 73,
        R10 = 74,
        R11 = 75,
        R12 = 76,
        R13 = 77,
        R14 = 78,
        R15 = 79,
        ER0 = 80,
        ER1 = 81,
        ER2 = 82,
        ER3 = 83,
        ER4 = 84,
        ER5 = 85,
        ER6 = 86,
        ER7 = 87,
        ER8 = 88,
        ER9 = 89,
        ER10 = 90,
        ER11 = 91,
        ER12 = 92,
        ER13 = 93,
        ER14 = 94,
        ER15 = 95,
        EX0 = 96,
        EX1 = 97,
        EX2 = 98,
        EX3 = 99,
        EX4 = 100,
        EX5 = 101,
        EX6 = 102,
        EX7 = 103,
        EX8 = 104,
        EX9 = 105,
        EX10 = 106,
        EX11 = 107,
        EX12 = 108,
        EX13 = 109,
        EX14 = 110,
        EX15 = 111,
        EA0 = 108,
        EA1 = 109,
        EA2 = 110,
        EA3 = 111,
        EA4 = 112,
        EA5 = 113,
        EA6 = 114,
        EA7 = 115,
        EA8 = 116,
        EA9 = 117,
        EA10 = 118,
        EA11 = 119,
        EA12 = 120,
        EA13 = 121,
        EA14 = 122,
        EA15 = 123,
    };

    static const char* const    m_Names[];

    // constructors
    GeneralRegister()
        : Word36()
    {}

    GeneralRegister( const UINT32 value )
        : Word36( value )
    {}

    GeneralRegister( const UINT64 value )
        : Word36( value )
    {}

    // destructor
    virtual ~GeneralRegister(){}
};



#endif
