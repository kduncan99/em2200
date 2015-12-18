//  Models a logical node subsystem for emssp



#ifndef     EMSSP_SUBSYSTEM_H
#define     EMSSP_SUBSYSTEM_H



class SubSystem
{
private:
    const SuperString               m_Name;             //  of this subsystem
    std::list<ChannelModule*>       m_ChannelModules;   //  to which we are connected
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

    inline void                             clearChannelModules()       { m_ChannelModules.clear(); }
    inline const std::list<ChannelModule*>  getChannelModules() const   { return m_ChannelModules; }
    inline const std::list<Controller*>     getControllers() const      { return m_Controllers; }
    inline const std::list<Device*>         getDevices() const          { return m_Devices; }
    inline const SuperString&               getName() const             { return m_Name; }
};



#endif

