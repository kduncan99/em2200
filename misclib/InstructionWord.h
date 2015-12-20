//  InstructionWord class declaration
//  Copyright (c) 2015 by Kurt Duncan



#ifndef     EM2200_INSTRUCTION_WORD_H
#define     EM2200_INSTRUCTION_WORD_H



class   InstructionWord : public Word36
{
public:
    struct InterpretConversionData
    {
        bool                m_BasicModeFlag;    //  Converter function should consider m_UField if true;
                                                //      m_BaseRegister and m_DField if false
        UINT16              m_BaseRegister;
        UINT16              m_DField;
        UINT32              m_UField;
    };

    typedef std::string     (*INTERPRETCONVERSIONFUNCTION)(const void* const                pClientData,
                                                            const InterpretConversionData&  conversionData);


private:
    // instruction partial-word masks
    const static UINT64 MASK_F          = 0770000000000;
    const static UINT64 MASK_J          = 0007400000000;
    const static UINT64 MASK_A          = 0000360000000;
    const static UINT64 MASK_X          = 0000017000000;
    const static UINT64 MASK_H          = 0000000400000;
    const static UINT64 MASK_I          = 0000000200000;
    const static UINT64 MASK_U          = 0000000177777;
    const static UINT64 MASK_HIU        = 0000000777777;
    const static UINT64 MASK_B          = 0000000170000;    // extended-mode base
    const static UINT64 MASK_IB         = 0000000370000;    // extended-mode extended-base (5 bits, using I field)
    const static UINT64 MASK_D          = 0000000007777;    // extended-mode displacement

    const static UINT64 MASK_NOT_F      = 0007777777777;
    const static UINT64 MASK_NOT_J      = 0770377777777;
    const static UINT64 MASK_NOT_A      = 0777417777777;
    const static UINT64 MASK_NOT_X      = 0777760777777;
    const static UINT64 MASK_NOT_H      = 0777777377777;
    const static UINT64 MASK_NOT_I      = 0777777577777;
    const static UINT64 MASK_NOT_U      = 0777777600000;
    const static UINT64 MASK_NOT_HIU    = 0777777000000;
    const static UINT64 MASK_NOT_B      = 0777777607777;
    const static UINT64 MASK_NOT_IB     = 0777777407777;
    const static UINT64 MASK_NOT_D      = 0777777770000;

    // private data types, enumerators, etc
    enum AFieldSemantics
    {
        NONE = 0,       // a-field is not represented
        A,              // a-field is an A register
        B,              // a-field is a B register B2 to B15
        BEXEC,          // a-field is an exec B register B16 to B31 (a + 16)
        R,              // a-field is an R register
        X,              // a-field is an X register
    };

    enum Mode
    {
        BASIC,
        EXTENDED,
        EITHER,
    };

    struct ERNameEntry
    {
        UINT32          m_Index;
        const char*     m_Name;
    };

    typedef std::string     (*InterpretHandler)( const InstructionWord& );

    struct InstructionInfo
    {
    public:
        Mode                m_Mode;
        UINT8               m_FField;
        UINT8               m_JField;
        UINT8               m_AField;
        bool                m_JFlag;            // true if match is required on j-field, and to prevent j-field interpretation
        bool                m_AFlag;            // true if match is required on a-field, and to prevent a-field interpretation
        bool                m_GRSFlag;          // true if u < 0200 might imply a GRS address
        bool                m_ImmFlag;          // if jFlag is false, this flag is true to allow j>=016; ignored if jFlag is true
        AFieldSemantics     m_ASemantics;       // indicates usage of a-field; should be AFieldSemantics.NONE if aFlag is true
        const char*         m_Mnemonic;         // instruction mnemonic
        bool                m_UseBMSemantics;   // for EM jump instructions which use u field instead of b and d fields
        InterpretHandler    m_Handler;          // pointer to special handler function...
                                                    //   grsFlag, immFlag, aSemantics, and pMnemonic are ignored
    };

    static const ERNameEntry        m_ERNameTable[];
    static const InstructionInfo    m_InstructionInfoTable[];
    static const char* const        m_JFieldNames[];

    static std::string          handleBT( const InstructionWord& instruction );
    static std::string          handleER( const InstructionWord& instruction );
    static std::string          handleJGD( const InstructionWord& instruction );
    static std::string          interpret( const InstructionWord&       instruction,
                                            const std::string&          mnemonic,
                                            const bool                  extendedMode,
                                            const AFieldSemantics       aSemantics,
                                            const bool                  jField,
                                            const bool                  grsFlag,
                                            const bool                  forceBMSemantics,
                                            const bool                  execModeRegistersFlag,
                                            INTERPRETCONVERSIONFUNCTION conversionFunction,
                                            const void* const           pClientData );


public:
    // constructors
    InstructionWord()
        : Word36()
    {}

