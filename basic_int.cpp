#include "grammar.h"
#include "runtime.h"
#include "parse_utils.hpp"
#include "platform.h"

#include <boost/algorithm/string/predicate.hpp>
#include <fstream>


void InteractiveMode()
{                                            
    std::cout << "\033[96m" "-------------------------\n";
    std::cout << "Interactive mode\n";
    std::cout << "-------------------------\n" "\033[0m";

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

        if( !runtime::Parse( str, 0, preparse::line_rule(), runtime, res, err ) )
        {
            std::cerr << "\033[91m" "-------------------------\n";
            std::cerr << "Preparse failed\n" << str << "\n";
            std::cerr << "Error: \"" << err << "\"\n";
            std::cerr << "-------------------------\n" "\033[0m";

            return false;
        }
    }

    return true;
}

bool Execute( runtime::Runtime& runtime )
{
    runtime.Start();

    for(;;) 
    {
        const auto [pStr, lineNum, offset] = runtime.GetNextLine();

        if( !pStr )
            return true;

        runtime::value_t res{};
        std::string err{};

        if( !runtime::Parse( *pStr, offset, main_pass::line_rule(), runtime, res, err ) )
        {
            std::cerr << "\033[91m" "-------------------------\n";
            std::cerr << "Execute failed\n" << lineNum << '\t' << *pStr << "\n";
            std::cerr << "Error: " << err << "\n";
            std::cerr << "-------------------------\n" "\033[0m";
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

    EnableConsoleColors();
    
    std::cout << "\033[97m" "----------------------------------------------------\n" 
                            "                    BASIC_INT\n" 
                            "     basically, a very basic BASIC interpreter\n"
                            "               Dmitrii Shesterkin 2021\n" 
                            "----------------------------------------------------\n" "\033[0m";

    if( argc <= 1 )
    {
        std::cout << "\nBASIC_INT [FILE [...]]\n\n";
        std::cout << "  FILE\tBASIC program file. if the file name ends on \".input\"\n"
                         "  \tits content is used as fake input for the next program\n";
    }

    runtime::Runtime runtime;

    for( int i = 1; i < argc; ++i )
    {
        if( boost::algorithm::ends_with( argv[i], ".input" ) )
        {
            std::cout << "\033[96m" "-------------------------\n";
            std::cout << "Read input: " << argv[i] << std::endl;
            std::cout << "-------------------------\n" "\033[0m";

            std::ifstream flIn( argv[i] );
            std::string str;

            while( std::getline( flIn, str ) )
                runtime.AddFakeInput( std::move(str) );

            continue;
        }

        std::cout << "\033[96m" "-------------------------\n";
        std::cout << "Running: " << argv[i] << std::endl;
        std::cout << "-------------------------\n" "\033[0m";

        bool res = Preparse( argv[i], runtime );

        if( res )
            res = Execute( runtime );

        std::cout << std::endl << (res ? "\033[92m" "[SUCCESS]" "\033[0m" : "\033[91m" "[FAILURE]" "\033[0m")  << std::endl << std::endl;

        runtime.Clear();
    }

    InteractiveMode();

    return 0;
}
