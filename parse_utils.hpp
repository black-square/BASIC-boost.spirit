#ifndef BASIC_INT_PARSE_UTIL_H
#define BASIC_INT_PARSE_UTIL_H

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>

#include "runtime.h"

namespace runtime
{
    template< class ParserT, class RuntimeT, class ResultT>
    bool Parse( std::string_view str, unsigned offset, ParserT&& parser, RuntimeT& runtime, ResultT& result, std::string& err )
    {
        using boost::spirit::x3::with;
        using boost::spirit::x3::ascii::space_type;
        using runtime::runtime_tag;

        auto&& exec = with<runtime_tag>( std::ref( runtime ) )[
            with<line_begin_tag>(str.begin()) [
                parser
            ]
        ];

        space_type space;

        bool r = false;
        auto cur = str.begin() + offset;
        const auto end = str.end();
        err.clear();

        try
        {
            for( ;; )
            {
                r = phrase_parse( cur, end, exec, space, result );

                if( !r || cur == end )
                    break;

                if( !runtime.IsExpectedToContinueLineExecution() )
                {
                    cur = end;
                    break;
                }
            }
        }
        catch( const std::runtime_error& e )
        {
            err = e.what();
        }

        if( !r || cur != end )
        {
            err = (!r ? "ERROR[" : "UNEXPECTED[") + err + "]";

            if( cur != end )
            {
                err += " \"";
                err.append( str.begin(), cur );
                err += "><";
                err.append( cur, end );
                err += "\"";
            }

            return false;
        }

        return true;
    }

    template<class RangeT, class OutputFncT>
    void ListAllArrayElements( const RangeT& dimensions, OutputFncT&& out )
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

    template<class GrammarT>
    struct TestExecutor
    {
        TestExecutor( GrammarT&& grammar ) :
            grammar{ grammar }
        {}

        auto operator()( std::string_view str )
        {
            using runtime::value_t;

            value_t res{};
            std::string err{};
 
            runtime.ClearProgram();
            runtime.AddLine( 100, str );
            runtime.Start();

            for( ;; )
            {
                const auto [pStr, lineNum, offset] = runtime.GetNextLine();

                if( !pStr )
                    break;

                if( !Parse( *pStr, offset, grammar, runtime, res, err ) )
                    return value_t{ std::move( err ) };
            }

            auto&& strOut = runtime.GetOutput();

            return strOut.empty() || res != value_t{} ? res : value_t{ strOut };
        }

        const GrammarT grammar;
        TestRuntime runtime;
    };

    template<class GrammarT>
    struct TestExecutorClear : TestExecutor<GrammarT>
    {
        using TBase = TestExecutor<GrammarT>;
        using TBase::TestExecutor;
        using TBase::runtime;

        auto operator()( std::string_view str )
        {
            runtime.Clear();
            return TBase::operator()( str );
        }
    };
}


#endif // BASIC_INT_PARSE_UTIL_H


