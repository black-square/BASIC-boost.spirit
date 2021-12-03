#define BOOST_TEST_MODULE BasicInt
#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/included/unit_test.hpp>

#include "parse_utils.hpp"
#include "grammar.h"

namespace runtime // Enables ADL for these methods for BOOST_TEST
{
    inline bool operator==( const value_t& v1, int v2 )
    {
        return v1 == value_t{ static_cast<int_t>(v2) };
    }

    inline bool operator==( const value_t& v1, float v2 )
    {
        return v1 == value_t{ v2 };
    }

    inline bool operator==( const value_t& v1, const char* v2 )
    {
        return v1 == value_t{ v2 };
    }
}

BOOST_AUTO_TEST_CASE( expression_test )
{
    runtime::TestExecutorClear calc{ main_pass::expression_rule() };

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

    BOOST_TEST( calc( R"(("a " + "b")+" c")" ) == "a b c" );

    BOOST_TEST( calc( "SQR(156.25)" ) == 12.5f );
    BOOST_TEST( calc( "ABS(12.34)" ) == 12.34f );
    BOOST_TEST( calc( "-12.34" ) == -12.34f );
    BOOST_TEST( calc( "ABS(-12.34)" ) == 12.34f );
    BOOST_TEST( calc( R"(left$("applesoft", 5))" ) == "apple" );
    BOOST_TEST( calc( R"(rnd(5) >= 0 and rnd(5) < 5)" ) == 1 );
}

BOOST_AUTO_TEST_CASE( line_parser_test )
{
    runtime::TestExecutorClear calc{ main_pass::statement_seq_rule() };

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
    BOOST_TEST( calc( R"(print tab(3);"a";)" ) == "   a" );

    BOOST_TEST( calc( R"(x$ = "test":print x$)" ) == "test\n" );
    BOOST_TEST( calc( R"(x% = 12:print x%)" ) == "12\n" );
    BOOST_TEST( calc( R"(x% = 2.5:print x%)" ) == "2\n" );

    BOOST_TEST( calc( R"(a(2+1)=42 : print a(3);)" ) == "42" );
    BOOST_TEST( calc( R"(n = 3: a(3, 8)=42*2 : print a(n, n*2+2);)" ) == "84" );
    BOOST_TEST( calc( R"(n = 3: a(2, n*2)=43 : b( a(2,6), 1,2,3) = 400/4 : print b(43, 1,   2, 3);)" ) == "100" );

    BOOST_TEST( calc( R"(if 1=1 then print "OK")" ) == "OK\n" );
    BOOST_TEST( calc( R"(if 0=1 then print "OK")" ) == 0 );
    BOOST_TEST( calc( R"(if "" + "" then print "OK")" ) == 0 );
    BOOST_TEST( calc( R"(if "" + "" + "a" then print "OK")" ) == "OK\n" );
    BOOST_TEST( calc( R"(x=23 : if x==23 then print "OK")" ) == "OK\n" );
    BOOST_TEST( calc( R"(if 2 then if 3 then x$ = "OK": print x$;)" ) == "OK" );
    BOOST_TEST( calc( R"(if 2 then if 2 - 1 * 2 then x$ = "OK": print x$;)" ) == 0 );
    BOOST_TEST( calc( R"(if 2 then if 2 - 1 * 3 then x$ = "OK": print x$;)" ) == "OK" );

    // "If several statements occur after the THEN, separated by colons, 
    //  then they will be executed if and only if the expression is true."
    BOOST_TEST( calc( R"(if 0 then print "false":print "next")" ) == 0 );
    BOOST_TEST( calc( R"(DIM A(10, 10): ro = 3: A(3, 10) = 50: IF A(RO,10)<>0 THEN print "test")" ) == "test\n" );
    
    BOOST_TEST( calc( R"(DEF FN A(w) = 2 * W + W: PRINT FN A(23);)" ) == "69" );
    BOOST_TEST( calc( R"(DEF FNB(X) = 4 + 3: G = FNB(23): PRINT G;)" ) == "7" );
    BOOST_TEST( calc( R"(DEF FNB(X) = 4 + 3: DEF FNA(Y) = FNB(1000) + Y: PRINT FNA(100);)" ) == "107" );
    BOOST_TEST( calc( R"(DEF FNB(X) = X * X: DEF FNA(Y) = FNB(Y) * 3: PRINT FNA(10);)" ) == "300" );

    BOOST_TEST( calc( R"(print "before": for i=1 to 3: print "body": next:print "after")" ) == "before\nbody\nbody\nbody\nafter\n" );
}

BOOST_AUTO_TEST_CASE( ListAllArrayElements )
{
    static constexpr auto calc = []( std::vector<runtime::int_t> dimensions )
    {
        int num = 0;
        runtime::ListAllArrayElements( dimensions, [&num, &dimensions](const auto & indices )
        { 
            BOOST_TEST( indices.size() == dimensions.size() );

            for( int i = 0; i != indices.size(); ++i )
            {
                BOOST_TEST( indices[i] >= 0 );
                BOOST_TEST( indices[i] <= dimensions[i] );
            }
                
            ++num; 
        });

        return num;
    };

    BOOST_TEST( calc({2}) == 3 );
    BOOST_TEST( calc({2, 3}) == 12 );
    BOOST_TEST( calc({2, 3, 4}) == 60 );
    BOOST_TEST( calc({2, 3, 4, 5}) == 360 );
    BOOST_TEST( calc({0, 0, 0}) == 1 );
    BOOST_TEST( calc({2, 0, 4}) == 15 );
}

void RunTests( const char *szPath)
{
    const char* rgBootTestArgs[] = { szPath, "--log_level=test_suite" };
    boost::unit_test::unit_test_main( init_unit_test, std::size( rgBootTestArgs ), const_cast<char**>(rgBootTestArgs) );
}
