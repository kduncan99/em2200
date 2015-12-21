//  SubSystem.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Models a logical node subsystem for emssp



#ifndef     EMSSP_SUBSYSTEM_H
#define     EMSSP_SUBSYSTEM_H



class SubSystem
{
private:
    const SuperString               m_Name;             //  of this subsystem
    const std::list<Controller*>    m_Controllers;      //  in this subsystem
    std::list<Device*>              m_Devices;          //  in this subsystem

public:
    SubSystem( const std::string&               name,
               const std::list<Controller*>&    controllers )
    :m_Name( name ),
    m_Controllers( controllers )
    {}

    ~SubSystem(){};

    void                                    removeDevice( Device* const pDevice );

    inline const std::list<Controller*>     getControllers() const      { return m_Controllers; }
    inline const std::list<Device*>         getDevices() const          { return m_Devices; }
    inline const SuperString&               getName() const             { return m_Name; }
};



#endif

