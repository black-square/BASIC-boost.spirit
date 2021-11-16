//#define BOOST_SPIRIT_X3_DEBUG
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

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
    ///////////////////////////////////////////////////////////////////////////////
    //  The calculator grammar
    ///////////////////////////////////////////////////////////////////////////////
    namespace calculator_grammar
    {
        using x3::uint_;
        using x3::char_;

        x3::rule<class mult_div, int> const mult_div( "mult_div" );
        x3::rule<class exponent, int> const exponent( "exponent" );
        x3::rule<class term, int> const term( "term" );
        x3::rule<class add_sub, int> const add_sub( "add_sub" );
        x3::rule<class relational, int> const relational( "relational" );
        x3::rule<class log_and, int> const log_and( "log_and" );
        x3::rule<class log_or, int> const log_or( "log_or" );

        const auto cpy_op = []( auto& ctx ) { _val( ctx ) = _attr( ctx ); };
        const auto add_op = [](auto & ctx){ _val(ctx) += _attr(ctx); };
        const auto sub_op = []( auto& ctx ) { _val( ctx ) -= _attr( ctx ); };
        const auto neg_op = []( auto& ctx ) { _val( ctx ) = -_attr( ctx ); };      
        const auto mul_op = []( auto& ctx ) { _val( ctx ) *= _attr( ctx ); };
        const auto div_op = []( auto& ctx ) { _val( ctx ) /= _attr( ctx ); };
        const auto exp_op = []( auto& ctx ) { _val( ctx ) = (int)std::pow(_val( ctx ), _attr( ctx )); };

        const auto not_op =        []( auto& ctx ) { _val( ctx ) = _attr( ctx ) ? 0 : 1; };
        const auto eq_op =         []( auto& ctx ) { _val( ctx ) = _val( ctx ) == _attr( ctx ); };
        const auto not_eq_op =     []( auto& ctx ) { _val( ctx ) = _val( ctx ) != _attr( ctx ); };
        const auto less_op =       []( auto& ctx ) { _val( ctx ) = _val( ctx ) < _attr( ctx ); };
        const auto greater_op =    []( auto& ctx ) { _val( ctx ) = _val( ctx ) > _attr( ctx ); };
        const auto less_eq_op =    []( auto& ctx ) { _val( ctx ) = _val( ctx ) <= _attr( ctx ); };
        const auto greater_eq_op = []( auto& ctx ) { _val( ctx ) = _val( ctx ) >= _attr( ctx ); };

        const auto and_op =        []( auto& ctx ) { _val( ctx ) = _val( ctx ) && _attr( ctx ); };
        const auto or_op =         []( auto& ctx ) { _val( ctx ) = _val( ctx ) || _attr( ctx ); };

        auto const expression =
            log_or;

        auto const term_def =
            uint_[cpy_op]
             | '(' >> expression[cpy_op] >> ')'
             | ('-' >> term[neg_op])
             | ('+' >> term[cpy_op])
             | ("not" >> term[not_op])
            ;

        auto const exponent_def =
            term[cpy_op] >> *(
                ('^' >> term[exp_op])
            );

        auto const mult_div_def =
            exponent[cpy_op] >> *(
                ('*' >> exponent[mul_op]) |
                ('/' >> exponent[div_op])
             );

        auto const add_sub_def =
            mult_div[cpy_op] >> *(
                ('+' >> mult_div[add_op]) |
                ('-' >> mult_div[sub_op])
            );

        auto const relational_def =
            add_sub[cpy_op] >> *(
                ('=' >> add_sub[eq_op]) |
                ('<' >> char_('>') >> add_sub[not_eq_op]) |
                ('<' >> add_sub[less_op]) |
                ('>' >> add_sub[greater_op]) |
                ('<' >> char_('=') >> add_sub[less_eq_op]) |
                ('>' >> char_('=') >> add_sub[greater_eq_op])
            );

        auto const log_and_def =
            relational[cpy_op] >> *( 
                "and" >> relational[and_op]
            );

        auto const log_or_def =
            log_and[cpy_op] >> *(
                "or" >> log_and[or_op]
            );

        BOOST_SPIRIT_DEFINE( exponent, mult_div, term, add_sub, relational, log_and, log_or );

        const auto calculator = expression;
    }

    using calculator_grammar::calculator;

}

BOOST_AUTO_TEST_CASE( simple_calc )
{
    static const auto calc = []( std::string_view str )
    {
        auto& calc = client::calculator;    // Our grammar
        int res = 0;
        boost::spirit::x3::ascii::space_type space;

        auto iter = str.begin();
        const auto end = str.end();

        bool r = phrase_parse( iter, end, calc, space, res );

        if( !r || iter != end )
            return INT_MIN;

        return res;
    };

    BOOST_TEST( calc( "3+4" ) == 7 );
    BOOST_TEST( calc( "3+4*2" ) == 11 );
    BOOST_TEST( calc( "3+(4*2)" ) == 11 );
    BOOST_TEST( calc( "(3+4)*2" ) == 14 );
    BOOST_TEST( calc( "-4^3" ) == -64 );
    BOOST_TEST( calc( "1 + ( 2 - 3 )" ) == 0 );
    BOOST_TEST( calc( "2 * 4 ^ 2 - 34" ) == -2 );
    BOOST_TEST( calc( "-1 + 2 - 3 ^ 4 * 5 - 6" ) == -410 );
    BOOST_TEST( calc( "( ( 1 + ( 2 - 3 ) + 5 ) / 2 ) ^ 2" ) == 4 );
    BOOST_TEST( calc( "5 + ( ( 1 + 2 ) * 4 ) - 3" ) == 14 );
    BOOST_TEST( calc( "5 +  1 + 2  * 4 ^ 2 - 34" ) == 4 );
    BOOST_TEST( calc( "-(123-2+5*5)-23 *32*(22-4-6)" ) == -8978 );


    BOOST_TEST( calc( "1 + not 2^3 + 4" ) == 5 );
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


}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    boost::unit_test::unit_test_main( init_unit_test, argc, argv );
    
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
        int res = 0;

        iterator_type iter = str.begin();
        iterator_type end = str.end();
        boost::spirit::x3::ascii::space_type space;
        bool r = phrase_parse( iter, end, calc, space, res );

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
            std::cout << "Parsing failed\n";
            std::cout << "stopped at: \"" << rest << "\"\n";
            std::cout << "-------------------------\n";
        }
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}
