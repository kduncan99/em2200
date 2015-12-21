//  UserRunInfo class declaration
//  Copyright (c) 2015 by Kurt Duncan
//
//  RunInfo generics for everything that isn't the EXEC
//  We keep the READ symbiont buffers here, not because we expect to use them per se,
//  (they are needed in ControlModeRunInfo for sure), but because they group nicely
//  with the PRINT$ and PUNCH$ buffers, and we *do* need those here.


#ifndef     EXECLIB_USER_RUN_INFO_H
#define     EXECLIB_USER_RUN_INFO_H



#include    "RunInfo.h"
#include    "SymbiontBuffer.h"



//  annoying forward-references
class   Exec;



class   UserRunInfo : public RunInfo
{
public:
    enum SymbiontStatus
    {
        SST_SUCCESSFUL,         //  Output was posted, or input was read
        SST_BAD_FORMAT,         //  Could not interpret input as an SDF image, or output SDF type is invalid
                                //      Could also be used for output, on images that exceed allowable length.
        SST_CONTROL_IMAGE,      //  A control image was read - reissue command after checking for system stop.
        SST_END_OF_FILE,        //  077 control record encountered on input for file buffer
        SST_IO_ERROR,           //  IO error on file buffer, or RSI session has been closed
        SST_NOT_READY,          //  RSI Output buffer is full, or RSI Input buffer is empty.  Try again later.
        SST_PARTIAL_IMAGE,      //  A partial image was read - reissue command after checking for system stop.
    };

private:
    SymbiontStatus              drainPrintBuffer();
    SymbiontStatus              loadReadBuffer();

protected:
    typedef std::list<SymbiontBuffer*>          SYMBIONTBUFFERS;
    typedef SYMBIONTBUFFERS::iterator           ITSYMBIONTBUFFERS;
    typedef SYMBIONTBUFFERS::const_iterator     CITSYMBIONTBUFFERS;

    LSTRING						m_ConsoleLog;
    SYMBIONTBUFFERS             m_SymbiontBufferPrint;
    SYMBIONTBUFFERS             m_SymbiontBufferPunch;
    SYMBIONTBUFFERS             m_SymbiontBufferRead;

    UserRunInfo( Exec* const        pExec,
                 const State        initialState,
                 const std::string& originalRunId,
                 const std::string& actualRunId,
                 const std::string& accountId,
                 const std::string& projectId,
                 const std::string& userId,
                 const UINT32       options,
                 const char         schedulingPriority,
                 const char         processorDispatchingPriority );

public:
    virtual ~UserRunInfo();

    bool                        isPrivileged() const;
    SymbiontStatus              pollFromRead( std::string* const    pText,
                                                UINT8* const        pCharacterSet = 0 );
    bool                        popSymbiontBufferPrint();
    bool                        popSymbiontBufferPunch();
    bool                        popSymbiontBufferRead();
    SymbiontStatus              post042ToPrint( const SymbiontBuffer::CharacterSet charSet );
    SymbiontStatus              post077ToPrint();
    void                        postInfoToPrint();
    void                        postToConsoleLog( const std::string& message );
    SymbiontStatus              postToPrint( const SymbiontBuffer::CharacterSet charSet,
                                                const std::string&              text,
                                                const COUNT                     spacing = 1,
                                                const bool                      eject = false );
    void                        setErrorMode();

    //  RunInfo interface (passed through to ControlModeRunInfo)
    virtual void                cleanActivities() = 0;
    virtual bool                hasLiveActivity() const = 0;
    virtual void                killActivities() const = 0;

    //  getters
    inline const LSTRING&       getConsoleLog() const               { return m_ConsoleLog; }
    inline SymbiontBuffer*      getSymbiontBufferPrint() const
    {
        assert ( isAttached() );
        return ( m_SymbiontBufferPrint.size() > 0 ) ? m_SymbiontBufferPrint.back() : 0;
    }

    inline SymbiontBuffer*      getSymbiontBufferPunch() const
    {
        assert ( isAttached() );
        return ( m_SymbiontBufferPunch.size() > 0 ) ? m_SymbiontBufferPunch.back() : 0;
    }

    inline SymbiontBuffer*      getSymbiontBufferRead() const
    {
        assert ( isAttached() );
        return ( m_SymbiontBufferRead.size() > 0 ) ? m_SymbiontBufferRead.back() : 0;
    }

    //  setters, appenders
    inline void                 pushSymbiontBufferPrint( SymbiontBuffer* const pBuffer )
    {
        assert ( isAttached() );
        m_SymbiontBufferPrint.push_back( pBuffer );
    }

    inline void                 pushSymbiontBufferPunch( SymbiontBuffer* const pBuffer )
    {
        assert ( isAttached() );
        m_SymbiontBufferPunch.push_back( pBuffer );
    }

    inline void                 pushSymbiontBufferRead( SymbiontBuffer* const pBuffer )
    {
        assert ( isAttached() );
        m_SymbiontBufferRead.push_back( pBuffer );
    }

    //  other useful inlines
    inline SymbiontStatus   postToPrint( const std::string&     text,
                                            const COUNT         spacing = 1,
                                            const bool          pageEject = false )
    {
        assert( isAttached() );
        if ( m_SymbiontBufferPrint.size() == 0 )
            return SST_IO_ERROR;
        else
            return postToPrint( m_SymbiontBufferPrint.back()->getCurrentCharacterSet(), text, spacing, pageEject );
    }

    //  virtuals
    virtual void    dump( std::ostream&         stream,
                            const std::string&  prefix,
                            const DUMPBITS      dumpBits ) = 0;
};



#endif
