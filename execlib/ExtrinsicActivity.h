//  ExtrinsicActivity - an activity which runs in the 36-bit interpreter
//  - something fully compatible with a real 2200.
//  Copyright (c) 2015 by Kurt Duncan



#ifndef EXECLIB_EXTRINSIC_ACTIVITY_H
#define EXECLIB_EXTRINSIC_ACTIVITY_H



#include    "Activity.h"



class ExtrinsicActivity : public Activity
{
public:
    ExtrinsicActivity( Exec* const          pExec,
                       const std::string&   activityName,
                       RunInfo* const       pRunInfo )
        :Activity( pExec, activityName, pRunInfo )
    {}

    virtual ~ExtrinsicActivity();

    virtual void                    dump( std::ostream&         stream,
                                            const std::string&  prefix,
                                            const DUMPBITS      dumpBits ) = 0;
};



#endif
