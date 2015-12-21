//  Task.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Represents an executing task (or program)



#ifndef     EXECLIB_TASK_H
#define     EXECLIB_TASK_H



#include    "Activity.h"



class Task : public Lockable
{
private:
    typedef     std::list<Activity*>                ACTIVITIES;
    typedef     ACTIVITIES::iterator                ITACTIVITIES;
    typedef     ACTIVITIES::const_iterator          CITACTIVITIES;

    ACTIVITIES                  m_Activities;               //  Container of pointers to activities owned by this task
    const std::string           m_RunId;                    //  RunInfo's actual runid
    const std::string           m_TaskName;                 //  Taskname for the task

public:
    Task( const std::string&    runId,
            const std::string&  taskName );
    ~Task();

    void                        cleanActivities();
    void                        dump( std::ostream&         stream,
                                        const std::string&  prefix,
                                        const DUMPBITS      dumpBits ) const;
    bool                        hasLiveActivity() const;
    void                        killActivities() const;

    inline void                 appendActivity( Activity* const pActivity )     { m_Activities.push_back( pActivity ); }
};



#endif
