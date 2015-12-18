//  SecurityContext.h
//
//  Tracks what a particular run is allowed to do



#ifndef     EXECLIB_SECURITY_CONTEXT_H
#define     EXECLIB_SECURITY_CONTEXT_H



#include    "SecurityManager.h"



class   SecurityContext
{
    bool                    m_CanBypassAccountCheck;        //  Change max granules for different account
    bool                    m_CanBypassFileKeys;            //  Assign files having keys, without specifying them
    bool                    m_CanBypassPrivacyInhibit;      //  Assign file normally inhibited by account-id or project-id
    bool                    m_CanBypassReadWriteInhibits;   //  Assign files for read/write, having read/write inhibits
    bool                    m_HasSecurityFileAssigned;      //  SYS$*DLOC$ is assigned to the run
    const bool              m_IsPrivilegedUserId;           //  Security officer ID (or other privileged user-id)

public:
    SecurityContext( const bool isPrivilegedUserId )
        :m_CanBypassAccountCheck( false ),
        m_CanBypassFileKeys( false ),
        m_CanBypassPrivacyInhibit( false ),
        m_CanBypassReadWriteInhibits( false ),
        m_HasSecurityFileAssigned( false ),
        m_IsPrivilegedUserId( isPrivilegedUserId )
    {}

    SecurityContext( const SecurityManager::UserProfile& userProfile )
        :m_CanBypassAccountCheck( false ),
        m_CanBypassFileKeys( false ),
        m_CanBypassPrivacyInhibit( false ),
        m_CanBypassReadWriteInhibits( false ),
        m_HasSecurityFileAssigned( false ),
        m_IsPrivilegedUserId( userProfile.m_Privileged )
    {}

    inline bool     canBypassAccountCheck() const
    {
        return m_CanBypassAccountCheck || m_HasSecurityFileAssigned || m_IsPrivilegedUserId;
    }

    inline bool     canBypassFileKeys() const
    {
        return m_CanBypassFileKeys || m_HasSecurityFileAssigned || m_IsPrivilegedUserId;
    }

    inline bool     canBypassPrivacyInhibit() const
    {
        return m_CanBypassPrivacyInhibit || m_HasSecurityFileAssigned || m_IsPrivilegedUserId;
    }

    inline bool     canBypassReadWriteInhibits() const
    {
        return m_CanBypassReadWriteInhibits || m_HasSecurityFileAssigned || m_IsPrivilegedUserId;
    }

    inline bool     hasSecurityFileAssigned() const                 { return m_HasSecurityFileAssigned; }
    inline bool     isPrivilegedUserId() const                      { return m_IsPrivilegedUserId; }
    inline void     setHasSecurityFileAssigned( const bool flag )   { m_HasSecurityFileAssigned = flag; }
};


class   ExecSecurityContext : public SecurityContext
{
public:
    ExecSecurityContext()
        :SecurityContext( true )
    {}
};



#endif
