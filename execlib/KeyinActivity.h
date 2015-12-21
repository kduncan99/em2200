//	KeyinActivity class declaration
//  Copyright (c) 2015 by Kurt Duncan



#ifndef		EXECLIB_KEYIN_ACTIVITY_H
#define		EXECLIB_KEYIN_ACTIVITY_H



#include    "IntrinsicActivity.h"



class KeyinActivity : public IntrinsicActivity
{
protected:
    ConsoleManager*                 m_pConsoleManager;
    const SuperString               m_KeyinId;
    const SuperString               m_Option;
    const std::vector<SuperString>  m_Parameters;
    const Word36                    m_Routing;

    void                            displayGeneralError() const;
    void                            displayInvalidOption() const;
    void                            displayInvalidParameter() const;
    void                            displayNoParameters() const;
    void                            displayOptionNotAllowed() const;
    void                            displayParametersNotAllowed() const;

    // virtuals
    void                            dump( std::ostream&         stream,
                                          const std::string&    prefix,
                                          const DUMPBITS        dumpBits );
    virtual void                    handler() = 0;
    virtual bool                    isAllowed() const = 0;

    // Worker interface
    void                            worker();

public:
    KeyinActivity( Exec* const                      pExec,
                   const std::string&               KeyinId,
                   const std::string&               Option,
                   const std::vector<SuperString>&  Params,
                   const Word36&                    Routing );

    static KeyinActivity*           createKeyin( Exec* const        pExec,
                                                 const std::string& KeyinId,
                                                 const Word36&      Routing );
};



#endif

