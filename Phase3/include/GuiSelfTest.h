#ifndef GUISELFTEST_H
#define GUISELFTEST_H

// GuiSelfTest.h: the test modes of the program (--selftest, --selftest-flow, --screenshots) and their log.
// A test mode drives the real window, measures it and writes selftest.txt next to the exe.

#include <string>

using namespace std;

// The test modes.
enum testkind
{
    TEST_NONE,
    TEST_LAYOUT,       // --selftest
    TEST_FLOW,         // --selftest-flow
    TEST_SCREENSHOTS,  // --screenshots <folder>
    TEST_DPICHANGE     // --selftest-dpichange (real WM_DPICHANGED messages, no dpi override)
};

// Starts the log (the file is emptied). From now on testmode() is true.
void testlogopen(const string& path);

// Tells whether the program runs in a test mode.
bool testmode();

// Adds one line to the log.
void testlog(const string& line);

// Runs the test mode on the window that was already made and returns the exit code of the program:
// 0 everything fine, 3 one or more checks failed. folder is the screenshot folder (screenshots only).
int testrun(int kind, const string& folder);

#endif
