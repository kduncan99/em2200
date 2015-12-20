//	BootActivity.h
//  Copyright (c) 2015 by Kurt Duncan
//
//	Boot-time activities



#ifndef     EXECLIB_BOOT_ACTIVITY_H
#define     EXECLIB_BOOT_ACTIVITY_H



#include    "FacilitiesManager.h"
#include    "IntrinsicActivity.h"



class	BootActivity : public IntrinsicActivity
{
private:
    class SystemFileInfo
    {
    public:
        const std::string           m_Qualifier;
        const std::string           m_Filename;
        const std::string           m_ReadKey;
        const std::string           m_WriteKey;
        const UINT32                m_AsgOptions;
        const UINT32                m_CatOptions;
        const std::string           m_EquipmentType;
        const COUNT                 m_InitialReserve;
        const COUNT                 m_MaximumSize;

        SystemFileInfo( const std::string&  qualifier,
                        const std::string&  filename,
                        const std::string&  readKey,
                        const std::string&  writeKey,
                        const UINT32        asgOptions,
                        const UINT32        catOptions,
                        const std::string&  equipmentType,
                        const COUNT         initialReserve = 0,
                        const COUNT         maximumSize = 9999 )
            :m_Qualifier( qualifier ),
            m_Filename( filename ),
            m_ReadKey( readKey ),
            m_WriteKey( writeKey ),
            m_AsgOptions( asgOptions ),
            m_CatOptions( catOptions ),
            m_EquipmentType( equipmentType ),
            m_InitialReserve( initialReserve ),
            m_MaximumSize( maximumSize )
        {}
    };

    ConsoleManager*     m_pConsoleManager;
    SuperString         m_DefaultDiskType;          //  Asg mnemonic for disk files not specifically covered otherwise
    SuperString         m_DlocAssignMnemonic;       //  DLOCASGMNE  - asg mnemonic for SYS$*DLOC$
    SuperString         m_GenfAssignMnemonic;       //  GENFASGMNE  - asg mnemonic for SYS$*GENF$
    COUNT               m_GenfInitialReserve;       //  GENFINTRES  - initial reserve for SYS$*GENF$
    SuperString         m_LibAssignMnemonic;        //  LIBASGMNE   - asg mnemonic for SYS$*LIB$
    COUNT               m_LibInitialReserve;        //  LIBINTRES   - initial reserve for SYS$*LIB$
    COUNT               m_LibMaxSize;               //  LIBMAXSIZ   - max size for SYS$*LIB$
//  REWDRV      - deals with rewinding drives on recovery boots
    SuperString         m_RunAssignMnemonic;        //  RUNASGMNE   - asg mnemonic for SYS$*RUN$
    COUNT               m_RunInitialReserve;        //  RUNINTRES   - initial reserve for SYS$*RUN$
    COUNT               m_RunMaxSize;               //  RUNMAXSIZE  - max size for SYS$*RUN$
//  SSEQPT      - asg mnemonic for EXEC and system processor @ASG of tape files

    bool                        assignSystemFile( SystemFileInfo* const pSystemFileInfo );
    bool                        catalogSystemFile( SystemFileInfo* const pSystemFileInfo );
    bool                        checkFacStatus( const FacilitiesManager::Result&    result,
                                                const std::string&                  message );
    bool                        confirmFixedDeviceCount();
    bool                        confirmJK13();
    void                        displayJumpKeysSet();
    void                        displaySessionNumber();
    void                        displayStartupMessages();
    void                        initialBoot();
    bool                        initializeDloc();
    bool                        initializeGenf();
    bool                        initializeMassStorage();
    void                        recoveryBoot();
    void                        waitForDownPackKeyins();
    void                        waitForModifyConfig();
    void                        worker();

public:
    BootActivity( Exec* const pExec );

    //  InstrinsicActivity interface
    void                        dump( std::ostream&         stream,
                                      const std::string&    prefix,
                                      const DUMPBITS        dumpBits );
};



#endif

