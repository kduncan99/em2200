//  FileSpecification header file



#ifndef     EXECLIB_FILE_SPECIFICATION_H
#define     EXECLIB_FILE_SPECIFICATION_H



class   FileSpecification
{
    enum Status
    {
        ST_SUCCESSFUL,
        ST_INVALID_QUALIFIER,
        ST_INVALID_FILENAME,
        ST_INVALID_CYCLE,
        ST_INVALID_READ_KEY,
        ST_INVALID_WRITE_KEY,
        ST_INVALID_SYNTAX,
    };

public:
    bool                m_QualifierSpecified;       // if true and m_Qualifier is blank, then this is an implied qualifier
    SuperString         m_Qualifier;
    SuperString         m_FileName;
    bool                m_AbsoluteCycleSpecified;   // if true, m_Cycle is an absolute file cycle
    bool                m_RelativeCycleSpecified;   // if true, m_Cycle is a relative file cycle
    short int           m_Cycle;                    // relative or absolute, based on whichever (if any) was specified
    bool                m_KeysSpecified;            // if true, at least one '/' follows filename (or cycle)
    SuperString         m_ReadKey;
    SuperString         m_WriteKey;

    FileSpecification();

    void                clear();
    bool                isFileNameOnly() const;
    std::string         toString( const bool hideKeys = false ) const;
};


std::ostream& operator<< ( std::ostream&, const FileSpecification& );


#endif
