//#define BOOST_SPIRIT_X3_DEBUG
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

#include <boost/algorithm/string/replace.hpp>

#include <iostream>
#include <string>
#include <list>
#include <numeric>

#define BOOST_TEST_MODULE BasicInt
#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/included/unit_test.hpp>


namespace x3 = boost::spirit::x3;


namespace client
{
    using float_t = float;
    using int_t = int16_t;
    using str_t = std::string;

    struct value_t : x3::variant<
        int_t,
        float_t,
        str_t
    >
    {
        using base_type::base_type;
        using base_type::operator=;

        friend bool operator==( const value_t& v1, const value_t& v2 )
        {
            return v1.var == v2.var;
        }

        friend bool operator!=( const value_t& v1, const value_t& v2 )
        {
            return !(v1 == v2);
        }
    };

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

    using linenum_t = unsigned long long;

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
        void Assign( const std::string& name, const value_t& val )
        {
            mVars[name] = val;
        }

        value_t Read( const std::string& name )
        {
            return mVars[name];
        }

        void Clear()
        {
            mVars.clear();
        }

    private:
        std::unordered_map<std::string, value_t> mVars;
    };

    class Runtime: public RuntimeBase
    {
    public:
        void AddLine( linenum_t line, std::string_view str )
        {
            mProgram.emplace(line, str);
        }

        std::string *GetNextLine()
        {
            const auto it = mProgram.lower_bound(mCurLine);

            if( it == mProgram.end() )
                return nullptr;

            std::cout << "<" << it->first << "\t\t" << it->second << ">" << std::endl;

            mCurLine = it->first + 1;

            return &it->second;
        }

        template<class T>
        void Print( T && val ) const
        {
            std::cout << val;
        }

    private:
        std::map<linenum_t, std::string> mProgram;
        linenum_t mCurLine = 0;
    };

    namespace operations
    {
        using boost::fusion::at_c;

        // All arithmetic operations are done in floating point.No matter what
        // the operands to + , -, *, / , and^ are, they will be converted to floating
        // point.The functions SIN, COS, ATN, TAN, SQR, LOG, EXPand RND also
        // convert their arguments to floating point and give the result as such.
        float_t ForseFloat( const value_t& v )
        {
            struct Impl
            {
                float_t operator()( float_t v ) const { return v; }
                float_t operator()( int_t v ) const { return static_cast<float_t>(v); }
                float_t operator()( const str_t& v ) const { throw std::runtime_error( "Cannot be string" ); }
            };

            return boost::apply_visitor( Impl{}, v );
        }

        // The operators AND, OR, NOT force both operands to be integers between
        // -32767 and +32767 before the operation occurs.
        // When a number is converted to an integer, it is truncated (rounded down).
        // It will perform as if INT function was applied.No automatic conversion is 
        // done between strings and numbers
        int_t ForseInt( const value_t& v )
        {
            struct Impl
            {
                int_t operator()( float_t v ) const { return static_cast<int_t>(v); }
                int_t operator()( int_t v ) const { return v; }
                int_t operator()( const str_t& v ) const { throw std::runtime_error("Cannot be string"); }
            };

            return boost::apply_visitor( Impl{}, v );
        }

        constexpr auto cpy_op = []( auto& ctx )
        { 
            _val( ctx ) = _attr( ctx ); 
        };

        constexpr auto cpy_int_op = []( auto& ctx )
        { 
            _val( ctx ) = static_cast<int_t>( _attr( ctx) ); 
        };

        constexpr auto eq_op = []( auto& ctx )
        {
            auto&& op1 = _val( ctx );
            auto&& op2 = _attr( ctx );

            const str_t* pS1 = boost::get<str_t>( &op1 );
            const str_t* pS2 = boost::get<str_t>( &op2 );
            
            _val( ctx ) = int_t{ pS1 && pS2 ?
                *pS1 == *pS2 :
                ForseFloat( op1 ) == ForseFloat( op2 )
            };
        };

        constexpr auto not_eq_op = []( auto& ctx )
        {
            auto&& op1 = _val( ctx );
            auto&& op2 = _attr( ctx );

            const str_t* pS1 = boost::get<str_t>( &op1 );
            const str_t* pS2 = boost::get<str_t>( &op2 );

            _val( ctx ) = int_t{ pS1 && pS2 ?
                *pS1 != *pS2 :
                ForseFloat( op1 ) != ForseFloat( op2 )
            };
        };

        constexpr auto add_op = []( auto & ctx )
        { 
            auto && op1 = _val( ctx );
            auto && op2 = _attr( ctx );

            const str_t *pS1 = boost::get<str_t>(&op1);
            const str_t *pS2 = boost::get<str_t>(&op2);

            //Strings may also be concatenated (put or joined together) through
            //the use of the "+" operator.
            if( pS1 && pS2 )
                _val(ctx) = *pS1 + *pS2;
            else
                _val(ctx) = ForseFloat(op1) + ForseFloat(op2);
        };

        constexpr auto sub_op = []( auto& ctx )
        { 
            _val(ctx) = ForseFloat(_val( ctx )) - ForseFloat(_attr(ctx)); 
        };

        constexpr auto mul_op = []( auto& ctx )
        {
            _val( ctx ) = ForseFloat(_val( ctx)) * ForseFloat(_attr(ctx));
        };
                
        constexpr auto div_op = []( auto& ctx )
        {
            _val( ctx ) = ForseFloat(_val( ctx)) / ForseFloat(_attr(ctx));
        };

        constexpr auto exp_op = []( auto& ctx )
        { 
            _val( ctx ) = std::pow( ForseFloat(_val(ctx)), ForseFloat(_attr(ctx)) );
        };

        constexpr auto not_op = []( auto& ctx )
        { 
            _val( ctx ) = int_t{ !ForseInt(_attr(ctx)) };
        };

        constexpr auto neg_op = []( auto& ctx )
        { 
            _val( ctx ) = -ForseFloat(_attr( ctx )); };

        const auto less_op = []( auto& ctx )
        { 
            _val( ctx ) = int_t{ ForseFloat(_val( ctx )) < ForseFloat(_attr( ctx )) };
        };

        constexpr auto greater_op = []( auto& ctx )
        { 
            _val( ctx ) = int_t{ ForseFloat(_val( ctx )) > ForseFloat(_attr( ctx )) };
        };
        
        constexpr auto less_eq_op = []( auto& ctx )
        {
            _val( ctx ) = int_t{ ForseFloat(_val( ctx )) <= ForseFloat(_attr( ctx )) };
        };

        constexpr auto greater_eq_op = []( auto& ctx )
        { 
            _val( ctx ) = int_t{ ForseFloat(_val( ctx )) >= ForseFloat(_attr( ctx) ) };
        };

        constexpr auto and_op = []( auto& ctx )
        { 
            _val( ctx ) = int_t{ ForseInt(_val( ctx )) && ForseInt(_attr( ctx )) }; 
        };

        constexpr auto or_op = []( auto& ctx )
        { 
            _val( ctx ) = int_t{ ForseInt(_val( ctx )) || ForseInt(_attr( ctx )) };
        };

        constexpr auto test_op = []( auto& ctx ) {
           const auto[o1, o2] = _attr( ctx );
            _val( ctx ) = ForseFloat(o1) + ForseFloat(o2);
        };

        constexpr auto array_acc_op = []( auto& ctx ) {
            const auto &v = _attr( ctx );
            const auto &s = at_c<0>( v );
            const auto [o1, o2] = at_c<1>(v);
            std::cout << s <<'[' << o1 << ',' << o2 << ']' << std::endl;
            _val( ctx ) = ForseFloat(o1) * ForseFloat(o2);
        };

        constexpr auto print_op = []( auto& ctx ) 
        {
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            boost::apply_visitor( [&runtime](auto&& v) { runtime.Print(v); }, _attr( ctx ) );
        };

        constexpr auto assing_var_op = []( auto& ctx ) {
            auto&& v = _attr( ctx );
            auto&& name = at_c<0>( v );
            auto&& value = at_c<1>( v );
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            runtime.Assign( name, value );
        };

        constexpr auto read_var_op = []( auto& ctx ) {
            auto&& name = _attr( ctx );
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            _val( ctx ) = runtime.Read( name );
        };  
        
        constexpr auto if_statement_op = []( auto& ctx ) {
            auto&& v = _attr( ctx );

            struct Impl
            {
                bool operator()( float_t v ) const { return v != 0; }
                bool operator()( int_t v ) const { return v != 0; }
                bool operator()( const str_t& v ) const { return !v.empty(); }
            };

            const bool res = boost::apply_visitor( Impl{}, v );
            auto& action = x3::get<partial_parse_action_tag>( ctx ).get();

            action = res ? PartialParseAction::Continue : PartialParseAction::Discard;
        };
    }

    namespace grammar
    {
        using namespace operations;

        using x3::int_;
        constexpr x3::real_parser<float, x3::strict_real_policies<float>> strict_float = {};
        using x3::char_;
        using x3::lit;
        using x3::no_case;
        using x3::attr;
        using x3::eoi;
        
        using str_view = boost::iterator_range<std::string_view::const_iterator>;

        x3::rule<class mult_div, value_t> const mult_div( "mult_div" );
        x3::rule<class exponent, value_t> const exponent( "exponent" );
        x3::rule<class term, value_t> const term( "term" );
        x3::rule<class add_sub, value_t> const add_sub( "add_sub" );
        x3::rule<class relational, value_t> const relational( "relational" );
        x3::rule<class log_and, value_t> const log_and( "log_and" );
        x3::rule<class log_or, value_t> const log_or( "log_or" );
        x3::rule<class double_args, std::tuple<value_t, value_t>> const double_args( "double_args" );
        x3::rule<class identifier, std::string> const identifier( "identifier" );
        x3::rule<class instruction, value_t> const instruction( "instruction" );
        x3::rule<class line_parser, value_t> const line_parser( "line_parser" );

        const auto expression =
            log_or;

        const auto line_parser_def =
            instruction % ':';

        const auto instruction_end = 
            ':' | eoi;

        const auto print_comma = 
            ',' >> attr( value_t{ "\t" } )[print_op];

        const auto print_arg = 
            +(+print_comma | expression[print_op]);

        const auto print_instruction =
            (no_case["print"] >> print_arg >> *(';' >> print_arg) >> (
                ';' | (&instruction_end >> attr( value_t{ "\n" } )[print_op])       
            )) |
            no_case["print"] >> attr( value_t{ "\n" } )[print_op];

        const auto instruction_def =
            no_case["text"] |
            no_case["home"] |            
            print_instruction |
            (no_case["if"] >> expression >> no_case["then"] )[if_statement_op] |
            (-no_case["let"] >> identifier >> '=' >> expression )[assing_var_op]
        ;

        const auto identifier_def = x3::raw[ x3::lexeme[(x3::alpha |  '_') >> *(x3::alnum | '_' )] ];

        const auto double_args_def =
            '(' >> expression >> ',' >> expression >> ')';
            
        const auto term_def =
            strict_float[cpy_op]      
             | int_[cpy_int_op]
             | x3::lexeme['"' >> *~char_('"') >> '"'][cpy_op]    
             | '(' >> expression[cpy_op] >> ')'
             | ('-' >> term[neg_op])
             | ('+' >> term[cpy_op])
             | (no_case["not"] >> term[not_op])
             | (no_case["sum"] >> double_args[test_op])
             | identifier[read_var_op]
             | (identifier >> double_args)[array_acc_op]
            ;

        const auto exponent_def =
            term[cpy_op] >> *(
                ('^' >> term[exp_op])
            );

        const auto mult_div_def =
            exponent[cpy_op] >> *(
                ('*' >> exponent[mul_op]) |
                ('/' >> exponent[div_op])
             );

        const auto add_sub_def =
            mult_div[cpy_op] >> *(
                ('+' >> mult_div[add_op]) |
                ('-' >> mult_div[sub_op])
            );

        const auto relational_def =
            add_sub[cpy_op] >> *(
                ((lit("==") | '=') >> add_sub[eq_op]) |
                ('<' >> lit('>') >> add_sub[not_eq_op]) |
                ('<' >> add_sub[less_op]) |
                ('>' >> add_sub[greater_op]) |
                ('<' >> lit('=') >> add_sub[less_eq_op]) |
                ('>' >> lit('=') >> add_sub[greater_eq_op])
            );

        const auto log_and_def =
            relational[cpy_op] >> *( 
                no_case["and"] >> relational[and_op]
            );

        const auto log_or_def =
            log_and[cpy_op] >> *(
                no_case["or"] >> log_and[or_op]
            );

        BOOST_SPIRIT_DEFINE( exponent, mult_div, term, add_sub, relational, log_and, log_or, 
            double_args, identifier, instruction, line_parser
        );

        const auto calculator = expression;
    }

    namespace preparse
    {
        constexpr auto add_num_line = []( auto& ctx )
        {
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            const auto [line, str] = _attr( ctx );

            runtime.AddLine( line, str );
        };

        using x3::char_;
        using x3::ulong_long;
        using x3::eoi;

        x3::rule<class line> const line( "line" );
        x3::rule<class num_line, std::tuple<linenum_t, std::string>> const num_line( "num_line" );

        const auto preparser = line;

        const auto num_line_def =
            x3::ulong_long >> x3::lexeme[+char_];

        const auto line_def =  
            num_line[add_num_line] |
            eoi
            ;

        BOOST_SPIRIT_DEFINE( line, num_line );
    }

    using grammar::calculator;
    using preparse::preparser;
    using grammar::line_parser;
}

