//  FileSpecification.cpp
//
//  Some simple sauce for the FileSpecification class


#include    "execlib.h"


FileSpecification::FileSpecification()
    :m_QualifierSpecified( false ),
    m_AbsoluteCycleSpecified( false ),
    m_RelativeCycleSpecified( false ),
    m_Cycle( 0 ),
    m_KeysSpecified( false )
{}


void
FileSpecification::clear()
{
    m_QualifierSpecified = false;
    m_Qualifier.clear();
    m_FileName.clear();
    m_AbsoluteCycleSpecified = false;
    m_RelativeCycleSpecified = false;
    m_Cycle = 0;
    m_KeysSpecified = false;
    m_ReadKey.clear();
    m_WriteKey.clear();
}


bool
FileSpecification::isFileNameOnly() const
{
    return ( !m_QualifierSpecified
                && !m_AbsoluteCycleSpecified
                && !m_RelativeCycleSpecified
                && !m_KeysSpecified );
}


std::string
FileSpecification::toString
(
    const bool          hideKeys
) const
{
    std::stringstream strm;
    if ( m_QualifierSpecified )
        strm << m_Qualifier << "*";
    strm << m_FileName;
    if ( m_AbsoluteCycleSpecified )
        strm << "(" << m_Cycle << ")";
    else if ( m_RelativeCycleSpecified )
        strm << "(" << ( m_Cycle > 0 ? "+" : "" ) << m_Cycle << ")";
    if ( m_KeysSpecified )
    {
        if ( hideKeys )
            strm << "//////////////";
        else
            strm << "/" << m_ReadKey << "/" << m_WriteKey;
    }

    return strm.str();
}


std::ostream&
operator<<
(
    std::ostream&               stream,
    const FileSpecification&    specification
)
{
    stream << specification.toString( true );
    return stream;
}
