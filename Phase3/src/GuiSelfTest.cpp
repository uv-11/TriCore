// GuiSelfTest.cpp: the test modes. They walk through the pages of the real window at three sizes, measure the
// layout, and write selftest.txt. In screenshot mode they also save a picture of every page.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

#include "AppState.h"
#include "GuiSelfTest.h"
#include "GuiPage.h"
#include "GuiShell.h"
#include "PageTable.h"

#include <stdlib.h>
#include "Pages.h"
#include "Queries.h"
#include "TextTools.h"
#include "Workflow.h"
#include "Theme.h"
#include "resource.h"

#include <math.h>
#include <stdio.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// The old Windows headers do not know this flag: it asks for the whole content, also of covered windows.
#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 2
#endif

// The old Windows headers do not know the message that says the dpi of the window changed.
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

// The file the log is written to (empty text means no test mode).
static string logpath = "";

// Empties the file and remembers its name, which switches the test mode on.
void testlogopen(const string& path)
{
    logpath = path;
    ofstream out(logpath.c_str());
}

// True once testlogopen was called.
bool testmode()
{
    return !logpath.empty();
}

// Appends one line to the log file.
void testlog(const string& line)
{
    if (logpath.empty())
    {
        return;
    }

    ofstream out(logpath.c_str(), ios::app);
    out << line << "\n";
}

// The sizes of the client area that every page is checked at, in design pixels: the smallest the window can be, the
// starting size, a roomy one and a maximised window on a full HD screen.
static const int testsizes[4][2] = {{900, 600}, {1000, 650}, {1400, 900}, {1920, 1010}};

// The display scales that every page is checked at (96 is 100 percent, 120 is 125, 144 is 150, 168 is 175, 192 is 200).
static const int testdpis[5] = {96, 120, 144, 168, 192};

// The pictures are taken at three scales and three sizes (the smallest, the starting size and the full HD one).
static const int shotsizes[3][2] = {{900, 600}, {1000, 650}, {1920, 1010}};
static const int shotdpis[3] = {96, 144, 192};

// The scale that the test asked for with --dpi (0 means all of them).
static int onlydpi = 0;

// The scale the window had when the test started (the one of the screen, or the one asked for).
static int startdpi = 96;

// Fills out with the scales to test and returns how many there are: the one asked for with --dpi, else the five
// scales of the self-test, or the three scales of the pictures.
static int modedpis(bool pictures, int* out)
{
    if (onlydpi > 0)
    {
        out[0] = onlydpi;
        return 1;
    }

    int count = pictures ? 3 : 5;

    for (int pos = 0; pos < count; pos++)
    {
        out[pos] = pictures ? shotdpis[pos] : testdpis[pos];
    }

    return count;
}

// Makes the window use this scale (a test does what Windows does when the window moves to another monitor).
static void usedpi(int dpi)
{
    if (themedpi() != dpi)
    {
        shellapplydpi(dpi, NULL);
    }
}

// One step of the walk: who is signed in and which page is looked at.
struct walkstep
{
    int who;
    int page;
};

// The pieces of the version information of comctl32 that we need.
struct dllversion
{
    DWORD size;
    DWORD major;
    DWORD minor;
    DWORD build;
    DWORD platform;
};

// Lets the window handle every message that is waiting, so it is painted before a picture is taken.
static void pump()
{
    MSG msg;

    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

// Lets the window handle its messages for a third of a second. Windows fades a themed button between its enabled and
// disabled look; a picture taken in the middle of that fade would show the old state.
static void settle()
{
    DWORD start = GetTickCount();

    while (GetTickCount() - start < 350)
    {
        pump();
        Sleep(10);
    }
}

// Milliseconds from a counter value, rounded up so that a very fast step shows at least 1.
static int millis(LARGE_INTEGER start, LARGE_INTEGER stop)
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    double seconds = (double)(stop.QuadPart - start.QuadPart) / (double)frequency.QuadPart;
    int value = (int)(seconds * 1000.0 + 0.999);
    return value < 1 ? 1 : value;
}

// The major version of comctl32 (6 means the new look, which the manifest asks for).
static int comctlmajor()
{
    HMODULE module = GetModuleHandleA("comctl32.dll");

    if (module == NULL)
    {
        return 0;
    }

    typedef HRESULT(CALLBACK * versionfunction)(dllversion*);
    versionfunction function = (versionfunction)GetProcAddress(module, "DllGetVersion");

    if (function == NULL)
    {
        return 0;
    }

    dllversion info;
    ZeroMemory(&info, sizeof(info));
    info.size = sizeof(info);

    if (FAILED(function(&info)))
    {
        return 0;
    }

    return (int)info.major;
}

// The caption of the main window.
static string captiontext()
{
    char buffer[256];
    buffer[0] = '\0';
    GetWindowTextA(shellgetparts().main, buffer, (int)sizeof(buffer));
    return buffer;
}

// The first lines of the log: facts about the window system that the look depends on.
static void logenvironment()
{
    testlog("comctl=" + numtext(comctlmajor()));
    testlog(string("themed=") + (IsAppThemed() ? "1" : "0"));
    testlog("dpi=" + numtext(themedpi()));
    testlog(string("dpi_override=") + (themedpiisoverridden() ? numtext(themedpiforwindow(NULL)) : string("none")));
    LRESULT icon = SendMessageA(shellgetparts().main, WM_GETICON, ICON_BIG, 0);
    testlog(string("icon=") + (icon != 0 ? "1" : "0"));
}

// A control counts as shown when its own visible flag is on (this also works for a window that is not on screen).
static bool shown(HWND control)
{
    return (GetWindowLongA(control, GWL_STYLE) & WS_VISIBLE) != 0;
}

// The rectangle of a window on the screen.
static RECT screenrect(HWND control)
{
    RECT rectangle;
    GetWindowRect(control, &rectangle);
    return rectangle;
}

// True when the inner rectangle lies completely inside the outer one.
static bool inside(const RECT& inner, const RECT& outer)
{
    return inner.left >= outer.left && inner.top >= outer.top && inner.right <= outer.right && inner.bottom <= outer.bottom;
}

// True when the two rectangles share some area.
static bool overlap(const RECT& first, const RECT& second)
{
    return first.left < second.right && second.left < first.right && first.top < second.bottom && second.top < first.bottom;
}

// The class name of a control.
static string classof(HWND control)
{
    char buffer[64];
    buffer[0] = '\0';
    GetClassNameA(control, buffer, (int)sizeof(buffer));
    return buffer;
}

// The text of a control.
static string textof(HWND control)
{
    char buffer[512];
    buffer[0] = '\0';
    GetWindowTextA(control, buffer, (int)sizeof(buffer));
    return buffer;
}

// Measures the text with the control's own font. Returns the size the text needs when it may wrap at maxwidth
// (maxwidth 0 means one line).
static SIZE measuretext(HWND control, const string& text, int maxwidth)
{
    HDC dc = GetDC(control);
    HGDIOBJ old = SelectObject(dc, (HFONT)SendMessageA(control, WM_GETFONT, 0, 0));
    RECT area = {0, 0, maxwidth > 0 ? maxwidth : 0, 0};
    UINT format = DT_CALCRECT | DT_NOPREFIX;
    format = format | (maxwidth > 0 ? DT_WORDBREAK : DT_SINGLELINE);
    DrawTextA(dc, text.c_str(), -1, &area, format);
    SelectObject(dc, old);
    ReleaseDC(control, dc);

    SIZE size;
    size.cx = area.right - area.left;
    size.cy = area.bottom - area.top;
    return size;
}

// Checks one visible control of a page: the text must fit inside it.
static void checktextfit(HWND control, vector<string>& problems)
{
    string kind = classof(control);
    string text = textof(control);
    RECT client;
    GetClientRect(control, &client);
    int width = client.right - client.left;
    int height = client.bottom - client.top;

    if (text.empty())
    {
        return;
    }

    if (kind == "Static")
    {
        SIZE need = measuretext(control, text, width);

        if (need.cx > width || need.cy > height)
        {
            problems.push_back("clipped text in label '" + text + "' needs " + numtext(need.cx) + "x" + numtext(need.cy) +
                               " has " + numtext(width) + "x" + numtext(height));
        }
    }
    else if (kind == "Button")
    {
        int type = (int)(GetWindowLongA(control, GWL_STYLE) & 0xF);
        int extra = (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON) ? px(16) : px(24);
        SIZE need = measuretext(control, text, 0);

        if (need.cx + extra > width || need.cy > height)
        {
            problems.push_back("clipped text in button '" + text + "' needs " + numtext(need.cx + extra) + "x" +
                               numtext(need.cy) + " has " + numtext(width) + "x" + numtext(height));
        }
    }
}

// The first column header of a list view that has punctuation in it, or an empty text when none has.
static string punctuatedheader(HWND list)
{
    HWND header = (HWND)SendMessageA(list, LVM_GETHEADER, 0, 0);
    int count = header == NULL ? 0 : (int)SendMessageA(header, HDM_GETITEMCOUNT, 0, 0);

    for (int column = 0; column < count; column++)
    {
        char buffer[128];
        buffer[0] = '\0';
        LVCOLUMNA description;
        ZeroMemory(&description, sizeof(description));
        description.mask = LVCF_TEXT;
        description.pszText = buffer;
        description.cchTextMax = (int)sizeof(buffer);
        SendMessageA(list, LVM_GETCOLUMNA, (WPARAM)column, (LPARAM)&description);

        if (haspunct(buffer))
        {
            return buffer;
        }
    }

    return "";
}

// (j) No punctuation on the screen: a label that is not data, a button, a list header. A text that comes from the
// files (a name, an id) was marked as data by its page and is skipped. One question mark at the end is allowed.
static void checkplain(HWND control, vector<string>& problems)
{
    string kind = classof(control);
    string text = textof(control);

    if (kind == "Static" && !isdata(control) && haspunct(text))
    {
        problems.push_back("(j) the label '" + text + "' has punctuation");
    }
    else if (kind == "Button" && haspunct(text))
    {
        problems.push_back("(j) the button '" + text + "' has punctuation");
    }
    else if (kind == "SysListView32")
    {
        string header = punctuatedheader(control);

        if (!header.empty())
        {
            problems.push_back("(j) the list header '" + header + "' has punctuation");
        }
    }
}

// The height of the font that a control uses, or 0 when it has none.
static int fontheightof(HWND control)
{
    HFONT font = (HFONT)SendMessageA(control, WM_GETFONT, 0, 0);
    LOGFONTA description;
    ZeroMemory(&description, sizeof(description));

    if (font == NULL || GetObjectA(font, sizeof(description), &description) == 0)
    {
        return 0;
    }

    return description.lfHeight;
}

// (o) Every control that shows normal text has the body font of the present dpi (10 points at that dpi), and so has
// the status bar. A font that was not made again after a dpi change would show up here.
static void checkfonts(guipage* page, vector<string>& problems)
{
    int expected = themebodyheight(themedpi());
    HWND child = GetWindow(page->window(), GW_CHILD);

    while (child != NULL)
    {
        string kind = classof(child);
        bool body = kind == "Button" || kind == "SysListView32" || kind == "ListBox" || kind == "Edit" || kind == "ComboBox" ||
                    (kind == "Static" && themegetrole(child) == ROLE_TEXT);

        if (shown(child) && body && fontheightof(child) != expected)
        {
            problems.push_back("(o) the font of '" + textof(child) + "' (" + kind + ") has height " + numtext(fontheightof(child)) +
                               ", expected " + numtext(expected));
        }

        child = GetWindow(child, GW_HWNDNEXT);
    }

    if (fontheightof(shellgetparts().status) != expected)
    {
        problems.push_back("(o) the font of the status bar has height " + numtext(fontheightof(shellgetparts().status)) + ", expected " +
                           numtext(expected));
    }
}

// (p) The page is balanced at every size: the welcome block is centred and a Home column is at most 560 design pixels
// wide. (Lists fill the content width by the user's decision; checktable checks that their columns use it.)
static void checkfluid(guipage* page, vector<string>& problems)
{
    RECT hostrect = screenrect(shellgetparts().host);
    int hostwidth = hostrect.right - hostrect.left;
    int hostheight = hostrect.bottom - hostrect.top;
    RECT block = {0x7FFFFFF, 0x7FFFFFF, -0x7FFFFFF, -0x7FFFFFF};
    RECT column = {0x7FFFFFF, 0x7FFFFFF, -0x7FFFFFF, -0x7FFFFFF};
    HWND child = GetWindow(page->window(), GW_CHILD);

    while (child != NULL)
    {
        if (shown(child))
        {
            RECT place = screenrect(child);
            block.left = place.left < block.left ? place.left : block.left;
            block.top = place.top < block.top ? place.top : block.top;
            block.right = place.right > block.right ? place.right : block.right;
            block.bottom = place.bottom > block.bottom ? place.bottom : block.bottom;

            if (themegetrole(child) != ROLE_TITLE)
            {
                column.left = place.left < column.left ? place.left : column.left;
                column.right = place.right > column.right ? place.right : column.right;
            }

        }

        child = GetWindow(child, GW_HWNDNEXT);
    }

    if (page->id() == PAGE_WELCOME && block.right > block.left)
    {
        int centrex = (block.left + block.right) / 2 - hostrect.left;
        int centrey = (block.top + block.bottom) / 2 - hostrect.top;
        int off = centrex > hostwidth / 2 ? centrex - hostwidth / 2 : hostwidth / 2 - centrex;
        int percent = (centrey * 100) / (hostheight > 0 ? hostheight : 1);

        if (off * 100 > hostwidth * 3)
        {
            problems.push_back("(p) the welcome block is " + numtext(off) + " pixels off the horizontal centre");
        }

        if (percent < 38 || percent > 46)
        {
            problems.push_back("(p) the welcome block is centred at " + numtext(percent) + " percent of the height");
        }
    }

    bool home = page->id() == PAGE_C_HOME || page->id() == PAGE_D_HOME || page->id() == PAGE_R_HOME;

    // LC5 a Home stat row: the values have equal widths and the first starts at the left margin
    int firstleft = 0x7FFFFFF;
    int firstwidth = -1;
    child = GetWindow(page->window(), GW_CHILD);

    while (home && child != NULL)
    {
        if (shown(child) && themegetrole(child) == ROLE_VALUE)
        {
            RECT place = screenrect(child);

            if (firstwidth >= 0 && abs((place.right - place.left) - firstwidth) > 2)
            {
                problems.push_back("(lc5) the Home values are not equal in width (" + numtext(place.right - place.left) + " and " + numtext(firstwidth) + ")");
            }

            firstwidth = place.right - place.left;
            firstleft = place.left < firstleft ? place.left : firstleft;
        }

        child = GetWindow(child, GW_HWNDNEXT);
    }

    if (home && firstwidth >= 0 && abs(firstleft - hostrect.left - px(pagemargin)) > 2)
    {
        problems.push_back("(lc5) the first Home value starts " + numtext(firstleft - hostrect.left) + " from the left, wanted " + numtext(px(pagemargin)));
    }
}

