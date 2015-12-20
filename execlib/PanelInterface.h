//  PanelInterface.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Abstract class which must be instantiated by the implementor.
//  Used by Exec to communicate with whoever is driving it.


#ifndef     EXECLIB_PANEL_INTERFACE_H
#define     EXECLIB_PANEL_INTERFACE_H



class   PanelInterface
{
public:
    typedef     INDEX       JUMPKEY;

protected:
    PanelInterface(){}

public:
    virtual ~PanelInterface(){}

    virtual bool            getJumpKey( const JUMPKEY jumpKey ) const = 0;
    virtual Word36          getJumpKeys() const = 0;
    virtual void            setJumpKey( const JUMPKEY   jumpKey,
                                        const bool      value ) = 0;
    virtual void            setJumpKeys( const Word36& jumpKeys ) = 0;
    virtual void            setStatusMessage( const std::string& message ) = 0;
    virtual void            setStopCodeMessage( const std::string& message ) = 0;
};



#endif

