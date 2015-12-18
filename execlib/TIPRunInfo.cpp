//  TIPRunInfo implementation



#include    "execlib.h"



//  private, protected methods



//  constructors, destructors



//  public methods

//  dump()
//
//  For debugging
void
    TIPRunInfo::dump
    (
    std::ostream&       stream,
    const std::string&  prefix,
    const DUMPBITS      dumpBits
    )
{
    stream << prefix << "TIPRunInfo" << std::endl;
    RunInfo::dump( stream, prefix, dumpBits );
}


