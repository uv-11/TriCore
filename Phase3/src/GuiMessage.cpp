// GuiMessage.cpp: message boxes (real windows in normal mode, log lines in a test mode).

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>

#include "GuiMessage.h"
#include "GuiSelfTest.h"

using namespace std;

// The program name is the title of every box.
static const char* const boxtitle = "CommunityBridge";

// A test mode only writes the text to the log.
void guishowerror(HWND parent, const string& text)
{
    if (testmode())
    {
        testlog("message=" + text);
        return;
    }

    MessageBoxA(parent, text.c_str(), boxtitle, MB_OK | MB_ICONERROR);
}

// A test mode logs the question and answers Yes.
bool guiaskyes(HWND parent, const string& text)
{
    if (testmode())
    {
        testlog("question=" + text);
        return true;
    }

    return MessageBoxA(parent, text.c_str(), boxtitle, MB_YESNO | MB_ICONQUESTION) == IDYES;
}
