//  Processor.h
//
//  Base class for all processors.
//  Anything common to all processors goes here.  Not that there is anything like that at the moment...



#ifndef     HARDWARELIB_PROCESSOR_H
#define     HARDWARELIB_PROCESSOR_H



#include    "Node.h"



class   Processor : public Node
{
public:
    enum ProcessorType
    {
        IP,
        IOP,
    };

private:
    const ProcessorType         m_ProcessorType;

protected:
    Processor( const ProcessorType  processorType,
               const std::string&   name )
        :Node( Category::PROCESSOR, name ),
                m_ProcessorType( processorType )
    {}

public:
    virtual ~Processor(){}

    inline ProcessorType        getProcessorType() const        { return m_ProcessorType; }
    inline const char*          getProcessorTypeString() const  { return getProcessorTypeString( m_ProcessorType ); }

    //  For debugging
    virtual void                dump( std::ostream& stream ) const;

    static const char*          getProcessorTypeString( const ProcessorType processorType );
};



#endif

