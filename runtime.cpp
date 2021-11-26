#include "runtime.h"
#include "parse_utils.hpp"
#include "grammar.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace runtime
{
std::ostream& operator<<( std::ostream& os, const value_t& v )
{
    struct Visitor
    {
        void operator()( float_t v ) const { os << v << 'f'; }
        void operator()( int_t v ) const { os << v << 'i'; }
        void operator()( str_t v ) const
        {
            boost::replace_all( v, "\n", "\\n" );
            boost::replace_all( v, "\t", "\\t" );
            boost::replace_all( v, "\r", "\\r" );
            os << '"' << v << '"';
        }

        std::ostream& os;
    };

    boost::apply_visitor( Visitor{ os }, v );

    return os;
}

float_t ForceFloat( const value_t& v )
{
    struct Impl
    {
        float_t operator()( float_t v ) const { return v; }
        float_t operator()( int_t v ) const { return static_cast<float_t>(v); }
        float_t operator()( const str_t& v ) const { throw std::runtime_error( "Cannot be string" ); }
    };

    return boost::apply_visitor( Impl{}, v );
}

int_t ForceInt( const value_t& v )
{
    struct Impl
    {
        int_t operator()( float_t v ) const { return static_cast<int_t>(v); }
        int_t operator()( int_t v ) const { return v; }
        int_t operator()( const str_t& v ) const { throw std::runtime_error( "Cannot be string" ); }
    };

    return boost::apply_visitor( Impl{}, v );
}

const str_t& ForceStr( const value_t& v )
{
    struct Impl
    {
        const str_t& operator()( float_t v ) const { throw std::runtime_error( "Must be string" ); }
        const str_t& operator()( int_t v ) const { throw std::runtime_error( "Must be string" ); }
        const str_t& operator()( const str_t& v ) const { return v; }
    };

    return boost::apply_visitor( Impl{}, v );
}

value_t AddImpl( const value_t& op1, const value_t& op2 )
{
    const str_t* pS1 = boost::get<str_t>( &op1 );
    const str_t* pS2 = boost::get<str_t>( &op2 );

    //Strings may also be concatenated (put or joined together) through
    //the use of the "+" operator.
    return pS1 && pS2 ?
        value_t{ *pS1 + *pS2 } :
        value_t{ ForceFloat( op1 ) + ForceFloat( op2 ) };
}

int_t LessEqImpl( const value_t& op1, const value_t& op2 )
{
    return int_t{ ForceFloat( op1 ) <= ForceFloat( op2 ) };
}

bool ToBoolImpl( const value_t& v )
{
    struct Impl
    {
        bool operator()( float_t v ) const { return v != 0; }
        bool operator()( int_t v ) const { return v != 0; }
        bool operator()( const str_t& v ) const { return !v.empty(); }
    };

    return boost::apply_visitor( Impl{}, v );
}

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

    mVars[name] = res;
}

value_t Runtime::Load( std::string name ) const
{
    if( name.empty() )
        throw std::runtime_error( "variable name cannot be empty" );

    boost::algorithm::to_lower( name );

    const auto itVar = mVars.find( name );

    if( itVar != mVars.end() )
        return itVar->second;

    std::cerr << "\033[93m" "WARNING: Access var before init: "  << name << "\033[0m" << std::endl;

    switch( DetectVarType( name ) )
    {
    case ValueType::Str:
        return value_t{ str_t{} };

    default:
        return value_t{ int_t{} };
    };
}

ValueType Runtime::DetectVarType( std::string_view str )
{
    for( char c : str )
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

void Runtime::AddLine( linenum_t line, std::string_view str )
{
    mProgram.emplace( line, str );
}

std::tuple<const std::string*, linenum_t> Runtime::GetNextLine()
{
    const auto it = mProgram.lower_bound( mCurLine );

    if( it == mProgram.end() )
        return {};

    Goto( it->first + 1 );

    return { &it->second, it->first };
}

void Runtime::Next( const std::string& varName )
{
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

    const int_t eqRes = LessEqImpl( curVal, cur.targetVal );

    if( eqRes != 0 )
    {
        if( cur.startBodyLine >= mCurLine )
            throw std::runtime_error("Same line NEXT isn't supported");

        Goto( cur.startBodyLine );
    }
    else
        mForLoopStack.pop_back();
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

void Runtime::Input( const std::string& prompt, const std::string& name )
{
    if( name.empty() )
        throw std::runtime_error( "variable name cannot be empty" );

    value_t res;
    std::string str;

    for( ;; )
    {
        std::cout << prompt;

        if( mFakeInput.empty() )
        {
            if( !std::getline( std::cin, str ) )
                throw std::runtime_error( "std::getline() error" );
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

void Runtime::Read( std::string name )
{
    if( mCurDataIdx >= mData.size() )
        throw std::runtime_error( "Out of DATA" );

    Store( std::move( name ), mData[mCurDataIdx] );
    ++mCurDataIdx;
}

void Runtime::Clear()
{
    mVars.clear();
    mFunctions.clear();
    mProgram.clear();
    mForLoopStack.clear();
    mGosubStack.clear();
    mFakeInput.clear();
    mCurLine = 0;
    mData.clear();
    mCurDataIdx = 0;
}

value_t FunctionRuntime::Calculate( const Runtime& rootRuntime, std::string_view exprStr, std::string_view varName, value_t varValue )
{
    FunctionRuntime runtime{ rootRuntime, varName, std::move(varValue) };

    std::string err{};
    value_t res;

    if( !Parse( exprStr, main_pass::expression_rule(), runtime, res, err ) )
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