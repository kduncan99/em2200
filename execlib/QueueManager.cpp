//	QueueManager.cpp
//  Copyright (c) 2015 by Kurt Duncan
//
//	Implementation of QueueManager class
//  Manages input and output queues (i.e., backlog and print/punch queues)



#include	"execlib.h"



//  private methods



//  Constructors, destructors

QueueManager::QueueManager
(
    Exec* const         pExec
)
:ExecManager( pExec )
{
}


//  Public methods

//  cleanup()
void
QueueManager::cleanup()
{
}


//  dump()
//
//  For debugging
void
QueueManager::dump
(
    std::ostream&       stream,
    const DUMPBITS      dumpBits
)
{
    stream << "QueueManager ----------" << std::endl;
}


//  shutdown()
//
//  Exec would like to shut down
void
QueueManager::shutdown()
{
    SystemLog::write("QueueManager::shutdown()");
}


//  startup()
//
//  Initializes the manager - Exec is booting
bool
QueueManager::startup()
{
    SystemLog::write("QueueManager::startup()");

    return true;
}


//  terminate()
//
//  Final termination of exec
void
QueueManager::terminate()
{
    SystemLog::write("QueueManager::terminate()");
}

