//	IoActivity.h
//  Copyright (c) 2015 by Kurt Duncan
//
//	Manages almost all IO for the exec



#ifndef		EXECLIB_IO_ACTIVITY_H
#define		EXECLIB_IO_ACTIVITY_H



#include    "IntrinsicActivity.h"
#include	"IoManager.h"



class	IoActivity : public IntrinsicActivity
{
private:
	IoManager* const	        m_pIoManager;
    void                        worker();

public:
	IoActivity( Exec* const         pExec,
                IoManager* const    pIoManager );

    inline bool                 isReady()                           { return Worker::isWorkerActive(); }

    //  IntrinsicActivity interface
    void                        dump( std::ostream&         stream,
                                        const std::string&  prefix,
                                        const DUMPBITS      dumpBits );
};



#endif

