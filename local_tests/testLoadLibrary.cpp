#include <iostream>
#include "windows.h"

int main()
{
    std::cout << "Testing...\n";
    const char* dllPath1 = "C:\\WINDOWS\\system32\\ws2_32.dll";
    const char* dllPath2 = "C:\\work\\github\\fmu-rabbitmq\\local_tests\\unifmu.dll";
    std::cout << "Value of flag (load_with_altered_search_path): " << LOAD_WITH_ALTERED_SEARCH_PATH << "\n";

    std::cout << "Value of flag (search user dirs): " << LOAD_LIBRARY_SEARCH_USER_DIRS << "\n";
    /*
    Note that the standard search strategy and the alternate search strategy specified by LoadLibraryEx with LOAD_WITH_ALTERED_SEARCH_PATH
    differ in just one way: The standard search begins in the calling application's directory, and the alternate search begins in the directory
    of the executable module that LoadLibraryEx is loading.
    */
    HMODULE h = LoadLibraryEx(dllPath2, NULL,LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!h)
    {
        std::cout << "Library could not be loaded. Error code: " << GetLastError() << "\n";
    }
    else
    {
        std::cout << "Library loaded\n";
    }
    std::cout << "Testing finished.\n";
   
}