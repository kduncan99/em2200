//  NetServer implementation



#include    "misclib.h"



//  private methods

//  worker()
//
//  Sits on the indicated port and accepts connections
void
NetServer::worker()
{
    //  Create socket
    int listenSocket = socket( AF_INET, SOCK_STREAM, 0 );
    if ( listenSocket == -1 )
    {
        std::stringstream strm;
        strm << "NetServer " << Worker::getWorkerName() << " Cannot create socket";
        SystemLog::write( strm.str() );
        return;
    }

    //  Create socketAddressIn
    struct sockaddr_in socketAddressIn;
    int socketAddressSize = sizeof( sockaddr_in );

    socketAddressIn.sin_family = AF_INET;
    socketAddressIn.sin_addr.s_addr = htonl( m_ListenAddress );
    socketAddressIn.sin_port = ntohs( m_ListenPort );

    //  Bind
    if ( bind( listenSocket, reinterpret_cast<struct sockaddr *>( &socketAddressIn ), socketAddressSize ) < 0 )
    {
        std::stringstream strm;
        strm << "NetServer " << Worker::getWorkerName() << " Cannot bind socket";
        SystemLog::write( strm.str() );
        close( listenSocket );
        return;
    }

    //  Set up listen
    if ( listen( listenSocket, BACKLOG ) < 0 )
    {
        std::stringstream strm;
        strm << "NetServer " << Worker::getWorkerName() << " Cannot listen on socket";
        SystemLog::write( strm.str() );
        close( listenSocket );
        return;
    }

    //  Set up container to hold all the client sockets for open connections
    std::list<int> clientSockets;

    //  Now loop listening for incoming connections on the listen socket,
    //  and incoming data on any registered client sockets.
    while ( !isWorkerTerminating() )
    {
        //  Set up poll
        int socketCount = 1 + clientSockets.size();
        struct pollfd* pPollStructs = new struct pollfd[socketCount];

        pPollStructs[0].fd = listenSocket;
        pPollStructs[0].events = POLLIN | POLLPRI;
        pPollStructs[0].revents = 0;

        int px = 1;
        for ( auto itcs = clientSockets.begin(); itcs != clientSockets.end(); ++itcs )
        {
            pPollStructs[px].fd = *itcs;
            pPollStructs[px].events = POLLIN | POLLPRI | POLLRDHUP;
            pPollStructs[px].revents = 0;
            ++px;
        }

        //  Poll for anything, waiting for up to 5000ms
        int pollResult = poll( pPollStructs, socketCount, 5000 );
        if ( pollResult < 0 )
        {
            std::stringstream strm;
            strm << "NetServer poll() failed:" << errno << ":" << strerror( errno );
            SystemLog::write( strm.str() );

            delete[] pPollStructs;
            break;
        }

        //  Go see what there is to do (if anything)
        for ( px = 0; px < socketCount; ++px )
        {
            if ( pPollStructs[px].revents & POLLIN )
            {
                //???? for listen socket, a connect is ready
                //  for other sockets, read is ready
                std::cout << "px=" << px << ":POLLIN" << std::endl;//????
            }
            else if ( pPollStructs[px].revents & POLLPRI )
            {
                std::cout << "px=" << px << ":POLLPRI" << std::endl;//????
            }
            else if ( pPollStructs[px].revents & POLLRDHUP )
            {
                std::cout << "px=" << px << ":POLLRDHUP" << std::endl;//????
            }
        }

        delete[] pPollStructs;
    }

    //???? how to close client sockets / conversations?

    close( listenSocket );
}


NetServer::NetServer
(
    const std::string&   workerName,
    const ulong          listenAddress,
    const short          listenPort
)
:Worker( workerName ),
        m_ListenAddress( listenAddress ),
        m_ListenPort( listenPort )
{
}


NetServer::NetServer
(
    const std::string&       workerName,
    const short              listenPort
)
:Worker( workerName ),
        m_ListenAddress( INADDR_ANY ),
        m_ListenPort( listenPort )
{
}


NetServer::~NetServer()
{
};