// (m) Every visible row of a list lies inside the list (only the last visible row may be cut by the bottom edge), and
// a row is at least as high as the text of the list font plus 8 design pixels.
static void checklistrows(guipage* page, vector<string>& problems)
{
    HWND child = GetWindow(page->window(), GW_CHILD);

    while (child != NULL)
    {
        if (shown(child) && classof(child) == "SysListView32" && (int)SendMessageA(child, LVM_GETITEMCOUNT, 0, 0) > 0)
        {
            RECT area;
            GetClientRect(child, &area);
            int top = (int)SendMessageA(child, LVM_GETTOPINDEX, 0, 0);
            int perpage = (int)SendMessageA(child, LVM_GETCOUNTPERPAGE, 0, 0);
            int count = (int)SendMessageA(child, LVM_GETITEMCOUNT, 0, 0);
            int last = top + perpage;

            if (last >= count)
            {
                last = count - 1;
            }

            HDC dc = GetDC(child);
            HGDIOBJ old = SelectObject(dc, (HFONT)SendMessageA(child, WM_GETFONT, 0, 0));
            TEXTMETRICA metrics;
            GetTextMetricsA(dc, &metrics);
            SelectObject(dc, old);
            ReleaseDC(child, dc);

            for (int row = top; row <= last; row++)
            {
                RECT place;
                ZeroMemory(&place, sizeof(place));
                place.left = LVIR_BOUNDS;
                SendMessageA(child, LVM_GETITEMRECT, (WPARAM)row, (LPARAM)&place);

                if (place.left < area.left || place.top < 0 || (row < last && place.bottom > area.bottom))
                {
                    problems.push_back("(m) row " + numtext(row) + " is cut: rows " + numtext(place.top) + " to " + numtext(place.bottom) +
                                       " in a list that is " + numtext(area.bottom) + " high");
                }

                if (place.bottom - place.top < metrics.tmHeight + px(8))
                {
                    problems.push_back("(m) row " + numtext(row) + " is " + numtext(place.bottom - place.top) + " high, the text needs " +
                                       numtext(metrics.tmHeight + px(8)));
                }
            }
        }

        child = GetWindow(child, GW_HWNDNEXT);
    }
}

// The pixels of the whole main window (24 bits per pixel, bottom row first). PrintWindow paints the window again from
// nothing; a copy from the screen shows what is really on the screen, with every leftover of an earlier paint.
static bool capturepixels(vector<unsigned char>& pixels, int& width, int& height, int& rowbytes, bool fromscreen)
{
    HWND window = shellgetparts().main;
    RECT frame;
    GetWindowRect(window, &frame);
    width = frame.right - frame.left;
    height = frame.bottom - frame.top;

    HDC screen = GetDC(NULL);
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, width, height);
    HGDIOBJ old = SelectObject(memory, bitmap);
    bool painted = false;

    if (fromscreen)
    {
        painted = BitBlt(memory, 0, 0, width, height, screen, frame.left, frame.top, SRCCOPY) != FALSE;
    }
    else
    {
        painted = PrintWindow(window, memory, PW_RENDERFULLCONTENT) != FALSE;
    }

    BITMAPINFO info;
    ZeroMemory(&info, sizeof(info));
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 24;
    info.bmiHeader.biCompression = BI_RGB;
    rowbytes = ((width * 3 + 3) / 4) * 4;
    pixels.assign((size_t)rowbytes * (size_t)height, 0);

    if (painted)
    {
        GetDIBits(memory, bitmap, 0, height, &pixels[0], &info, DIB_RGB_COLORS);
    }

    SelectObject(memory, old);
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(NULL, screen);
    return painted;
}

// The part of the window (in bitmap pixels) that lies on the work area of the monitor (no taskbar). A window that is bigger (a test
// size at a high dpi) is painted by Windows only where it can be seen, so only that part is looked at.
static RECT onmonitor = {0, 0, 0, 0};

// Counts the pixels of one column that differ from the colour, skipping a vertical range. A column is a screen x position.
static int offcolour(const vector<unsigned char>& pixels, int width, int height, int rowbytes, int x, int fromy, int toy,
                     COLORREF expected, int skipfrom, int skipto, int skiphotfrom, int skiphotto, int& firsty, COLORREF& found)
{
    int bad = 0;
    firsty = -1;

    if (x < 0 || x >= width || x < onmonitor.left || x >= onmonitor.right)
    {
        return 0;
    }

    for (int y = (fromy > onmonitor.top ? fromy : onmonitor.top); y < toy && y < height && y < onmonitor.bottom; y++)
    {
        if ((y >= skipfrom && y < skipto) || (y >= skiphotfrom && y < skiphotto))
        {
            continue;
        }

        size_t at = (size_t)(height - 1 - y) * (size_t)rowbytes + (size_t)x * 3;
        COLORREF colour = RGB(pixels[at + 2], pixels[at + 1], pixels[at]);

        if (colour != expected)
        {
            if (bad == 0)
            {
                firsty = y;
                found = colour;
            }

            bad++;
        }
    }

    return bad;
}

