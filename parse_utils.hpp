#ifndef BASIC_INT_PARSE_UTIL_H
#define BASIC_INT_PARSE_UTIL_H

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>

#include "runtime.h"

namespace runtime
{
    template< class ParserT, class RuntimeT, class ResultT>
    bool Parse( std::string_view str, ParserT&& parser, RuntimeT& runtime, ResultT& result, std::string& err )
    {
        using boost::spirit::x3::with;
        using boost::spirit::x3::ascii::space_type;
        using runtime::PartialParseAction;
        using runtime::runtime_tag;
        using runtime::partial_parse_action_tag;

        PartialParseAction parseAction{};

        auto&& exec = with<runtime_tag>( std::ref( runtime ) )[
            with<partial_parse_action_tag>( std::ref( parseAction ) )[
                parser
            ]
        ];

        space_type space;

        bool r = false;
        auto cur = str.begin();
        const auto end = str.end();
        err.clear();

        try
        {
            for( ;; )
            {
                parseAction = PartialParseAction::Undefined;
                r = phrase_parse( cur, end, exec, space, result );

                if( !r || cur == end )
                    break;

                if( parseAction != PartialParseAction::Continue )
                {
                    if( parseAction == PartialParseAction::Discard )
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

    template<class GrammarT>
    struct TestExecutor
    {
        TestExecutor( GrammarT&& grammar ) :
            grammar{ grammar }
        {}

        auto operator()( std::string_view str )
        {
            using runtime::value_t;

            runtime.ClearOutput();

            value_t res{};
            std::string err{};

            if( !Parse( str, grammar, runtime, res, err ) )
            {
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


