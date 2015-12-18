//  DataHandler implementation
//  Copyright (c) 2015 by Kurt Duncan



#include    "misclib.h"



//  private, protected methods

//  processInputLine()
//
//  sub-function of load() -- handles a single line of input
bool
DataHandler::processInputLine
(
    const std::string&      text
)
{
    //  Ignore blank lines
    SuperString worker = text;
    worker.trimLeadingSpaces();
    worker.trimTrailingSpaces();
    if ( worker.size() == 0 )
        return true;

    //  Strip label from inputline
    std::string label = worker.strip();
    worker.trimLeadingSpaces();

    //  Locate DataSet, or create one as needed
    DataSet* pDataSet = 0;
    DataMap::iterator itdm = m_DataMap.find( label );
    if ( itdm != m_DataMap.end() )
        pDataSet = itdm->second;
    else
    {
        pDataSet = new DataSet();
        m_DataMap[label] = pDataSet;
    }

    //????

    return true;
}



//  constructors, destructors



//  public methods

//  clear()
//
//  Clears the data and the needs-persisted flag.
void
DataHandler::clear()
{
    while ( !m_DataMap.empty() )
    {
        DataMap::iterator itdm = m_DataMap.end();
        --itdm;
        delete itdm->second;
        m_DataMap.erase( itdm );
    }

    m_NeedsPersisted = false;
}


//  get()
//
//  Retrieves the number of DataLine objects contained by the DataSet corresponding to the given label.
COUNT
DataHandler::get
(
    const std::string&  label
) const
{
    COUNT result = 0;
    lock();
    DataMap::const_iterator itdm = m_DataMap.find( label );
    if ( itdm != m_DataMap.end() )
    {
        const DataSet* pDataSet = itdm->second;
        result = pDataSet->size();
    }
    unlock();
    return result;
}


//  get()
//
//  Retrieves the number of DataField objects contained by the DataLine described by
//  the given DataSet label and DataLine lineNumber.
COUNT
DataHandler::get
(
    const std::string&  label,
    const INDEX         lineNumber
) const
{
    COUNT result = 0;
    lock();
    DataMap::const_iterator itdm = m_DataMap.find( label );
    if ( itdm != m_DataMap.end() )
    {
        const DataSet* pDataSet = itdm->second;
        DataSet::const_iterator itds = pDataSet->find( lineNumber );
        if ( itds != pDataSet->end() )
        {
            const DataLine* pDataLine = itds->second;
            result = pDataLine->size();
        }
    }
    unlock();
    return result;
}


//  get()
//
//  Retrieves the number of DataSubfield objects contained by the DataField object
//  described by the DataSet label, DataLine lineNumber, and DataField fieldNumber.
COUNT
DataHandler::get
(
    const std::string&  label,
    const INDEX         lineNumber,
    const INDEX         fieldNumber
) const
{
    COUNT result = 0;
    lock();
    DataMap::const_iterator itdm = m_DataMap.find( label );
    if ( itdm != m_DataMap.end() )
    {
        const DataSet* pDataSet = itdm->second;
        DataSet::const_iterator itds = pDataSet->find( lineNumber );
        if ( itds != pDataSet->end() )
        {
            const DataLine* pDataLine = itds->second;
            DataLine::const_iterator itdl = pDataLine->find( fieldNumber );
            if ( itdl != pDataLine->end() )
            {
                const DataField* pDataField = itdl->second;
                result = pDataField->size();
            }
        }
    }
    unlock();
    return result;
}


//  get()
//
//  Retrieves a pointer to the string value described by the indicated
//  label, lineNumber, fieldNumber, and subfieldNumber.
const std::string*
DataHandler::get
(
    const std::string&  label,
    const INDEX         lineNumber,
    const INDEX         fieldNumber,
    const INDEX         subfieldNumber
) const
{
    const std::string* pResult = 0;
    lock();
    DataMap::const_iterator itdm = m_DataMap.find( label );
    if ( itdm != m_DataMap.end() )
    {
        const DataSet* pDataSet = itdm->second;
        DataSet::const_iterator itds = pDataSet->find( lineNumber );
        if ( itds != pDataSet->end() )
        {
            const DataLine* pDataLine = itds->second;
            DataLine::const_iterator itdl = pDataLine->find( fieldNumber );
            if ( itdl != pDataLine->end() )
            {
                const DataField* pDataField = itdl->second;
                DataField::const_iterator itdf = pDataField->find( subfieldNumber );
                if ( itdf != pDataField->end() )
                    pResult = itdf->second;
            }
        }
    }
    unlock();
    return pResult;
}


