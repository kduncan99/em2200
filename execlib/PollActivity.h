//	PollActivity.h
//
//	Periodic polling thread



#ifndef		EXECLIB_POLL_ACTIVITY_H
#define		EXECLIB_POLL_ACTIVITY_H



#include	"IntrinsicActivity.h"



class	PollActivity : public IntrinsicActivity
{
private:
    enum LocalState
    {
        CHECK_TIME              = m_InitialState,
        NEW_DAY_ACTIONS,
        SIX_MINUTE_ACTIONS,
        SIX_SECOND_ACTIONS,
        ONE_SECOND_ACTIONS,
    };

    UINT32                  m_PreviousMidnightDayOfWeek;
    EXECTIME                m_Previous1Second;
    EXECTIME                m_Previous6Minute;
    EXECTIME                m_Previous6Second;

    //  IntrinsicActivity interface
    void                        dump( std::ostream&         stream,
                                        const std::string&  prefix,
                                        const DUMPBITS      dumpBits );

    void                    newDayActions() const;
    void                    oneSecondActions();
    void                    sixMinuteActions() const;
    void                    sixSecondActions() const;
    void                    worker();

public:
    PollActivity( Exec* const pExec );
};



#endif

