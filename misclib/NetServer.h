//  NetServer header file, describing a base class for more-specific handlers



#ifndef     MISCLIB_NETSERVER_H
#define     MISCLIB_NETSERVER_H



#include    "Lockable.h"
#include    "Worker.h"



class   NetServer : public Worker, public Lockable
{
private:
    const ulong             m_ListenAddress;
    const short             m_ListenPort;

    static const int        BACKLOG = 3;

    void                    worker() override;

protected:
//    virtual void            handleInput( const int clientSocket ) = 0;//????
//    virtual void            handleNewConnection( const int clientSocket ) = 0;
//    virtual void            handleSocketClosed( const int clientSocket ) = 0;//????

public:
    //???? c'tors should be protected - not prot now due to testing
    NetServer( const std::string&   workerName,
               const ulong          listenAddress,
               const short          listenPort );

    NetServer( const std::string&       workerName,
               const short              listenPort );

    virtual ~NetServer();

//    bool                    closeClientSocket( const int clientSocket );
//    bool                    sendClientSocket( const int         clientSocket,
//                                              const char* const pData,
//                                              const int         dataSize );
};



#endif /* MISCLIB_NETSERVER_H */

