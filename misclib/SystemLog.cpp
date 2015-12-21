//	SystemLog.cpp
//  Copyright (c) 2015 by Kurt Duncan



#include	"misclib.h"



//  statics

SystemLog*  SystemLog::m_pInstance = 0;



//	private, protected methods



//	constructors, destructors

SystemLog::SystemLog()
{
    assert( !m_pInstance );
    m_pInstance = this;
    m_pStream = 0;
}


SystemLog::~SystemLog()
{
    if ( m_pStream )
        close();
    m_pInstance = 0;
}



//	public methods

//  close()
//
//  Closes the underlying log file
bool
    SystemLog::close()
{
    if ( !m_pStream )
        return false;

    m_pStream->close();
    delete m_pStream;
    m_pStream = 0;

    return true;
}


//  open()
//
//  Opens the log file
bool
    SystemLog::open
    (
    const std::string&      fileName
    )
{
    if ( m_pStream )
        return false;

    m_Enabled = true;
    m_pStream = new std::ofstream();
    m_pStream->open( fileName );
    return true;
}


//  writeEntry()
//
//  Writes an entry to the SystemLog
void
	SystemLog::writeEntry
	(
	const std::string&		text
	)
{
    if ( m_Enabled )
    {
        lock();
        *m_pStream << text << std::endl;
        unlock();
    }
}
