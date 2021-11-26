#include "platform.h"

#ifdef _WIN32
#include <Windows.h>
#include <conio.h>
#endif

void EnableConsoleColors()
{
    //List of codes: https://stackoverflow.com/a/33206814/3415353
#ifdef _WIN32
    const HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
    DWORD dwMode = 0;
    GetConsoleMode( hConsole, &dwMode );
    SetConsoleMode( hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING );
#endif
}

int GetPressedKbKey()
{
#ifdef _WIN32
    return _kbhit();
#else
    return 0;
#endif
}