//  Node.h
//  Copyright (c) 2015 by Kurt Duncan
//
//  Most basic class for all virtual hardware



#ifndef     HARDWARELIB_NODE_H
#define     HARDWARELIB_NODE_H



class   Node
{
public:
    typedef     UINT32                                  NODE_ADDRESS;
    typedef     std::set<Node*>                         ANCESTORS;
    typedef     std::map<NODE_ADDRESS, Node*>           DESCENDANTS;

    enum class Category
    {
        PROCESSOR,
        CHANNEL_MODULE,
        CONTROLLER,
        DEVICE,
    };


private:
    const Category              m_Category;
    const SuperString           m_Name;


protected:
    ANCESTORS                   m_Ancestors;
    DESCENDANTS                 m_Descendants;

    //  Convenience wrapper for derived objects which need to write to the system log.
    //  Automatically prepends node category and name to the message.
    inline void writeLogEntry( const std::string& text ) const
    {
        std::stringstream strm;
        strm << getCategoryString( m_Category ) << " " << m_Name << ":" << text;
        SystemLog::write( strm.str() );
    }

public:
    Node( const Category        category,
          const std::string&    name )
        :m_Category( category ),
                m_Name( name )
    {}

    virtual ~Node(){}

    //  For debugging
    virtual void                dump( std::ostream& stream ) const;

    //  Invoked when the config is built, and before we allow anyone into it.
    virtual void                initialize(){}

    //  This gets invoked anytime an async IO (which this object invoked) completes.
    //  At this level, nothing is done.  Derived classes may overload this and do something.
    virtual void                signal( Node* const pSource ){}

    //  Invoked just before tearing down the configuration.
    virtual void                terminate(){}

    //  inline getters
    inline const ANCESTORS&     getAncestors() const        { return m_Ancestors; }
    inline Category             getCategory() const         { return m_Category; }
    inline const char*          getCategoryString() const   { return getCategoryString( m_Category ); }
    inline const DESCENDANTS&   getDescendants() const      { return m_Descendants; }
    inline const SuperString&   getName() const             { return m_Name; }

    //  statics
    static bool                 connect( Node* const    pAncestor,
                                         Node* const    pDescendant );
    static bool                 connect( Node* const        pAncestor,
                                         const NODE_ADDRESS ancestorAddress,
                                         Node* const        pDescendant );
    static bool                 disconnect( Node* const pAncestor,
                                            Node* const pDescendant );
    static const char*          getCategoryString( const Category category );
};



#endif

