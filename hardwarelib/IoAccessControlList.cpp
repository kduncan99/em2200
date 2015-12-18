//  Non-inlined methods of IoAccessControlList object



#include    "hardwarelib.h"



//  public methods

IoAccessControlList::Iterator
IoAccessControlList::begin()
{
    return Iterator( this, m_IoAccessControlWords.begin(), 0 );
}


void
IoAccessControlList::clear()
{
    m_IoAccessControlWords.clear();
}


COUNT
IoAccessControlList::getExtent() const
{
    COUNT extent = 0;
    for ( CITIOACWS itacw = m_IoAccessControlWords.begin(); itacw != m_IoAccessControlWords.end(); ++itacw )
        extent += itacw->m_BufferSize;
    return extent;
}


void
IoAccessControlList::getSubList
(
    IoAccessControlList* const  pContainer,
    const Iterator&             startingPoint,
    const COUNT                 extent
)
{
    pContainer->clear();
    Iterator tempIter = startingPoint;
    COUNT extentRemaining = extent;

    while ( (extentRemaining > 0) && (tempIter != end()) )
    {
        COUNT acwRemaining = tempIter.acwRemaining();
#ifdef WIN32
        COUNT subExtent = min(extentRemaining, acwRemaining);
#else
        COUNT subExtent = std::min(extentRemaining, acwRemaining);
#endif

        IoAccessControlWord newACW = IoAccessControlWord( *tempIter, subExtent, tempIter.bufferAddressModifier() );
        pContainer->push_back( newACW );

        extentRemaining -= subExtent;
        tempIter += subExtent;
    }
}


void
IoAccessControlList::dump
(
    std::ostream&       stream,
    const std::string&  prefix
) const
{
    stream << prefix << "IoAccessControlList:" << std::endl;
    for ( CITIOACWS itacw = m_IoAccessControlWords.begin(); itacw != m_IoAccessControlWords.end(); ++itacw )
    {
        stream << prefix << "  pBuffer:0x" << std::hex << itacw->m_pBuffer
                << " Size:0" << std::oct << itacw->m_BufferSize
                << " Mod:" << miscGetExecIoBufferAddressModifierString( itacw->m_AddressModifier )
                << std::endl;
    }
}


IoAccessControlList::Iterator
IoAccessControlList::end()
{
    return Iterator( this, m_IoAccessControlWords.end(), 0 );
}


void
IoAccessControlList::push_back
(
    const IoAccessControlWord&  acw
)
{
    m_IoAccessControlWords.push_back( acw );
}


//  public methods for interanal Iterator class

COUNT
IoAccessControlList::Iterator::acwRemaining() const
{
    return m_InternalIterator->m_BufferSize - m_InternalIndex;
}


bool
IoAccessControlList::Iterator::atEnd() const
{
    return ( m_InternalIterator == m_pIoAccessControlList->m_IoAccessControlWords.end() );
}


ExecIoBufferAddressModifier
IoAccessControlList::Iterator::bufferAddressModifier() const
{
    return m_InternalIterator->m_AddressModifier;
}


bool
IoAccessControlList::Iterator::operator==
(
    const Iterator&     comp
)
{
    return ( (m_InternalIterator == comp.m_InternalIterator) && (m_InternalIndex == comp.m_InternalIndex) );
}


bool
IoAccessControlList::Iterator::operator!=
(
    const Iterator&     comp
)
{
    return !(*this == comp );
}


void
IoAccessControlList::Iterator::operator++()
{
    ++m_InternalIndex;
    if ( m_InternalIndex == m_InternalIterator->m_BufferSize )
    {
        ++m_InternalIterator;
        m_InternalIndex = 0;
    }
}


void
IoAccessControlList::Iterator::operator+=
(
    const COUNT     extent
)
{
    COUNT extentRemaining = extent;
    while ( extentRemaining && !atEnd() )
    {
        COUNT acwRemain = acwRemaining();
        if ( extentRemaining >= acwRemain )
        {
            extentRemaining -= acwRemain;
            ++m_InternalIterator;
            m_InternalIndex = 0;
        }
        else
        {
            m_InternalIndex += extentRemaining;
            extentRemaining = 0;
        }
    }
}


void
IoAccessControlList::Iterator::operator--()
{
    if ( m_InternalIndex == 0 )
    {
        --m_InternalIterator;
        m_InternalIndex = m_InternalIterator->m_BufferSize;
    }
    --m_InternalIndex;
}


Word36*
IoAccessControlList::Iterator::operator*()
{
    return m_InternalIterator->getWord( m_InternalIndex );
}

