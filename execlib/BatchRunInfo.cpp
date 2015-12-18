//  BatchRunInfo implementation



#include    "execlib.h"



//  private, protected methods



//  constructors, destructors



//  public methods

//  dump()
//
//  For debugging
void
    BatchRunInfo::dump
    (
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
    )
{
    stream << prefix << "BatchRunInfo" << std::endl;
    RunInfo::dump( stream, prefix, dumpBits );
}


