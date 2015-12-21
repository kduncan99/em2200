//  Configuration
//  Copyright (c) 2015 by Kurt Duncan
//
//  Stores, manages, and provides look-ups for config tags and hardware configs

/*TODO:CONFIG
GCCMIN
Determines whether ER RSI$ code for batch
mode, demand mode, and batch punch is turned
on.
Set to the maximum number of terminals active
through RSI$ at any one time. All RSI$ features
are turned on if greater than 0.
If GCCMIN > 0 then RSICNT must be > 0.

RSICNT
Maximum number of terminals interfacing with
remote symbiont interface (RSI) including RSI$
remote, batch, and demand.
If RSICNT=0, all related RSI code is turned off.

Per DSchroth:
RSICNT always seems to be tied to DEMAND, while GCCMIN is tied to RSI$ terminals (both batch and demand)
*/



#include    "execlib.h"



//  statics



//  private methods

//  establishValue()
//
//  Helper function for establishing a new basic config value
bool
Configuration::establishValue
(
    const std::string&      key,
    const Value* const      pValue
)
{
    bool result = false;
    SuperString foldedKey = key;
    foldedKey.foldToUpperCase();

    ITDICTIONARY itEntry = m_Dictionary.find( foldedKey );
    if ( itEntry == m_Dictionary.end() )
    {
        m_Dictionary[foldedKey] = pValue;
        result = true;
    }

    return result;
}


//  getEntry()
//
//  Finds the requested entry - const version
const Configuration::Value*
Configuration::getEntry
(
    const SuperString&      key
) const
{
    SuperString foldedKey = key;
    foldedKey.foldToUpperCase();

    const Value* pValue = 0;
    CITDICTIONARY itd = m_Dictionary.find( foldedKey );
    if ( itd != m_Dictionary.end() )
        pValue = itd->second;
    return pValue;
}



//  constructors, destructors

Configuration::Configuration()
{
    //  Basic configuration values (temporary) TODO:CONFIG
    establishValue( "ACCTASGMNE", new StringValue( "F" ) );
    establishValue( "ACCTINTRES", new IntegerValue( 0 ) );
    establishValue( "CONSOLETYPE", new StringValue( "SMART" ) );
    establishValue( "DCLUTS", new IntegerValue( 26849 ) );
    establishValue( "DLOCASGMNE", new StringValue( "F" ) );
    establishValue( "EXECL", new BoolValue( true ) );           //  extended ECL (e.g., DIR, RINFO)
    establishValue( "EXKEYINS", new BoolValue( true ) );        //  extended console keyins (e.g., PREP)
    establishValue( "GCCMIN", new IntegerValue( 8 ) );
    establishValue( "GENFASGMNE", new StringValue( "F" ) );
    establishValue( "GENFINTRES", new IntegerValue( 100 ) );    //  Traditionally, default is 1000
    establishValue( "IDS", new StringValue( "LOCAL" ) );
    establishValue( "LIBASGMNE", new StringValue( "F" ) );
    establishValue( "LIBINTRES", new IntegerValue( 0 ) );
    establishValue( "LIBMAXSIZ", new IntegerValue( 99999 ) );
    establishValue( "MAXATMP", new IntegerValue( 3 ) );
    establishValue( "MAXGRN", new IntegerValue( 256 ) );
    establishValue( "MDFALT", new StringValue( "F" ) );
    establishValue( "MSTRACC", new StringValue("") );
    establishValue( "OVRACC", new StringValue("INSTALLATION") );
    establishValue( "OVRUSR", new StringValue("INSTALLATION") );
    establishValue( "RELUNUSEDRES", new BoolValue( true ) );
    establishValue( "RSICNT", new IntegerValue( 8 ) ) ;
    establishValue( "RUNASGMNE", new StringValue( "F2" ) );
    establishValue( "RUNINTRES", new IntegerValue( 1 ) );
    establishValue( "RUNMAXSIZ", new IntegerValue( 10000 ) );
    establishValue( "SECOFFDEF", new StringValue("ADMIN") );
    establishValue( "SFTIMESTAMP", new BoolValue( false ) );
    establishValue( "SKDATA", new BoolValue( true ) );
    establishValue( "SSPBP", new BoolValue( true ) );
    establishValue( "STDMSAVL", new FloatValue( 3.0 ) );
    establishValue( "STDMSTRT", new FloatValue( 1.5 ) );
    establishValue( "TDFALT", new StringValue( "T" ) );
    establishValue( "TPFMAXSIZ", new IntegerValue( 256 ) );
    establishValue( "TPFTYP", new StringValue( "F" ) );
    establishValue( "USERASGMNE", new StringValue( "F" ) );
    establishValue( "USERINTRES", new IntegerValue( 1 ) );
    establishValue( "WDFALT", new StringValue( "D" ) );
    establishValue( "ZOPTBATCHREJ", new BoolValue( true ) );
}


Configuration::~Configuration()
{
    for ( ITDICTIONARY itEntry = m_Dictionary.begin(); itEntry != m_Dictionary.end(); ++itEntry )
        delete itEntry->second;
}



//  public methods



//  public static methods

bool
Configuration::getBoolValue
(
    const std::string&          key
) const
{
    const BoolValue* pbv = dynamic_cast<const BoolValue*>(getEntry( key ) );
    assert( pbv );
    return pbv->m_Value;
}


float
Configuration::getFloatValue
(
    const std::string&          key
) const
{
    const FloatValue* pfv = dynamic_cast<const FloatValue*>(getEntry( key ));
    assert( pfv );
    return pfv->m_Value;
}


UINT64
Configuration::getIntegerValue
(
    const std::string&          key
) const
{
    const IntegerValue* piv = dynamic_cast<const IntegerValue*>(getEntry( key ));
    assert( piv );
    return piv->m_Value;
}


const SuperString&
Configuration::getStringValue
(
    const std::string&          key
) const
{
    const StringValue* psv = dynamic_cast<const StringValue*>(getEntry( key ));
    assert( psv );
    return psv->m_Value;
}

