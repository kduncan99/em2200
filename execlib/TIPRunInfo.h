//  TIPRunInfo class declaration
//  Copyright (c) 2015 by Kurt Duncan
//
//  Anything specific to TIP transactions (which are a special kind of run)



#ifndef     EXECLIB_TIP_RUN_INFO_H
#define     EXECLIB_TIP_RUN_INFO_H



#include    "UserRunInfo.h"



//  annoying forward-references
class   Exec;



class   TIPRunInfo : public UserRunInfo
{
private:

public:
    void            dump( std::ostream&         stream,
                            const std::string&  prefix,
                            const DUMPBITS      dumpBits );

    inline bool     isTIP() const                               { return true; }
};



#endif
