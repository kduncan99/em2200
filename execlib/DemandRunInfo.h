//  DemandRunInfo class declaration
//  Copyright (c) 2015 by Kurt Duncan
//
//  Some stuff specific to DEMAND mode



#ifndef     EXECLIB_DEMAND_RUN_INFO_H
#define     EXECLIB_DEMAND_RUN_INFO_H



#include    "ControlModeRunInfo.h"



//  annoying forward-references
class   Exec;



class   DemandRunInfo : public ControlModeRunInfo
{
private:
    bool                    m_InputAllowed;
    const COUNT             m_RSISessionNumber;

public:
    DemandRunInfo( Exec* const          pExec,
                   const std::string&   originalRunId,
                   const std::string&   actualRunId,
                   const std::string&   accountId,
                   const std::string&   projectId,
                   const std::string&   userId,
                   const UINT32         options,
                   const char           schedulingPriority,
                   const char           processorDispatchingPriority,
                   const COUNT          rsiSessionNumber );
    ~DemandRunInfo();

    void            dump( std::ostream&         stream,
                            const std::string&  prefix,
                            const DUMPBITS      dumpBits );

    inline COUNT    getRSISessionNumber() const                 { return m_RSISessionNumber; }
    inline bool     inControlMode() const                       { return m_pTask == 0; }
    inline bool     isDemand() const                            { return true; }
    inline bool     isInputAllowed() const                      { return m_InputAllowed; }
    inline void     setInputAllowed( const bool value )         { m_InputAllowed = value; }
};



#endif
