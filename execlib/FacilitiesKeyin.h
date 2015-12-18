//  FacilitiesKeyin.h
//
//  Describes functionality and data common to FD, DN, SU, RV, and UP keyins.
//  This is NOT abstract, because it has to instantiate itself for purposes of maintaining the static lock.
//  The static lock prevents multiple facilities keyins from executing concurrently.



#ifndef     EMEXEC_FACILITIES_KEYIN_H
#define     EMEXEC_FACILITIES_KEYIN_H



#include    "DeviceManager.h"
#include    "Exec.h"
#include    "KeyinActivity.h"



class   FacilitiesKeyin : public KeyinActivity
{
private:
    static std::recursive_mutex m_StaticLock;
    static COUNT                m_StaticObjectCount;

protected:
    enum PollResult
    {
        PR_YES,
        PR_NO,
        PR_CANCELED
    };

    typedef     std::list<const DeviceManager::NodeEntry*>  NODEENTRYLIST;
    typedef     NODEENTRYLIST::iterator                     ITNODEENTRYLIST;
    typedef     NODEENTRYLIST::const_iterator               CITNODEENTRYLIST;

    typedef     std::set<const DeviceManager::NodeEntry*>   NODEENTRYSET;
    typedef     NODEENTRYSET::iterator                      ITNODEENTRYSET;
    typedef     NODEENTRYSET::const_iterator                CITNODEENTRYSET;

    DeviceManager*              m_pDeviceManager;
    MFDManager*                 m_pMFDManager;

    void                        generateOutput( NODEENTRYSET& nodeSet ) const;
    bool                        generateOutputForDisk( const DeviceManager::NodeEntry* const    pNodeEntry,
                                                       std::string* const                       pOutput,
                                                       bool* const                              pShortFlag ) const;
    bool                        generateOutputForEntry( const DeviceManager::NodeEntry* const   pNodeEntry,
                                                        std::string* const                      pOutput,
                                                        bool* const                             pShortFlag ) const;
    bool                        getDownstreamNodes( const DeviceManager::NodeEntry* const   pParentNode,
                                                    NODEENTRYSET* const                     pDevices ) const;
    void                        notifyDoesNotExist( const std::string& componentName ) const;
    void                        notifyInternalError( const DeviceManager::NodeEntry* const pNodeEntry ) const;
    void                        notifyKeyinAborted( const DeviceManager::NodeEntry* const pNodeEntry ) const;
    void                        notifyKeyinAlreadyPerformed( const DeviceManager::NodeEntry* const pNodeEntry ) const;
    void                        notifyKeyinNotAllowed( const DeviceManager::NodeEntry* const pNodeEntry ) const;
    PollResult                  pollOperator( const std::string& prompt ) const;

    FacilitiesKeyin( Exec* const                        pExec,
                     const SuperString&                 KeyinId,
                     const SuperString&                 Option,
                     const std::vector<SuperString>&    Params,
                     const Word36&                      Routing );
    virtual ~FacilitiesKeyin(){}

    //  lock and unlock the static mutex to single-thread facilities keyins
    inline void                 staticLock() const      { m_StaticLock.lock(); }
    inline void                 staticUnlock() const    { m_StaticLock.unlock(); }
};



#endif

