//	RSIActivity.h
//
//	Does legwork for RSIManager



#ifndef		EXECLIB_RSI_ACTIVITY_H
#define		EXECLIB_RSI_ACTIVITY_H



#include    "IntrinsicActivity.h"



class	RSIActivity : public IntrinsicActivity
{
private:
    RSIManager* const           m_pRSIManager;

    void                        worker();

public:
	RSIActivity( Exec* const        pExec,
                 RSIManager* const  pRSIManager );

    //  IntrinsicActivity interface
    void                        dump( std::ostream&         stream,
                                      const std::string&  prefix,
                                      const DUMPBITS      dumpBits );
};



#endif
