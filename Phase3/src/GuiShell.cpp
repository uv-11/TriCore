// GuiShell.cpp: the main window with its sidebar, page host and status bar.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>

#include "GuiShell.h"
#include "GuiLayout.h"
#include "Pages.h"
#include "Theme.h"
#include "resource.h"

#include <fstream>
#include <vector>

using namespace std;

// The old Windows headers do not know the message that says the dpi of the window changed.
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

// The old Windows headers do not know the keyboard cue flags of WM_CHANGEUISTATE.
#ifndef UIS_SET
#define UIS_SET 1
#endif
#ifndef UISF_HIDEFOCUS
#define UISF_HIDEFOCUS 0x1
#endif

// One item of the sidebar: the page it opens and its text.
struct navitem
{
    int page;
    const char* text;
};

// The sidebar of the coordinator.
static const navitem coordnav[] = {{PAGE_C_HOME, "Home"},
                                   {PAGE_C_DONATIONS, "Donations"},
                                   {PAGE_C_OFFERS, "Offers"},
                                   {PAGE_C_RECIPIENTS, "Recipients"},
                                   {PAGE_C_TRANSACTIONS, "Transactions"},
                                   {PAGE_C_SUMMARY, "Summary"},
                                   {PAGE_NONE, NULL}};

// The sidebar of a donor.
static const navitem donornav[] = {{PAGE_D_HOME, "Home"},
                                   {PAGE_D_DONATIONS, "My donations"},
                                   {PAGE_D_OFFERS, "Offers"},
                                   {PAGE_D_GIVEN, "Given out"},
                                   {PAGE_D_ADD, "Add donation"},
                                   {PAGE_NONE, NULL}};

// The sidebar of a recipient.
static const navitem recipnav[] = {{PAGE_R_HOME, "Home"},
                                   {PAGE_R_INCOMING, "Incoming offers"},
                                   {PAGE_R_PAST, "Past offers"},
                                   {PAGE_R_RECEIVED, "Received food"},
                                   {PAGE_R_EDIT, "My details"},
                                   {PAGE_NONE, NULL}};

// The width in pixels of the edge line a themed list view draws at the end of its column (measured: 2 at every dpi).
static const int themeedgeline = 2;

// The class names of the main window, the page host and the sidebar panel.
static const char* const mainclass = "CommunityBridgeMain";
static const char* const hostclass = "CommunityBridgeHost";
static const char* const sideclass = "CommunityBridgeSidebar";

// Everything the shell keeps in memory.
struct shellstate
{
    shellparts parts;            // the windows
    HWND whocaption;             // "Signed in as"
    HWND whorole;                // the role of the person
    HWND whoname;                // the name of the person
    HWND switchuser;             // the Switch user button
    vector<guipage*> pages;      // every page, made once
    int current;                 // the page that is shown
    int previous;                // the page that was shown before
    int who;                     // who is signed in
    string personid;             // id of the person
    string personname;           // name of the person
    bool sidebar;                // whether the sidebar is shown
    bool syncing;                // true while the program itself changes the selected sidebar item
    string statustext;           // the text in the status bar
    HWND lastfocus;              // the control that had the focus when the window was deactivated
    bool allowhuge;              // true in a test: the window may be bigger than the screen
    string dpilog;               // the dpi log file (--dpi-log), empty when there is none
    int dpiline;                 // the source line of the WM_DPICHANGED case once it ran
};

static shellstate state;

// Makes every page once. They are only shown and hidden after this.
static void buildpages(HWND host)
{
    state.pages.push_back(newpagewelcome());
    state.pages.push_back(newpagedonorhome());
    state.pages.push_back(newpagedonordonations());
    state.pages.push_back(newpagedonoroffers());
    state.pages.push_back(newpagedonorgiven());
    state.pages.push_back(newpagedonoradd());
    state.pages.push_back(newpagecoordhome());
    state.pages.push_back(newpagecoorddonations());
    state.pages.push_back(newpagecoordoffers());
    state.pages.push_back(newpagecoordrecipients());
    state.pages.push_back(newpagecoordtransactions());
    state.pages.push_back(newpagecoordsummary());
    state.pages.push_back(newpagematch());
    state.pages.push_back(newpagerecipienthome());
    state.pages.push_back(newpagerecipientincoming());
    state.pages.push_back(newpagerecipientpast());
    state.pages.push_back(newpagerecipientreceived());
    state.pages.push_back(newpagerecipientedit());

    for (size_t pos = 0; pos < state.pages.size(); pos++)
    {
        state.pages[pos]->build(host);
    }
}