    InstructionWord( const UINT32 value )
        : Word36( value )
    {}

    InstructionWord( const UINT64 value )
        : Word36( value )
    {}

    InstructionWord( const UINT16 f,
                        const UINT8 j,
                        const UINT8 a,
                        const UINT8 x,
                        const bool h,
                        const bool i,
                        const UINT16 u );

    InstructionWord( const UINT16 f,
                        const UINT8 j,
                        const UINT8 a,
                        const UINT8 x,
                        const bool h,
                        const bool i,
                        const UINT8 b,
                        const UINT16 d );

    InstructionWord( const UINT16 f,
                        const UINT8 j,
                        const UINT8 a,
                        const UINT8 x,
                        const UINT32 u );

    // getters
    inline UINT32       getF() const            { return static_cast<UINT32>((getValue() & MASK_F) >> 30); }
    inline UINT32       getJ() const            { return static_cast<UINT32>((getValue() & MASK_J) >> 26); }
    inline UINT32       getA() const            { return static_cast<UINT32>((getValue() & MASK_A) >> 22); }
    inline UINT32       getX() const            { return static_cast<UINT32>((getValue() & MASK_X) >> 18); }
    inline bool         getH() const            { return (getValue() & MASK_H) ? true : false; }
    inline bool         getI() const            { return (getValue() & MASK_I) ? true : false; }
    inline UINT32       getHIU() const          { return static_cast<UINT32>((getValue() & MASK_HIU)); }
    inline UINT32       getU() const            { return static_cast<UINT32>((getValue() & MASK_U)); }
    inline UINT32       getB() const            { return static_cast<UINT32>((getValue() & MASK_B) >> 12); }
    inline UINT32       getIB() const           { return static_cast<UINT32>((getValue() & MASK_IB) >> 12); }
    inline UINT32       getD() const            { return static_cast<UINT32>((getValue() & MASK_D)); }

    // setters
    inline void         setF( const UINT32 value )      { setValue( (getValue() & MASK_NOT_F) | (static_cast<UINT64>(value & 077) << 30) ); }
    inline void         setJ( const UINT32 value )      { setValue( (getValue() & MASK_NOT_J) | ((value & 017) << 26) ); }
    inline void         setA( const UINT32 value )      { setValue( (getValue() & MASK_NOT_A) | ((value & 017) << 22) ); }
    inline void         setX( const UINT32 value )      { setValue( (getValue() & MASK_NOT_X) | ((value & 017) << 18) ); }
    inline void         setH( const bool flag )         { setValue( (getValue() & MASK_NOT_H) | (flag ? 0400000 : 0) ); }
    inline void         setI( const bool flag )         { setValue( (getValue() & MASK_NOT_I) | (flag ? 0200000 : 0) ); }
    inline void         setHIU( const UINT32 value )    { setValue( (getValue() & MASK_NOT_HIU) | (value & 0777777) ); }
    inline void         setU( const UINT32 value )      { setValue( (getValue() & MASK_NOT_U) | (value & 0177777) ); }
    inline void         setB( const UINT32 value )      { setValue( (getValue() & MASK_NOT_B) | ((value & 017) << 12) ); }
    inline void         setIB( const UINT32 value )     { setValue( (getValue() & MASK_NOT_IB) | ((value & 037) << 12) ); }
    inline void         setD( const UINT32 value )      { setValue( (getValue() & MASK_NOT_D) | (value & 07777) ); }

    // other useful things
    inline void         clear()
    {
        setValue( static_cast<UINT64>(0) );
    }

    inline std::string  getMnemonic( const bool extendedMode ) const
    {
        return getMnemonic( *this, extendedMode );
    }

    inline std::string  interpret( const bool                   extendedMode,
                                    const bool                  execModeRegistersFlag = false,
                                    INTERPRETCONVERSIONFUNCTION conversionFunction = 0,
                                    const void* const           pClientData = 0) const
    {
        return interpret( *this, extendedMode, execModeRegistersFlag, conversionFunction, pClientData );
    }

    static std::string  getMnemonic( const InstructionWord&     instruction, const bool extendedMode );
    static std::string  interpret( const InstructionWord&       instruction,
                                    const bool                  extendedMode,
                                    const bool                  execModeRegistersFlag = false,
                                    INTERPRETCONVERSIONFUNCTION conversionFunction = 0,
                                    const void* const           pClientData = 0 );
};



#endif
