//	SystemLog.h - header file for generic system log
//  Copyright (c) 2015 by Kurt Duncan



#ifndef     MISCLIB_SYSTEM_LOG_H
#define     MISCLIB_SYSTEM_LOG_H



class	SystemLog : public Lockable
{
private:
    std::ofstream*              m_pStream;
    bool                        m_Enabled;

    static SystemLog*		    m_pInstance;

public:
    SystemLog();
    ~SystemLog();

    bool                        close();
    void                        enable( const bool flag );
    bool                        open( const std::string& fileName );
    void					    writeEntry( const std::string& text );

    static inline SystemLog* getInstance()
    {
        return m_pInstance;
    }

    static inline void write
        (
        const std::string&      text
        )
    {
        SystemLog* psl = getInstance();
        if ( psl )
            psl->writeEntry( text );
    }
};



#endif
