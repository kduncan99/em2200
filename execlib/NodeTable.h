//  NodeTable.h
//
//  Loaded by emexec, and passed to Exec upon instantiation.
//  It is conceivable that the owner (emexec) can update this while it is held by Exec;
//  This means that Exec must protect itself from damage which could be caused by such antics.
//
//  NOTE: Unlike many other containers, we do not consider ourselves the 'owner' of the contained
//  entities - thus, it is up to whoever creates and populates this object, to ensure proper
//  disposal of all nodes before (or after, as the case may be) destruction of *this* object.
//
//  May be extended, if that is desirable.



#ifndef     EXECLIB_NODE_TABLE_H
#define     EXECLIB_NODE_TABLE_H



class   NodeTable
{
protected:
    typedef     std::set<Node*>             NODES;
    typedef     NODES::iterator             ITNODES;
    typedef     NODES::const_iterator       CITNODES;

    NODES                           m_NodeSet;

public:
    NodeTable(){}
    virtual ~NodeTable(){}

    virtual void                    addNode( Node* const pNode )    { m_NodeSet.insert( pNode ); }

    inline Node*                    getNode( const std::string& nodeName ) const
    {
        for ( CITNODES itn = m_NodeSet.begin(); itn != m_NodeSet.end(); ++itn )
        {
            if ( (*itn)->getName().compareNoCase( nodeName ) == 0 )
                return *itn;
        }

        return 0;
    }

    inline const std::set<Node*>&   getNodeSet() const              { return m_NodeSet; }

    virtual void                    removeNode( Node* const pNode ) { m_NodeSet.erase( pNode ); }
};



#endif
