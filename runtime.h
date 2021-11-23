#ifndef BASIC_INT_RUNTIME_H
#define BASIC_INT_RUNTIME_H

#include <string>
#include <map>
#include <deque>
#include <iostream>
#include <sstream>

#include "value.hpp"


namespace runtime
{
    std::ostream& operator<<( std::ostream& os, const value_t& v );

    // All arithmetic operations are done in floating point.No matter what
    // the operands to + , -, *, / , and^ are, they will be converted to floating
    // point.The functions SIN, COS, ATN, TAN, SQR, LOG, EXPand RND also
    // convert their arguments to floating point and give the result as such.
    float_t ForceFloat( const value_t& v );

    // The operators AND, OR, NOT force both operands to be integers between
    // -32767 and +32767 before the operation occurs.
    // When a number is converted to an integer, it is truncated (rounded down).
    // It will perform as if INT function was applied.No automatic conversion is 
    // done between strings and numbers
    int_t ForceInt( const value_t& v );

    const str_t& ForceStr( const value_t& v );

    value_t AddImpl( const value_t& op1, const value_t& op2 );

    int_t LessEqImpl( const value_t& op1, const value_t& op2 );

    bool ToBoolImpl( const value_t& v );

    using linenum_t = unsigned long long;
    constexpr linenum_t MaxLineNum = std::numeric_limits<linenum_t>::max();

    struct partial_parse_action_tag;

    enum class PartialParseAction
    {
        Undefined,
        Continue,
        Discard
    };

    struct runtime_tag;

    class RuntimeBase
    {
    public:
        void Store( std::string name, value_t val );

        value_t Load( std::string name );

        void Clear()
        {
            mVars.clear();
        }

    public:
        static ValueType DetectVarType( std::string_view str );

    private:
        std::unordered_map<std::string, value_t> mVars;
    };

    class Runtime : public RuntimeBase
    {
    public:
        void AddLine( linenum_t line, std::string_view str );

        std::tuple<const std::string*, linenum_t> GetNextLine();

        void Goto( linenum_t line )
        {
            mCurLine = line;
        }

        void Gosub( linenum_t line )
        {
            mGosubStack.push_back( mCurLine );
            Goto( line );
        }

        void Return()
        {
            if( mGosubStack.empty() )
                throw std::runtime_error( "Mismatched GOSUB/RETURN statement" );

            Goto( mGosubStack.back() );
            mGosubStack.pop_back();
        }

        void ForLoop( std::string varName, value_t initVal, value_t targetVal, value_t stepVal )
        {
            Store( varName, std::move( initVal ) );
            mForLoopStack.push_back( { std::move( varName ), std::move( targetVal ), std::move( stepVal ), mCurLine } );
        }

        void Next( const std::string& varName );

        template<class T>
        void Print( T&& val ) const
        {
            std::cout << val;
        }

        void Input( const std::string& prompt, const std::string& name );

        void AddFakeInput( std::string str )
        {
            mFakeInput.push_back( std::move( str ) );
        }

        void AddData( int v )
        {
            mData.push_back( value_t{ static_cast<int_t>(v) } );
        }

        void AddData( float v )
        {
            mData.push_back( value_t{ float_t{v} } );
        }

        void AddData( str_t v )
        {
            mData.push_back( value_t{ std::move( v ) } );
        }

        void Read( std::string name );

        void Restore()
        {
            mCurDataIdx = 0;
        }

        void Clear();

    private:
        struct ForLoopItem
        {
            std::string varName;
            value_t targetVal;
            value_t stepVal;
            linenum_t startBodyLine;
        };

    private:
        std::map<linenum_t, std::string> mProgram;
        std::vector<ForLoopItem> mForLoopStack;
        std::vector<linenum_t> mGosubStack;
        std::deque<std::string> mFakeInput;
        linenum_t mCurLine = 0;
        std::vector<value_t> mData;
        size_t mCurDataIdx = 0;

    };

    class TestRuntime : public runtime::Runtime
    {
    public:
        template<class T>
        void Print( T&& val )
        {
            mStrOut << val;
        }

        void Clear()
        {
            Runtime::Clear();
            ClearOutput();
        }

        void ClearOutput()
        {
            mStrOut.str( "" );
        }

        std::string GetOutput() const
        {
            return mStrOut.str();
        }

    private:
        std::stringstream mStrOut;
    };
}



#endif // BASIC_INT_RUNTIME_H