// Finds a page by number.
guipage* shellpage(int pageid)
{
    for (size_t pos = 0; pos < state.pages.size(); pos++)
    {
        if (state.pages[pos]->id() == pageid)
        {
            return state.pages[pos];
        }
    }

    return NULL;
}

// The windows of the shell.
const shellparts& shellgetparts()
{
    return state.parts;
}

// The page that is shown.
int shellcurrent()
{
    return state.current;
}

// The page before the present one.
int shellprevious()
{
    return state.previous;
}

// Who is signed in.
int shellwho()
{
    return state.who;
}

// The id of the person.
const string& shellpersonid()
{
    return state.personid;
}

// The name of the person.
const string& shellpersonname()
{
    return state.personname;
}

// The sidebar is only shown for a signed-in person.
bool shellsidebarshown()
{
    return state.sidebar;
}

// Returns the sidebar items of a role.
static const navitem* navof(int who)
{
    if (who == WHO_COORDINATOR)
    {
        return coordnav;
    }

    if (who == WHO_DONOR)
    {
        return donornav;
    }

    if (who == WHO_RECIPIENT)
    {
        return recipnav;
    }

    return NULL;
}

// Number of items in the sidebar list.
int shellnavcount()
{
    return (int)SendMessageA(state.parts.nav, LVM_GETITEMCOUNT, 0, 0);
}

// The page that an item of the sidebar opens (it is the data of the item).
int shellnavpage(int index)
{
    LVITEMA item;
    ZeroMemory(&item, sizeof(item));
    item.mask = LVIF_PARAM;
    item.iItem = index;

    if (!SendMessageA(state.parts.nav, LVM_GETITEMA, 0, (LPARAM)&item))
    {
        return PAGE_NONE;
    }

    return (int)item.lParam;
}

