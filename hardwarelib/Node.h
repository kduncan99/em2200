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
    typedef     std::map<NODE_ADDRESS, Node*>           CHILDNODES;
    typedef     CHILDNODES::iterator                    ITCHILDNODES;
    typedef     CHILDNODES::const_iterator              CITCHILDNODES;

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
    CHILDNODES                  m_ChildNodes;

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

    //  Removes a child node from this node, but does NOT delete it
    inline void deregisterChildNode( const NODE_ADDRESS childAddress )
    {
        m_ChildNodes.erase( childAddress );
    }

    //  Attaches a child node to this node, at a particular address
    inline void registerChildNode( const NODE_ADDRESS  childAddress,
                                   Node* const         pNode )
    {
        m_ChildNodes[childAddress] = pNode;
    }

    //  This gets invoked anytime an async IO (which this object invoked) completes.
    //  At this level, nothing is done.  Derived classes may overload this and do something.
    virtual void                signal( Node* const pSource ){}

    //  Invoked just before tearing down the configuration.
    virtual void                terminate(){}

    //  inline getters
    inline Category             getCategory() const         { return m_Category; }
    inline const char*          getCategoryString() const   { return getCategoryString( m_Category ); }
    inline const CHILDNODES&    getChildNodes() const       { return m_ChildNodes; }
    inline const SuperString&   getName() const             { return m_Name; }

    //  statics
    static const char*          getCategoryString( const Category category );
};



#endif

