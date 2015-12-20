//  SimpleFile header file
//  Copyright (c) 2015 by Kurt Duncan
//
//  Abstracts the lower-level file operations for the supported platforms.
//  We do this instead of using c++ file operations so we can get better information about failures
//  Intended to be thread-safe, and all operations are atomic.



#ifndef     MISCLIB_SIMPLE_FILE_H
#define     MISCLIB_SIMPLE_FILE_H



class   SimpleFile: public Lockable
{
public:
    //  Open parameters
    static const unsigned int   READ        = 0x0001;
    static const unsigned int   WRITE       = 0x0002;
    static const unsigned int   TRUNCATE    = 0x0010;   //  Truncate file on open if it exists
    static const unsigned int   NEW         = 0x0100;   //  Only succeed if file doesn't exist, and we create a new one
    static const unsigned int   EXISTING    = 0x0200;   //  Only succeed if file does exist

private:
#ifdef  WIN32
    HANDLE                      m_FileHandle;
#else
    int                         m_FileHandle;
#endif
    const std::string           m_FileName;
    bool                        m_OpenFlag;

public:
    SimpleFile( const std::string& fileName )
            :Lockable(),
            m_FileName( fileName ),
            m_OpenFlag( false )
            {}

    ~SimpleFile();

    SYSTEMERRORCODE     close();
    SYSTEMERRORCODE     getFileSize( COUNT64* const pSize ) const;
    SYSTEMERRORCODE     open( const unsigned int parameters );
    SYSTEMERRORCODE     read( const COUNT64         byteAddress,
                              BYTE* const           pBuffer,
                              const COUNT           byteCount,
                              COUNT* const          pBytesRead ) const;
    SYSTEMERRORCODE     write( const COUNT64        byteAddress,
                               const BYTE* const    pBuffer,
                               const COUNT          byteCount,
                               COUNT* const         pBytesWritten ) const;

    inline const std::string&   getFileName() const     { return m_FileName; }
    inline bool                 isOpen() const          { return m_OpenFlag; }
};



#endif	/* MISCLIB_SIMPLE_FILE_H */

