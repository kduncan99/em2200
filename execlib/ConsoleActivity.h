//	ConsoleActivity.h
//
//	Manages almost all IO for the exec



#ifndef		EXECLIB_CONSOLE_ACTIVITY_H
#define		EXECLIB_CONSOLE_ACTIVITY_H



#include	"ConsoleManager.h"
#include    "IntrinsicActivity.h"



class	ConsoleActivity : public IntrinsicActivity
{
private:
	ConsoleManager* const       m_pConsoleManager;
    void                        worker();

public:
	ConsoleActivity( Exec* const            pExec,
                     ConsoleManager* const  pConsoleManager );

    inline bool                 isReady()                           { return Worker::isWorkerActive(); }

    //  IntrinsicActivity interface
    void                        dump( std::ostream&         stream,
                                      const std::string&  prefix,
                                      const DUMPBITS      dumpBits );
};



#endif

