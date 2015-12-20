//	DNKeyin.h
//  Copyright (c) 2015 by Kurt Duncan
//
//	DN keyin handler



#ifndef		EXECLIB_DN_KEYIN_H
#define		EXECLIB_DN_KEYIN_H



#include    "DeviceManager.h"
#include    "Exec.h"
#include	"FacilitiesKeyin.h"



class	DNKeyin : public FacilitiesKeyin
{
private:
    // KeyinThread interface
    void                        handleComponentList();
    void                        handleControllerList();
    void                        handleInterface() const;
    void                        handlePackList() const;
    void						handler();
    bool						isAllowed() const;
    bool                        verifyDevice( const DeviceManager::NodeEntry* const pDeviceEntry ) const;

public:
    DNKeyin( Exec* const                        pExec,
             const SuperString&                 KeyinId,
             const SuperString&                 Option,
             const std::vector<SuperString>&    Params,
             const Word36&                      Routing );
};



#endif

