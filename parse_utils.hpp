#ifndef BASIC_INT_PARSE_UTIL_H
#define BASIC_INT_PARSE_UTIL_H

#include "runtime.h"
#include "grammar.h"

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>

namespace runtime
{
    struct ParseArgs
    {
        const std::string_view str{};
        std::string_view::iterator cur{};
        const std::string_view::iterator end{};
        ParseMode parseMode{};
        boost::spirit::x3::ascii::space_type spaceParser{};

        ParseArgs( std::string_view str, unsigned offset ):
            str{ str }, cur{ str.begin() + offset }, end{ str.end()}
        {}

        template<class RuntimeT, class ParserT>
        auto MakeFullParser( RuntimeT && runtime, ParserT && parser )
        {
            using boost::spirit::x3::with;

            return
                with<runtime_tag>( std::ref( runtime ) )[
                    with<line_begin_tag>( str.begin() )[
                        with<parse_mode_tag>( std::ref( parseMode ) )[
                            parser
                        ]
                    ]
                ];
        }
    };


    template <class ParseFnc>
    bool ParseSingle( std::string_view str, unsigned offset, std::string& err, ParseFnc && fnc )
    {
        using runtime::ParseMode;

        bool r = false;
        err.clear();

        ParseArgs args{ str, offset };

        try
        {
            r = fnc( args );
        }
        catch( const std::runtime_error& e )
        {
            err = e.what();
        }

        if( !r || args.cur != args.end )
        {
            err = (!r ? "ERROR[" : "UNEXPECTED[") + err + "]";

            if( args.cur != args.end )
            {
                err += " \"";
                err.append( args.str.begin(), args.cur );
                err += "><";
                err.append( args.cur, args.end );
                err += "\"";
            }

            return false;
        }

        return true;
    }

    template< class ParserT, class RuntimeT, class ResultT>
    bool ParseSequence( std::string_view str, unsigned offset, ParserT&& parser, RuntimeT& runtime, ResultT& result, std::string& err )
    {
        const auto parseFnc = [&runtime, &parser, &result]( auto &args )
        {
            SkipStatementRuntime skipRuntime;

            auto sequence_separator{ main_pass::sequence_separator_rule() };
            auto&& statementParser = args.MakeFullParser( runtime, parser );
            auto&& statementSkipper = args.MakeFullParser( skipRuntime, main_pass::statement_rule() );
            auto&& elseStatementParser = args.MakeFullParser( runtime, main_pass::else_statement_rule() );
            auto&& elseStatementSkipper = args.MakeFullParser( skipRuntime, main_pass::else_statement_rule() );

            enum class State
            {
                ParseStatement,
                SkipStatement,
                ParseElse,
                SkipElse,
                SkipHeadSeparator,
                SkipTailSeparator
            };

            using TStateQueue = std::deque<State>;

            static constexpr auto normalParseModeToStates = []( ParseMode mode, TStateQueue&stateQueue )
            {
                switch( mode )
                {
                case ParseMode::Normal:
                    //Nothing
                    break;

                case ParseMode::ParseStatementSkipElse:
                    stateQueue = { State::ParseStatement, State::SkipElse };
                    break;

                case ParseMode::SkipStatementParseElse:
                    stateQueue = { State::SkipStatement, State::ParseElse };
                    break;

                case ParseMode::ParseElse:
                    stateQueue = { State::ParseElse };
                    break;

                case ParseMode::SkipElse:
                    assert(false); //Cannot be reached in the normal mode
                    [[fallthrough]];

                default:
                    throw std::runtime_error( "Unexpected mode" );
                }
            };

            static constexpr auto skippingParseModeToStates = []( ParseMode mode, TStateQueue& stateQueue )
            {
                switch( mode )
                {
                case ParseMode::Normal:
                    //Nothing
                    break;

                case ParseMode::SkipStatementParseElse:
                    [[fallthrough]];
                case ParseMode::ParseStatementSkipElse:
                    stateQueue = { State::SkipStatement, State::SkipElse };
                    break;

                case ParseMode::SkipElse:
                    [[fallthrough]];
                case ParseMode::ParseElse:
                    stateQueue = { State::SkipElse };
                    break;

                default:
                    throw std::runtime_error( "Unexpected mode" );
                }
            };

            TStateQueue stateQueue{ State::SkipHeadSeparator, State::ParseStatement, State::SkipTailSeparator };

            for(;;)
            {
                if( stateQueue.empty() )
                    throw std::runtime_error("State machine logic is broken");

                State curState = stateQueue.front();
                stateQueue.pop_front();

                args.parseMode = ParseMode::Normal;
               
                switch( curState )
                {
                    case State::ParseStatement:
                        if( !phrase_parse( args.cur, args.end, statementParser, args.spaceParser, result ) )
                            return false;

                        if( !runtime.IsExpectedToContinueLineExecution() )
                        {
                            args.cur = args.end;
                            return true;
                        }

                        normalParseModeToStates( args.parseMode, stateQueue );
                        break;

                    case State::SkipStatement:
                        if( !phrase_parse(args.cur, args.end, statementSkipper, args.spaceParser) )
                            return false;

                        skippingParseModeToStates( args.parseMode, stateQueue );
                        break;
                     
                    case State::ParseElse:                        
                        if( !phrase_parse( args.cur, args.end, elseStatementParser, args.spaceParser ) )
                        {
                            // We have a false condition of IF and no ELSE, it means we need to skip the rest 
                            // of the line: "If several statements occur after the THEN, separated by colons, 
                            // then they will be executed if and only if the expression is true."
                            runtime.GotoNextLine();
                            args.cur = args.end;
                            return true;
                        }

                        //Else can have an embedded goto
                        if( !runtime.IsExpectedToContinueLineExecution() )
                        {
                            args.cur = args.end;
                            return true;
                        }
                        
                        stateQueue = { State::ParseStatement, State::SkipTailSeparator };
                        break;

                    case State::SkipElse:
                        //ELSE could be optional
                        if( phrase_parse( args.cur, args.end, elseStatementSkipper, args.spaceParser ) )
                            stateQueue = { State::SkipStatement, State::SkipTailSeparator };
                        else
                            stateQueue = { State::SkipTailSeparator };

                        break;

                    case State::SkipHeadSeparator:
                        phrase_parse( args.cur, args.end, sequence_separator, args.spaceParser );
                        break;

                    case State::SkipTailSeparator:
                        if( phrase_parse(args.cur, args.end, sequence_separator, args.spaceParser) )
                        {
                            assert( stateQueue.empty() );
                            stateQueue = {State::ParseStatement, State::SkipTailSeparator };
                        }
                        else
                        {
                            //The separator is not found and it means the sequence is ended
                            return true;
                        }

                        break;
                }
            }

            assert(false);
        };

        return runtime::ParseSingle( str, offset, err, parseFnc );
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

                if( !ParseSequence( *pStr, offset, grammar, runtime, res, err ) )
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


