//  HttpServer derived from NetServer



#ifndef     MISCLIB_HTTPSERVER_H
#define     MISCLIB_HTTPSERVER_H

#include    "NetServer.h"


class   HttpServer : public NetServer
{
protected:
//    void            handleNewConnection( const int clientSocket ) override;
    
    
public:
    HttpServer( const std::string&  workerName,
                const long          listenAddress,
                const short         listenPort )
    :NetServer( workerName, listenAddress, listenPort )
    {}
};



#endif /* MISCLIB_HTTPSERVER_H */

