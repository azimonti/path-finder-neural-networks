/************************/
/*     main.cpp         */
/*    Version 1.0       */
/*     2022/06/24       */
/*  © Davide Pasca      */
/*  © Marco Azimonti    */
/************************/

#include "gl_app.h"
#include "log/log.h"
#include <png.h>

#ifdef WIN32
static void showWinConsole();
#endif

int main(int, char**)
{
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png) abort();
#ifdef WIN32
    showWinConsole();
#endif
    bool file_log = false;
    LOGGER_PARAM(logging::LEVELMAX, logging::INFO);
    LOGGER_PARAM(logging::LOGLINE, true);
    LOGGER_PARAM(logging::LOGTIME, true);
    if (file_log)
    {
        LOGGER_PARAM(logging::FILENAME, "out_pathfinder.log");
        LOGGER_PARAM(logging::FILEOUT, true);
    }
    GLApp app;
    app.onInit();
    app.mainLoop();
    return 0;
}

#ifdef WIN32
# undef APIENTRY
# include <Windows.h>
static void showWinConsole()
{
    ShowWindow( GetConsoleWindow(), SW_SHOW );
}
#endif
