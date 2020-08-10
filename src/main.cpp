///////////////////////////////////////
// main.cpp

#include "stdafx.h"
#include "ContainerApp.h"
#include "Sound.h"
#include "Engine.h"
#include "Text.h"
#include "Utility.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Dwrite.lib")
#pragma comment (lib, "Windowscodecs.lib")

bool load_lexer()
{
    const auto lexer_lib = LoadLibrary(L"scilexer.DLL");
    if (lexer_lib == nullptr)
    {
        MessageBox(nullptr,
            L"The Scintilla SciLexer DLL could not be loaded.",
            L"Error loading Scintilla",
            MB_OK | MB_ICONERROR);
        exit(1);
    }
    return false;
}

void init_audio();
int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    load_lexer();
    init_audio();
    Engine::Start();

    try
    {
        return Utility::get_app()->Run();
    }
    catch (const CException &e)
    {
        MessageBox(NULL, e.GetText(), AtoT(e.what()), MB_ICONERROR);
        return -1;
    }
}
