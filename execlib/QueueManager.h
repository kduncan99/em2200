//  QueueManager.h header file



#ifndef     EMEXEC_QUEUE_MANAGER_H
#define     EMEXEC_QUEUE_MANAGER_H



#include    "ExecManager.h"



class   QueueManager : public ExecManager
{
private:

public:
    QueueManager( Exec* const pExec );

    bool                            submitDeck( const std::string& fileName );

	//  ExecManager interface
    void                            cleanup();
    void                            dump( std::ostream&     stream,
                                          const DUMPBITS    dumpBits );
	void							shutdown();
	bool							startup();
    void                            terminate();
};



#endif
