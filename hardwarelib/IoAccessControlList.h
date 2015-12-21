//  IoAccessControlList
//  Copyright (c) 2015 by Kurt Duncan
//
//  Some nice functions built around a standard list of IoAccessControlWords



#ifndef     HARDWARELIB_IO_ACCESS_CONTROL_LIST
#define     HARDWARELIB_IO_ACCESS_CONTROL_LIST



class   IoAccessControlList
{
public:
    typedef     std::list<IoAccessControlWord>      IOACWS;
    typedef     IOACWS::iterator                    ITIOACWS;
    typedef     IOACWS::const_iterator              CITIOACWS;

private:
    IOACWS      m_IoAccessControlWords;

public:
    class   Iterator
    {
    private:
        const IoAccessControlList*  m_pIoAccessControlList;
        ITIOACWS                    m_InternalIterator;
        INDEX                       m_InternalIndex;

    public:
        Iterator( const IoAccessControlList* const  pList,
                  const ITIOACWS                    iterator,
                  const INDEX                       index )
            :m_pIoAccessControlList( pList ),
            m_InternalIterator( iterator ),
            m_InternalIndex( index )
        {}

        Iterator()
            :m_pIoAccessControlList( 0 ),
            m_InternalIndex( 0 )
        {}

        COUNT                       acwRemaining() const;
        bool                        atEnd() const;
        ExecIoBufferAddressModifier bufferAddressModifier() const;
        bool                        operator==( const Iterator& comp );
        bool                        operator!=( const Iterator& comp );
        void                        operator+=( const COUNT wordCount );
        void                        operator++();
        void                        operator--();
        Word36*                     operator*();
    };

    Iterator                begin();
    void                    clear();
    void                    dump( std::ostream&         stream,
                                  const std::string&    prefix ) const;
    Iterator                end();
    COUNT                   getExtent() const;
    void                    getSubList( IoAccessControlList* const  pContainer,
                                        const Iterator&             startingPoint,
                                        const COUNT                 extent );
    void                    push_back( const IoAccessControlWord& acw );

    inline const IOACWS&    getAccessControlWords() const           { return m_IoAccessControlWords; }
};



#endif