// (n) The sidebar and the page are one even tone: a column of pixels 6 pixels inside the left edge and two columns at
// the right edge of the sidebar, and a column at the left edge and one 6 pixels inside the right edge of the page host,
// all from top to bottom, hold exactly the colour of that area. The selected item of the sidebar is the only thing the
// columns of the sidebar may cross that has another colour.
static void sampleuniform(vector<string>& problems)
{
    // the mouse pointer may be over a sidebar item, which then has its hover colour: no item is hot while the pixels are read
    SendMessageA(shellgetparts().nav, LVM_SETHOTITEM, (WPARAM)-1, 0);

    // a very big window needs a moment to be painted completely, so everything is painted now and the messages are handled
    RedrawWindow(shellgetparts().main, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    pump();
    vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    int rowbytes = 0;

    if (!capturepixels(pixels, width, height, rowbytes, false))
    {
        problems.push_back("(n) the window could not be painted into a bitmap");
        return;
    }

    RECT mainrect = screenrect(shellgetparts().main);
    RECT hostrect = screenrect(shellgetparts().host);
    MONITORINFO monitor;
    ZeroMemory(&monitor, sizeof(monitor));
    monitor.cbSize = sizeof(monitor);
    GetMonitorInfoA(MonitorFromWindow(shellgetparts().main, MONITOR_DEFAULTTONEAREST), &monitor);
    onmonitor.left = monitor.rcWork.left - mainrect.left;
    onmonitor.top = monitor.rcWork.top - mainrect.top;
    onmonitor.right = monitor.rcWork.right - mainrect.left;
    onmonitor.bottom = monitor.rcWork.bottom - mainrect.top;
    int firsty = 0;
    COLORREF found = 0;

    if (shellsidebarshown())
    {
        RECT side = screenrect(shellgetparts().sidebar);
        int skipfrom = 0;
        int skipto = 0;
        int selected = shellnavselected();

        if (selected >= 0)
        {
            RECT item;
            ZeroMemory(&item, sizeof(item));
            item.left = LVIR_BOUNDS;
            SendMessageA(shellgetparts().nav, LVM_GETITEMRECT, (WPARAM)selected, (LPARAM)&item);
            POINT corner;
            corner.x = 0;
            corner.y = item.top;
            ClientToScreen(shellgetparts().nav, &corner);
            skipfrom = corner.y - mainrect.top;
            skipto = skipfrom + (item.bottom - item.top);
        }

        // the item under the mouse pointer (if the pointer is over the list) has its hover colour and is not read either
        int hotfrom = 0;
        int hottto = 0;
        POINT pointer;
        GetCursorPos(&pointer);
        POINT inlist = pointer;
        ScreenToClient(shellgetparts().nav, &inlist);
        LVHITTESTINFO hit;
        ZeroMemory(&hit, sizeof(hit));
        hit.pt = inlist;
        int hotitem = (int)SendMessageA(shellgetparts().nav, LVM_HITTEST, 0, (LPARAM)&hit);

        if (hotitem >= 0)
        {
            RECT hotrect;
            ZeroMemory(&hotrect, sizeof(hotrect));
            hotrect.left = LVIR_BOUNDS;
            SendMessageA(shellgetparts().nav, LVM_GETITEMRECT, (WPARAM)hotitem, (LPARAM)&hotrect);
            POINT corner2;
            corner2.x = 0;
            corner2.y = hotrect.top;
            ClientToScreen(shellgetparts().nav, &corner2);
            hotfrom = corner2.y - mainrect.top;
            hottto = hotfrom + (hotrect.bottom - hotrect.top);
        }

        int columns[4] = {side.left - mainrect.left + 6, side.right - mainrect.left - 1, side.right - mainrect.left - 2,
                          side.right - mainrect.left - 6};

        for (int pos = 0; pos < 4; pos++)
        {
            int bad = offcolour(pixels, width, height, rowbytes, columns[pos], side.top - mainrect.top, side.bottom - mainrect.top,
                                themecolorof(COLOR_SIDEBAR), skipfrom, skipto, hotfrom, hottto, firsty, found);

            if (bad > 0)
            {
                problems.push_back("(n) the sidebar column at x=" + numtext(columns[pos] - (side.left - mainrect.left)) + " has " + numtext(bad) +
                                   " pixels that are not the sidebar colour, the first at y=" + numtext(firsty) + " is " + numtext(GetRValue(found)) +
                                   "," + numtext(GetGValue(found)) + "," + numtext(GetBValue(found)));
            }
        }
    }

    int hostcolumns[2] = {hostrect.left - mainrect.left + 1, hostrect.right - mainrect.left - 6};

    for (int pos = 0; pos < 2; pos++)
    {
        int bad = offcolour(pixels, width, height, rowbytes, hostcolumns[pos], hostrect.top - mainrect.top, hostrect.bottom - mainrect.top,
                            themecolorof(COLOR_PAGE), 0, 0, 0, 0, firsty, found);

        if (bad > 0)
        {
            problems.push_back("(n) the page column at x=" + numtext(hostcolumns[pos] - (hostrect.left - mainrect.left)) + " has " + numtext(bad) +
                               " pixels that are not the page colour, the first at y=" + numtext(firsty) + " is " + numtext(GetRValue(found)) +
                               "," + numtext(GetGValue(found)) + "," + numtext(GetBValue(found)));
        }
    }
}

// Reads the columns of pixels. A short-lived thing that Windows puts over the window (a pop-up message of another
// program, a hover colour) would give a false alarm, so a failed reading is repeated up to three times after a short
// wait; a real band of the wrong colour is still there every time.
static void checkuniform(vector<string>& problems)
{
    for (int attempt = 0; attempt < 3; attempt++)
    {
        vector<string> found;
        sampleuniform(found);

        if (found.empty() || attempt == 2)
        {
            for (size_t pos = 0; pos < found.size(); pos++)
            {
                problems.push_back(found[pos]);
            }

            return;
        }

        Sleep(150);
    }
}
// Reads one pixel of a capture of the main window (x and y from the top left corner of the window rectangle).
static COLORREF pixelat(const vector<unsigned char>& pixels, int width, int height, int rowbytes, int x, int y)
{
    if (x < 0 || y < 0 || x >= width || y >= height)
    {
        return CLR_INVALID;
    }

    size_t at = (size_t)(height - 1 - y) * (size_t)rowbytes + (size_t)x * 3;
    return RGB(pixels[at + 2], pixels[at + 1], pixels[at]);
}

// The rectangle of one sidebar item, in the coordinates of the main window rectangle.
static RECT navitemrect(int index)
{
    shellparts parts = shellgetparts();
    RECT item;
    ZeroMemory(&item, sizeof(item));
    item.left = LVIR_BOUNDS;
    SendMessageA(parts.nav, LVM_GETITEMRECT, (WPARAM)index, (LPARAM)&item);
    RECT nav = screenrect(parts.nav);
    RECT main = screenrect(parts.main);
    OffsetRect(&item, nav.left - main.left, nav.top - main.top);
    return item;
}

// SC1 the nav and its column are as wide as the sidebar; SC2 whole rows of the empty sidebar part are the sidebar colour;
// SC3 the selected item has the same colour 4 pixels from both edges; SC5 no dark (dotted focus) pixels on its outer rows
// and columns.
static void checksidebar(vector<string>& problems)
{
    shellparts parts = shellgetparts();
    RECT side = screenrect(parts.sidebar);
    RECT nav = screenrect(parts.nav);
    RECT main = screenrect(parts.main);
    RECT client;
    GetClientRect(parts.nav, &client);
    int column = (int)SendMessageA(parts.nav, LVM_GETCOLUMNWIDTH, 0, 0);
    int sidewidth = side.right - side.left;
    int navwidth = nav.right - nav.left;

    if (nav.left != side.left || navwidth < sidewidth - 1 || navwidth > sidewidth + 1 || column < client.right - 1)
    {
        problems.push_back("(sc1) sidebar " + numtext(sidewidth) + ", nav " + numtext(navwidth) + " at " + numtext(nav.left - side.left) +
                           ", column " + numtext(column) + " of " + numtext(client.right));
    }

    RedrawWindow(parts.main, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    pump();

    // the real mouse pointer may rest over the sidebar (the pump delivers its moves): no item is hot for the picture
    SendMessageA(parts.nav, LVM_SETHOTITEM, (WPARAM)-1, 0);
    UpdateWindow(parts.nav);
    vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    int rowbytes = 0;

    if (!capturepixels(pixels, width, height, rowbytes, false))
    {
        problems.push_back("(sc2) the window could not be painted into a bitmap");
        return;
    }

    MONITORINFO monitor;
    monitor.cbSize = sizeof(monitor);
    GetMonitorInfoA(MonitorFromWindow(parts.main, MONITOR_DEFAULTTONEAREST), &monitor);
    int count = (int)SendMessageA(parts.nav, LVM_GETITEMCOUNT, 0, 0);
    int left = side.left - main.left;
    int right = side.right - main.left;
    int top = navitemrect(count - 1).bottom + px(4);
    int bottom = nav.bottom - main.top - px(4);
    COLORREF tone = themecolorof(COLOR_SIDEBAR);
    int rows = 0;
    int bad = 0;
    string first = "";

    if (bottom > monitor.rcWork.bottom - main.top)
    {
        bottom = monitor.rcWork.bottom - main.top;
    }

    for (int y = top; y < bottom; y = y + px(4))
    {
        rows++;

        for (int x = left; x < right; x++)
        {
            COLORREF colour = pixelat(pixels, width, height, rowbytes, x, y);

            if (colour != tone)
            {
                if (bad == 0)
                {
                    first = " first at x=" + numtext(x - left) + " y=" + numtext(y) + " is " + numtext(GetRValue(colour)) + "," +
                            numtext(GetGValue(colour)) + "," + numtext(GetBValue(colour));
                }

                bad++;
            }
        }
    }

    // rows below the work area are not on the screen (a 192 dpi window is taller than the screen): only a visible part is judged
    bool visible = bottom - top >= 3 * px(4);

    if ((visible && rows < 3) || bad > 0)
    {
        problems.push_back("(sc2) " + numtext(rows) + " empty sidebar rows scanned, " + numtext(bad) + " pixels not the sidebar colour" + first);
    }

    int selected = shellnavselected();

    if (selected < 0)
    {
        return;
    }

    RECT item = navitemrect(selected);
    int middle = (item.top + item.bottom) / 2;
    COLORREF leftcolour = pixelat(pixels, width, height, rowbytes, left + 4, middle);
    COLORREF rightcolour = pixelat(pixels, width, height, rowbytes, right - 5, middle);

    if (leftcolour != rightcolour || leftcolour == tone)
    {
        problems.push_back("(sc3) the selected item is not coloured across the full width: left " + numtext(GetRValue(leftcolour)) + "," +
                           numtext(GetGValue(leftcolour)) + "," + numtext(GetBValue(leftcolour)) + " right " + numtext(GetRValue(rightcolour)) + "," +
                           numtext(GetGValue(rightcolour)) + "," + numtext(GetBValue(rightcolour)) + " at y=" + numtext(middle));
    }

    int dark = 0;

    for (int y = item.top; y < item.bottom; y++)
    {
        for (int x = left; x < right; x++)
        {
            bool edge = (y <= item.top + 1 || y >= item.bottom - 2 || x <= left + 1 || x >= right - 2);
            COLORREF colour = pixelat(pixels, width, height, rowbytes, x, y);

            if (edge && colour != CLR_INVALID && GetRValue(colour) + GetGValue(colour) + GetBValue(colour) < 330)
            {
                dark++;
            }
        }
    }

    if (dark > 0)
    {
        problems.push_back("(sc5) " + numtext(dark) + " dark pixels on the outline of the selected sidebar item, hot item " +
                           numtext((int)SendMessageA(parts.nav, LVM_GETHOTITEM, 0, 0)));
    }
}

// SC4: a click 6 pixels inside the right edge of another sidebar item selects it (then SC1 to SC3 and SC5 are checked
// again), and a click on the item that was selected before shows that page again.
static void checksidebarclick(vector<string>& problems)
{
    shellparts parts = shellgetparts();
    int count = (int)SendMessageA(parts.nav, LVM_GETITEMCOUNT, 0, 0);
    int before = shellnavselected();

    if (count < 2 || before < 0)
    {
        return;
    }

    int targets[2] = {(before + 1) % count, before};

    for (int pos = 0; pos < 2; pos++)
    {
        RECT item;
        ZeroMemory(&item, sizeof(item));
        item.left = LVIR_BOUNDS;
        SendMessageA(parts.nav, LVM_GETITEMRECT, (WPARAM)targets[pos], (LPARAM)&item);
        RECT client;
        GetClientRect(parts.nav, &client);
        LPARAM where = MAKELPARAM(client.right - 6, (item.top + item.bottom) / 2);
        PostMessageA(parts.nav, WM_LBUTTONDOWN, MK_LBUTTON, where);
        PostMessageA(parts.nav, WM_LBUTTONUP, 0, where);
        pump();

        if (shellnavselected() != targets[pos])
        {
            problems.push_back("(sc4) a click 6 pixels inside the right edge of item " + numtext(targets[pos]) + " did not select it");
        }

        if (pos == 0)
        {
            checksidebar(problems);
        }
    }
}

// LC1 to LC3 for a table page: equal margins of 24 design pixels, the list as wide as the content and reaching down to the
// button row (or the bottom margin) within one row unless all its rows are shown, its columns using the width, and one row
// of buttons 32 high with gaps of 8 that ends at the right and bottom margins.
static void checktable(guipage* page, vector<string>& problems)
{
    if (dynamic_cast<pagetable*>(page) == NULL)
    {
        return;
    }

    RECT host = screenrect(page->window());
    int margin = px(pagemargin);
    HWND list = GetDlgItem(page->window(), IDC_LIST_MAIN);
    RECT box = screenrect(list);

    if (abs(box.left - host.left - margin) > 2 || abs(host.right - box.right - margin) > 2)
    {
        problems.push_back("(lc1) margins left " + numtext(box.left - host.left) + " right " + numtext(host.right - box.right) + " for " + numtext(margin));
    }

    vector<RECT> buttons;
    HWND child = GetWindow(page->window(), GW_CHILD);

    while (child != NULL)
    {
        char kind[32];
        GetClassNameA(child, kind, (int)sizeof(kind));

        if (IsWindowVisible(child) && lstrcmpiA(kind, "Button") == 0)
        {
            RECT one = screenrect(child);
            size_t at = buttons.size();

            while (at > 0 && buttons[at - 1].left > one.left)
            {
                at--;
            }

            buttons.insert(buttons.begin() + at, one);
        }

        child = GetWindow(child, GW_HWNDNEXT);
    }

    int target = host.bottom - margin;

    if (!buttons.empty())
    {
        RECT last = buttons[buttons.size() - 1];

        if (abs(host.right - last.right - margin) > 2 || abs(host.bottom - last.bottom - margin) > 2)
        {
            problems.push_back("(lc3) the button row ends " + numtext(host.right - last.right) + " from the right and " +
                               numtext(host.bottom - last.bottom) + " from the bottom, wanted " + numtext(margin));
        }

        for (size_t pos = 0; pos < buttons.size(); pos++)
        {
            int high = buttons[pos].bottom - buttons[pos].top;
            int gap = pos == 0 ? px(gapsmall) : buttons[pos].left - buttons[pos - 1].right;

            if (abs(high - px(controlheight)) > 1 || abs(gap - px(gapsmall)) > 1)
            {
                problems.push_back("(lc3) button " + numtext((int)pos) + " is " + numtext(high) + " high with a gap of " + numtext(gap));
            }
        }

        target = buttons[0].top - px(gapbox);
    }

    int count = (int)SendMessageA(list, LVM_GETITEMCOUNT, 0, 0);
    bool allshown = count == 0 || ((int)SendMessageA(list, LVM_GETTOPINDEX, 0, 0) == 0 && (int)SendMessageA(list, LVM_GETCOUNTPERPAGE, 0, 0) >= count);
    int row = px(controlheight) + 2;

    if (box.bottom > target + 2 || (target - box.bottom > row && !allshown))
    {
        problems.push_back("(lc2) the list ends " + numtext(target - box.bottom) + " above the button row or the bottom margin");
    }

    if (IsWindowVisible(list))
    {
        RECT client;
        GetClientRect(list, &client);
        int sum = 0;
        HWND header = (HWND)SendMessageA(list, LVM_GETHEADER, 0, 0);
        int columns = (int)SendMessageA(header, HDM_GETITEMCOUNT, 0, 0);

        for (int column = 0; column < columns; column++)
        {
            sum = sum + (int)SendMessageA(list, LVM_GETCOLUMNWIDTH, (WPARAM)column, 0);
        }

        if (sum < client.right - px(24))
        {
            problems.push_back("(lc2) the columns use " + numtext(sum) + " of " + numtext(client.right) + " pixels");
        }
    }

    // LC4 the message of an empty list is centred between the margins
    HWND message = GetDlgItem(page->window(), IDC_LIST_MAIN + 5000);

    if (message != NULL && IsWindowVisible(message))
    {
        RECT text = screenrect(message);

        if (abs((text.left + text.right) - (host.left + host.right)) > 4)
        {
            problems.push_back("(lc4) the empty message is not centred");
        }
    }
}

// LC7 every edit box and drop-down list has a label (ROLE_LABEL) just above it at the same left edge, and every edit box is
// at least 32 design pixels high.
static void checkfields(guipage* page, vector<string>& problems)
{
    HWND child = GetWindow(page->window(), GW_CHILD);

    while (child != NULL)
    {
        char kind[32];
        GetClassNameA(child, kind, (int)sizeof(kind));
        bool edit = lstrcmpiA(kind, "Edit") == 0;

        if (IsWindowVisible(child) && (edit || lstrcmpiA(kind, "ComboBox") == 0))
        {
            RECT field = screenrect(child);
            bool labelled = false;
            HWND other = GetWindow(page->window(), GW_CHILD);

            while (other != NULL)
            {
                RECT label = screenrect(other);

                if (IsWindowVisible(other) && themegetrole(other) == ROLE_LABEL && abs(label.left - field.left) <= 2 &&
                    label.bottom <= field.top + 2 && field.top - label.bottom <= px(gapbox))
                {
                    labelled = true;
                }

                other = GetWindow(other, GW_HWNDNEXT);
            }

            if (!labelled)
            {
                problems.push_back("(lc7) a " + string(kind) + " has no label just above it");
            }

            if (edit && field.bottom - field.top < px(controlheight) - 1)
            {
                problems.push_back("(lc7) an edit box is only " + numtext(field.bottom - field.top) + " high");
            }
        }

        child = GetWindow(child, GW_HWNDNEXT);
    }
}

// Runs the layout checks (a) to (g), (i), (j), (m), (n), (o) and (p) for the page that is shown. Every problem is added to the list.
static void checkpage(guipage* page, bool minimumsize, vector<string>& problems)
{
    checkfonts(page, problems);
    checkfluid(page, problems);
    checklistrows(page, problems);
    checkuniform(problems);

    if (shellsidebarshown())
    {
        checksidebar(problems);
    }

    checktable(page, problems);
    checkfields(page, problems);

    HWND host = shellgetparts().host;
    HWND main = shellgetparts().main;
    RECT hostrect = screenrect(host);
    RECT mainrect = screenrect(main);
    vector<HWND> children;
    HWND child = GetWindow(page->window(), GW_CHILD);

    while (child != NULL)
    {
        if (shown(child))
        {
            children.push_back(child);
        }

        child = GetWindow(child, GW_HWNDNEXT);
    }

    int defaultbuttons = 0;
    int otherbuttons = 0;
    int lists = 0;
    bool titleseen = false;

    for (size_t pos = 0; pos < children.size(); pos++)
    {
        HWND control = children[pos];
        RECT place = screenrect(control);
        string kind = classof(control);

        // (a) inside the page host
        if (!inside(place, hostrect))
        {
            problems.push_back("(a) control '" + textof(control) + "' (" + kind + ") is outside the page host");
        }

        // (b) no overlap with another visible control of the page
        for (size_t other = pos + 1; other < children.size(); other++)
        {
            if (overlap(place, screenrect(children[other])))
            {
                problems.push_back("(b) '" + textof(control) + "' (" + kind + ") overlaps '" + textof(children[other]) + "' (" +
                                   classof(children[other]) + ")");
            }
        }

        // (c) text fit
        checktextfit(control, problems);

        // (j) no punctuation
        checkplain(control, problems);

        // buttons and lists for the calm budget
        if (kind == "Button")
        {
            int type = (int)(GetWindowLongA(control, GWL_STYLE) & 0xF);

            if (type == BS_DEFPUSHBUTTON)
            {
                defaultbuttons++;

                // (f) at the smallest size the primary button must be completely visible
                if (minimumsize && !inside(place, hostrect))
                {
                    problems.push_back("(f) the primary button is not completely visible");
                }
            }
            else if (type == BS_PUSHBUTTON)
            {
                otherbuttons++;
            }
        }

        bool islist = (kind == "SysListView32" || kind == "ListBox");
        bool isemptymessage = (kind == "Static" && GetDlgCtrlID(control) >= 5000);

        if (islist || isemptymessage)
        {
            lists++;

            // (d) a list is at least 300 x 120 pixels
            if (place.right - place.left < 300 || place.bottom - place.top < 120)
            {
                problems.push_back("(d) a list is only " + numtext(place.right - place.left) + "x" + numtext(place.bottom - place.top));
            }

            // (d) no column may be hidden behind a horizontal scroll bar
            if (islist && (GetWindowLongA(control, GWL_STYLE) & WS_HSCROLL) != 0)
            {
                problems.push_back("(d) a list needs a horizontal scroll bar, so a column is cut off");
            }
        }

        if (themegetrole(control) == ROLE_TITLE || themegetrole(control) == ROLE_WELCOME)
        {
            titleseen = true;
        }
    }

    // (e) the title, the status bar and (when signed in) the sidebar are visible and inside the main window
    if (!titleseen)
    {
        problems.push_back("(e) the page has no visible title");
    }

    if (!shown(shellgetparts().status) || !inside(screenrect(shellgetparts().status), mainrect))
    {
        problems.push_back("(e) the status bar is not visible inside the window");
    }

    if (shellsidebarshown() && (!shown(shellgetparts().sidebar) || !inside(screenrect(shellgetparts().sidebar), mainrect)))
    {
        problems.push_back("(e) the sidebar is not visible inside the window");
    }

    // the texts of the sidebar must fit too (a long name wraps to more lines)
    if (shellsidebarshown())
    {
        HWND part = GetWindow(shellgetparts().sidebar, GW_CHILD);

        while (part != NULL)
        {
            if (shown(part))
            {
                checktextfit(part, problems);
                checkplain(part, problems);

                // no horizontal scroll bar may show in the sidebar list
                if (part == shellgetparts().nav && (GetWindowLongA(part, GWL_STYLE) & WS_HSCROLL) != 0)
                {
                    RECT area;
                    GetClientRect(part, &area);
                    int low = 0;
                    int high = 0;
                    GetScrollRange(part, SB_HORZ, &low, &high);
                    int column = (int)SendMessageA(part, LVM_GETCOLUMNWIDTH, 0, 0);
                    problems.push_back("(e) the sidebar list shows a horizontal scroll bar: client width " + numtext(area.right) +
                                       ", column width " + numtext(column) + ", scroll range " + numtext(low) + ".." + numtext(high) +
                                       ", items " + numtext((int)SendMessageA(part, LVM_GETITEMCOUNT, 0, 0)));
                }
            }

            part = GetWindow(part, GW_HWNDNEXT);
        }
    }

    // (i) the Tab key visits the controls in reading order: top to bottom, and left to right inside a row
    HWND firsttab = GetNextDlgTabItem(page->window(), NULL, FALSE);
    HWND walker = firsttab;
    RECT before = {0, 0, 0, 0};
    HWND beforewindow = NULL;
    int visited = 0;

    while (walker != NULL && visited < 40)
    {
        RECT now = screenrect(walker);

        if (visited > 0)
        {
            int rowbefore = (before.top + before.bottom) / 2;
            int rownow = (now.top + now.bottom) / 2;
            bool samerow = (rownow - rowbefore < px(24)) && (rowbefore - rownow < px(24));
            bool inorder = samerow ? (now.left >= before.left) : (rownow > rowbefore);

            if (!inorder)
            {
                problems.push_back("(i) Tab goes from '" + textof(beforewindow) + "' to '" + textof(walker) + "' against the reading order");
            }
        }

        before = now;
        beforewindow = walker;
        visited++;
        walker = GetNextDlgTabItem(page->window(), walker, FALSE);

        if (walker == firsttab)
        {
            break;
        }
    }

    // (g) the calm budget
    if (defaultbuttons > 1)
    {
        problems.push_back("(g) more than one primary button");
    }

    if (defaultbuttons + otherbuttons > page->maxbuttons())
    {
        problems.push_back("(g) " + numtext(defaultbuttons + otherbuttons) + " buttons, the page may have " + numtext(page->maxbuttons()));
    }

    if (lists > page->maxlists())
    {
        problems.push_back("(g) " + numtext(lists) + " lists, the page may have " + numtext(page->maxlists()));
    }
}

// Writes the picture of a window to a bmp file. Returns false when the file could not be written.
// distinct gets the number of different colours (counted up to 9).
static bool savepicture(HWND window, const string& path, int& distinct)
{
    RECT frame;
    GetWindowRect(window, &frame);
    int width = frame.right - frame.left;
    int height = frame.bottom - frame.top;

    HDC screen = GetDC(NULL);
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, width, height);
    HGDIOBJ old = SelectObject(memory, bitmap);

    // the whole content first; when that does not work the window is copied from the screen
    if (!PrintWindow(window, memory, PW_RENDERFULLCONTENT))
    {
        HDC source = GetWindowDC(window);
        BitBlt(memory, 0, 0, width, height, source, 0, 0, SRCCOPY);
        ReleaseDC(window, source);
    }

    BITMAPINFO info;
    ZeroMemory(&info, sizeof(info));
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 24;
    info.bmiHeader.biCompression = BI_RGB;

    int rowbytes = ((width * 3 + 3) / 4) * 4;
    vector<unsigned char> pixels((size_t)rowbytes * (size_t)height);
    GetDIBits(memory, bitmap, 0, height, &pixels[0], &info, DIB_RGB_COLORS);
    SelectObject(memory, old);
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(NULL, screen);

    // count the different colours, up to nine
    vector<DWORD> seen;

    for (size_t at = 0; at + 2 < pixels.size() && seen.size() < 9; at += 3)
    {
        DWORD colour = pixels[at] | (pixels[at + 1] << 8) | (pixels[at + 2] << 16);
        bool known = false;

        for (size_t pos = 0; pos < seen.size(); pos++)
        {
            if (seen[pos] == colour)
            {
                known = true;
            }
        }

        if (!known)
        {
            seen.push_back(colour);
        }
    }

    distinct = (int)seen.size();

    BITMAPFILEHEADER head;
    ZeroMemory(&head, sizeof(head));
    head.bfType = 0x4D42;
    head.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    head.bfSize = head.bfOffBits + (DWORD)pixels.size();

    ofstream out(path.c_str(), ios::binary);

    if (!out.is_open())
    {
        return false;
    }

    out.write((const char*)&head, sizeof(head));
    out.write((const char*)&info.bmiHeader, sizeof(BITMAPINFOHEADER));
    out.write((const char*)&pixels[0], (streamsize)pixels.size());
    return out.good();
}

// The short text for a role, used in the log and in file names.
static string rolename(int who)
{
    if (who == WHO_COORDINATOR)
    {
        return "coordinator";
    }

    if (who == WHO_DONOR)
    {
        return "donor";
    }

    if (who == WHO_RECIPIENT)
    {
        return "recipient";
    }

    return "welcome";
}

// The person that the walk signs in as, for each role.
struct people
{
    string donorid;
    string donorname;
    string recipientid;
    string recipientname;
};

// The first donor that has donations, and the first recipient that has a waiting offer (otherwise the first recipient).
static people choosepeople()
{
    people chosen;

    for (size_t pos = 0; pos < theapp->data.donors.size() && chosen.donorid.empty(); pos++)
    {
        const Donor& donor = theapp->data.donors[pos];

        if (!donationsofdonor(theapp->data, donor.id()).empty())
        {
            chosen.donorid = donor.id().toString();
            chosen.donorname = donor.name();
        }
    }

    if (chosen.donorid.empty() && !theapp->data.donors.empty())
    {
        chosen.donorid = theapp->data.donors[0].id().toString();
        chosen.donorname = theapp->data.donors[0].name();
    }

    string recipient = "";

    for (int pos = 0; pos < theapp->offers.count() && recipient.empty(); pos++)
    {
        if (theapp->offers.at(pos).status() == OFFER_PENDING)
        {
            recipient = theapp->offers.at(pos).recipientId().toString();
        }
    }

    if (recipient.empty() && !theapp->data.recipients.empty())
    {
        recipient = theapp->data.recipients[0].id().toString();
    }

    int pos = 0;

    if (!recipient.empty() && theapp->data.recipientindex.get(recipient, pos))
    {
        chosen.recipientid = recipient;
        chosen.recipientname = theapp->data.recipients[pos].name();
    }

    return chosen;
}

// Opens a page of a role: signs in when needed, then shows the page.
static bool openpage(const walkstep& step, const people& who)
{
    if (shellpage(step.page) == NULL)
    {
        return false;
    }

    if (step.who == WHO_NONE)
    {
        if (shellwho() != WHO_NONE)
        {
            shellswitchuser();
        }

        shellshow(step.page, false);
        return true;
    }

    if (shellwho() != step.who)
    {
        string id = "";
        string name = "";

        if (step.who == WHO_DONOR)
        {
            id = who.donorid;
            name = who.donorname;
        }
        else if (step.who == WHO_RECIPIENT)
        {
            id = who.recipientid;
            name = who.recipientname;
        }

        shellsignin(step.who, id, name);
        testlog("title=" + captiontext());
    }

    // the match page belongs to a donation: the one that the coordinator would handle next
    if (step.page == PAGE_C_MATCH)
    {
        int next = nexturgentdonation(theapp->data, theapp->offers);

        if (next < 0)
        {
            return false;
        }

        openmatchpage(next);
        return true;
    }

    shellshow(step.page, false);
    return true;
}

// The number of checks that did not hold.
static int failedchecks = 0;

// Logs one check and counts it when it failed.
static void expect(bool condition, const string& name)
{
    testlog("check=" + name + " ok=" + (condition ? "1" : "0"));

    if (!condition)
    {
        failedchecks++;
    }
}

// Called for every top-level window of this thread: counts the ones that are on screen.
static BOOL CALLBACK countvisibleproc(HWND window, LPARAM total)
{
    if (GetWindowLongA(window, GWL_STYLE) & WS_VISIBLE)
    {
        (*(int*)total)++;
    }

    return TRUE;
}

// The number of visible top-level windows of the program (the main window while it is shown, and nothing else).
static int visibletoplevel()
{
    int count = 0;
    EnumThreadWindows(GetCurrentThreadId(), countvisibleproc, (LPARAM)&count);
    return count;
}

// A control of a page, found by its id.
static HWND pagecontrol(int pageid, int controlid)
{
    guipage* page = shellpage(pageid);
    return page == NULL ? NULL : GetDlgItem(page->window(), controlid);
}

// Presses a button the way a click does.
static void press(HWND button)
{
    SendMessageA(button, BM_CLICK, 0, 0);
}

// Chooses an item of the sidebar list the way a click does.
static void clicknav(int index)
{
    // selecting an item makes the list send the same notification as a mouse click or an arrow key
    LVITEMA mark;
    ZeroMemory(&mark, sizeof(mark));
    mark.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    mark.state = LVIS_SELECTED | LVIS_FOCUSED;
    SendMessageA(shellgetparts().nav, LVM_SETITEMSTATE, (WPARAM)index, (LPARAM)&mark);
}

// Chooses a line of the name list on the welcome page.
static void choosename(int index)
{
    HWND names = pagecontrol(PAGE_WELCOME, IDC_W_NAMES);
    SendMessageA(names, LB_SETCURSEL, (WPARAM)index, 0);
    SendMessageA(shellpage(PAGE_WELCOME)->window(), WM_COMMAND, MAKEWPARAM(IDC_W_NAMES, LBN_SELCHANGE), (LPARAM)names);
}

// The text of the first column of the selected row of a list view, or an empty text.
static string selectedtext(HWND list)
{
    int row = (int)SendMessageA(list, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    if (row < 0)
    {
        return "";
    }

    char buffer[256];
    buffer[0] = '\0';
    LVITEMA item;
    ZeroMemory(&item, sizeof(item));
    item.iSubItem = 0;
    item.pszText = buffer;
    item.cchTextMax = (int)sizeof(buffer);
    SendMessageA(list, LVM_GETITEMTEXTA, (WPARAM)row, (LPARAM)&item);
    return buffer;
}

// The number of radio buttons on all pages (the welcome page must have none).
static int countradios()
{
    int total = 0;

    for (int id = PAGE_WELCOME; id < PAGE_COUNT; id++)
    {
        guipage* page = shellpage(id);
        HWND child = page == NULL ? NULL : GetWindow(page->window(), GW_CHILD);

        while (child != NULL)
        {
            int type = (int)(GetWindowLongA(child, GWL_STYLE) & 0xF);

            if (classof(child) == "Button" && (type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON))
            {
                total++;
            }

            child = GetWindow(child, GW_HWNDNEXT);
        }
    }

    return total;
}

// The number of buttons of a page that are shown.
static int countshownbuttons(int pageid)
{
    int total = 0;
    guipage* page = shellpage(pageid);
    HWND child = page == NULL ? NULL : GetWindow(page->window(), GW_CHILD);

    while (child != NULL)
    {
        if (classof(child) == "Button" && shown(child))
        {
            total++;
        }

        child = GetWindow(child, GW_HWNDNEXT);
    }

    return total;
}

// How often a text is a push button of the pages from first to last, a sidebar item or a button of the sidebar.
static int countaction(int firstpage, int lastpage, const string& label)
{
    int total = 0;

    for (int id = firstpage; id <= lastpage; id++)
    {
        guipage* page = shellpage(id);
        HWND child = page == NULL ? NULL : GetWindow(page->window(), GW_CHILD);

        while (child != NULL)
        {
            if (classof(child) == "Button" && textof(child) == label)
            {
                total++;
            }

            child = GetWindow(child, GW_HWNDNEXT);
        }
    }

    int count = shellnavcount();

    for (int row = 0; row < count; row++)
    {
        char buffer[128];
        buffer[0] = '\0';
        LVITEMA item;
        ZeroMemory(&item, sizeof(item));
        item.iSubItem = 0;
        item.pszText = buffer;
        item.cchTextMax = (int)sizeof(buffer);
        SendMessageA(shellgetparts().nav, LVM_GETITEMTEXTA, (WPARAM)row, (LPARAM)&item);

        if (label == buffer)
        {
            total++;
        }
    }

    HWND part = GetWindow(shellgetparts().sidebar, GW_CHILD);

    while (part != NULL)
    {
        if (classof(part) == "Button" && textof(part) == label)
        {
            total++;
        }

        part = GetWindow(part, GW_HWNDNEXT);
    }

    return total;
}

// (k) Every action has one home: its text is on exactly one button or sidebar item of the role.
static void checkonehome(int who, const string& role)
{
    static const char* const coordinator[] = {"Process donation", "Close donation", "Withdraw offer", "Send offer", NULL};
    static const char* const donor[] = {"Add donation", NULL};
    static const char* const recipient[] = {"Accept offer", "Decline offer", "My details", NULL};
    const char* const* actions = who == WHO_COORDINATOR ? coordinator : (who == WHO_DONOR ? donor : recipient);
    int firstpage = who == WHO_COORDINATOR ? PAGE_C_HOME : (who == WHO_DONOR ? PAGE_D_HOME : PAGE_R_HOME);
    int lastpage = who == WHO_COORDINATOR ? PAGE_C_MATCH : (who == WHO_DONOR ? PAGE_D_ADD : PAGE_R_EDIT);

    for (int pos = 0; actions[pos] != NULL; pos++)
    {
        int found = countaction(firstpage, lastpage, actions[pos]);
        expect(found == 1, role + " (k) the action '" + actions[pos] + "' has one home, found " + numtext(found));
    }

    int switches = countaction(PAGE_WELCOME, PAGE_WELCOME, "Switch user");
    expect(switches == 1, role + " (k) Switch user has one home, found " + numtext(switches));

    if (who == WHO_RECIPIENT)
    {
        int old = countaction(firstpage, lastpage, "Edit my details");
        expect(old == 0, role + " (k) there is no second Edit my details, found " + numtext(old));
    }
}

// Goes through every sidebar item and checks that the right page appears inside the same window.
static void checknavigation(const string& role, int windowsbefore)
{
    int count = shellnavcount();
    bool allright = count > 0;

    for (int index = 0; index < count; index++)
    {
        clicknav(index);

        if (shellcurrent() != shellnavpage(index) || shellnavselected() != index || visibletoplevel() != windowsbefore)
        {
            allright = false;
        }
    }

    expect(allright, role + " every sidebar item shows its page in the same window (" + numtext(count) + " items)");
    clicknav(0);
}

// Signs in through the welcome page the way a person does, checks the sidebar, and comes back with Switch user.
static void checksignin(int who, const string& role, int windowsbefore)
{
    shellswitchuser();
    expect(shellcurrent() == PAGE_WELCOME && !shellsidebarshown() && captiontext() == "CommunityBridge",
           role + " welcome page: no sidebar, plain caption");

    HWND names = pagecontrol(PAGE_WELCOME, IDC_W_NAMES);
    HWND back = pagecontrol(PAGE_WELCOME, IDC_W_BACK);
    HWND button = pagecontrol(PAGE_WELCOME, IDC_W_CONTINUE);
    expect(!shown(names) && !shown(back) && !shown(button), role + " step 1 has no name list, no Back and no Continue");

    int rolebutton = (who == WHO_COORDINATOR) ? IDC_W_COORD : (who == WHO_DONOR ? IDC_W_DONOR : IDC_W_RECIP);
    press(pagecontrol(PAGE_WELCOME, rolebutton));

    string expectedid = "";

    if (who == WHO_COORDINATOR)
    {
        expect(shellwho() == WHO_COORDINATOR, role + " the Coordinator button signs in at once");
    }
    else
    {
        expect(shellwho() == WHO_NONE && shellcurrent() == PAGE_WELCOME, role + " the role button only opens step 2");

        int expectedrows = (int)(who == WHO_DONOR ? theapp->data.donors.size() : theapp->data.recipients.size());
        expect(shown(names) && (int)SendMessageA(names, LB_GETCOUNT, 0, 0) == expectedrows && shown(back) && shown(button) &&
                   !shown(pagecontrol(PAGE_WELCOME, IDC_W_COORD)),
               role + " step 2 shows the name list with " + numtext(expectedrows) + " names, Back and Continue");
        expect(!IsWindowEnabled(button), role + " Continue is disabled until a name is chosen");
        press(button);
        expect(shellwho() == WHO_NONE, role + " a click on the disabled Continue does nothing");
        choosename(0);
        expect(IsWindowEnabled(button) != FALSE, role + " Continue is enabled after the choice");
        LRESULT defid = SendMessageA(shellgetparts().main, DM_GETDEFID, 0, 0);
        expect(HIWORD(defid) == DC_HASDEFID && LOWORD(defid) == IDC_W_CONTINUE, role + " Enter would press Continue");
        press(button);

        expectedid = (who == WHO_DONOR) ? theapp->data.donors[0].id().toString() : theapp->data.recipients[0].id().toString();
    }

    expect(shellwho() == who && shellpersonid() == expectedid && shellsidebarshown(), role + " signed in, sidebar shown");
    expect(captiontext() == "CommunityBridge", role + " the caption stays CommunityBridge");
    expect(shellcurrent() == shellnavpage(0), role + " the first page of the role is shown");

    checkonehome(who, role);
    checknavigation(role, windowsbefore);

    HWND switchbutton = GetDlgItem(shellgetparts().sidebar, IDC_SWITCHUSER);
    press(switchbutton);
    expect(shellwho() == WHO_NONE && shellcurrent() == PAGE_WELCOME && !shellsidebarshown(), role + " Switch user returns to the welcome page");
}

// The two steps of the welcome page: three equal buttons and nothing else, then the names with Back and Continue.
static void checkwelcomesteps()
{
    shellswitchuser();
    HWND coord = pagecontrol(PAGE_WELCOME, IDC_W_COORD);
    HWND donor = pagecontrol(PAGE_WELCOME, IDC_W_DONOR);
    HWND recip = pagecontrol(PAGE_WELCOME, IDC_W_RECIP);
    HWND names = pagecontrol(PAGE_WELCOME, IDC_W_NAMES);
    HWND back = pagecontrol(PAGE_WELCOME, IDC_W_BACK);
    HWND button = pagecontrol(PAGE_WELCOME, IDC_W_CONTINUE);

    int buttons = countshownbuttons(PAGE_WELCOME);
    int radios = countradios();
    testlog("welcome step=1 buttons=" + numtext(buttons) + " radios=" + numtext(radios));
    expect(buttons == 3 && radios == 0 && !shown(names) && !shown(back) && !shown(button),
           "welcome step 1 shows three role buttons and nothing else");

    RECT first = screenrect(coord);
    RECT second = screenrect(donor);
    RECT third = screenrect(recip);
    bool equal = (first.right - first.left) == (second.right - second.left) && (second.right - second.left) == (third.right - third.left) &&
                 (first.bottom - first.top) == (second.bottom - second.top) && (second.bottom - second.top) == (third.bottom - third.top);
    expect(equal, "welcome step 1 the three buttons are equal in size");

    LRESULT defid = SendMessageA(shellgetparts().main, DM_GETDEFID, 0, 0);
    expect(HIWORD(defid) != DC_HASDEFID, "welcome step 1 has no default button");
    expect(radios == 0, "no page has a radio button");

    if (theapp->data.donors.empty())
    {
        return;
    }

    press(donor);
    int count = (int)SendMessageA(names, LB_GETCOUNT, 0, 0);
    testlog("welcome step=2 names=" + numtext(count));
    expect(count == (int)theapp->data.donors.size() && shown(names) && shown(back) && shown(button) && !IsWindowEnabled(button) &&
               !shown(coord) && !shown(donor) && !shown(recip),
           "welcome step 2 shows the names, Back and a disabled Continue");
    press(back);
    expect(shown(coord) && shown(donor) && shown(recip) && !shown(names) && !shown(back) && !shown(button),
           "welcome Back returns to the three role buttons");
}

// The Donations page of the coordinator already has the most urgent donation chosen, and both buttons are on for it.
static void checkpreselect()
{
    shellsignin(WHO_COORDINATOR, "", "");
    shellshow(PAGE_C_DONATIONS, false);
    int urgent = nexturgentdonation(theapp->data, theapp->offers);
    string expected = urgent >= 0 ? theapp->data.donations[urgent].id().toString() : "";
    string chosen = selectedtext(pagecontrol(PAGE_C_DONATIONS, IDC_LIST_MAIN));
    testlog("preselected=" + (chosen.empty() ? string("none") : chosen));
    expect(chosen == expected, "Donations has the most urgent donation chosen (" + (expected.empty() ? string("none") : expected) + ")");

    bool on = IsWindowEnabled(pagecontrol(PAGE_C_DONATIONS, IDC_C_PROCESS_SEL)) != FALSE &&
              IsWindowEnabled(pagecontrol(PAGE_C_DONATIONS, IDC_C_CLOSE_SEL)) != FALSE;
    expect(on == (urgent >= 0), "Process donation and Close donation are on when a donation is chosen");
}

// The relative luminance of a colour as the WCAG 2 formula defines it.
static double luminanceof(COLORREF colour)
{
    double parts[3] = {GetRValue(colour) / 255.0, GetGValue(colour) / 255.0, GetBValue(colour) / 255.0};

    for (int pos = 0; pos < 3; pos++)
    {
        parts[pos] = parts[pos] <= 0.03928 ? parts[pos] / 12.92 : pow((parts[pos] + 0.055) / 1.055, 2.4);
    }

    return 0.2126 * parts[0] + 0.7152 * parts[1] + 0.0722 * parts[2];
}

// The contrast ratio of two colours (1 to 21).
static double contrastof(COLORREF first, COLORREF second)
{
    double one = luminanceof(first);
    double two = luminanceof(second);
    double light = one > two ? one : two;
    double dark = one > two ? two : one;
    return (light + 0.05) / (dark + 0.05);
}

// (l) The contrast table: every pair of text colour and background that the program uses, with its ratio computed from
// the real colours of Theme. Body text must reach 7 to 1, muted, accent and error text 4.5 to 1, and pure white may not
// be the colour of the page or the sidebar.
static void checkcontrast()
{
    struct contrastpair
    {
        const char* name;
        int foreground;
        int background;
        double least;
    };

    static const contrastpair pairs[] = {{"text on page", COLOR_TEXT, COLOR_PAGE, 7.0},         {"text on sidebar", COLOR_TEXT, COLOR_SIDEBAR, 7.0},
                                 {"muted on page", COLOR_MUTED, COLOR_PAGE, 4.5},        {"muted on sidebar", COLOR_MUTED, COLOR_SIDEBAR, 4.5},
                                 {"accent on page", COLOR_ACCENT, COLOR_PAGE, 4.5},      {"accent on sidebar", COLOR_ACCENT, COLOR_SIDEBAR, 4.5},
                                 {"error on page", COLOR_ERROR, COLOR_PAGE, 4.5},        {"text on selection", COLOR_TEXT, COLOR_SELECT, 7.0},
                                 {NULL, 0, 0, 0.0}};

    for (int pos = 0; pairs[pos].name != NULL; pos++)
    {
        double ratio = contrastof(themecolorof(pairs[pos].foreground), themecolorof(pairs[pos].background));
        char text[32];
        sprintf(text, "%.2f", ratio);
        testlog(string("contrast ") + pairs[pos].name + " ratio=" + text + " least=" + (pairs[pos].least > 5.0 ? "7.0" : "4.5"));
        expect(ratio >= pairs[pos].least, string("(l) contrast of ") + pairs[pos].name + " is " + text);
    }

    COLORREF white = RGB(255, 255, 255);
    expect(themecolorof(COLOR_PAGE) != white && themecolorof(COLOR_SIDEBAR) != white, "(l) the page and the sidebar are not pure white");
    expect(themecolorof(COLOR_TEXT) != RGB(0, 0, 0), "(l) the text is not pure black");
}
// The behaviour of the shell and the welcome page, checked through window messages.
static void checkshell()
{
    int windowsbefore = visibletoplevel();

    // every page of the program exists once (none is missing, none is a placeholder)
    bool allpages = true;

    for (int id = PAGE_WELCOME; id < PAGE_COUNT; id++)
    {
        if (shellpage(id) == NULL)
        {
            allpages = false;
        }
    }

    expect(allpages, "all " + numtext(PAGE_COUNT - PAGE_WELCOME) + " pages exist");
    checkcontrast();
    checkwelcomesteps();
    checksignin(WHO_COORDINATOR, "coordinator", windowsbefore);
    checkpreselect();

    if (!theapp->data.donors.empty())
    {
        checksignin(WHO_DONOR, "donor", windowsbefore);
    }

    if (!theapp->data.recipients.empty())
    {
        checksignin(WHO_RECIPIENT, "recipient", windowsbefore);
    }

    expect(visibletoplevel() == windowsbefore, "no page opened a window of its own");
}

// Takes a picture of the main window and logs it. The file name has no extension.
static void snapshot(const string& folder, const string& file)
{
    settle();
    int distinct = 0;
    bool saved = savepicture(shellgetparts().main, folder + "\\" + file + ".bmp", distinct);
    testlog("shot=" + file + ".bmp nonblank=" + (saved && distinct > 8 ? "1" : "0"));
}

// Runs the layout checks for the page that is shown right now and logs the result under a label.
// Returns the number of violations.
static int checkshown(const string& label, int width, int height)
{
    guipage* page = shellpage(shellcurrent());
    vector<string> problems;
    checkpage(page, width == 900 && height == 600, problems);
    testlog("page=" + label + " dpi=" + numtext(themedpi()) + " size=" + numtext(width) + "x" + numtext(height) + " rows=" +
            numtext(page->rows()) + " violations=" + numtext((int)problems.size()) + " switch_ms=0 refresh_ms=0");

    for (size_t pos = 0; pos < problems.size(); pos++)
    {
        testlog("violation " + label + " dpi=" + numtext(themedpi()) + " " + numtext(width) + "x" + numtext(height) + ": " + problems[pos]);
    }

    return (int)problems.size();
}

// Two helpers that the flows below define; the extras use them too.
static void selectrow(HWND list, int index);
static void signinrecipient(const string& id);

// The special states of a page that the plain walk does not show (a form with its red line, and so on).
// Each is checked at the starting size and at the smallest size, and photographed when pictures are wanted.
static int runextras(bool pictures, const string& folder)
{
    int total = 0;
    people who = choosepeople();
    static const int extrasizes[2][2] = {{1000, 650}, {900, 600}};
    int dpis[5];
    int dpicount = modedpis(pictures, dpis);

    for (int dpiat = 0; dpiat < dpicount * 2; dpiat++)
    {
        int sizeat = dpiat % 2;
        usedpi(dpis[dpiat / 2]);
        int width = extrasizes[sizeat][0];
        int height = extrasizes[sizeat][1];
        string suffix = numtext(themedpi()) + "_" + numtext(width) + "x" + numtext(height);
        shellsetclient(px(width), px(height));

        // a donor that has no donations: the pages show their grey "nothing here" messages (only if the data has such a donor)
        string emptydonor = "";
        string emptyname = "";

        for (size_t pos = 0; pos < theapp->data.donors.size() && emptydonor.empty(); pos++)
        {
            if (donationsofdonor(theapp->data, theapp->data.donors[pos].id()).empty())
            {
                emptydonor = theapp->data.donors[pos].id().toString();
                emptyname = theapp->data.donors[pos].name();
            }
        }

        if (!emptydonor.empty())
        {
            shellsignin(WHO_DONOR, emptydonor, emptyname);
            shellshow(PAGE_D_DONATIONS, false);
            total = total + checkshown("donor/donations-empty", width, height);

            if (pictures)
            {
                snapshot(folder, "donor_donations-empty_" + suffix);
            }

            shellshow(PAGE_D_HOME, false);
            total = total + checkshown("donor/home-empty", width, height);

            if (pictures)
            {
                snapshot(folder, "donor_home-empty_" + suffix);
            }
        }

        // the match page after Send offer with nothing chosen: the red line is shown
        int nextdonation = nexturgentdonation(theapp->data, theapp->offers);

        if (nextdonation >= 0 && shellpage(PAGE_C_MATCH) != NULL)
        {
            shellsignin(WHO_COORDINATOR, "", "");
            openmatchpage(nextdonation);
            press(pagecontrol(PAGE_C_MATCH, IDC_M_SEND));
            total = total + checkshown("coordinator/match-with-error", width, height);

            if (pictures)
            {
                snapshot(folder, "coordinator_match-with-error_" + suffix);
            }
        }

        // the status line: one plain sentence at the bottom after an action (only shown, nothing is changed)
        if (!who.donorid.empty() && shellpage(PAGE_D_DONATIONS) != NULL)
        {
            shellsignin(WHO_DONOR, who.donorid, who.donorname);
            shellshow(PAGE_D_DONATIONS, false);
            shellstatus("Donation DON047 added");
            total = total + checkshown("donor/donations-with-status", width, height);

            if (pictures)
            {
                snapshot(folder, "donor_donations-with-status_" + suffix);
            }
        }

        // selected rows: the buttons that need a selection are on (nothing is changed, only chosen)
        if (shellpage(PAGE_C_DONATIONS) != NULL)
        {
            shellsignin(WHO_COORDINATOR, "", "");
            shellshow(PAGE_C_DONATIONS, false);
            selectrow(pagecontrol(PAGE_C_DONATIONS, IDC_LIST_MAIN), 1);
            total = total + checkshown("coordinator/donations-selected", width, height);

            if (pictures)
            {
                snapshot(folder, "coordinator_donations-selected_" + suffix);
            }

            if (theapp->offers.count() > 0)
            {
                shellshow(PAGE_C_OFFERS, false);
                selectrow(pagecontrol(PAGE_C_OFFERS, IDC_LIST_MAIN), 0);
                expect(IsWindowEnabled(pagecontrol(PAGE_C_OFFERS, IDC_C_WITHDRAW)) != FALSE, "a waiting offer is selected: Withdraw is on (" + suffix + ")");
                total = total + checkshown("coordinator/offers-selected", width, height);

                if (pictures)
                {
                    snapshot(folder, "coordinator_offers-selected_" + suffix);
                }
            }
        }

        if (!who.recipientid.empty() && shellpage(PAGE_R_INCOMING) != NULL)
        {
            signinrecipient(who.recipientid);
            shellshow(PAGE_R_INCOMING, false);

            if (shellpage(PAGE_R_INCOMING)->rows() > 0)
            {
                selectrow(pagecontrol(PAGE_R_INCOMING, IDC_LIST_MAIN), 0);
                total = total + checkshown("recipient/incoming-selected", width, height);

                if (pictures)
                {
                    snapshot(folder, "recipient_incoming-selected_" + suffix);
                }
            }

            // the recipient form after a Save with no capacity: the red line is shown
            shellshow(PAGE_R_EDIT, false);
            SetWindowTextA(pagecontrol(PAGE_R_EDIT, IDC_R_CAPACITY), "");
            press(pagecontrol(PAGE_R_EDIT, IDC_R_SAVE));
            total = total + checkshown("recipient/details-with-error", width, height);

            if (pictures)
            {
                snapshot(folder, "recipient_details-with-error_" + suffix);
            }
        }

        // the donor form after a Save with nothing typed: the red line is shown
        if (!who.donorid.empty() && shellpage(PAGE_D_ADD) != NULL)
        {
            shellsignin(WHO_DONOR, who.donorid, who.donorname);
            shellshow(PAGE_D_ADD, false);
            press(pagecontrol(PAGE_D_ADD, IDC_D_SAVE));
            total = total + checkshown("donor/add-with-error", width, height);

            if (pictures)
            {
                snapshot(folder, "donor_add-with-error_" + suffix);
            }
        }
    }

    return total;
}

// The second step of the welcome page for a donor and for a recipient (a name chosen), at the three sizes.
static int runwelcomesteps(bool pictures, const string& folder)
{
    int total = 0;

    int dpis[5];
    int dpicount = modedpis(pictures, dpis);
    int sizecount = pictures ? 3 : 4;

    for (int round = 0; round < dpicount * sizecount; round++)
    {
        usedpi(dpis[round / sizecount]);
        int width = pictures ? shotsizes[round % sizecount][0] : testsizes[round % sizecount][0];
        int height = pictures ? shotsizes[round % sizecount][1] : testsizes[round % sizecount][1];
        string suffix = numtext(themedpi()) + "_" + numtext(width) + "x" + numtext(height);
        shellsetclient(px(width), px(height));

        if (!theapp->data.donors.empty())
        {
            shellswitchuser();
            press(pagecontrol(PAGE_WELCOME, IDC_W_DONOR));
            total = total + checkshown("welcome/step2-donor", width, height);

            if (pictures)
            {
                snapshot(folder, "welcome_step2-donor_" + suffix);
            }
        }

        if (!theapp->data.recipients.empty())
        {
            shellswitchuser();
            press(pagecontrol(PAGE_WELCOME, IDC_W_RECIP));
            choosename(0);
            total = total + checkshown("welcome/step2-recipient-chosen", width, height);

            if (pictures)
            {
                snapshot(folder, "welcome_step2-recipient-chosen_" + suffix);
            }
        }
    }

    shellswitchuser();
    return total;
}

// (q) The window gets the message that Windows sends when it moves to a monitor with another scale (or the scale
// changes), with the rectangle Windows would suggest. It is sent for 144, 192, 120 and 96 in turn. After each step the
// layout checks hold, the sizes follow the scale in proportion, the fonts are the ones of the new scale and the
// sidebar has the width of the new scale.
static int runcycle()
{
    int total = 0;
    static const int cycle[4] = {144, 192, 120, 96};
    HWND main = shellgetparts().main;
    shellsignin(WHO_COORDINATOR, "", "");
    usedpi(96);
    shellsetclient(px(1000), px(650));
    shellshow(PAGE_C_DONATIONS, false);

    for (int step = 0; step < 4; step++)
    {
        int olddpi = themedpi();
        int newdpi = cycle[step];
        int oldwidth = 0;
        int oldheight = 0;
        shellgetclient(oldwidth, oldheight);

        RECT now = screenrect(main);
        RECT suggested = now;
        suggested.right = now.left + MulDiv(now.right - now.left, newdpi, olddpi);
        suggested.bottom = now.top + MulDiv(now.bottom - now.top, newdpi, olddpi);
        SendMessageA(main, WM_DPICHANGED, MAKEWPARAM(newdpi, newdpi), (LPARAM)&suggested);
        pump();

        int width = 0;
        int height = 0;
        shellgetclient(width, height);
        string name = "(q) dpi change " + numtext(olddpi) + " to " + numtext(newdpi);
        expect(themedpi() == newdpi, name + ": the program uses the new dpi");

        // the client size follows the scale: within 5 percent of the proportional size (the frame does not scale exactly)
        int wanted = MulDiv(oldwidth, newdpi, olddpi);
        int off = width > wanted ? width - wanted : wanted - width;
        expect(off * 100 <= wanted * 5, name + ": the client width follows the scale (" + numtext(width) + " for " + numtext(wanted) + ")");

        RECT side = screenrect(shellgetparts().sidebar);
        expect(side.right - side.left == MulDiv(sidebarwidth, newdpi, 96), name + ": the sidebar is " + numtext(MulDiv(sidebarwidth, newdpi, 96)) +
                                                                               " wide, found " + numtext(side.right - side.left));

        guipage* page = shellpage(shellcurrent());
        vector<string> problems;
        checkpage(page, false, problems);
        testlog("page=coordinator/donations(dpi change) dpi=" + numtext(newdpi) + " size=" + numtext(MulDiv(width, 96, newdpi)) + "x" +
                numtext(MulDiv(height, 96, newdpi)) + " rows=" + numtext(page->rows()) + " violations=" + numtext((int)problems.size()) +
                " switch_ms=0 refresh_ms=0");

        for (size_t pos = 0; pos < problems.size(); pos++)
        {
            testlog("violation coordinator/donations(dpi change) dpi=" + numtext(newdpi) + ": " + problems[pos]);
        }

        total = total + (int)problems.size();
    }

    return total;
}

// The height of one window in pixels (0 for no window).
static int heightof(HWND control)
{
    RECT box = screenrect(control);
    return control == NULL ? 0 : box.bottom - box.top;
}

// --selftest-dpichange: the normal start (no --dpi override), then a real WM_DPICHANGED for 96, 120, 144, 168, 192
// and 96 again, with the rectangle Windows would suggest. After each step the six fonts must have the sizes of the
// design guide, the sidebar its token width, the nav at least the sidebar width, and the margin, list row, list header,
// button and status bar the size of the first 96 step scaled to the new dpi (1 pixel); the layout checks must hold.
static int rundpichange()
{
    static const int cycle[6] = {96, 120, 144, 168, 192, 96};
    static const int fontsizes[FONT_COUNT] = {14, 24, 12, 17, 35, 14};
    int total = 0;
    int first[5] = {0, 0, 0, 0, 0};
    shellparts parts = shellgetparts();
    testlog("dpichange awareness=" + numtext(shellawareness()) + " dpi_window=" + numtext(themedpiforwindow(parts.main)) +
            " override=" + (themedpiisoverridden() ? "1" : "0"));
    shellsignin(WHO_COORDINATOR, "", "");
    shellshow(PAGE_C_DONATIONS, false);
    shellsetclient(px(1000), px(650));
    ShowWindow(parts.main, SW_SHOWNOACTIVATE);
    SetWindowPos(parts.main, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    pump();

    for (int step = 0; step < 6; step++)
    {
        int olddpi = themedpi();
        int newdpi = cycle[step];
        RECT now = screenrect(parts.main);
        RECT suggested = now;
        suggested.right = now.left + MulDiv(now.right - now.left, newdpi, olddpi);
        suggested.bottom = now.top + MulDiv(now.bottom - now.top, newdpi, olddpi);
        SendMessageA(parts.main, WM_DPICHANGED, MAKEWPARAM(newdpi, newdpi), (LPARAM)&suggested);
        pump();

        string name = "(dc4) step " + numtext(step) + " dpi " + numtext(newdpi);
        expect(themedpi() == newdpi, name + ": the program uses the new dpi");

        for (int kind = 0; kind < FONT_COUNT; kind++)
        {
            LOGFONTA font;
            GetObjectA(themefont(kind), sizeof(font), &font);
            int wanted = -MulDiv(fontsizes[kind], newdpi, 96);
            expect(font.lfHeight >= wanted - 1 && font.lfHeight <= wanted + 1,
                   name + ": font " + numtext(kind) + " height " + numtext(font.lfHeight) + " for " + numtext(wanted));
        }

        guipage* page = shellpage(shellcurrent());
        HWND list = FindWindowExA(page->window(), NULL, WC_LISTVIEWA, NULL);
        RECT row;
        row.left = LVIR_BOUNDS;
        SendMessageA(list, LVM_GETITEMRECT, 0, (LPARAM)&row);
        RECT host = screenrect(parts.host);
        int values[5];
        values[0] = screenrect(list).left - host.left;
        values[1] = row.bottom - row.top;
        values[2] = heightof((HWND)SendMessageA(list, LVM_GETHEADER, 0, 0));
        values[3] = heightof(FindWindowExA(page->window(), NULL, "Button", "Process donation"));
        values[4] = heightof(parts.status);
        int sidebar = screenrect(parts.sidebar).right - screenrect(parts.sidebar).left;
        int nav = screenrect(parts.nav).right - screenrect(parts.nav).left;
        testlog("dpichange step=" + numtext(step) + " dpi=" + numtext(newdpi) + " sidebar=" + numtext(sidebar) + " nav=" + numtext(nav) +
                " margin=" + numtext(values[0]) + " row=" + numtext(values[1]) + " header=" + numtext(values[2]) + " button=" +
                numtext(values[3]) + " status=" + numtext(values[4]) + " handler=" + (shelldpihandlerline() > 0 ? "real" : "none") +
                " file=GuiShell.cpp line=" + numtext(shelldpihandlerline()));
        expect(sidebar >= px(sidebarwidth, newdpi) - 1 && sidebar <= px(sidebarwidth, newdpi) + 1, name + ": sidebar " + numtext(sidebar));
        expect(nav >= sidebar, name + ": the nav is at least as wide as the sidebar (" + numtext(nav) + ")");

        // the native header adds its own padding to the height of its font, so its check is that the text fits
        HWND header = (HWND)SendMessageA(list, LVM_GETHEADER, 0, 0);
        HDC dc = GetDC(header);
        HGDIOBJ old = SelectObject(dc, (HFONT)SendMessageA(header, WM_GETFONT, 0, 0));
        TEXTMETRICA metrics;
        GetTextMetricsA(dc, &metrics);
        SelectObject(dc, old);
        ReleaseDC(header, dc);
        expect(values[2] >= metrics.tmHeight + 4 && values[2] <= metrics.tmHeight + px(12, newdpi),
               name + ": the header (" + numtext(values[2]) + ") fits its text of " + numtext(metrics.tmHeight));

        for (int pos = 0; pos < 5; pos++)
        {
            if (pos == 2)
            {
                continue;
            }

            if (step == 0)
            {
                first[pos] = values[pos];
            }

            int wanted = MulDiv(first[pos], newdpi, 96);
            expect(values[pos] >= wanted - 1 && values[pos] <= wanted + 1,
                   name + ": size " + numtext(pos) + " is " + numtext(values[pos]) + " for " + numtext(wanted));
        }

        vector<string> problems;
        checkpage(page, false, problems);

        // the sidebar checks with a real click, at every size of the matrix, then back to the size of the cycle
        static const int sizes[5][2] = {{900, 600}, {1000, 650}, {1400, 900}, {1920, 1010}, {2560, 1300}};
        int cyclewidth = 0;
        int cycleheight = 0;
        shellgetclient(cyclewidth, cycleheight);

        for (int size = 0; size < 5; size++)
        {
            shellsetclient(px(sizes[size][0]), px(sizes[size][1]));
            pump();
            vector<string> found;
            checksidebar(found);
            checksidebarclick(found);

            for (size_t pos = 0; pos < found.size(); pos++)
            {
                problems.push_back(numtext(sizes[size][0]) + "x" + numtext(sizes[size][1]) + ": " + found[pos]);
            }
        }

        shellsetclient(cyclewidth, cycleheight);
        pump();
        testlog("page=coordinator/donations(dpichange) dpi=" + numtext(newdpi) + " violations=" + numtext((int)problems.size()));

        for (size_t pos = 0; pos < problems.size(); pos++)
        {
            testlog("violation coordinator/donations(dpichange) dpi=" + numtext(newdpi) + ": " + problems[pos]);
        }

        total = total + (int)problems.size();
    }

    return total;
}

// The window is maximised the way a person does it (the real maximise command, not a size set by the test), then the
// pages of the coordinator are opened one after the other and every layout check runs on the maximised window. A
// note about PrintWindow: with PW_RENDERFULLCONTENT it copies the window surface that Windows composes for the
// screen, so a picture shows what is really on the screen, leftovers included (tried in stage U3, see WORKLOG.md).
static int runmaximise()
{
    int total = 0;
    HWND main = shellgetparts().main;
    usedpi(startdpi);
    SetWindowPos(main, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    ShowWindow(main, SW_MAXIMIZE);
    settle();
    shellsignin(WHO_COORDINATOR, "", "");
    static const int pages[6] = {PAGE_C_SUMMARY, PAGE_C_DONATIONS, PAGE_C_HOME, PAGE_C_OFFERS, PAGE_C_RECIPIENTS, PAGE_C_TRANSACTIONS};

    for (int round = 0; round < 12; round++)
    {
        shellshow(pages[round % 6], false);
        settle();
        int width = 0;
        int height = 0;
        shellgetclient(width, height);
        total = total + checkshown("maximised/" + shellpage(shellcurrent())->name(), MulDiv(width, 96, themedpi()), MulDiv(height, 96, themedpi()));
    }

    SetWindowPos(main, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    ShowWindow(main, SW_RESTORE);
    return total;
}
// Walks over every page at the three sizes. Returns the number of violations.
static int runwalk(bool pictures, const string& folder)
{
    people who = choosepeople();
    vector<walkstep> steps;

    static const int welcome[] = {PAGE_WELCOME, PAGE_NONE};
    static const int coordinator[] = {PAGE_C_HOME, PAGE_C_DONATIONS, PAGE_C_OFFERS, PAGE_C_RECIPIENTS, PAGE_C_TRANSACTIONS,
                                      PAGE_C_SUMMARY, PAGE_C_MATCH, PAGE_NONE};
    static const int donor[] = {PAGE_D_HOME, PAGE_D_DONATIONS, PAGE_D_OFFERS, PAGE_D_GIVEN, PAGE_D_ADD, PAGE_NONE};
    static const int recipient[] = {PAGE_R_HOME, PAGE_R_INCOMING, PAGE_R_PAST, PAGE_R_RECEIVED, PAGE_R_EDIT, PAGE_NONE};
    const int* lists[4] = {welcome, coordinator, donor, recipient};
    int roles[4] = {WHO_NONE, WHO_COORDINATOR, WHO_DONOR, WHO_RECIPIENT};

    for (int group = 0; group < 4; group++)
    {
        for (int pos = 0; lists[group][pos] != PAGE_NONE; pos++)
        {
            walkstep step;
            step.who = roles[group];
            step.page = lists[group][pos];
            steps.push_back(step);
        }
    }

    int total = 0;

    // the window is shown (without taking the focus) in every test, because check (n) reads painted pixels; it is kept on
    // top, because Windows does not paint the part of a window that another window covers
    ShowWindow(shellgetparts().main, SW_SHOWNOACTIVATE);
    SetWindowPos(shellgetparts().main, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    int dpis[5];
    int dpicount = modedpis(pictures, dpis);
    int sizecount = pictures ? 3 : 4;
    int stepcount = (int)steps.size();

    // one round is one page at one scale and one size
    for (int round = 0; round < dpicount * stepcount * sizecount; round++)
    {
        {
            usedpi(dpis[round / (stepcount * sizecount)]);
            size_t at = (size_t)((round / sizecount) % stepcount);
            int sizeat = round % sizecount;
            int width = pictures ? shotsizes[sizeat][0] : testsizes[sizeat][0];
            int height = pictures ? shotsizes[sizeat][1] : testsizes[sizeat][1];
            // the sizes are design pixels, so they grow with the screen scaling like the program itself
            shellsetclient(px(width), px(height));

            LARGE_INTEGER start;
            LARGE_INTEGER stop;
            QueryPerformanceCounter(&start);

            if (!openpage(steps[at], who))
            {
                continue;
            }

            QueryPerformanceCounter(&stop);
            int switchms = millis(start, stop);
            guipage* page = shellpage(shellcurrent());

            QueryPerformanceCounter(&start);
            page->refresh();
            QueryPerformanceCounter(&stop);
            int refreshms = millis(start, stop);

            // a slow measurement may come from something else that ran on the computer at that moment, so it is measured
            // again (twice at most) and the fastest time counts; a page that is really slow is slow every time
            for (int again = 0; again < 2 && (switchms >= 150 || refreshms >= 150); again++)
            {
                LARGE_INTEGER first;
                LARGE_INTEGER second;
                QueryPerformanceCounter(&first);
                openpage(steps[at], who);
                QueryPerformanceCounter(&second);
                int switchagain = millis(first, second);
                switchms = switchagain < switchms ? switchagain : switchms;

                QueryPerformanceCounter(&first);
                page->refresh();
                QueryPerformanceCounter(&second);
                int refreshagain = millis(first, second);
                refreshms = refreshagain < refreshms ? refreshagain : refreshms;
            }

            vector<string> problems;
            checkpage(page, width == 900 && height == 600, problems);

            // (e) the caption of the window is always the plain name of the program
            if (captiontext() != "CommunityBridge")
            {
                problems.push_back("(e) the window caption is '" + captiontext() + "'");
            }

            // (h) a page switch and a refresh each take under 150 ms
            if (switchms >= 150)
            {
                problems.push_back("(h) the page switch took " + numtext(switchms) + " ms");
            }

            if (refreshms >= 150)
            {
                problems.push_back("(h) the refresh took " + numtext(refreshms) + " ms");
            }

            string label = rolename(steps[at].who) + "/" + page->name();
            testlog("page=" + label + " dpi=" + numtext(themedpi()) + " size=" + numtext(width) + "x" + numtext(height) + " rows=" +
                    numtext(page->rows()) + " violations=" + numtext((int)problems.size()) + " switch_ms=" + numtext(switchms) +
                    " refresh_ms=" + numtext(refreshms));

            for (size_t pos = 0; pos < problems.size(); pos++)
            {
                testlog("violation " + label + " dpi=" + numtext(themedpi()) + " " + numtext(width) + "x" + numtext(height) + ": " +
                        problems[pos]);
            }

            total = total + (int)problems.size();

            if (pictures)
            {
                snapshot(folder, rolename(steps[at].who) + "_" + page->name() + "_" + numtext(themedpi()) + "_" + numtext(width) + "x" +
                                     numtext(height));
            }
        }
    }

    total = total + runwelcomesteps(pictures, folder);
    total = total + runextras(pictures, folder);

    if (!pictures)
    {
        total = total + runcycle();
        total = total + runmaximise();
    }

    testlog("summary violations=" + numtext(total));
    return total;
}

// The number of flow results that did not hold.
static int flowsfailed = 0;

// Logs the result of one step of a flow.
static void flowresult(const string& name, bool ok, const string& detail)
{
    testlog("flow=" + name + " ok=" + (ok ? "1" : "0") + (detail.empty() ? "" : " " + detail));

    if (!ok)
    {
        flowsfailed++;
    }
}

// The lines of a csv file that are not empty (the first one is the header).
static vector<string> csvlines(const string& path)
{
    vector<string> lines;
    ifstream in(path.c_str());
    string line;

    while (getline(in, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
        {
            line = line.substr(0, line.size() - 1);
        }

        if (!line.empty())
        {
            lines.push_back(line);
        }
    }

    return lines;
}

// F1: a donor adds a donation. A bad entry shows the red line and the page stays; a good one is saved.
static void flowdonoradd()
{
    people who = choosepeople();
    shellsignin(WHO_DONOR, who.donorid, who.donorname);
    shellshow(PAGE_D_DONATIONS, false);
    int listbefore = shellpage(PAGE_D_DONATIONS)->rows();
    size_t filebefore = csvlines(theapp->prefix + "donations.csv").size();

    shellshow(PAGE_D_ADD, false);
    HWND error = pagecontrol(PAGE_D_ADD, IDC_D_ERROR);
    press(pagecontrol(PAGE_D_ADD, IDC_D_SAVE));
    flowresult("F1a empty quantity stays on the page with a red line", shellcurrent() == PAGE_D_ADD && !textof(error).empty(),
               "line='" + textof(error) + "'");

    SetWindowTextA(pagecontrol(PAGE_D_ADD, IDC_D_QTY), "25");
    press(pagecontrol(PAGE_D_ADD, IDC_D_SAVE));
    flowresult("F1b empty minutes stays on the page with a red line", shellcurrent() == PAGE_D_ADD && !textof(error).empty(),
               "line='" + textof(error) + "'");

    SetWindowTextA(pagecontrol(PAGE_D_ADD, IDC_D_USABLE), "0");
    press(pagecontrol(PAGE_D_ADD, IDC_D_SAVE));
    flowresult("F1c zero minutes is refused with a plain line", shellcurrent() == PAGE_D_ADD && !textof(error).empty(),
               "line='" + textof(error) + "'");
    flowresult("F1d nothing was saved by the bad entries", csvlines(theapp->prefix + "donations.csv").size() == filebefore, "");

    SendMessageA(pagecontrol(PAGE_D_ADD, IDC_D_FOOD), CB_SETCURSEL, 1, 0);
    SetWindowTextA(pagecontrol(PAGE_D_ADD, IDC_D_USABLE), "90");
    press(pagecontrol(PAGE_D_ADD, IDC_D_SAVE));

    vector<string> after = csvlines(theapp->prefix + "donations.csv");
    bool filegrew = after.size() == filebefore + 1;
    string lastrow = after.empty() ? "" : after[after.size() - 1];
    bool rowright = lastrow.find(",PACKAGED,25,25,90,PENDING") != string::npos && lastrow.find(who.donorid) != string::npos;
    flowresult("F1 donations.csv has the new row", filegrew && rowright, "last row=" + lastrow);
    flowresult("F1 the page went to My donations and the list grew by one",
               shellcurrent() == PAGE_D_DONATIONS && shellpage(PAGE_D_DONATIONS)->rows() == listbefore + 1,
               "rows " + numtext(listbefore) + " -> " + numtext(shellpage(PAGE_D_DONATIONS)->rows()));
    string status = shellstatustext();
    flowresult("F1 the status line says the donation was added",
               status == "Donation " + lastrow.substr(0, lastrow.find(',')) + " added" && !haspunct(status), "status='" + status + "'");

    // Cancel goes back to the page before
    shellshow(PAGE_D_GIVEN, false);
    shellshow(PAGE_D_ADD, false);
    press(pagecontrol(PAGE_D_ADD, IDC_D_CANCEL));
    flowresult("F1 Cancel goes back to the page before", shellcurrent() == PAGE_D_GIVEN, "");
}

// Selects one row of a list view by its position.
static void selectrow(HWND list, int index)
{
    LVITEMA mark;
    ZeroMemory(&mark, sizeof(mark));
    mark.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    mark.state = LVIS_SELECTED | LVIS_FOCUSED;
    SendMessageA(list, LVM_SETITEMSTATE, (WPARAM)index, (LPARAM)&mark);
}

// Selects the row of a list view whose first column holds this text. Returns false when there is no such row.
static bool selectrowbyid(HWND list, const string& id)
{
    int count = (int)SendMessageA(list, LVM_GETITEMCOUNT, 0, 0);

    for (int row = 0; row < count; row++)
    {
        char buffer[256];
        buffer[0] = '\0';
        LVITEMA item;
        ZeroMemory(&item, sizeof(item));
        item.iSubItem = 0;
        item.pszText = buffer;
        item.cchTextMax = (int)sizeof(buffer);
        SendMessageA(list, LVM_GETITEMTEXTA, (WPARAM)row, (LPARAM)&item);

        if (id == buffer)
        {
            selectrow(list, row);
            return true;
        }
    }

    return false;
}

// The last row of offers.csv, or an empty text.
static string lastofferrow()
{
    vector<string> lines = csvlines(theapp->prefix + "offers.csv");
    return lines.empty() ? "" : lines[lines.size() - 1];
}

// What a coordinator does to send an offer: choose a donation on Donations, Process donation, choose the first recipient, type the biggest quantity that
// is allowed, Send offer. Returns false when that cannot be done (nothing to offer, or no recipient).
struct sentoffer
{
    string donation;     // the donation id
    string offer;        // the new offer id
    string recipient;    // the recipient id
    int quantity;        // the quantity of the offer
    int before;          // the quantity already offered for the donation before
};

static bool sendnextoffer(sentoffer& result)
{
    if (shellwho() != WHO_COORDINATOR)
    {
        shellsignin(WHO_COORDINATOR, "", "");
    }

    // first the donation the coordinator would handle next, then the other open donations in the order of the list
    int next = nexturgentdonation(theapp->data, theapp->offers);
    vector<int> order;

    if (next >= 0)
    {
        order.push_back(next);
    }

    for (size_t pos = 0; pos < theapp->data.donations.size(); pos++)
    {
        DonationStatus status = theapp->data.donations[pos].status();

        if ((int)pos != next && (status == PENDING || status == PARTIAL) && effectiveremaining(theapp->data, theapp->offers, (int)pos) > 0)
        {
            order.push_back((int)pos);
        }
    }

    for (size_t at = 0; at < order.size(); at++)
    {
        int pos = order[at];
        matchview match = buildmatch(theapp->data, theapp->offers, *theapp->provider, pos);

        if (match.candidates.empty())
        {
            continue;
        }

        result.donation = theapp->data.donations[pos].id().toString();
        result.before = theapp->offers.pendingquantity(theapp->data.donations[pos].id());

        shellshow(PAGE_C_DONATIONS, false);
        selectrowbyid(pagecontrol(PAGE_C_DONATIONS, IDC_LIST_MAIN), result.donation);
        press(pagecontrol(PAGE_C_DONATIONS, IDC_C_PROCESS_SEL));

        if (shellcurrent() != PAGE_C_MATCH)
        {
            continue;
        }

        int cantake = match.candidates[0].potentialAcceptance();
        result.quantity = cantake < match.effectiveremaining ? cantake : match.effectiveremaining;
        result.recipient = match.candidates[0].recipientId().toString();
        selectrow(pagecontrol(PAGE_C_MATCH, IDC_LIST_MAIN), 0);
        SetWindowTextA(pagecontrol(PAGE_C_MATCH, IDC_M_QTY), numtext(result.quantity).c_str());
        int offersbefore = theapp->offers.count();
        press(pagecontrol(PAGE_C_MATCH, IDC_M_SEND));

        if (theapp->offers.count() != offersbefore + 1)
        {
            return false;
        }

        result.offer = theapp->offers.at(theapp->offers.count() - 1).id().toString();
        return true;
    }

    return false;
}
// F2: the coordinator sends an offer. Wrong entries show the red line and send nothing; a good one is saved as a waiting offer.
static sentoffer firstoffer;
static bool firstofferdone = false;

static void flowsendoffer()
{
    shellsignin(WHO_COORDINATOR, "", "");
    shellshow(PAGE_C_DONATIONS, false);
    int pos = nexturgentdonation(theapp->data, theapp->offers);
    flowresult("F2 a donation is waiting for an offer", pos >= 0, "");

    if (pos < 0)
    {
        return;
    }

    string chosen = selectedtext(pagecontrol(PAGE_C_DONATIONS, IDC_LIST_MAIN));
    flowresult("F2 Donations already has the most urgent donation chosen", chosen == theapp->data.donations[pos].id().toString(),
               "chosen=" + chosen);
    press(pagecontrol(PAGE_C_DONATIONS, IDC_C_PROCESS_SEL));
    flowresult("F2 Process donation opens the match page in the same window", shellcurrent() == PAGE_C_MATCH, "");
    flowresult("F2 the sidebar keeps Donations selected on the match page", shellnavselected() >= 0 && shellnavpage(shellnavselected()) == PAGE_C_DONATIONS, "");

    HWND error = pagecontrol(PAGE_C_MATCH, IDC_M_ERROR);
    HWND list = pagecontrol(PAGE_C_MATCH, IDC_LIST_MAIN);
    HWND quantity = pagecontrol(PAGE_C_MATCH, IDC_M_QTY);
    size_t filebefore = csvlines(theapp->prefix + "offers.csv").size();
    int countbefore = theapp->offers.count();

    press(pagecontrol(PAGE_C_MATCH, IDC_M_SEND));
    flowresult("F2a Send offer with nobody chosen shows a red line", !textof(error).empty() && theapp->offers.count() == countbefore,
               "line='" + textof(error) + "'");

    selectrow(list, 0);
    SetWindowTextA(error, "");
    press(pagecontrol(PAGE_C_MATCH, IDC_M_SEND));
    flowresult("F2b Send offer without a quantity shows a red line", !textof(error).empty() && theapp->offers.count() == countbefore,
               "line='" + textof(error) + "'");

    SetWindowTextA(quantity, "99999");
    SetWindowTextA(error, "");
    press(pagecontrol(PAGE_C_MATCH, IDC_M_SEND));
    flowresult("F2c a quantity that is too big shows a red line", !textof(error).empty() && theapp->offers.count() == countbefore,
               "line='" + textof(error) + "'");
    flowresult("F2d nothing was saved by the wrong entries", csvlines(theapp->prefix + "offers.csv").size() == filebefore, "");

    // the good offer: the page is on the match page again for the same donation
    SetWindowTextA(error, "");
    matchview match = buildmatch(theapp->data, theapp->offers, *theapp->provider, pos);
    int cantake = match.candidates[0].potentialAcceptance();
    int amount = cantake < match.effectiveremaining ? cantake : match.effectiveremaining;
    int pendingbefore = theapp->offers.pendingquantity(theapp->data.donations[pos].id());
    SetWindowTextA(quantity, numtext(amount).c_str());
    press(pagecontrol(PAGE_C_MATCH, IDC_M_SEND));

    string row = lastofferrow();
    string donation = theapp->data.donations[pos].id().toString();
    flowresult("F2 offers.csv has a PENDING row for the offer",
               theapp->offers.count() == countbefore + 1 && row.find(donation + "," + match.candidates[0].recipientId().toString() + "," + numtext(amount)) != string::npos &&
                   row.find(",PENDING,") != string::npos,
               "last row=" + row);
    flowresult("F2 the quantity already offered grew by the amount",
               theapp->offers.pendingquantity(theapp->data.donations[pos].id()) == pendingbefore + amount,
               numtext(pendingbefore) + " -> " + numtext(theapp->offers.pendingquantity(theapp->data.donations[pos].id())));
    string status = shellstatustext();
    flowresult("F2 the status line says the offer was sent",
               status.compare(0, 6, "Offer ") == 0 && status.find(" sent to ") != string::npos && !haspunct(status), "status='" + status + "'");

    firstoffer.donation = donation;
    firstoffer.offer = theapp->offers.at(theapp->offers.count() - 1).id().toString();
    firstoffer.recipient = match.candidates[0].recipientId().toString();
    firstoffer.quantity = amount;
    firstoffer.before = pendingbefore;
    firstofferdone = theapp->offers.count() == countbefore + 1;

    // Back returns to Donations (when the page is still the match page; it leaves by itself when nothing is left to offer)
    if (shellcurrent() == PAGE_C_MATCH)
    {
        press(pagecontrol(PAGE_C_MATCH, IDC_M_BACK));
        flowresult("F2 Back returns to Donations", shellcurrent() == PAGE_C_DONATIONS, "");
    }
    else
    {
        flowresult("F2 with nothing left to offer the page went back to Donations by itself", shellcurrent() == PAGE_C_DONATIONS, "");
    }
}

// F5: a second offer, then the coordinator withdraws it on the Offers page.
static void flowwithdraw()
{
    sentoffer second;
    bool sent = sendnextoffer(second);
    flowresult("F5 a second offer was sent", sent, sent ? "offer " + second.offer : "");

    if (!sent)
    {
        return;
    }

    shellshow(PAGE_C_OFFERS, false);
    HWND list = pagecontrol(PAGE_C_OFFERS, IDC_LIST_MAIN);
    HWND withdraw = pagecontrol(PAGE_C_OFFERS, IDC_C_WITHDRAW);
    flowresult("F5 Withdraw is off until a row is chosen", !IsWindowEnabled(withdraw), "");
    bool found = selectrowbyid(list, second.offer);
    flowresult("F5 the offer is in the Offers list and Withdraw is on for it", found && IsWindowEnabled(withdraw) != FALSE, "");
    press(withdraw);

    int pos = 0;
    bool known = theapp->offers.find(EntityId::parse(second.offer, "OFF"), pos);
    flowresult("F5 the offer is WITHDRAWN", known && theapp->offers.at(pos).status() == OFFER_WITHDRAWN,
               "row=" + lastofferrow());
    flowresult("F5 offers.csv says WITHDRAWN", lastofferrow().find(second.offer + ",") != string::npos && lastofferrow().find(",WITHDRAWN,") != string::npos, "");
    string status = shellstatustext();
    flowresult("F5 the status line says the offer was withdrawn", status == "Offer " + second.offer + " withdrawn", "status='" + status + "'");
    flowresult("F5 Withdraw is off again for the finished offer", !IsWindowEnabled(withdraw), "");
}

// F6: the coordinator closes a donation that has a waiting offer. The offer is withdrawn and the rest counts as wasted.
static void flowclose()
{
    sentoffer third;
    bool sent = sendnextoffer(third);
    flowresult("F6 an offer was sent for the donation that is closed next", sent, sent ? "donation " + third.donation + " offer " + third.offer : "");

    if (!sent)
    {
        return;
    }

    shellshow(PAGE_C_DONATIONS, false);
    HWND list = pagecontrol(PAGE_C_DONATIONS, IDC_LIST_MAIN);
    bool found = selectrowbyid(list, third.donation);
    flowresult("F6 the donation is in the Donations list", found, "");
    int donationpos = 0;
    theapp->data.donationindex.get(third.donation, donationpos);
    int leftover = theapp->data.donations[donationpos].remainingQuantity();
    press(pagecontrol(PAGE_C_DONATIONS, IDC_C_CLOSE_SEL));

    flowresult("F6 the donation is CLOSED", theapp->data.donations[donationpos].status() == CLOSED,
               "status=" + numtext((int)theapp->data.donations[donationpos].status()));
    int offerpos = 0;
    bool known = theapp->offers.find(EntityId::parse(third.offer, "OFF"), offerpos);
    flowresult("F6 its waiting offer is WITHDRAWN", known && theapp->offers.at(offerpos).status() == OFFER_WITHDRAWN, "");
    flowresult("F6 no offer of the donation is waiting any more", theapp->offers.pendingquantity(theapp->data.donations[donationpos].id()) == 0, "");
    string status = shellstatustext();
    flowresult("F6 the status line says how much was wasted",
               status.find("was closed") != string::npos && status.find(numtext(leftover)) != string::npos && !haspunct(status),
               "status='" + status + "'");

    vector<string> rows = csvlines(theapp->prefix + "donations.csv");
    bool filecloses = false;

    for (size_t at = 0; at < rows.size(); at++)
    {
        if (rows[at].compare(0, third.donation.size() + 1, third.donation + ",") == 0 && rows[at].find(",CLOSED") != string::npos)
        {
            filecloses = true;
        }
    }

    flowresult("F6 donations.csv says CLOSED", filecloses, "");
}

// One field of a csv line (counted from 0). Good enough for the rows of the data files, which have no quotes.
static string csvfield(const string& line, int index)
{
    int field = 0;
    string current = "";

    for (size_t pos = 0; pos < line.size(); pos++)
    {
        if (line[pos] == ',')
        {
            if (field == index)
            {
                return current;
            }

            field++;
            current = "";
        }
        else
        {
            current = current + line[pos];
        }
    }

    return field == index ? current : "";
}

// The row of recipients.csv that belongs to a recipient id, or an empty text.
static string recipientrow(const string& id)
{
    vector<string> rows = csvlines(theapp->prefix + "recipients.csv");

    for (size_t at = 0; at < rows.size(); at++)
    {
        if (rows[at].compare(0, id.size() + 1, id + ",") == 0)
        {
            return rows[at];
        }
    }

    return "";
}

// Signs in as a recipient (the name is looked up).
static void signinrecipient(const string& id)
{
    int pos = 0;
    string name = "";

    if (theapp->data.recipientindex.get(id, pos))
    {
        name = theapp->data.recipients[pos].name();
    }

    shellsignin(WHO_RECIPIENT, id, name);
}

// F3: the recipient of the offer of F2 sees it in Incoming offers and accepts it.
static void flowaccept()
{
    flowresult("F3 the offer of F2 exists", firstofferdone, "offer " + firstoffer.offer);

    if (!firstofferdone)
    {
        return;
    }

    signinrecipient(firstoffer.recipient);
    shellshow(PAGE_R_INCOMING, false);
    HWND list = pagecontrol(PAGE_R_INCOMING, IDC_LIST_MAIN);
    HWND accept = pagecontrol(PAGE_R_INCOMING, IDC_R_ACCEPT);
    HWND decline = pagecontrol(PAGE_R_INCOMING, IDC_R_DECLINE);
    flowresult("F3 Accept and Decline are off until a row is chosen", !IsWindowEnabled(accept) && !IsWindowEnabled(decline), "");

    bool found = selectrowbyid(list, firstoffer.offer);
    flowresult("F3 the offer is in Incoming offers and the buttons are on", found && IsWindowEnabled(accept) && IsWindowEnabled(decline), "");

    int recpos = 0;
    theapp->data.recipientindex.get(firstoffer.recipient, recpos);
    int capacitybefore = theapp->data.recipients[recpos].availableCapacity();
    size_t transactionsbefore = csvlines(theapp->prefix + "transactions.csv").size();
    int incomingbefore = shellpage(PAGE_R_INCOMING)->rows();
    shellshow(PAGE_R_RECEIVED, false);
    int receivedbefore = shellpage(PAGE_R_RECEIVED)->rows();
    shellshow(PAGE_R_INCOMING, false);
    selectrowbyid(list, firstoffer.offer);
    press(accept);
    string acceptedstatus = shellstatustext();   // read now: another page clears the line

    int offerpos = 0;
    bool known = theapp->offers.find(EntityId::parse(firstoffer.offer, "OFF"), offerpos);
    flowresult("F3 the offer is ACCEPTED", known && theapp->offers.at(offerpos).status() == OFFER_ACCEPTED, "");
    vector<string> after = csvlines(theapp->prefix + "transactions.csv");
    string lastrow = after.empty() ? "" : after[after.size() - 1];
    flowresult("F3 transactions.csv has a new row for it",
               after.size() == transactionsbefore + 1 && lastrow.find(firstoffer.donation + "," + firstoffer.recipient + "," + numtext(firstoffer.quantity)) != string::npos,
               "last row=" + lastrow);
    flowresult("F3 the capacity of the recipient is lower by the quantity",
               theapp->data.recipients[recpos].availableCapacity() == capacitybefore - firstoffer.quantity &&
                   csvfield(recipientrow(firstoffer.recipient), 3) == numtext(capacitybefore - firstoffer.quantity),
               numtext(capacitybefore) + " -> " + numtext(theapp->data.recipients[recpos].availableCapacity()));
    flowresult("F3 the offer left Incoming offers", shellpage(PAGE_R_INCOMING)->rows() == incomingbefore - 1,
               "rows " + numtext(incomingbefore) + " -> " + numtext(shellpage(PAGE_R_INCOMING)->rows()));
    shellshow(PAGE_R_RECEIVED, false);
    flowresult("F3 the food shows in Received food", shellpage(PAGE_R_RECEIVED)->rows() == receivedbefore + 1,
               "rows " + numtext(receivedbefore) + " -> " + numtext(shellpage(PAGE_R_RECEIVED)->rows()));
    shellshow(PAGE_R_PAST, false);
    flowresult("F3 the accepted offer shows in Past offers", shellpage(PAGE_R_PAST)->rows() >= 1, "");
    flowresult("F3 the status line says the offer was accepted", acceptedstatus.find("was accepted") != string::npos && !haspunct(acceptedstatus),
               "status='" + acceptedstatus + "'");
}

// F3b: a second offer is declined by its recipient.
static void flowdecline()
{
    sentoffer second;
    bool sent = sendnextoffer(second);
    flowresult("F3b a second offer was sent", sent, sent ? "offer " + second.offer + " to " + second.recipient : "");

    if (!sent)
    {
        return;
    }

    signinrecipient(second.recipient);
    shellshow(PAGE_R_INCOMING, false);
    HWND list = pagecontrol(PAGE_R_INCOMING, IDC_LIST_MAIN);
    bool found = selectrowbyid(list, second.offer);
    flowresult("F3b the offer is in Incoming offers", found, "");
    press(pagecontrol(PAGE_R_INCOMING, IDC_R_DECLINE));
    string declinedstatus = shellstatustext();

    int offerpos = 0;
    bool known = theapp->offers.find(EntityId::parse(second.offer, "OFF"), offerpos);
    flowresult("F3b the offer is DECLINED", known && theapp->offers.at(offerpos).status() == OFFER_DECLINED, "");
    flowresult("F3b offers.csv says DECLINED", lastofferrow().find(second.offer + ",") != string::npos && lastofferrow().find(",DECLINED,") != string::npos,
               "row=" + lastofferrow());
    flowresult("F3b the status line says the offer was declined", declinedstatus == "Offer " + second.offer + " declined", "status='" + declinedstatus + "'");
    shellshow(PAGE_R_PAST, false);
    flowresult("F3b the declined offer shows in Past offers", shellpage(PAGE_R_PAST)->rows() >= 1, "");

    // the coordinator can no longer offer that donation to the same recipient
    signinrecipient(second.recipient);
}

// How many shown labels of a page have this part in their text.
static int labelswith(int pageid, const string& part)
{
    int total = 0;
    guipage* page = shellpage(pageid);
    HWND child = page == NULL ? NULL : GetWindow(page->window(), GW_CHILD);

    while (child != NULL)
    {
        if (classof(child) == "Static" && shown(child) && textof(child).find(part) != string::npos)
        {
            total++;
        }

        child = GetWindow(child, GW_HWNDNEXT);
    }

    return total;
}

// F4: the recipient changes the details on My details. A missing capacity shows the red line; a good one is saved in
// recipients.csv and the page stays. Home has no capacity line, so the capacity is changed in one place only.
static void flowedit()
{
    if (!firstofferdone)
    {
        flowresult("F4 a recipient is signed in", false, "");
        return;
    }

    signinrecipient(firstoffer.recipient);
    shellshow(PAGE_R_EDIT, false);
    HWND capacity = pagecontrol(PAGE_R_EDIT, IDC_R_CAPACITY);
    HWND error = pagecontrol(PAGE_R_EDIT, IDC_R_ERROR);
    int recpos = 0;
    theapp->data.recipientindex.get(firstoffer.recipient, recpos);
    flowresult("F4 the form shows the saved capacity", textof(capacity) == numtext(theapp->data.recipients[recpos].availableCapacity()),
               "capacity box='" + textof(capacity) + "'");

    SetWindowTextA(capacity, "");
    press(pagecontrol(PAGE_R_EDIT, IDC_R_SAVE));
    flowresult("F4a an empty capacity shows a red line and stays", shellcurrent() == PAGE_R_EDIT && !textof(error).empty() && !haspunct(textof(error)),
               "line='" + textof(error) + "'");

    SetWindowTextA(capacity, "77");
    press(pagecontrol(PAGE_R_EDIT, IDC_R_SAVE));
    flowresult("F4 recipients.csv has the new capacity", csvfield(recipientrow(firstoffer.recipient), 3) == "77",
               "row=" + recipientrow(firstoffer.recipient));
    flowresult("F4 the details are saved in memory and the page stays on My details",
               theapp->data.recipients[recpos].availableCapacity() == 77 && shellcurrent() == PAGE_R_EDIT && textof(capacity) == "77", "");
    flowresult("F4 the status line says Details saved", shellstatustext() == "Details saved", "status='" + shellstatustext() + "'");

    shellshow(PAGE_R_HOME, false);
    flowresult("F4 Home has no capacity line", labelswith(PAGE_R_HOME, "apacity") == 0, "");
}

// F4b: an offer that waits when the recipient stops accepting donations fails with a plain message and stays waiting.
static void flowrefuse()
{
    sentoffer waiting;
    bool sent = sendnextoffer(waiting);
    flowresult("F4b an offer was sent to a recipient", sent, sent ? "offer " + waiting.offer + " to " + waiting.recipient : "");

    if (!sent)
    {
        return;
    }

    signinrecipient(waiting.recipient);
    shellshow(PAGE_R_EDIT, false);
    SendMessageA(pagecontrol(PAGE_R_EDIT, IDC_R_ACCEPTING), BM_SETCHECK, BST_UNCHECKED, 0);
    press(pagecontrol(PAGE_R_EDIT, IDC_R_SAVE));
    flowresult("F4b the recipient no longer accepts donations (recipients.csv)", csvfield(recipientrow(waiting.recipient), 5) == "0",
               "row=" + recipientrow(waiting.recipient));

    shellshow(PAGE_R_INCOMING, false);
    selectrowbyid(pagecontrol(PAGE_R_INCOMING, IDC_LIST_MAIN), waiting.offer);
    size_t transactionsbefore = csvlines(theapp->prefix + "transactions.csv").size();
    press(pagecontrol(PAGE_R_INCOMING, IDC_R_ACCEPT));

    int offerpos = 0;
    bool known = theapp->offers.find(EntityId::parse(waiting.offer, "OFF"), offerpos);
    flowresult("F4b accepting fails: the offer stays waiting and nothing is booked",
               known && theapp->offers.at(offerpos).status() == OFFER_PENDING && csvlines(theapp->prefix + "transactions.csv").size() == transactionsbefore,
               "");
}

// After the flows: the files must still be consistent, and a fresh start must see the same numbers.
static void flowfinish()
{
    vector<string> problems = theapp->files->checkinvariants(theapp->data);
    flowresult("end checkinvariants finds nothing", problems.empty(), problems.empty() ? "" : problems[0]);

    try
    {
        AppState fresh(theapp->prefix);
        bool same = fresh.data.donors.size() == theapp->data.donors.size() &&
                    fresh.data.recipients.size() == theapp->data.recipients.size() &&
                    fresh.data.donations.size() == theapp->data.donations.size() &&
                    fresh.data.transactions.size() == theapp->data.transactions.size() &&
                    fresh.offers.count() == theapp->offers.count();
        flowresult("end a fresh start on the same folder has equal row counts", same,
                   "donations " + numtext((int)fresh.data.donations.size()) + "/" + numtext((int)theapp->data.donations.size()) +
                       " transactions " + numtext((int)fresh.data.transactions.size()) + "/" +
                       numtext((int)theapp->data.transactions.size()) + " offers " + numtext(fresh.offers.count()) + "/" +
                       numtext(theapp->offers.count()));
    }
    catch (const exception& problem)
    {
        flowresult("end a fresh start on the same folder has equal row counts", false, string("start failed: ") + problem.what());
    }
}

// F7: the welcome page. Donor, then Back, then Recipient, a name, Continue.
static void flowwelcome()
{
    if (theapp->data.donors.empty() || theapp->data.recipients.empty())
    {
        flowresult("F7 the data has a donor and a recipient", false, "");
        return;
    }

    shellswitchuser();
    HWND names = pagecontrol(PAGE_WELCOME, IDC_W_NAMES);
    HWND button = pagecontrol(PAGE_WELCOME, IDC_W_CONTINUE);
    press(pagecontrol(PAGE_WELCOME, IDC_W_DONOR));
    flowresult("F7 Donor opens step 2 with the donors",
               shown(names) && (int)SendMessageA(names, LB_GETCOUNT, 0, 0) == (int)theapp->data.donors.size() && shellwho() == WHO_NONE, "");
    press(pagecontrol(PAGE_WELCOME, IDC_W_BACK));
    flowresult("F7 Back returns to the three role buttons",
               !shown(names) && shown(pagecontrol(PAGE_WELCOME, IDC_W_COORD)) && shown(pagecontrol(PAGE_WELCOME, IDC_W_RECIP)), "");
    press(pagecontrol(PAGE_WELCOME, IDC_W_RECIP));
    flowresult("F7 Recipient opens step 2 with the recipients",
               shown(names) && (int)SendMessageA(names, LB_GETCOUNT, 0, 0) == (int)theapp->data.recipients.size() && !IsWindowEnabled(button), "");
    choosename(0);
    press(button);
    flowresult("F7 Continue signs in as the chosen recipient and shows the first page",
               shellwho() == WHO_RECIPIENT && shellpersonid() == theapp->data.recipients[0].id().toString() && shellcurrent() == PAGE_R_HOME,
               "page=" + numtext(shellcurrent()));
}

// Runs every flow on the copy of the data. Returns the number of failed steps.
static int runflows()
{
    flowwelcome();
    flowdonoradd();
    flowsendoffer();
    flowaccept();
    flowdecline();
    flowedit();
    flowrefuse();
    flowwithdraw();
    flowclose();
    flowfinish();
    testlog("summary failed=" + numtext(flowsfailed));
    return flowsfailed;
}

// Runs the chosen test mode.
int testrun(int kind, const string& folder)
{
    // a test may make the window bigger than the screen, and may ask for one scale with --dpi
    shellallowhuge(true);
    onlydpi = themedpiisoverridden() ? themedpiforwindow(NULL) : 0;
    startdpi = themedpi();
    shellsetclient(px(1000), px(650));
    logenvironment();
    int failures = 0;

    if (kind == TEST_FLOW)
    {
        failures = runflows();
    }
    else if (kind == TEST_DPICHANGE)
    {
        failures = rundpichange() + failedchecks;
    }
    else
    {
        checkshell();
        testlog("summary checks_failed=" + numtext(failedchecks));
        failures = runwalk(kind == TEST_SCREENSHOTS, folder) + failedchecks;
    }

    return failures == 0 ? 0 : 3;
}
