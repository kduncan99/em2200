//  TapeController.h
//
//  Header file for a virtual tape controller.



#ifndef     HARDWARELIB_TAPE_CONTROLLER_H
#define     HARDWARELIB_TAPE_CONTROLLER_H



#include    "Controller.h"



class   TapeController : public Controller
{
public:
    TapeController( const std::string& name )
        :Controller( ControllerType::TAPE, name )
    {}

    ~TapeController(){}

    inline COUNT                getContainingBufferSize( const COUNT requestedBufferSize ) const    { return requestedBufferSize; }
};



#endif
