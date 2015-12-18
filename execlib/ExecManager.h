//	ExecManager class declaration
//
//  ExecManager is any particular singleton entity which manages a subset of the Exec's
//  responsibilities.  Such entities may require boot-time initialization, and
//  carefully-sequenced shutdown notifications.



#ifndef		EXECLIB_EXEC_MANAGER_H
#define		EXECLIB_EXEC_MANAGER_H



//  Annoying but necessary forward-references
class   Exec;
class   AccountManager;
class   ConsoleManager;
class   DeviceManager;
class   FacilitiesManager;
class   IoManager;
class   MFDManager;
class   QueueManager;
class   RSIManager;
class   SecurityManager;



class ExecManager : public Lockable
{
protected:
    Exec* const                 m_pExec;

    ExecManager( Exec* const pExec )
        :m_pExec( pExec )
    {}

    AccountManager*      getAccountManager() const;
    ConsoleManager*      getConsoleManager() const;
    DeviceManager*       getDeviceManager() const;
    FacilitiesManager*   getFacilitiesManager() const;
    IoManager*           getIoManager() const;
    MFDManager*          getMFDManager() const;
    QueueManager*        getQueueManager() const;
    RSIManager*          getRSIManager() const;
    SecurityManager*     getSecurityManager() const;


public:
    virtual ~ExecManager(){}

    virtual void                cleanup();      //  will be called just prior to deletion of Exec and subordinate objects
                                                //      clean up lingering artifacts which were left laying about for dump()
    virtual void                dump( std::ostream&     stream,
                                      const DUMPBITS    dumpBits ) = 0;
                                                //  If called by GUI app, pActivity will be 0 (NULL)
    virtual bool                startup();      //  will be called during Exec boot, before BootThread starts
                                                //      a) clean up any lingering unwanted artifacts from previous session
                                                //      b) set up anything necessary for the next session
                                                //          this may include polling the configurator for CONFIG values
    virtual void                shutdown();     //  will be called during Exec stop, before terminating existing threads
                                                //      mainly used to allow managers to release any held activities
                                                //      so that they can terminate properly before the managers terminate
    virtual void                terminate();    //  will be called during Exec stop, after existing threads terminate
                                                //      finish up anything lingering, and end the manager's activity (if any)
                                                //      do NOT clean up more than necessary - leave as much as possible
                                                //      for any putative dump()
};



#endif