//  load()
//
//  Reads a quasi-SGS-formatted input file, and builds data content based there-on.
bool
DataHandler::load
(
    const std::string&  fileName
)
{
    lock();
    assert( m_DataMap.empty() );

    std::ifstream inputFile;
    inputFile.open( fileName );

    bool doneFlag = false;
    bool errorFlag = false;
    std::string instr;
    while ( !doneFlag && !errorFlag )
    {
        std::getline( inputFile, instr );
        if ( inputFile.fail() || inputFile.bad() )
            errorFlag = true;
        else
        {
            if ( !processInputLine( instr ) )
                errorFlag = true;
            if ( inputFile.eof() )
                doneFlag = true;
        }
    }
    inputFile.close();
    unlock();

    return !errorFlag;
}


//  remove()
//
//  Removes the entire DataSet indicated by the given label
bool
DataHandler::remove
(
    const std::string&  label
)
{
    bool result = false;
    lock();
    DataMap::const_iterator itdm = m_DataMap.find( label );
    if ( itdm != m_DataMap.end() )
    {
        delete itdm->second;
        m_DataMap.erase( itdm );
        result = true;
    }
    unlock();
    return result;
}


//  remove()
//
//  Removes a particular DataLine from the DataSet indicated by the given label and line number.
//  If the DataSet becomes empty as a result, it also is removed.
bool
DataHandler::remove
(
    const std::string&  label,
    const INDEX         lineNumber
)
{
    bool result = false;
    lock();
    DataMap::const_iterator itdm = m_DataMap.find( label );
    if ( itdm != m_DataMap.end() )
    {
        DataSet* pDataSet = itdm->second;
        DataSet::const_iterator itds = pDataSet->find( lineNumber );
        if ( itds != pDataSet->end() )
        {
            delete itds->second;
            pDataSet->erase( itds );
            result = true;

            if ( pDataSet->empty() )
                m_DataMap.erase( itdm );
        }
    }
    unlock();
    return result;
}


//  remove()
//
//  Removes a particular DataField from the DataLine indicated by the given label, line, and field number.
//  If the DataField becomes empty as a result, it is also removed, as well with the DataSet.
bool
DataHandler::remove
(
    const std::string&  label,
    const INDEX         lineNumber,
    const INDEX         fieldNumber
)
{
    bool result = false;
    lock();
    DataMap::const_iterator itdm = m_DataMap.find( label );
    if ( itdm != m_DataMap.end() )
    {
        DataSet* pDataSet = itdm->second;
        DataSet::const_iterator itds = pDataSet->find( lineNumber );
        if ( itds != pDataSet->end() )
        {
            DataLine* pDataLine = itds->second;
            DataLine::const_iterator itdl = pDataLine->find( fieldNumber );
            if ( itdl != pDataLine->end() )
            {
                delete itdl->second;
                pDataLine->erase( itdl );
                result = true;

                if ( pDataLine->empty() )
                {
                    pDataSet->erase( itds );
                    if ( pDataSet->empty() )
                        m_DataMap.erase( itdm );
                }
            }
        }
    }
    unlock();
    return result;
}


//  remove()
//
//  Removes a particular subfield from the DataField indicated by the given label, line, field, and subfield number.
//  All entities becoming empty as a result, are removed.
bool
DataHandler::remove
(
    const std::string&  label,
    const INDEX         lineNumber,
    const INDEX         fieldNumber,
    const INDEX         subfieldNumber
)
{
    bool result = false;
    lock();
    DataMap::const_iterator itdm = m_DataMap.find( label );
    if ( itdm != m_DataMap.end() )
    {
        DataSet* pDataSet = itdm->second;
        DataSet::const_iterator itds = pDataSet->find( lineNumber );
        if ( itds != pDataSet->end() )
        {
            DataLine* pDataLine = itds->second;
            DataLine::const_iterator itdl = pDataLine->find( fieldNumber );
            if ( itdl != pDataLine->end() )
            {
                DataField* pDataField = itdl->second;
                DataField::const_iterator itdf = pDataField->find( subfieldNumber );
                if ( itdf != pDataField->end() )
                {
                    delete itdf->second;
                    pDataField->erase( itdf );
                    result = true;

                    if ( pDataField->empty() )
                    {
                        pDataLine->erase( itdl );
                        if ( pDataLine->empty() )
                        {
                            pDataSet->erase( itds );
                            if ( pDataSet->empty() )
                                m_DataMap.erase( itdm );
                        }
                    }
                }
            }
        }
    }
    unlock();
    return result;
}


