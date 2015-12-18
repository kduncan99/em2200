//  DiskController.h
//
//  Header file for a virtual disk controller.



#ifndef     HARDWARELIB_DISK_CONTROLLER_H
#define     HARDWARELIB_DISK_CONTROLLER_H



#include    "Controller.h"



class   DiskController : public Controller
{
public:
    DiskController( const std::string& name )
        :Controller( ControllerType::DISK, name )
    {}

    ~DiskController(){};

    COUNT                       getContainingBufferSize( const COUNT requestedBufferSize ) const;
};



#endif
