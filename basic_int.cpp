//#define BOOST_SPIRIT_X3_DEBUG
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

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
        float_t,
        int_t,
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
            void operator()( const str_t& v ) const { os << '"' << v << '"'; }

            std::ostream& os;
        };
        
        boost::apply_visitor(Visitor{os}, v);

        return os;
    }

    using linenum_t = unsigned long long;

    struct runtime_tag;

    class Runtime
    {
    public:
        void AddLine( linenum_t line, std::string_view str )
        {
            std::cout << line << "\t\t" << str << std::endl;
        }
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
    }

    namespace grammar
    {
        using namespace operations;

        using x3::int_;
        constexpr x3::real_parser<float, x3::strict_real_policies<float>> strict_float = {};
        using x3::char_;
        using x3::lit;
        using x3::no_case;
        
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


        const auto expression =
            log_or;

        const auto identifier_def = x3::raw[ x3::lexeme[(x3::alpha |  '_') >> *(x3::alnum | '_' )]];

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
                ('=' >> add_sub[eq_op]) |
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

        BOOST_SPIRIT_DEFINE( exponent, mult_div, term, add_sub, relational, log_and, log_or, double_args, identifier );

        const auto calculator = expression;
    }

    namespace preparse_operations
    {
        constexpr auto add_num_line = []( auto& ctx ) 
        {
            auto& runtime = x3::get<runtime_tag>( ctx ).get();
            const auto [line, str] = _attr( ctx );

            runtime.AddLine( line, str );  
        };
    }

    namespace preparse_grammar
    {
        using namespace preparse_operations;

        using x3::char_;
        using x3::ulong_long;

        x3::rule<class line> const line( "line" );
        x3::rule<class num_line, std::tuple<linenum_t, std::string>> const num_line( "num_line" );

        const auto preparser = line;

        const auto num_line_def = 
            x3::ulong_long >> x3::lexeme[+char_];

        const auto line_def =
            num_line[add_num_line]
            ;

        BOOST_SPIRIT_DEFINE( line, num_line );
    }

    using grammar::calculator;
    using preparse_grammar::preparser;
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

BOOST_AUTO_TEST_CASE( simple_calc )
{
    using namespace client;

    static const auto calc = []( std::string_view str )
    {
        auto& calc = calculator;    // Our grammar
        value_t res{};
        boost::spirit::x3::ascii::space_type space;

        auto iter = str.begin();
        const auto end = str.end();

        bool r = false;
        std::string err{}; 

        try
        {
            r = phrase_parse( iter, end, calc, space, res );
        }
        catch( const std::runtime_error& e )
        {
            err = e.what();
        }

        if( !r || iter != end )
            return value_t{"ERROR: " + err};

        return res;
    };


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


void InteractiveMode()
{
    using namespace client;

    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Expression parser...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Type an expression...or [q or Q] to quit\n\n";

    typedef std::string::const_iterator iterator_type;


    std::string str;
    while( std::getline( std::cin, str ) )
    {
        if( str.empty() || str[0] == 'q' || str[0] == 'Q' )
            break;

        auto& calc = client::calculator;    // Our grammar
        value_t res{};

        iterator_type iter = str.begin();
        iterator_type end = str.end();
        boost::spirit::x3::ascii::space_type space;

        bool r = false;
        std::string err{};

        try
        {
            r = phrase_parse( iter, end, calc, space, res );
        }
        catch( const std::runtime_error& e )
        {
            err = e.what();
        }

        if( r && iter == end )
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            std::cout << "\nResult: " << res << std::endl;
            std::cout << "-------------------------\n";
        }
        else
        {
            std::string rest( iter, end );
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed " << err << "\n";
            std::cout << "stopped at: \"" << rest << "\"\n";
            std::cout << "-------------------------\n";
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    boost::unit_test::unit_test_main( init_unit_test, 1, argv );

    if( argc != 2 )
    {
        InteractiveMode();
        return 0;
    }
    
    using boost::spirit::x3::with;

    std::ifstream flIn{ argv[1] };

    std::string str;
    
    client::Runtime runtime;

    auto && parser = with<client::runtime_tag>( std::ref( runtime ) )[ 
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

            break;
        }
    }

    return 0;
}
