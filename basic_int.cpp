#include "grammar.h"
#include "runtime.h"
#include "parse_utils.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <fstream>

void InteractiveMode()
{                                            
    std::cout << "-------------------------\n";
    std::cout << "Interactive mode\n";
    std::cout << "-------------------------\n";

    runtime::TestExecutor calc{ main_pass::line_rule() };

    std::string str;

    for(;;)
    {
        std::cout << ">>> ";

        if( !std::getline( std::cin, str ) || str.empty() )
            break;

        const auto res = calc(str);

        std::cout << res << std::endl;
    }
}

bool Preparse( const char* szFileName, runtime::Runtime &runtime )
{
    std::ifstream flIn{ szFileName };

    std::string str;

    while( std::getline( flIn, str ) )
    {
        boost::spirit::x3::unused_type res{};
        std::string err{};

        if( !runtime::Parse( str, preparse::line_rule(), runtime, res, err ) )
        {
            std::cout << "-------------------------\n";
            std::cout << "Preparse failed\n" << str << "\n";
            std::cout << "Error: \"" << err << "\"\n";
            std::cout << "-------------------------\n";

            return false;
        }
    }

    return true;
}

bool Execute( runtime::Runtime& runtime )
{
    for(;;) 
    {
        const auto [pStr, lineNum] = runtime.GetNextLine();

        if( !pStr )
            return true;

        runtime::value_t res{};
        std::string err{};

        if( !runtime::Parse( *pStr, main_pass::line_rule(), runtime, res, err ) )
        {
            std::cout << "-------------------------\n";
            std::cout << "Execute failed\n" << lineNum << '\t' << *pStr << "\n";
            std::cout << "Error: " << err << "\n";
            std::cout << "-------------------------\n";
            return false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    extern void RunTests( const char* szPath );

    RunTests(argv[0]);

    runtime::Runtime runtime;

    for( int i = 1; i < argc; ++i )
    {
        if( boost::algorithm::ends_with( argv[i], ".input" ) )
        {
            std::cout << "-------------------------\n";
            std::cout << "Read input: " << argv[i] << std::endl;
            std::cout << "-------------------------\n";

            std::ifstream flIn( argv[i] );
            std::string str;

            while( std::getline( flIn, str ) )
                runtime.AddFakeInput( std::move(str) );

            continue;
        }

        std::cout << "-------------------------\n";
        std::cout << "Running: " << argv[i] << std::endl;
        std::cout << "-------------------------\n";

        bool res = Preparse( argv[i], runtime );

        if( res )
            res = Execute( runtime );

        std::cout << std::endl << (res ? "[SUCCESS]": "[FAIL]")  << std::endl << std::endl;

        runtime.Clear();
    }

    InteractiveMode();

    return 0;
}
