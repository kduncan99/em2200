//	FFKeyin.h
//  Copyright (c) 2015 by Kurt Duncan
//
//	FF keyin handler



#ifndef		EXECLIB_FF_KEYIN_H
#define		EXECLIB_FF_KEYIN_H



#include    "Exec.h"
#include	"KeyinActivity.h"



class	FFKeyin : public KeyinActivity
{
private:
    enum ParseResult
    {
        SUCCESSFUL,
        NOT_FOUND,
        SYNTAX_ERROR,
    };

    UINT16                      m_AbsoluteCycle;
    SuperString                 m_Filename;
    INDEX                       m_Index;
    SuperString                 m_Input;
    SuperString                 m_Qualifier;

    bool                        atEnd();
    ParseResult                 parseAsterisk();
    ParseResult                 parseCycle();
    ParseResult                 parseFilename();
    ParseResult                 parsePeriod();
    ParseResult                 parseQualifier();

    // KeyinThread interface
    void						handler();
    bool						isAllowed() const;

public:
    FFKeyin( Exec* const                        pExec,
             const SuperString&                 KeyinId,
             const SuperString&                 Option,
             const std::vector<SuperString>&    Params,
             const Word36&                      Routing );
};



#endif