//  save()
//
//  Saves the datamap to a text file in something closely resembling SGS format
void
DataHandler::save
(
    const std::string&  fileName
)
{
    std::ofstream outputFile;
    outputFile.open( fileName );
    lock();

    for ( DataMap::const_iterator itdm = m_DataMap.begin(); itdm != m_DataMap.end(); ++itdm )
    {
        //  Once through here per unique label
        const std::string& label = itdm->first;
        const DataSet* pDataSet = itdm->second;
        for ( DataSet::const_iterator itds = pDataSet->begin(); itds != pDataSet->end(); ++itds )
        {
            //  Once through here per SGS line
            const DataLine* pDataLine = itds->second;
            outputFile << label;

            for ( DataLine::const_iterator itdl = pDataLine->begin(); itdl != pDataLine->end(); ++itdl )
            {
                //  Once through here per field
                const DataField* pDataField = itdl->second;
                outputFile << " ";
                bool first = true;

                for ( DataField::const_iterator itdf = pDataField->begin(); itdf != pDataField->end(); ++itdf )
                {
                    //  And once through here per subfield
                    if ( first )
                        first = false;
                    else
                        outputFile << ",";

                    std::string* pValue = itdf->second;
                    bool singleQuote = false;
                    bool doubleQuote = false;

                    for ( INDEX vx = 0; vx < pValue->size(); ++vx )
                    {
                        char ch = (*pValue)[vx];
                        if ( (ch == ASCII_SPACE) || (ch == ',') || (ch == ';') || (ch == '"') )
                        {
                            singleQuote = true;
                            break;
                        }
                        else if (ch == '\'')
                        {
                            doubleQuote = true;
                            break;
                        }
                    }

                    if ( singleQuote )
                        outputFile << '\'';
                    else if ( doubleQuote )
                        outputFile << '"';
                    outputFile << (*itdf->second);
                    if ( singleQuote )
                        outputFile << '\'';
                    else if ( doubleQuote )
                        outputFile << '"';
                }
            }

            outputFile << std::endl;
        }
    }
    unlock();

    outputFile.close();
}


//  set()
//
//  Sets up a given subfield value, creating any currently-non-existing items in the path
bool
DataHandler::set
(
    const std::string&      label,
    const INDEX             lineNumber,
    const INDEX             fieldNumber,
    const INDEX             subfieldNumber,
    const std::string&      value
)
{
    //  Is the label valid? It can have only alphas (upper or lower case) and digits
    for ( INDEX lx = 0; lx < label.size(); ++lx )
    {
        char ch = label[lx];
        if ( !isalpha(ch) && !isdigit(ch) )
            return false;
    }

    //  Is the value valid? It cannot any characters < ASCII_SPACE nor >= ASCII_DEL,
    //  nor can it contain both single and double quote marks.
    bool singleQuote = false;
    bool doubleQuote = false;
    for ( INDEX vx = 0; vx < value.size(); ++vx )
    {
        char ch = value[vx];
        if ( (ch < ASCII_SPACE) || (ch >= ASCII_DEL) )
            return false;
        if ( ch == '"' )
            doubleQuote = true;
        else if ( ch == '\'' )
            singleQuote = true;
    }

    if ( singleQuote && doubleQuote )
        return false;

    DataSet* pDataSet = 0;
    DataLine* pDataLine = 0;
    DataField* pDataField = 0;
    DataSubfield* pDataSubfield = 0;

    lock();
    DataMap::iterator itdm = m_DataMap.find( label );
    if ( itdm != m_DataMap.end() )
        pDataSet = itdm->second;
    else
    {
        pDataSet = new DataSet();
        m_DataMap[label] = pDataSet;
    }

    DataSet::iterator itds = pDataSet->find( lineNumber );
    if ( itds != pDataSet->end() )
        pDataLine = itds->second;
    else
    {
        pDataLine = new DataLine();
        (*pDataSet)[lineNumber] = pDataLine;
    }

    DataLine::iterator itdl = pDataLine->find( fieldNumber );
    if ( itdl != pDataLine->end() )
        pDataField = itdl->second;
    else
    {
        pDataField = new DataField();
        (*pDataLine)[fieldNumber] = pDataField;
    }

    DataField::iterator itdf = pDataField->find( subfieldNumber );
    if ( itdf != pDataField->end() )
        pDataSubfield = itdf->second;
    else
    {
        pDataSubfield = new DataSubfield();
        (*pDataField)[subfieldNumber] = pDataSubfield;
    }

    pDataSubfield->clear();
    pDataSubfield->append( value );

    unlock();
    return true;
}



//  enclosed class methods

DataHandler::DataField::~DataField()
{
    for ( iterator it = begin(); it != end(); ++it )
        delete it->second;
    clear();
}


DataHandler::DataLine::~DataLine()
{
    for ( iterator it = begin(); it != end(); ++it )
        delete it->second;
    clear();
}


DataHandler::DataSet::~DataSet()
{
    for ( iterator it = begin(); it != end(); ++it )
        delete it->second;
    clear();
}

