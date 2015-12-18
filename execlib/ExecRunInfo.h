//  ExecRunInfo class declaration
//
//  Stuff which pertains only to the Exec



#ifndef     EXECLIB_EXEC_RUN_INFO_H
#define     EXECLIB_EXEC_RUN_INFO_H



#include    "RunInfo.h"



//  annoying forward-references
class   Exec;



class   ExecRunInfo : public RunInfo
{
public:
    ExecRunInfo( Exec* const        pExec,
                 const std::string& overheadAccountId,
                 const std::string& overheadUserId );
    ~ExecRunInfo();

    void            dump( std::ostream&         stream,
                          const std::string&    prefix,
                          const DUMPBITS        dumpBits );
    inline bool     isExec() const                      { return true; }
    inline bool     isPrivileged() const                { return true; }
};



#endif
