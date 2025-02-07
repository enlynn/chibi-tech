#ifndef _BOOTSTRAP_H_
#define _BOOTSTRAP_H_

#include "std/types.h"
#include <stdlib.h>

extern void client_main(int argc, char** p_argv);

#if CT_PLATFORM_WIN32
#include "std/win32/win32.h"
#include <shellapi.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
    // Get the Command Line Args and convert them to ansi
    //
    char** p_argv = NULL;
    int    argc   = 0;

    WCHAR** p_wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (p_wargv) {
        // Count the number of bytes necessary to store the UTF-8 versions of those strings
        int n = 0;
        for (int i = 0;  i < argc;  i++) {
            n += WideCharToMultiByte( CP_UTF8, 0, p_wargv[i], -1, NULL, 0, NULL, NULL ) + 1;
        }

        // Allocate the argv[] array + all the UTF-8 strings
        p_argv = malloc((argc + 1) * sizeof(char *) + n);
        if (p_argv) {
            // Convert all wargv[] --> argv[]
            char * arg = (char*)&(p_argv[argc + 1]);
            for (int i = 0;  i < argc;  i++)
            {
              p_argv[i] = arg;
              arg += WideCharToMultiByte(CP_UTF8, 0, p_wargv[i], -1, arg, n, NULL, NULL) + 1;
            }

            p_argv[argc] = NULL;
        }
    }

    // Call the client's main function
    //
    client_main(argc, p_argv);

    // Cleanup leftover resources
    //
    if (p_argv) {
        free(p_argv);
    }

    return 0;
}
#elif CT_PLATFORM_LINUX
int main(int argc, char** p_argv) {
    client_main(argc, p_argv);
    return 0;
}
#else
#  error Unsupported platform.
#endif

#endif //_BOOTSTRAP_H_
