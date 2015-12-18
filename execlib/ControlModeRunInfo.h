//  ControlModeRunInfo class declaration
//
//  RunInfo generics for runs that can have control mode (i.e., batch and demand)



#ifndef     EXECLIB_CONTROL_MODE_RUN_INFO_H
#define     EXECLIB_CONTROL_MODE_RUN_INFO_H



#include    "Activity.h"
#include    "UserRunInfo.h"



//  annoying forward-references
class   Exec;



class   ControlModeRunInfo : public UserRunInfo
{
protected:
    Activity*                   m_pControlModeActivity;         //  An activity not affiliated with a Task object
    bool                        m_DataIgnoredMsgFlag;
    COUNT                       m_SkipCount;                    //  @JUMP and @TEST logic for skipping {n} statements
    std::string                 m_SkipLabel;                    //  if non-empty, it's the target label for @JUMP or @TEST skip to label
    LSTRING                     m_StatementImageStack;          //  Where we build up input control images

protected:
    ControlModeRunInfo( Exec* const         pExec,
                        const State         initialState,
                        const std::string&  originalRunId,
                        const std::string&  actualRunId,
                        const std::string&  accountId,
                        const std::string&  projectId,
                        const std::string&  userId,
                        const UINT32        options,
                        const char          schedulingPriority,
                        const char          processorDispatchingPriority );

public:
    virtual ~ControlModeRunInfo();

    //  UserRunInfo interface
    virtual void                cleanActivities();
    virtual bool                hasLiveActivity() const;
    virtual void                killActivities() const;

    //  getters
    inline Activity*            getControlModeActivity() const      { return m_pControlModeActivity; }
    inline bool                 getDataIgnoredMsgFlag() const       { return m_DataIgnoredMsgFlag; }
    inline COUNT                getSkipCount() const                { return m_SkipCount; }
    inline const std::string&   getSkipLabel() const                { return m_SkipLabel; }
    inline const LSTRING&       getStatementImageStack() const      { return m_StatementImageStack; }

    //  setters, appenders
    inline void                 appendStatementImage( const std::string& image )
    {
        assert( isAttached() );
        m_StatementImageStack.push_back( image );
    }

    inline void                 clearStatementImageStack()
    {
        assert( isAttached() );
        m_StatementImageStack.clear();
    }

    inline void                 decrementSkipCount()
    {
        assert( isAttached() );
        assert( m_SkipCount > 0 );
        --m_SkipCount;
    }

    inline void                 deleteControlModeActivity()
    {
        assert( isAttached() );
        assert( m_pControlModeActivity && m_pControlModeActivity->isTerminated() );
        delete m_pControlModeActivity;
        m_pControlModeActivity = 0;
    }

    inline void                 setControlModeActivity( Activity* const pActivity )
    {
        assert( isAttached() );
        assert( m_pControlModeActivity == 0 );
        m_pControlModeActivity = pActivity;
    }

    inline void                 setDataIgnoredMsgFlag( const bool flag )
    {
        assert( isAttached() );
        m_DataIgnoredMsgFlag = flag;
    }

    inline void                 setSkipCount( const COUNT count )
    {
        assert( isAttached() );
        m_SkipCount = count;
    }

    inline void                 setSkipLabel( const std::string& label )
    {
        assert( isAttached() );
        m_SkipLabel = label;
    }

    //  virtuals
    virtual void    dump( std::ostream&         stream,
                            const std::string&  prefix,
                            const DUMPBITS      dumpBits ) = 0;
};



#endif
