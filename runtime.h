#ifndef BASIC_INT_RUNTIME_H
#define BASIC_INT_RUNTIME_H

#include <string>
#include <map>
#include <deque>
#include <unordered_map>
#include <iostream>
#include <sstream>

#include "value.hpp"


namespace runtime
{
    std::ostream& operator<<( std::ostream& os, const value_t& v );

    // "All arithmetic operations are done in floating point.No matter what
    //  the operands to + , -, *, / , and^ are, they will be converted to floating
    //  point.The functions SIN, COS, ATN, TAN, SQR, LOG, EXPand RND also
    //  convert their arguments to floating point and give the result as such."
    float_t ForceFloat( const value_t& v );

    // "The operators AND, OR, NOT force both operands to be integers between
    //  -32767 and +32767 before the operation occurs.
    //  When a number is converted to an integer, it is truncated (rounded down).
    //  It will perform as if INT function was applied.No automatic conversion is 
    //  done between strings and numbers"
    int_t ForceInt( const value_t& v );

    const str_t& ForceStr( const value_t& v );

    value_t AddImpl( const value_t& op1, const value_t& op2 );

    int_t LessEqImpl( const value_t& op1, const value_t& op2 );

    bool ToBoolImpl( const value_t& v );

    using linenum_t = unsigned long long;
    constexpr linenum_t MaxLineNum = std::numeric_limits<linenum_t>::max();

    struct line_begin_tag;

    struct runtime_tag;

    class Runtime
    {
    public:
        void Store( std::string name, value_t val );
        value_t Load( std::string name ) const;

        void AddLine( linenum_t line, std::string_view str );
        void AppendToPrevLine( std::string_view str );
        void UpdateCurParseLine( linenum_t line );

        std::tuple<const std::string*, linenum_t, unsigned> GetNextLine();

        void Dim( std::string baseVarName, const std::vector<int_t> &dimentions );

        void Goto( linenum_t line );

        void GotoNextLine()
        {
            GotoImpl( { mProgramCounter.line + 1, 0 } );
        }

        void Gosub( linenum_t line, unsigned currentLineOffset )
        {
            const ProgramCounter pc{ mProgramCounter.line, currentLineOffset };

            mGosubStack.push_back( pc );
            Goto( line );
        }

        void Return()
        {
            if( mGosubStack.empty() )
                throw std::runtime_error( "Mismatched GOSUB/RETURN statement" );

            GotoImpl( mGosubStack.back() );
            mGosubStack.pop_back();
        }

        void ForLoop( std::string varName, value_t initVal, value_t targetVal, value_t stepVal, unsigned currentLineOffset );

        void Next( std::vector<std::string> varNames );

        void DefineFuntion( std::string fncName, std::string varName, std::string exprStr );

        value_t CallFuntion( std::string fncName, value_t arg ) const;

        template<class T>
        void Print( T&& val ) const
        {
            std::cout << val;
        }

        void Input( const std::string& prompt, const std::string& name );

        value_t Inkey();

        void AddFakeInput( std::string str )
        {
            mFakeInput.push_back( std::move( str ) );
        }

        void AddData( int v )
        {
            AddDataImpl( value_t{ static_cast<int_t>(v) } );
        }

        void AddData( float v )
        {
            AddDataImpl( value_t{ float_t{v} } );
        }

        void AddData( str_t v )
        {
            AddDataImpl( value_t{ std::move( v ) } );
        }

        void Read( std::string name );

        void Restore();

        void Restore( linenum_t line );

        void Randomize( unsigned int n );

        void ClearProgram();
        
        void Clear();

        void Start();

        bool IsExpectedToContinueLineExecution() const
        {
            return mProgramCounter.lineOffset == ProgramCounter::ContinueExecution;
        }

        void PrintVars( std::ostream &os ) const;

    private:
        struct ProgramCounter
        {
            static constexpr unsigned LineOffsetBits = 16;
            static constexpr unsigned ContinueExecution = ~(~unsigned(0) << LineOffsetBits);

            linenum_t line: sizeof(linenum_t) * CHAR_BIT - LineOffsetBits;
            linenum_t lineOffset: LineOffsetBits;
        };

        static_assert( sizeof(ProgramCounter) == sizeof(linenum_t) );

        struct ForLoopItem
        {
            std::string varName;
            value_t targetVal;
            value_t stepVal;
            ProgramCounter startBodyPC;
        };

        struct FunctionInfo
        {
            std::string varName;
            std::string exprStr;
        };

        void GotoImpl( ProgramCounter pc )
        {
            mProgramCounter = pc;
        }

        bool NextImpl( std::string varName );
        void AddDataImpl( value_t value ); 

        static ValueType DetectVarType( std::string_view name );
        static value_t GetDefaultValue( std::string_view name );

    private:
        std::unordered_map<std::string, value_t> mVars;
        std::map<std::string, FunctionInfo, std::less<>> mFunctions;
        std::map<linenum_t, std::string> mProgram;
        std::unordered_map<linenum_t, size_t> mLineToDataPos;
        std::vector<ForLoopItem> mForLoopStack;
        std::vector<ProgramCounter> mGosubStack;
        std::deque<std::string> mFakeInput;
        ProgramCounter mProgramCounter = {};
        std::vector<value_t> mData;
        size_t mCurDataIdx = 0;
    };

    class FunctionRuntime
    {
    public:
        static value_t Calculate( const Runtime& rootRuntime, std::string_view exprStr, std::string_view varName, value_t varValue );
        value_t Load( std::string name ) const;
        value_t CallFuntion( std::string fncName, value_t arg ) const 
        { 
            return mRootRuntime.CallFuntion( std::move(fncName), std::move(arg) );
        }

        value_t Inkey()
        {
            return value_t{};
        }
        
        bool IsExpectedToContinueLineExecution() const
        {
            return true;
        }

    private:
        FunctionRuntime( const Runtime &rootRuntime, std::string_view varName, value_t varValue ) : 
            mRootRuntime{ rootRuntime }, mVarName{ varName }, mVarValue{ varValue } {}

    private:
        const Runtime& mRootRuntime;
        std::string_view mVarName;
        value_t mVarValue;
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

        void Start()
        {
            Runtime::Start();
            ClearOutput();
        }

        std::string GetOutput() const
        {
            return mStrOut.str();
        }

    private:
        void ClearOutput()
        {
            mStrOut.str( "" );
        }

        std::stringstream mStrOut;
    };

    template<class RangeT, class OutputFncT>
    void ListAllArrayElements( const RangeT & dimensions, OutputFncT && out )
    {
        std::vector<RangeT::value_type> curIndex;

        curIndex.reserve( curIndex.size() );
        curIndex.push_back( 0 );

        for(;;)
        {
            if( curIndex.back() > dimensions[curIndex.size() - 1] )
            {
                curIndex.pop_back();

                if( curIndex.empty() )
                    break;

                ++curIndex.back();
            }
            else if( curIndex.size() < dimensions.size() )
            {
                curIndex.push_back( 0 );
            }
            else
            {
                out( curIndex );
                ++curIndex.back();
            }
        }
    }
}



#endif // BASIC_INT_RUNTIME_H

