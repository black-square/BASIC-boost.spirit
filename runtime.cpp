#include "runtime.h"
#include "parse_utils.hpp"
#include "grammar.h"
#include "platform.h"

#include <iostream>
#include <fstream>
#include <boost/algorithm/string/case_conv.hpp>

namespace runtime
{
static std::ofstream g_flInputLog;

void Runtime::Store( std::string name, value_t val )
{
    if( name.empty() )
        throw std::runtime_error( "variable name cannot be empty" );

    boost::algorithm::to_lower( name );

    value_t res;
    const str_t* pS = boost::get<str_t>( &val );

    switch( DetectVarType( name ) )
    {
    case ValueType::Str:
        if( !pS )
            throw std::runtime_error( "Expected String variable" );

        res = std::move( val );
        break;

    case ValueType::Int:
        res = ForceInt( val );
        break;

    default:
        res = ForceFloat( val );
    };

    const auto itVar = mVars.find( name );

    if( itVar == mVars.end() && IsArrayVar(name) )
        std::cerr << "\033[93m" "WARNING: Write array element before DIM: " << name << ", line: " << mProgramCounter.line << "\033[0m" << std::endl;

    mVars.insert_or_assign( itVar, std::move(name), std::move(res) );
}

value_t Runtime::Load( std::string name ) const
{
    if( name.empty() )
        throw std::runtime_error( "variable name cannot be empty" );

    boost::algorithm::to_lower( name );

    const auto itVar = mVars.find( name );

    if( itVar != mVars.end() )
        return itVar->second;

    std::cerr << "\033[93m" "WARNING: Access var before init: "  << name << ", line: " << mProgramCounter.line << "\033[0m" << std::endl;

    const auto val = GetDefaultValue( name );

    //Prevents more than one warning about the same var
    const_cast<Runtime *>(this)->mVars[std::move(name)] = val;

    return val;
}

void Runtime::Dim( std::string baseVarName, const std::vector<int_t>& dimentions )
{
    if( baseVarName.empty() )
        throw std::runtime_error( "variable name cannot be empty" );

    boost::algorithm::to_lower( baseVarName );

    if( dimentions.empty() )
    {
        mVars[std::move(baseVarName)] = GetDefaultValue(baseVarName);
        return;
    }

    ListAllArrayElements(dimentions, [baseVarName = std::move(baseVarName), this]( const auto &indices ) {
        std::string varName{ baseVarName };

        varName += '(';

        for( auto i: indices)
        {
            varName += std::to_string(i);
            varName += ',';
        }

        varName.back() = ')';

        mVars[std::move(varName)] = GetDefaultValue( baseVarName );
    });
}

ValueType Runtime::DetectVarType( std::string_view name )
{
    if( name.empty() )
        throw std::runtime_error( "variable name cannot be empty" );

    for( char c : name )
    {
        switch( c )
        {
        case '$': return ValueType::Str;
        case '%': return ValueType::Int;
        case '(': return ValueType::Float;
        }
    }

    return ValueType::Float;
}

bool Runtime::IsArrayVar( std::string_view name )
{
    for( char c : name )
        if( c == '(' )
            return true;

    return false;
}

value_t Runtime::GetDefaultValue( std::string_view name )
{
    switch( DetectVarType( name ) )
    {
    case ValueType::Str:
        return value_t{ str_t{} };

    case ValueType::Float:
        return value_t{ float_t{} };

    default:
        return value_t{ int_t{} };
    };
}

void Runtime::AddLine( linenum_t line, std::string_view str )
{
    if( !mProgram.empty() && line <= mProgram.rbegin()->first )
        throw std::runtime_error( "Line is in the wrong order " + std::to_string( line ) );

    const auto &[_, success] =  mProgram.emplace( line, str );

    if( !success )
        throw std::runtime_error( "Line was redefined " + std::to_string( line ) );

    mProgramCounter.line = line;
}

void Runtime::AppendToPrevLine( std::string_view str )
{
    const auto it = mProgram.find( mProgramCounter.line );

    if( it == mProgram.end() )
        throw std::runtime_error( "Previous line is unavailable" );

    it->second.append(" : ");
    it->second.append( str );
}

void Runtime::UpdateCurParseLine( linenum_t line )
{
    mProgramCounter.line = line;
}

std::tuple<const std::string*, linenum_t, unsigned> Runtime::GetNextLine()
{
    decltype(mProgram)::const_iterator it{};
    
    for(;;)
    {
        it = mProgram.lower_bound( mProgramCounter.line );

        if( it == mProgram.end() )
            return {};

        for( unsigned offset = mProgramCounter.lineOffset; offset < it->second.length(); ++offset )
            if( !std::isspace(it->second[offset]) )
            {
                GotoImpl( { it->first, ProgramCounter::ContinueExecution} );
                return { &it->second, it->first, offset };
            }         

        GotoImpl( {it->first + 1, 0} );
     }   
}

void Runtime::Goto( linenum_t line )
{
    if( line != MaxLineNum && mProgram.find(line) == mProgram.end() )
        throw std::runtime_error("Unknown line " + std::to_string( line ) );

    GotoImpl( { line, 0 } );
}

void Runtime::ForLoop( std::string varName, value_t initVal, value_t targetVal, value_t stepVal, unsigned currentLineOffset )
{
    Store( varName, std::move( initVal ) );

    const ProgramCounter pc{ mProgramCounter.line, currentLineOffset };

    boost::algorithm::to_lower( varName );

    mForLoopStack.push_back( { std::move( varName ), std::move( targetVal ), std::move( stepVal ), pc } );
}

bool Runtime::NextImpl( std::string varName )
{
    boost::algorithm::to_lower( varName );

    for( ;; )
    {
        if( mForLoopStack.empty() )
            throw std::runtime_error( "Mismatched FOR/NEXT statement" );

        if( varName.empty() || varName == mForLoopStack.back().varName )
            break;

        mForLoopStack.pop_back();
    }

    auto& cur = mForLoopStack.back();
    auto curVal = Load( cur.varName );
    curVal = AddImpl( curVal, cur.stepVal );
    Store( cur.varName, curVal );

    const int_t eqRes = ForceFloat( cur.stepVal ) < 0 ?
        LessEqImpl( cur.targetVal, curVal ) :
        LessEqImpl( curVal, cur.targetVal );

    if( eqRes != 0 )
    {
        GotoImpl( cur.startBodyPC );
        return true;
    }
    else
    {
        mForLoopStack.pop_back();
        return false;
    }
}

void Runtime::Next( std::vector<std::string> varNames )
{
    if( varNames.empty() )
    {
        NextImpl( {} );
        return;
    }

    for( auto &varName : varNames )
        if( NextImpl( std::move(varName) ) )
            return;
}

void Runtime::DefineFuntion( std::string fncName, std::string varName, std::string exprStr )
{
    boost::algorithm::to_lower( fncName );
    boost::algorithm::to_lower( varName );

    mFunctions.insert_or_assign( std::move( fncName ), FunctionInfo{ std::move( varName ), std::move( exprStr ) } );
}

value_t Runtime::CallFuntion( std::string fncName, value_t arg ) const
{
    boost::algorithm::to_lower( fncName );

    const auto it = mFunctions.find( fncName );

    if( it == mFunctions.end() )
        throw std::runtime_error("Unknown function name " + fncName );


    return FunctionRuntime::Calculate( *this, it->second.exprStr, it->second.varName, std::move(arg) );
}

template<class T>
inline void Runtime::PrintNumberImpl( T val ) const
{
    // "Printed numbers are always followed by a space.
    //  Positive numbers are preceded by a space.
    //  Negative numbers are preceded by a minus sign."
    if( val >= 0 )
        std::cout << ' ';

    std::cout << val << ' ';
}

void Runtime::Print( int_t val ) const
{
    PrintNumberImpl( val );
}

void Runtime::Print( float_t val ) const
{
    PrintNumberImpl( val );
}

void Runtime::Print( const str_t& val ) const
{
    std::cout << val;
}

void Runtime::Input( const std::string& prompt, const std::string& name )
{
    if( name.empty() )
        throw std::runtime_error( "variable name cannot be empty" );

    value_t res;
    std::string str;

    for( ;; )
    {
        std::cout << prompt;

        if( prompt.empty() || prompt.back() != '?' )
            std::cout << '?';

        std::cout << ' ';

        if( mFakeInput.empty() )
        {
            if( !std::getline( std::cin, str ) )
                throw std::runtime_error( "std::getline() error" );

            if( !g_flInputLog.is_open() )
                g_flInputLog.open("input.log");

            g_flInputLog << str << std::endl;
        }
        else
        {
            str = std::move( mFakeInput.front() );
            mFakeInput.pop_front();
            std::cout << str << std::endl;
        }

        const char* const pBeg = str.data();
        const char* const pEnd = pBeg + str.size();
        char* pLast = nullptr;

        switch( DetectVarType( name ) )
        {
        case ValueType::Str:
            res = std::move( str );
            pLast = const_cast<char*>(pEnd);
            break;

        case ValueType::Int:
            res = static_cast<int_t>(std::strtol( pBeg, &pLast, 10 ));
            break;

        default:
            res = std::strtof( pBeg, &pLast );
        };

        if( pLast == pEnd )
            break;

        std::cout << "?REENTER" << std::endl;
    }

    Store( name, res );
}

runtime::value_t Runtime::Inkey()
{
    if( mFakeInput.empty() )
    {
        const int key = GetPressedKbKey();
        return value_t{ key != 0 ? std::string( 1, (char)key ) : std::string( "" ) };
    }

    std::string res{ std::move( mFakeInput.front() ) };

    mFakeInput.pop_front();

    return value_t{res};
}

void Runtime::Read( std::string name )
{
    if( mCurDataIdx >= mData.size() )
        throw std::runtime_error( "Out of DATA" );

    Store( std::move( name ), mData[mCurDataIdx] );
    ++mCurDataIdx;
}

void Runtime::AddDataImpl( value_t value )
{
    mLineToDataPos.try_emplace( mProgramCounter.line, mData.size() );
    mData.push_back( std::move(value) );
}

void Runtime::Start()
{
    Randomize( (unsigned int)std::time(0) ); 
    mProgramCounter = {};
}

void Runtime::PrintVars( std::ostream& os ) const
{
    for ( auto &v: mVars)
        os << ' ' << v.first << '=' << v.second;
}

void Runtime::Restore()
{
    mCurDataIdx = 0;
}

void Runtime::Restore( linenum_t line )
{
    const auto it = mLineToDataPos.find(line);

    if( it == mLineToDataPos.end() )
        throw std::runtime_error( "Unknown line " + std::to_string( line ) );

    mCurDataIdx = it->second;
}

void Runtime::Randomize( unsigned int n )
{
    if( !mFakeInput.empty() )
    {    
        //We need a deterministic rand() for automation
        n = 0;
    }

    std::srand( n );
}

void Runtime::ClearProgram()
{
    mProgram.clear();
    mLineToDataPos.clear();
    mForLoopStack.clear();
    mGosubStack.clear();
    mProgramCounter = {};
}

void Runtime::Clear()
{
    ClearProgram();
    mVars.clear();
    mFunctions.clear();
    mFakeInput.clear();  
    mData.clear();
    mCurDataIdx = 0;
}

value_t FunctionRuntime::Calculate( const Runtime& rootRuntime, std::string_view exprStr, std::string_view varName, value_t varValue )
{
    FunctionRuntime runtime{ rootRuntime, varName, std::move(varValue) };

    std::string err{};
    value_t res;

    const auto parseFnc = [&runtime, &res]( auto& args )
    {
        return phrase_parse( args.cur, args.end, args.MakeFullParser(runtime, main_pass::expression_rule()), args.spaceParser, res );
    };

    if( !ParseSingle( exprStr, 0, err, parseFnc ) )
    {
        std::cerr << "\033[91m" "-------------------------\n";
        std::cerr << "Function execution failed\n" << exprStr << "\n";
        std::cerr << "Error: " << err << "\n";
        std::cerr << "-------------------------\n" "\033[0m";
        throw std::runtime_error( "Function execution failed" );
    }

    return res;
}

main_pass::value_t FunctionRuntime::Load( std::string name ) const
{
    boost::to_lower(name);

    if( name != mVarName )
        throw std::runtime_error( "Unknown variable inside the function body: " + name );

    return mVarValue;
}

}