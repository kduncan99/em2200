//  Implements SimpleFile



#include    "misclib.h"
#include "SimpleFile.h"



//  constructors, destructors

SimpleFile::~SimpleFile()
{
    if ( isOpen() )
        close();
}



//  public methods

//  close()
//
//  Closes the underlying system file
SYSTEMERRORCODE
SimpleFile::close()
{
    SYSTEMERRORCODE retn = SYSTEMERRORCODE_SUCCESS;

#ifdef  WIN32

    lock();
    if ( CloseHandle( m_FileHandle ) )
        m_OpenFlag = false;
    else
        retn = GetLastError();
    unlock();

#else

    lock();
    if ( ::close( m_FileHandle ) == 0 )
        m_OpenFlag = false;
    else
        retn = errno;
    unlock();

#endif

    return retn;
}


//  getFileSize()
//
//  Returns the current size of the file in bytes.
SYSTEMERRORCODE
SimpleFile::getFileSize
(
    COUNT64* const      pSize
) const
{
    SYSTEMERRORCODE result = SYSTEMERRORCODE_SUCCESS;

#ifdef  WIN32
#error TODO
#else

    struct stat fileInfo;
    if ( fstat( m_FileHandle, &fileInfo ) == -1 )
        result = errno;
    else
        *pSize = fileInfo.st_size;

#endif

    return result;
}


//  open()
//
//  Attempts to open and/or create a system file
SYSTEMERRORCODE
SimpleFile::open
(
    const unsigned int  parameters
)
{
    SYSTEMERRORCODE retn = SYSTEMERRORCODE_SUCCESS;

#ifdef  WIN32
    DWORD access = ((parameters & READ) ? GENERIC_READ : 0) | ((parameters & WRITE) ? GENERIC_WRITE : 0);
    DWORD share = 0;
    DWORD disposition = 0;
    if (parameters & NEW)
    {
        disposition = CREATE_NEW;
    }
    else if (parameters & EXISTING)
    {
        disposition = (parameters & TRUNCATE) ? TRUNCATE_EXISTING : OPEN_EXISTING;
    }
    else
    {
        disposition = (parameters & TRUNCATE) ? CREATE_ALWAYS : OPEN_ALWAYS;
    }
    DWORD flags = FILE_ATTRIBUTE_NORMAL;

    lock();
    m_FileHandle = CreateFile(m_FileName.c_str(),
                              access,
                              share,
                              0,
                              disposition,
                              flags,
                              0 );
    if ( m_FileHandle == INVALID_HANDLE_VALUE )
    {
        m_OpenFlag = false;
        retn = GetLastError();
    }
    else
        m_OpenFlag = true;
    unlock();

#else

    int openFlags = 0;
    if ( (parameters & READ) && (parameters & WRITE) )
        openFlags |= O_RDWR;
    else if ( parameters & READ )
        openFlags |= O_RDONLY;
    else if ( parameters & WRITE )
        openFlags |= O_WRONLY;
    else
        openFlags |= O_RDWR;

    if ( parameters & NEW )
        openFlags |= O_CREAT | O_EXCL;
    else if ( (parameters & EXISTING) == 0 )
        openFlags |= O_CREAT;

    if ( parameters & TRUNCATE )
        openFlags |= O_TRUNC;

    lock();
    m_FileHandle = ::open( m_FileName.c_str(), openFlags, 0666 );
    if ( m_FileHandle == -1 )
    {
        m_OpenFlag = false;
        retn = errno;
    }
    else
        m_OpenFlag = true;
    unlock();

#endif

    return retn;
}


//  read()
//
//  Reads bytes from the underlying file, up to a given maximum, from a particular byte offset.
SYSTEMERRORCODE
SimpleFile::read
(
    const COUNT64       byteAddress,
    BYTE* const         pBuffer,
    const COUNT         byteCount,
    COUNT* const        pBytesRead
) const
{
    SYSTEMERRORCODE retn = SYSTEMERRORCODE_SUCCESS;

#ifdef  WIN32

    LARGE_INTEGER distance;
    distance.QuadPart = byteAddress;

    lock();
    if ( !SetFilePointerEx( m_FileHandle, distance, 0, FILE_BEGIN ) )
        retn = GetLastError();
    else
    {
        DWORD bytesRead = 0;
        if ( !ReadFile( m_FileHandle, pBuffer, static_cast<DWORD>(byteCount), &bytesRead, 0 ) )
            retn = GetLastError();
        else
            *pBytesRead = bytesRead;
    }

    unlock();

#else

    lock();
    if ( lseek( m_FileHandle, byteAddress, SEEK_SET ) == -1 )
        retn = errno;
    else
    {
        off_t readResult = ::read( m_FileHandle, pBuffer, byteCount );
        if ( readResult == -1 )
            retn = errno;
        else
            *pBytesRead = readResult;
    }
    unlock();

#endif

    return retn;
}


//  write()
//
//  Writes a particular number of bytes to the underlying file at a given byte offset
SYSTEMERRORCODE
SimpleFile::write
(
    const COUNT64       byteAddress,
    const BYTE* const   pBuffer,
    const COUNT         byteCount,
    COUNT* const        pBytesWritten
) const
{
    SYSTEMERRORCODE retn = SYSTEMERRORCODE_SUCCESS;

#ifdef  WIN32

    LARGE_INTEGER distance;
    distance.QuadPart = byteAddress;

    lock();
    if ( !SetFilePointerEx( m_FileHandle, distance, 0, FILE_BEGIN ) )
        retn = GetLastError();
    else
    {
        DWORD bytesWritten = 0;
        if ( !WriteFile(m_FileHandle, pBuffer, static_cast<DWORD>(byteCount), &bytesWritten, 0) )
            retn = GetLastError();
        else
            *pBytesWritten = bytesWritten;
    }

    unlock();

#else

    lock();
    if ( lseek( m_FileHandle, byteAddress, SEEK_SET ) == -1 )
        retn = errno;
    else
    {
        off_t writeResult = ::write( m_FileHandle, pBuffer, byteCount );
        if ( writeResult == -1 )
            retn = errno;
        else
            *pBytesWritten = writeResult;
    }
    unlock();

#endif

    return retn;
}