namespace client // Enables ADL for these methods for BOOST_TEST
{
    inline bool operator==( const value_t& v1, int v2 )
    {
        return v1 == value_t{ static_cast<int_t>(v2) };
    } 
    
    inline bool operator==( const value_t& v1, float v2 )
    {
        return v1 == value_t{ v2 };
    }

    inline bool operator==( const value_t& v1, const char *v2 )
    {
        return v1 == value_t{ v2 };
    }
}

class TestRuntime : public client::RuntimeBase
{
public:
    template<class T>
    void Print( T&& val )
    {
        mStrOut << val;
    }

    void Clear()
    {
        RuntimeBase::Clear();
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

template< class ParserT, class RuntimeT, class ResultT>
bool Parse( std::string_view str, ParserT &parser, RuntimeT &runtime, ResultT &result, std::string &err )
{
    using boost::spirit::x3::with;
    using namespace client;

    PartialParseAction parseAction{};

    auto&& exec = with<runtime_tag>( std::ref( runtime ) )[
        with<partial_parse_action_tag>( std::ref( parseAction ) ) [
            parser
       ]
    ];

    boost::spirit::x3::ascii::space_type space;

    bool r = false;
    auto cur = str.begin();
    const auto end = str.end();
    err.clear();

    try
    {
        for( ;; ) 
        {
            parseAction = client::PartialParseAction::Undefined;
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
            err += " pos> ";
            err.append( cur, end );
        }

        return false;
    }

    return true;
}

template<class GrammarT>
struct TestExecutor
{
    TestExecutor(GrammarT &grammar):
        grammar{grammar}
    {}

    auto operator()( std::string_view str )
    {
        using client::value_t;

        runtime.ClearOutput();

        value_t res{};
        std::string err{};

        if( !Parse( str, grammar, runtime, res, err ) )
        {
            return value_t{ std::move(err) };
        }

        auto && strOut = runtime.GetOutput();

        return strOut.empty() || res != value_t{} ? res : value_t{ strOut };
    }

    const GrammarT &grammar;
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

BOOST_AUTO_TEST_CASE( expression_test )
{
    TestExecutorClear calc{client::calculator};

    BOOST_TEST( calc( "3+4" ) == 7.f );
    BOOST_TEST( calc( "3+4*2" ) == 11.f );
    BOOST_TEST( calc( "3+(4*2)" ) == 11.f );
    BOOST_TEST( calc( "(3+4)*2" ) == 14.f );
    BOOST_TEST( calc( "-4^3" ) == -64.f );
    BOOST_TEST( calc( "4.5^3" ) == 91.125f );
    BOOST_TEST( calc( "1 + ( 2 - 3 )" ) == 0.f );
    BOOST_TEST( calc( "2 * 4 ^ 2 - 34" ) == -2.f );
    BOOST_TEST( calc( "-1 + 2 - 3 ^ 4 * 5 - 6" ) == -410.f );
    BOOST_TEST( calc( "( ( 1 + ( 2 - 3 ) + 5 ) / 2 ) ^ 2" ) == 6.25f );
    BOOST_TEST( calc( "5 + ( ( 1 + 2 ) * 4 ) - 3" ) == 14.f );
    BOOST_TEST( calc( "5 +  1 + 2  * 4 ^ 2 - 34" ) == 4.f );
    BOOST_TEST( calc( "-(123-2+5*5)-23 *32*(22-4-6)" ) == -8978.f );


    BOOST_TEST( calc( "1 + not 2^3 + 4" ) == 5.f );
    BOOST_TEST( calc( "2 + 3 = 3 - 1 * -2" ) == 1 );
    BOOST_TEST( calc( "2 + (1+1+1) == 3 - 1 * -2" ) == 1 );
    BOOST_TEST( calc( "1 < 2 and 2 < 10" ) == 1 );
    BOOST_TEST( calc( "3+(4*2) + 3 = (3+4)*2" ) == 1 );
    BOOST_TEST( calc( "1 <= 1" ) == 1 );
    BOOST_TEST( calc( "1 <= 2" ) == 1 );
    BOOST_TEST( calc( "1 < = 1" ) == 1 );
    BOOST_TEST( calc( "1 < = 2" ) == 1 );
    BOOST_TEST( calc( "1 <> 1" ) == 0 );
    BOOST_TEST( calc( "1 >= 2" ) == 0 );
    BOOST_TEST( calc( "1 > = 1" ) == 1 );
    BOOST_TEST( calc( "1 > = 2" ) == 0 );

    BOOST_TEST( calc( R"(("a " + "b")+" c")") == "a b c" );
}

BOOST_AUTO_TEST_CASE( line_parser_test )
{
    TestExecutorClear calc{ client::line_parser };

    BOOST_TEST( calc( R"(print)" ) == "\n" );
    BOOST_TEST( calc( R"(Print "Hello World!")" ) == "Hello World!\n" );
    BOOST_TEST( calc( R"(Print "Hello World!" ; )" ) == "Hello World!" );
    BOOST_TEST( calc( R"(print 1+1;)" ) == "2" );
    BOOST_TEST( calc( R"(print "Test": print 42 ;)" ) == "Test\n42" );
    BOOST_TEST( calc( R"(xvar3 = 37 : print "X: "; 2 + xvar3 + 3)" ) == "X: 42\n" );
    BOOST_TEST( calc( R"(xvar3 = 37 : print "X: "; : print 2 + xvar3 + 3)" ) == "X: 42\n" );
    BOOST_TEST( calc( R"(print "t", 2+45 ;:print "!!!";)" ) == "t\t47!!!" );
    BOOST_TEST( calc( R"(print ,"t",,;)" ) == "\tt\t\t" );
    BOOST_TEST( calc( R"(print ,,,"t",, 2+45,""; "ab" + "cd";)" ) == "\t\t\tt\t\t47\tabcd" );
    

    BOOST_TEST( calc( R"(if 1=1 then print "OK")" ) == "OK\n" );
    BOOST_TEST( calc( R"(if 0=1 then print "OK")" ) == 0 );
    BOOST_TEST( calc( R"(if "" + "" then print "OK")" ) == 0 );
    BOOST_TEST( calc( R"(if "" + "" + "a" then print "OK")" ) == "OK\n" );
    BOOST_TEST( calc( R"(x=23 : if x==23 then print "OK")" ) == "OK\n" );
    BOOST_TEST( calc( R"(if 2 then if 3 then x = "OK": print x;)" ) == "OK" );
    BOOST_TEST( calc( R"(if 2 then if 2 - 1 * 2 then x = "OK": print x;)" ) == 0 );
    BOOST_TEST( calc( R"(if 2 then if 2 - 1 * 3 then x = "OK": print x;)" ) == "OK" );
}

void InteractiveMode()
{
    using namespace client;
    using boost::spirit::x3::with;

    std::cout << "Type an expression...or [q or Q] to quit\n\n";

    TestExecutor calc{ client::line_parser };

    std::string str;

    for(;;)
    {
        std::cout << ">>> ";

        if( !std::getline( std::cin, str ) || str.empty() || str[0] == 'q' || str[0] == 'Q' )
            break;

        const auto res = calc(str);

        std::cout << res << std::endl;
    }
}

bool Preparse( const char* szFileName, client::Runtime &runtime )
{
    using boost::spirit::x3::with;

    std::ifstream flIn{ szFileName };

    std::string str;

    auto&& parser = with<client::runtime_tag>( std::ref( runtime ) )[
        client::preparser
    ];

    while( std::getline( flIn, str ) )
    {
        auto iter = str.begin();
        auto end = str.end();
        boost::spirit::x3::ascii::space_type space;

        bool r = false;
        std::string err{};

        try
        {
            r = phrase_parse( iter, end, parser, space );
        }
        catch( const std::runtime_error& e )
        {
            err = e.what();
        }

        if( !r || iter != end )
        {
            std::string rest( iter, end );
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed " << err << "\n";
            std::cout << "stopped at: \"" << rest << "\"\n";
            std::cout << "-------------------------\n";

            return false;
        }
    }

    return true;
}

bool Execute( client::Runtime& runtime )
{
    using namespace client;

    while( std::string *pStr = runtime.GetNextLine() )
    {
        value_t res{};
        std::string err{};

        if( !Parse( *pStr, line_parser, runtime, res, err ) )
        {
            std::cout << "!!! " << err << std::endl;
            return false;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    const char *rgBootTestArgs[] = { argv[0], "--log_level=test_suite" };
    
    boost::unit_test::unit_test_main( init_unit_test, std::size(rgBootTestArgs), const_cast<char**>(rgBootTestArgs) );

    for( int i = 1; i < argc; ++i )
    {
        std::cout << "Running: " << argv[i] << std::endl;

        client::Runtime runtime;

        bool res = Preparse( argv[i], runtime );

        if( res )
            res = Execute( runtime );

        std::cout << (res ? "[SUCCESS]": "[FAIL]")  << std::endl;
    }

    InteractiveMode();

    return 0;
}
