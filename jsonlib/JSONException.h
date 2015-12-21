//  JSONException.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Header file for abstract base class from which all other JSON exception classes are derivce



#ifndef     JSONLIB_JSON_EXCEPTION_H
#define     JSONLIB_JSON_EXCEPTION_H



class   JSONException : public std::exception
{
private:
    const std::string       m_Reason;

protected:
    JSONException( const char* const pReason )
        :m_Reason( pReason )
    {}

    JSONException( const std::string& reason )
        :m_Reason( reason )
    {}

public:
    virtual ~JSONException(){}
};



#endif