// The selected item of the sidebar.
int shellnavselected()
{
    return (int)SendMessageA(state.parts.nav, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
}

// Writes the line in the status bar.
void shellstatus(const string& text)
{
    state.statustext = text;

    // the part is drawn by the program (WM_DRAWITEM in mainproc) on the sidebar tone, with room at the left edge
    SendMessageA(state.parts.status, SB_SETTEXTA, 0 | SBT_OWNERDRAW | SBT_NOBORDERS, (LPARAM)1);
    InvalidateRect(state.parts.status, NULL, TRUE);
}

// The status line as it is now.
string shellstatustext()
{
    return state.statustext;
}

// Returns the size of the client area.
void shellgetclient(int& width, int& height)
{
    RECT area;
    GetClientRect(state.parts.main, &area);
    width = area.right - area.left;
    height = area.bottom - area.top;
}

// Puts every control of the main window in place: the sidebar, the host and the status bar in one batch,
// then the page that is shown inside the host.
static void layoutmain(int width, int height)
{
    int statusheight = px(24);
    int sidewidth = state.sidebar ? px(sidebarwidth) : 0;
    int hostwidth = width - sidewidth;
    int hostheight = height - statusheight;
    guilayout layout;

    layout.place(state.parts.status, 0, height - statusheight, width, statusheight);
    layout.place(state.parts.host, sidewidth, 0, hostwidth, hostheight);
    layout.place(state.parts.sidebar, 0, 0, sidewidth, hostheight);
    layout.apply();

    // the controls of the sidebar are children of the panel, so their places are relative to it
    if (state.sidebar)
    {
        guilayout inside;
        int gap = px(16);
        int inner = sidewidth - 2 * gap;
        int buttonheight = px(32);
        int bottom = hostheight - gap;
        int buttontop = bottom - buttonheight;
        int nametop = buttontop - px(12) - px(60);
        int roletop = nametop - px(22);
        int captiontop = roletop - px(20);
        int navtop = px(72);
        int navheight = captiontop - gap - navtop;

        if (navheight < px(40))
        {
            navheight = px(40);
        }

        inside.place(state.parts.appname, gap, px(22), inner, px(32));
        inside.place(state.parts.nav, 0, navtop, sidewidth, navheight);
        inside.place(state.whocaption, gap, captiontop, inner, px(20));
        inside.place(state.whorole, gap, roletop, inner, px(22));
        inside.place(state.whoname, gap, nametop, inner, px(60));
        inside.place(state.switchuser, gap, buttontop, inner, buttonheight);
        inside.apply();

        // the one column is exactly as wide as the list, so the selection spans the whole sidebar (an automatic width
        // would fit the longest item on a list without a header)
        RECT navclient;
        GetClientRect(state.parts.nav, &navclient);
        SendMessageA(state.parts.nav, LVM_SETCOLUMNWIDTH, 0, MAKELPARAM(navclient.right, 0));

        // a themed list draws a line of 2 pixels (at every dpi) at the end of its column; a rectangular window region leaves
        // those two pixel columns out, so the sidebar colour shows there and the list keeps the full sidebar width
        SetWindowRgn(state.parts.nav, CreateRectRgn(0, 0, sidewidth - themeedgeline, navheight), TRUE);
    }

    guipage* page = shellpage(state.current);

    if (page != NULL)
    {
        SetWindowPos(page->window(), NULL, 0, 0, hostwidth, hostheight, SWP_NOZORDER | SWP_NOACTIVATE);
        page->onsize(hostwidth, hostheight);
    }
}

// Lays the window out for its present client size.
static void layoutnow()
{
    int width = 0;
    int height = 0;
    shellgetclient(width, height);
    layoutmain(width, height);
}

// The Windows 10 function that adds the frame of a window for a given dpi (loaded by name, the old headers lack it).
typedef BOOL(WINAPI* adjustfordpi)(LPRECT, DWORD, BOOL, DWORD, UINT);

// Grows a client rectangle to the rectangle of the whole window. It uses the frame of the window's own dpi when
// Windows can tell it; a test that asks for another dpi than the screen has gets the frame of the screen.
static void frameforwindow(HWND window, RECT& frame)
{
    DWORD style = (DWORD)GetWindowLongA(window, GWL_STYLE);
    DWORD exstyle = (DWORD)GetWindowLongA(window, GWL_EXSTYLE);
    HMODULE user = GetModuleHandleA("user32.dll");
    adjustfordpi function = user == NULL ? NULL : (adjustfordpi)GetProcAddress(user, "AdjustWindowRectExForDpi");

    if (function != NULL && !themedpiisoverridden())
    {
        function(&frame, style, FALSE, exstyle, (UINT)themedpiforwindow(window));
    }
    else
    {
        AdjustWindowRectEx(&frame, style, FALSE, exstyle);
    }
}

// Takes a new dpi: the fonts are made again and given to every control, every list gets rows of the new height, the
// window is moved and sized to the rectangle Windows suggested (a test passes none) and the page that is shown is
// filled and laid out again. The other pages are laid out when they are shown.
void shellapplydpi(int dpi, const RECT* suggested)
{
    themesetdpi(dpi);
    themeapplyfonts(state.parts.main);
    themereleasefonts();
    themerestylelists(state.parts.main);

    if (suggested != NULL)
    {
        SetWindowPos(state.parts.main, NULL, suggested->left, suggested->top, suggested->right - suggested->left,
                     suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    layoutnow();
    guipage* page = shellpage(state.current);

    if (page != NULL)
    {
        page->onshow();
        layoutnow();
    }

    RedrawWindow(state.parts.main, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

// Appends one line with the time to the dpi log (only with --dpi-log).
static void dpilogline(const string& text)
{
    if (state.dpilog.empty())
    {
        return;
    }

    SYSTEMTIME now;
    GetLocalTime(&now);
    ofstream out(state.dpilog.c_str(), ios::app);
    out << numtext(now.wHour) << ":" << numtext(now.wMinute) << ":" << numtext(now.wSecond) << " " << text << endl;
}

// Writes a rectangle as left,top,right,bottom.
static string recttext(const RECT& box)
{
    return numtext(box.left) + "," + numtext(box.top) + "," + numtext(box.right) + "," + numtext(box.bottom);
}

// The Windows 10 functions that tell the dpi awareness of a window (loaded by name, the old headers lack them).
typedef HANDLE(WINAPI* contextofwindow)(HWND);
typedef BOOL(WINAPI* contextsequal)(HANDLE, HANDLE);

// Compares the awareness of the main window with the four standard values.
int shellawareness()
{
    HMODULE user = GetModuleHandleA("user32.dll");
    contextofwindow get = user == NULL ? NULL : (contextofwindow)GetProcAddress(user, "GetWindowDpiAwarenessContext");
    contextsequal equal = user == NULL ? NULL : (contextsequal)GetProcAddress(user, "AreDpiAwarenessContextsEqual");

    if (get == NULL || equal == NULL || state.parts.main == NULL)
    {
        return 0;
    }

    HANDLE context = get(state.parts.main);

    for (int value = -4; value <= -1; value++)
    {
        if (equal(context, (HANDLE)(INT_PTR)value))
        {
            return value;
        }
    }

    return 0;
}

// Remembers the log file and writes the line of the start.
void shelldpilog(const string& path)
{
    state.dpilog = path;
    RECT box;
    GetWindowRect(state.parts.main, &box);
    dpilogline("start awareness=" + numtext(shellawareness()) + " dpi=" + numtext(themedpi()) + " window=" + recttext(box));
}

// The line is set by the WM_DPICHANGED case itself.
int shelldpihandlerline()
{
    return state.dpiline;
}

// A test may make the window bigger than the screen.
void shellallowhuge(bool allow)
{
    state.allowhuge = allow;
}

// Shows or hides the sidebar panel (and with it every control in it).
static void showsidebar(bool visible)
{
    state.sidebar = visible;
    ShowWindow(state.parts.sidebar, visible ? SW_SHOWNA : SW_HIDE);
}

// Selects the sidebar item that belongs to a page, if there is one.
static void selectnavfor(int pageid)
{
    int count = shellnavcount();

    for (int index = 0; index < count; index++)
    {
        if (shellnavpage(index) == pageid)
        {
            // this is not a click: the notification that the list sends must not open the page a second time
            LVITEMA mark;
            ZeroMemory(&mark, sizeof(mark));
            mark.stateMask = LVIS_SELECTED;
            mark.state = LVIS_SELECTED;
            state.syncing = true;
            SendMessageA(state.parts.nav, LVM_SETITEMSTATE, (WPARAM)index, (LPARAM)&mark);
            state.syncing = false;
            return;
        }
    }
}

// Switches pages inside the host. The new page is laid out and filled before it is shown, and the host
// does not repaint in between, so the switch is quick and does not flicker.
void shellshow(int pageid, bool takefocus)
{
    guipage* next = shellpage(pageid);

    if (next == NULL)
    {
        return;
    }

    guipage* old = shellpage(state.current);
    SendMessageA(state.parts.host, WM_SETREDRAW, FALSE, 0);

    if (old != NULL && old != next)
    {
        ShowWindow(old->window(), SW_HIDE);
    }

    if (state.current != pageid)
    {
        state.previous = state.current;
    }

    state.current = pageid;
    shellstatus("");
    selectnavfor(next->navid());
    layoutnow();
    next->onshow();
    ShowWindow(next->window(), SW_SHOWNA);
    SendMessageA(state.parts.host, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(state.parts.host, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);

    if (takefocus)
    {
        HWND first = GetNextDlgTabItem(next->window(), NULL, FALSE);

        if (first != NULL)
        {
            SetFocus(first);
        }
    }
}

// Back to the page before; without one, the first page of the role.
void shellback()
{
    int target = state.previous;
    const navitem* items = navof(state.who);

    if (target == PAGE_NONE || target == PAGE_WELCOME || shellpage(target) == NULL)
    {
        target = items != NULL ? items[0].page : PAGE_WELCOME;
    }

    shellshow(target, true);
}

// Fills the sidebar for the role, then opens the first page of that role.
void shellsignin(int who, const string& personid, const string& personname)
{
    const navitem* items = navof(who);
    state.who = who;
    state.personid = personid;
    state.personname = personname;

    SendMessageA(state.parts.nav, LVM_DELETEALLITEMS, 0, 0);

    for (int pos = 0; items != NULL && items[pos].text != NULL; pos++)
    {
        LVITEMA entry;
        ZeroMemory(&entry, sizeof(entry));
        entry.mask = LVIF_TEXT | LVIF_PARAM;
        entry.iItem = pos;
        entry.pszText = (char*)items[pos].text;
        entry.lParam = (LPARAM)items[pos].page;
        SendMessageA(state.parts.nav, LVM_INSERTITEMA, 0, (LPARAM)&entry);
    }

    string role = "Coordinator";

    if (who == WHO_DONOR)
    {
        role = "Donor";
    }
    else if (who == WHO_RECIPIENT)
    {
        role = "Recipient";
    }

    SetWindowTextA(state.whorole, role.c_str());
    SetWindowTextA(state.whoname, personname.c_str());

    SetWindowTextA(state.parts.main, "CommunityBridge");
    showsidebar(true);

    if (items != NULL)
    {
        shellshow(items[0].page, false);
    }
}

// Back to the welcome page: nobody is signed in and the sidebar is hidden.
void shellswitchuser()
{
    state.who = WHO_NONE;
    state.personid = "";
    state.personname = "";
    SendMessageA(state.parts.nav, LVM_DELETEALLITEMS, 0, 0);
    SetWindowTextA(state.parts.main, "CommunityBridge");
    showsidebar(false);
    shellshow(PAGE_WELCOME, true);
}

// Gives the window this client size, keeping its top left corner.
void shellsetclient(int width, int height)
{
    RECT frame = {0, 0, width, height};
    frameforwindow(state.parts.main, frame);
    SetWindowPos(state.parts.main, NULL, 0, 0, frame.right - frame.left, frame.bottom - frame.top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
}

// The main window procedure: size, minimum size, the sidebar, and the commands of the pages.
static LRESULT CALLBACK mainproc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT colour = 0;

    if (themecolour(msg, wparam, lparam, colour))
    {
        return colour;
    }

    // the status bar asks the program to draw its text (the owner of this round allowed it, decision D20)
    if (msg == WM_DRAWITEM)
    {
        const DRAWITEMSTRUCT* item = (const DRAWITEMSTRUCT*)lparam;

        if (item != NULL && item->hwndItem == state.parts.status)
        {
            themedrawstatus(item, state.statustext.c_str());
            return TRUE;
        }
    }

    if (msg == WM_SIZE)
    {
        if (wparam != SIZE_MINIMIZED && state.parts.status != NULL)
        {
            layoutmain(LOWORD(lparam), HIWORD(lparam));
        }

        return 0;
    }

    if (msg == WM_GETMINMAXINFO)
    {
        MINMAXINFO* info = (MINMAXINFO*)lparam;
        RECT frame = {0, 0, px(900), px(600)};
        frameforwindow(window, frame);
        info->ptMinTrackSize.x = frame.right - frame.left;
        info->ptMinTrackSize.y = frame.bottom - frame.top;

        // a test may make the window bigger than the screen (the biggest size at a high dpi)
        if (state.allowhuge)
        {
            info->ptMaxTrackSize.x = 16000;
            info->ptMaxTrackSize.y = 16000;
        }

        return 0;
    }

    // the window moved to a monitor with another scale, or the scale changed: the new dpi is in the low word
    if (msg == WM_DPICHANGED)
    {
        state.dpiline = __LINE__;
        dpilogline("dpichanged old=" + numtext(themedpi()) + " new=" + numtext(LOWORD(wparam)) + " suggested=" + recttext(*(const RECT*)lparam));
        shellapplydpi((int)LOWORD(wparam), (const RECT*)lparam);
        return 0;
    }

    if (msg == DM_GETDEFID)
    {
        guipage* page = shellpage(state.current);

        if (page != NULL && page->defaultid() != 0)
        {
            HWND button = GetDlgItem(page->window(), page->defaultid());

            if (button != NULL && IsWindowEnabled(button) && (GetWindowLongA(button, GWL_STYLE) & WS_VISIBLE))
            {
                return MAKELRESULT(page->defaultid(), DC_HASDEFID);
            }
        }

        return 0;
    }

    if (msg == WM_ACTIVATE)
    {
        // the keyboard focus comes back to the control that had it
        if (LOWORD(wparam) == WA_INACTIVE)
        {
            state.lastfocus = GetFocus();
        }
        else if (state.lastfocus != NULL && IsWindow(state.lastfocus))
        {
            SetFocus(state.lastfocus);
        }

        return 0;
    }

    if (msg == WM_COMMAND)
    {
        int id = LOWORD(wparam);
        int code = HIWORD(wparam);

        if (id == IDC_SWITCHUSER)
        {
            if (code == BN_CLICKED)
            {
                shellswitchuser();
            }
        }
        else
        {
            // Enter in a dialog-style window arrives here; the page decides what it means
            guipage* page = shellpage(state.current);

            if (page != NULL)
            {
                page->command(id, code);
            }
        }

        return 0;
    }

    // a click or an arrow key in the sidebar list selects an item: its page is shown
    if (msg == WM_NOTIFY)
    {
        const NMLISTVIEW* change = (const NMLISTVIEW*)lparam;

        if (change->hdr.idFrom == IDC_NAV && change->hdr.code == LVN_ITEMCHANGED && !state.syncing &&
            (change->uChanged & LVIF_STATE) != 0 && (change->uNewState & LVIS_SELECTED) != 0 &&
            (change->uOldState & LVIS_SELECTED) == 0)
        {
            shellshow(shellnavpage(change->iItem), false);
        }

        // a mouse click hides the focus outline again (a key press shows it, as Windows does)
        if (change->hdr.idFrom == IDC_NAV && change->hdr.code == (UINT)NM_CLICK)
        {
            SendMessageA(GetAncestor(window, GA_ROOT), WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0);
        }

        return 0;
    }

    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(window, msg, wparam, lparam);
}

// The page host only holds pages; it has the page background and passes the colour messages on.
static LRESULT CALLBACK hostproc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT colour = 0;

    if (themecolour(msg, wparam, lparam, colour))
    {
        return colour;
    }

    return DefWindowProcA(window, msg, wparam, lparam);
}

// The sidebar panel has the sidebar background, so the buttons and texts in it blend in. Its commands
// (the list and the Switch user button) are passed on to the main window.
static LRESULT CALLBACK sideproc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT colour = 0;

    if (themecolour(msg, wparam, lparam, colour))
    {
        return colour;
    }

    if (msg == WM_COMMAND || msg == WM_NOTIFY)
    {
        return SendMessageA(GetParent(window), msg, wparam, lparam);
    }

    return DefWindowProcA(window, msg, wparam, lparam);
}

// Registers the window classes. None of them has CS_HREDRAW or CS_VREDRAW.
static void registerclasses(HINSTANCE instance)
{
    WNDCLASSA description;
    ZeroMemory(&description, sizeof(description));
    description.style = 0;
    description.lpfnWndProc = mainproc;
    description.hInstance = instance;
    description.hCursor = LoadCursor(NULL, IDC_ARROW);
    description.hbrBackground = themepagebrush();
    description.lpszClassName = mainclass;
    RegisterClassA(&description);

    description.lpfnWndProc = hostproc;
    description.hIcon = NULL;
    description.lpszClassName = hostclass;
    RegisterClassA(&description);

    description.lpfnWndProc = sideproc;
    description.hbrBackground = themesidebarbrush();
    description.lpszClassName = sideclass;
    RegisterClassA(&description);
    description.hbrBackground = themepagebrush();

    guiregisterpageclass(instance);
}

// Makes the window, its controls and all pages. The window starts at 1000 by 650, centred on the work area.
bool shellcreate(HINSTANCE instance)
{
    state.parts.main = NULL;
    state.parts.host = NULL;
    state.parts.sidebar = NULL;
    state.parts.appname = NULL;
    state.parts.nav = NULL;
    state.parts.status = NULL;
    state.current = PAGE_NONE;
    state.previous = PAGE_NONE;
    state.who = WHO_NONE;
    state.sidebar = false;
    state.syncing = false;
    state.lastfocus = NULL;
    state.allowhuge = false;
    state.dpiline = 0;

    registerclasses(instance);

    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    DWORD exstyle = WS_EX_CONTROLPARENT;
    RECT frame = {0, 0, px(1000), px(650)};
    AdjustWindowRectEx(&frame, style, FALSE, exstyle);
    int width = frame.right - frame.left;
    int height = frame.bottom - frame.top;

    RECT area;
    SystemParametersInfoA(SPI_GETWORKAREA, 0, &area, 0);

    if (width > area.right - area.left)
    {
        width = area.right - area.left;
    }

    if (height > area.bottom - area.top)
    {
        height = area.bottom - area.top;
    }

    int left = area.left + (area.right - area.left - width) / 2;
    int top = area.top + (area.bottom - area.top - height) / 2;

    state.parts.main = CreateWindowExA(exstyle, mainclass, "CommunityBridge", style, left, top, width, height, NULL,
                                       NULL, instance, NULL);

    if (state.parts.main == NULL)
    {
        return false;
    }

    HWND main = state.parts.main;

    // the icon of the program for the title bar, the taskbar and Alt+Tab, in the sizes Windows asks for
    HICON big = (HICON)LoadImageA(instance, MAKEINTRESOURCEA(IDI_APP), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    HICON small = (HICON)LoadImageA(instance, MAKEINTRESOURCEA(IDI_APP), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    SendMessageA(main, WM_SETICON, ICON_BIG, (LPARAM)big);
    SendMessageA(main, WM_SETICON, ICON_SMALL, (LPARAM)small);

    // the sidebar comes first, so Tab goes through it before the page: the list, then Switch user
    state.parts.sidebar = CreateWindowExA(WS_EX_CONTROLPARENT, sideclass, "", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0,
                                          10, 10, main, NULL, instance, NULL);
    HWND panel = state.parts.sidebar;
    state.parts.appname = makelabel(panel, "CommunityBridge", ROLE_APPNAME);
    state.parts.nav = CreateWindowExA(0, WC_LISTVIEWA, "",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_SINGLESEL |
                                          LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                                      0, 0, 10, 10, panel, (HMENU)(INT_PTR)IDC_NAV, instance, NULL);
    themestylenav(state.parts.nav);
    LVCOLUMNA column;
    ZeroMemory(&column, sizeof(column));
    column.mask = LVCF_WIDTH;
    column.cx = px(sidebarwidth);
    SendMessageA(state.parts.nav, LVM_INSERTCOLUMNA, 0, (LPARAM)&column);
    state.whocaption = makelabel(panel, "Signed in as", ROLE_SIDEMUTED);
    state.whorole = makelabel(panel, "", ROLE_SIDETEXT);
    state.whoname = makelabel(panel, "", ROLE_SIDETEXT);
    markdata(state.whoname);
    state.switchuser = makebutton(panel, "Switch user", IDC_SWITCHUSER, false);

    state.parts.host = CreateWindowExA(WS_EX_CONTROLPARENT, hostclass, "", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                       0, 0, 10, 10, main, NULL, instance, NULL);

    state.parts.status = CreateWindowExA(0, STATUSCLASSNAMEA, "", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | CCS_NOPARENTALIGN | CCS_NORESIZE,
                                         0, 0, 10, 10, main, (HMENU)(INT_PTR)IDC_STATUSBAR, instance, NULL);
    int parts[1] = {-1};
    SendMessageA(state.parts.status, SB_SETPARTS, 1, (LPARAM)parts);
    themestylestatus(state.parts.status);
    shellstatus("");


    buildpages(state.parts.host);
    themeapplyfonts(main);

    // like a dialog: the dotted focus outline stays hidden after mouse clicks and appears once the keyboard is used
    SendMessageA(main, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0);
    showsidebar(false);

    // the window may be on a monitor whose scale is not the one the program started with (or a test asked for one)
    int windowdpi = themedpiforwindow(main);

    if (windowdpi != themedpi())
    {
        shellapplydpi(windowdpi, NULL);
    }

    // the program starts on the welcome page, without the sidebar
    shellshow(PAGE_WELCOME, true);
    layoutnow();
    return true;
}

// The message loop. IsDialogMessage gives the Tab key its usual meaning across all the controls of the window.
int shellrun()
{
    MSG msg;

    while (GetMessageA(&msg, NULL, 0, 0) > 0)
    {
        if (!IsDialogMessage(state.parts.main, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    return (int)msg.wParam;
}

// Deletes the pages (their windows are already gone with the main window).
void shelldestroy()
{
    for (size_t pos = 0; pos < state.pages.size(); pos++)
    {
        delete state.pages[pos];
    }

    state.pages.clear();
}
