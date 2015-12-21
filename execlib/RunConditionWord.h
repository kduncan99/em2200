//  RunConditionWord.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Models the RCW in the RunInfo object

/*TODO:TASK
Bit 7
At least one task, before the most recently executed task, has registered an error.

Bit 8
The most recently executed task registered an error.
These errors include, but are not limited to, error termination of language processors.
Input data errors, such as incorrect compiler language syntax, are also included.
Only situations that result in error or fatal error messages are treated as errors for
the purposes of bits 7 and 8 of the condition word setting.
For a task using bits 7 and 8, the following convention is used. At the very beginning
of a task, if bit 8 is set, indicating that the previous task registered an error, then set
bit 7. In any case, set bit 8. Then at the very end of the task, if it completes without
error, clear bit 8. Thus, if the task wants to register an unsuccessful completion or if
it terminates unexpectedly, the error is already registered. An unexpected
termination is defined as an error.
Both bits 7 and 8 must be checked, through ER COND$ or @TEST, to determine
whether a previous task has had an error termination.

Bit 9
Most recent activity termination was an abort termination (not error termination).

Bit 10
Most recent activity termination was an error termination.

Bit 11
One or more previous activity terminations of the current task, or previous task if
between executions, was an error termination. Refer to Section 3 for more details
on error termination.
Note: Bits 9-11 are cleared with the execution of the next task (@XQT). A call to a
postmortem dump processor such as @PMD is an exception since this call does not
cause alteration of the condition word.

Bits 9 - 11 and bits 0 - 6 in T1 can be modified only by the Executive. The Executive does not
change bits 7 and 8. T2 can be modified only by a control statement. T3 can be
modified only by an ER. Bits 7 and 8 can be modified by a control statement or an ER,
but not by the Executive. These two bits are maintained by the tasks that set them.
*/



#ifndef     EXECLIB_RUN_CONDITION_WORD_H
#define     EXECLIB_RUN_CONDITION_WORD_H



class RunConditionWord : public Word36
{
public:
    bool            anyPreviousTaskInError() const              { return getPreviousTaskError() || getPreviousPreviousTaskError(); }

    bool            getInhibitPageEjects() const                { return (getValue() & MASK_B3) != 0; }
    bool            getRescheduledAfterRecovery() const         { return (getValue() & MASK_B4) != 0; }
    bool            getInhibitRunTerminationOnError() const     { return (getValue() & MASK_B5) != 0; }
    bool            getPreviousPreviousTaskError() const        { return (getValue() & MASK_B7) != 0; }
    bool            getPreviousTaskError() const                { return (getValue() & MASK_B8) != 0; }
    bool            getMostRecentActivityAborted() const        { return (getValue() & MASK_B9) != 0; }
    bool            getMostRecentActivityErrored() const        { return (getValue() & MASK_B10) != 0; }
    bool            getOneOrMoreActivitiesErrored() const       { return (getValue() & MASK_B11) != 0; }

    void            clearInhibitPageEjects()                    { setValue( getValue() & MASK_NOT_B3 ); }
    void            clearRescheduledAfterRecovery()             { setValue( getValue() & MASK_NOT_B4 ); }
    void            clearInhibitRunTerminationOnError()         { setValue( getValue() & MASK_NOT_B5 ); }
    void            clearPreviousPreviousTaskError()            { setValue( getValue() & MASK_NOT_B7 ); }
    void            clearPreviousTaskError()                    { setValue( getValue() & MASK_NOT_B8 ); }
    void            clearMostRecentActivityAborted()            { setValue( getValue() & MASK_NOT_B9 ); }
    void            clearMostRecentActivityErrored()            { setValue( getValue() & MASK_NOT_B10 ); }
    void            clearOneOrMoreActivitiesErrored()           { setValue( getValue() & MASK_NOT_B11 ); }
    void            setInhibitPageEjects()                      { setValue( getValue() | MASK_B3 ); }
    void            setRescheduledAfterRecovery()               { setValue( getValue() | MASK_B4 ); }
    void            setInhibitRunTerminationOnError()           { setValue( getValue() | MASK_B5 ); }
    void            setPreviousPreviousTaskError()              { setValue( getValue() | MASK_B7 ); }
    void            setPreviousTaskError()                      { setValue( getValue() | MASK_B8 ); }
    void            setMostRecentActivityAborted()              { setValue( getValue() | MASK_B9 ); }
    void            setMostRecentActivityErrored()              { setValue( getValue() | MASK_B10 ); }
    void            setOneOrMoreActivitiesErrored()             { setValue( getValue() | MASK_B11 ); }
};



#endif
