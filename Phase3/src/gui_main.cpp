// gui_main.cpp: starts the program, loads the data and opens the one main window.
//   communitybridge_gui.exe [datafolder] [--selftest | --selftest-flow | --selftest-dpichange | --screenshots <folder>] [--dpi <n>] [--multi] [--dpi-log]
// Exit codes: 0 fine, 1 start-up problem (normal mode), 2 start-up problem in a test mode, 3 a check failed.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>

#include "AppState.h"
#include "Exceptions.h"
#include "GuiMessage.h"
#include "GuiSelfTest.h"
#include "GuiShell.h"
#include "TextTools.h"
#include "Theme.h"

#include <stdlib.h>

#include <fstream>
#include <string>
#include <vector>

using namespace std;

// The one global pointer to the program state. It is set after the start-up worked.
AppState* theapp = NULL;

// The name of the mutex that lets only one copy of the program run.
static const char* const mutexname = "CommunityBridgeSingleInstance";

// Returns the folder of the exe with a backslash at the end.
static string exefolder()
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    string text = path;
    size_t slash = text.find_last_of("\\/");

    if (slash == string::npos)
    {
        return "";
    }

    return text.substr(0, slash + 1);
}

// Checks whether a folder exists.
static bool folderexists(const string& folder)
{
    DWORD attributes = GetFileAttributesA(folder.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

// Makes sure a folder name ends with a slash, because the file names are glued behind it.
// A stray quote at the end (from a path that ended with a backslash) is removed first.
static string withslash(const string& folder)
{
    string text = folder;

    while (!text.empty() && text[text.size() - 1] == '"')
    {
        text = text.substr(0, text.size() - 1);
    }

    if (!text.empty() && text[text.size() - 1] != '/' && text[text.size() - 1] != '\\')
    {
        text = text + "\\";
    }

    return text;
}

// The full path of a folder in small letters with backslashes, so two names for the same folder compare equal.
static string normalised(const string& folder)
{
    char full[MAX_PATH];
    DWORD length = GetFullPathNameA(folder.c_str(), MAX_PATH, full, NULL);
    string text = length > 0 && length < MAX_PATH ? string(full) : folder;

    for (size_t pos = 0; pos < text.size(); pos++)
    {
        if (text[pos] == '/')
        {
            text[pos] = '\\';
        }
        else if (text[pos] >= 'A' && text[pos] <= 'Z')
        {
            text[pos] = (char)(text[pos] - 'A' + 'a');
        }
    }

    if (!text.empty() && text[text.size() - 1] != '\\')
    {
        text = text + "\\";
    }

    return text;
}

// The flow test changes files, so it refuses the real data: the sample data of Phase 3, the data folder next to
// the exe, and anything inside Phase 2.
static bool protectedfolder(const string& folder)
{
    string path = normalised(folder);
    string beside = normalised(exefolder() + "data");
    string sample = normalised(exefolder() + "..\\data");

    if (path == beside || path == sample)
    {
        return true;
    }

    if (path.find("\\phase2\\") != string::npos)
    {
        return true;
    }

    size_t tail = string("\\phase3\\data\\").size();
    return path.size() >= tail && path.substr(path.size() - tail) == "\\phase3\\data\\";
}

// Reports a problem at start-up: a message box, or a line of the test log.
static int reportfailure(bool testing, const string& problem, const string& folder)
{
    if (testing)
    {
        testlog("error=" + problem);
        return 2;
    }

    guishowerror(NULL, "CommunityBridge could not start\n\n" + plaintextkeepfiles(problem) + "\n\nData folder\n" + folder);
    return 1;
}

// Cuts the command line into words. A word can be in double quotes (then it may hold spaces), and a quote
// always starts or ends the quoted part, also after a backslash, so "C:\data\" stays a folder name.
static vector<string> splitwords(const string& line)
{
    vector<string> words;
    string current = "";
    bool inquotes = false;
    bool started = false;

    for (size_t pos = 0; pos < line.size(); pos++)
    {
        char letter = line[pos];

        if (letter == '"')
        {
            inquotes = !inquotes;
            started = true;
        }
        else if ((letter == ' ' || letter == '\t') && !inquotes)
        {
            if (started)
            {
                words.push_back(current);
                current = "";
                started = false;
            }
        }
        else
        {
            current = current + letter;
            started = true;
        }
    }

    if (started)
    {
        words.push_back(current);
    }

    return words;
}

// A second copy of the program: the window of the first one comes to the front.
static void bringfirstforward()
{
    HWND first = FindWindowA("CommunityBridgeMain", NULL);

    if (first != NULL)
    {
        if (IsIconic(first))
        {
            ShowWindow(first, SW_RESTORE);
        }

        SetForegroundWindow(first);
    }
}

// The entry point of the program. commandline holds the words after the program name.
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR commandline, int showcommand)
{
    (void)previous;

    int kind = TEST_NONE;
    bool multi = false;
    bool dpilog = false;
    int testdpi = 0;
    string folder = "";
    string shots = "";
    vector<string> words = splitwords(commandline);

    for (size_t pos = 0; pos < words.size(); pos++)
    {
        if (words[pos] == "--selftest")
        {
            kind = TEST_LAYOUT;
        }
        else if (words[pos] == "--selftest-flow")
        {
            kind = TEST_FLOW;
        }
        else if (words[pos] == "--selftest-dpichange")
        {
            kind = TEST_DPICHANGE;
        }
        else if (words[pos] == "--screenshots")
        {
            kind = TEST_SCREENSHOTS;

            if (pos + 1 < words.size())
            {
                pos++;
                shots = words[pos];
            }
        }
        else if (words[pos] == "--dpi-log")
        {
            dpilog = true;
        }
        else if (words[pos] == "--multi")
        {
            multi = true;
        }
        else if (words[pos] == "--dpi")
        {
            // a test asks for another dpi (96, 120, 144, 168, 192 or 240); it is only used in a test mode
            if (pos + 1 < words.size())
            {
                pos++;
                testdpi = atoi(words[pos].c_str());
            }
        }
        else if (folder.empty())
        {
            folder = words[pos];
        }
    }

    bool testing = (kind != TEST_NONE);

    // one running copy only (not in a test mode and not with --multi)
    HANDLE mutex = NULL;

    if (!testing && !multi)
    {
        mutex = CreateMutexA(NULL, FALSE, mutexname);

        if (mutex != NULL && GetLastError() == ERROR_ALREADY_EXISTS)
        {
            bringfirstforward();
            CloseHandle(mutex);
            return 0;
        }
    }

    INITCOMMONCONTROLSEX controls;
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&controls);
    themeinit();

    if (testing)
    {
        testlogopen(exefolder() + "selftest.txt");

        // the dpi of a test (ignored in the normal mode)
        if (testdpi >= 96 && testdpi <= 480)
        {
            themesetdpioverride(testdpi);
        }
    }

    // the data folder: the one on the command line, else "data" next to the exe
    if (folder.empty())
    {
        folder = exefolder() + "data";
    }

    folder = withslash(folder);
    int code = 0;

    if (!folderexists(folder))
    {
        code = reportfailure(testing, "The data folder was not found.", folder);
    }
    else if (kind == TEST_FLOW && protectedfolder(folder))
    {
        testlog("error=The flow test changes files, so it needs a copy of the data and refuses this folder.");
        code = 2;
    }
    else
    {
        try
        {
            theapp = new AppState(folder);

            bool made = shellcreate(instance);

            // --dpi-log: one line now, and one for every WM_DPICHANGED that arrives
            if (made && dpilog)
            {
                shelldpilog(exefolder() + "dpi_log.txt");
            }

            if (!made)
            {
                code = reportfailure(testing, "The window could not be made.", folder);
            }
            else if (testing)
            {
                if (kind == TEST_SCREENSHOTS)
                {
                    CreateDirectoryA(shots.c_str(), NULL);
                }

                code = testrun(kind, shots);
                DestroyWindow(shellgetparts().main);
            }
            else
            {
                ShowWindow(shellgetparts().main, showcommand);
                UpdateWindow(shellgetparts().main);
                code = shellrun();
            }
        }
        catch (const AppException& problem)
        {
            code = reportfailure(testing, problem.what(), folder);
        }
        catch (const exception& problem)
        {
            code = reportfailure(testing, problem.what(), folder);
        }
    }

    shelldestroy();
    delete theapp;
    theapp = NULL;

    if (mutex != NULL)
    {
        CloseHandle(mutex);
    }

    return code;
}
