//  Lockable.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Interface to make another class lockable...
//  Uses a pointer to a std::recursive_mutex because:
//      a thread may recurse on locking
//      Implementing classes may have many getting methods which need to lock, but should be otherwise const



#ifndef     MISCLIB_LOCKABLE_H
#define     MISCLIB_LOCKABLE_H



class   Lockable
{
private:
    std::recursive_mutex*   m_pMutex;

protected:
    Lockable()
    {
        m_pMutex = new std::recursive_mutex;
    }

public:
    ~Lockable()
    {
        delete m_pMutex;
        m_pMutex = 0;
    }

    inline void     lock() const        { m_pMutex->lock(); }
    inline bool     try_lock() const    { return m_pMutex->try_lock(); }
    inline void     unlock() const      { m_pMutex->unlock(); }
};



#endif

