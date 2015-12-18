//	FSKeyin.h
//
//	FS keyin handler



#ifndef		EXECLIB_FS_KEYIN_H
#define		EXECLIB_FS_KEYIN_H



#include    "DeviceManager.h"
#include    "Exec.h"
#include    "FacilitiesKeyin.h"



class	FSKeyin : public FacilitiesKeyin
{
private:
    bool                        generateAvailabilityOutput( const DeviceManager::NodeEntry* const pNodeEntry ) const;
    void                        handleALL() const;
    void                        handleADISK() const;
    void                        handleAVAIL() const;
    void                        handleCM() const;
    void                        handleComponent() const;
    void                        handleInterface() const;
    void                        handleIOP() const;
    void                        handleMS() const;
    void                        handlePACK() const;

    // KeyinThread interface
    void						handler();
    bool						isAllowed() const;

public:
    FSKeyin( Exec* const                        pExec,
             const SuperString&                 KeyinId,
             const SuperString&                 Option,
             const std::vector<SuperString>&    Params,
             const Word36&                      Routing );
};



#endif

