#ifndef GUISHELL_H
#define GUISHELL_H

// GuiShell.h: the one main window. It holds the sidebar, the page host (one page at a time) and the status bar.
// No page is ever a window of its own.

#include <windows.h>

#include "GuiPage.h"

#include <string>

using namespace std;

// Who is signed in.
enum accountkind
{
    WHO_NONE,
    WHO_COORDINATOR,
    WHO_DONOR,
    WHO_RECIPIENT
};

// The windows a test needs to look at.
struct shellparts
{
    HWND main;       // the main window
    HWND host;       // the page host
    HWND sidebar;    // the sidebar panel (holds the program name, the list and the user box)
    HWND appname;    // the program name in the sidebar
    HWND nav;        // the sidebar list
    HWND status;     // the status bar
};

// Makes the main window and every page (nothing is shown yet). Returns false when a window could not be made.
bool shellcreate(HINSTANCE instance);

// The windows of the shell.
const shellparts& shellgetparts();

// Shows a page inside the window. takefocus puts the keyboard focus on the first control of the page.
void shellshow(int pageid, bool takefocus);

// The number of the page that is shown.
int shellcurrent();

// The number of the page that was shown before the present one (PAGE_NONE when there is none).
int shellprevious();

// Shows the page that was shown before (or the Home page of the role when there is none).
void shellback();

// Returns a page by number, or NULL when it does not exist.
guipage* shellpage(int pageid);

// Signs a person in: fills the sidebar and shows the Home page of the role. For the coordinator the id and name are empty.
void shellsignin(int who, const string& personid, const string& personname);

// Signs out and shows the welcome page.
void shellswitchuser();

// Who is signed in, and the id and name of the person.
int shellwho();
const string& shellpersonid();
const string& shellpersonname();

// Shows a plain line in the status bar (an empty text clears it).
void shellstatus(const string& text);

// The text that the status bar shows now.
string shellstatustext();

// Tells whether the sidebar is shown (it is hidden on the welcome page).
bool shellsidebarshown();

// Takes a new dpi: fonts, list rows, sizes and the page that is shown follow it. suggested is the window rectangle that
// WM_DPICHANGED gives (the window is moved and sized to it); a test passes NULL.
void shellapplydpi(int dpi, const RECT* suggested);

// Starts the dpi log (--dpi-log): one line now, and one line for every WM_DPICHANGED that really arrives.
void shelldpilog(const string& path);

// The dpi awareness of the main window: -4 per-monitor v2, -3 per-monitor, -2 system, -1 unaware, 0 not known.
int shellawareness();

// The source line of the WM_DPICHANGED case of the real window procedure once it handled a message (0 before).
int shelldpihandlerline();

// A test may make the window bigger than the screen (the biggest size at a high dpi).
void shellallowhuge(bool allow);

// Gives the window this client size (used by the tests and the screenshots).
void shellsetclient(int width, int height);

// Returns the size of the client area.
void shellgetclient(int& width, int& height);

// Returns the number of items in the sidebar and the page number of an item.
int shellnavcount();
int shellnavpage(int index);

// Returns the number of the selected sidebar item, or -1.
int shellnavselected();

// Runs the message loop until the window is closed. Returns the exit code.
int shellrun();

// Frees the pages (call after the loop).
void shelldestroy();

#endif
