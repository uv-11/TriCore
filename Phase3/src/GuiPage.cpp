// GuiPage.cpp: the base of every page and the helpers that make controls.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>

#include "GuiPage.h"
#include "Theme.h"

#include <string.h>

using namespace std;

// The class name of a page window.
static const char* const pageclass = "CommunityBridgePage";

// Passes the messages of the controls on to the page. The colours come from the theme.
static LRESULT CALLBACK pageproc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT colour = 0;

    if (themecolour(msg, wparam, lparam, colour))
    {
        return colour;
    }

    guipage* page = (guipage*)GetWindowLongPtrA(window, GWLP_USERDATA);

    if (page != NULL && msg == WM_COMMAND)
    {
        page->command(LOWORD(wparam), HIWORD(wparam));
        return 0;
    }

    // the owner drawn name list of the welcome page asks for the height and the look of its lines
    if (msg == WM_MEASUREITEM)
    {
        MEASUREITEMSTRUCT* measure = (MEASUREITEMSTRUCT*)lparam;

        if (measure != NULL && measure->CtlType == ODT_LISTBOX)
        {
            measure->itemHeight = (UINT)themelistboxheight();
            return TRUE;
        }
    }

    if (msg == WM_DRAWITEM)
    {
        const DRAWITEMSTRUCT* item = (const DRAWITEMSTRUCT*)lparam;

        if (item != NULL && item->CtlType == ODT_LISTBOX)
        {
            themedrawlistbox(item);
            return TRUE;
        }
    }

    // the header of a list asks the program to draw it (the list passes the message on to its parent)
    if (msg == WM_NOTIFY)
    {
        const NMHDR* header = (const NMHDR*)lparam;
        char name[32];
        name[0] = '\0';

        if (header != NULL && header->code == (UINT)NM_CUSTOMDRAW)
        {
            GetClassNameA(header->hwndFrom, name, (int)sizeof(name));

            if (strcmp(name, "SysHeader32") == 0)
            {
                return themedrawheader((NMCUSTOMDRAW*)lparam);
            }
        }
    }

    if (page != NULL && msg == WM_NOTIFY)
    {
        page->notify((const NMHDR*)lparam);
        return 0;
    }

    return DefWindowProcA(window, msg, wparam, lparam);
}

// The class has the page background and no CS_HREDRAW or CS_VREDRAW, so a resize does not repaint everything.
void guiregisterpageclass(HINSTANCE instance)
{
    WNDCLASSA description;
    ZeroMemory(&description, sizeof(description));
    description.style = 0;
    description.lpfnWndProc = pageproc;
    description.hInstance = instance;
    description.hCursor = LoadCursor(NULL, IDC_ARROW);
    description.hbrBackground = themepagebrush();
    description.lpszClassName = pageclass;
    RegisterClassA(&description);
}

// Nothing is made yet.
guipage::guipage(int id, const string& name, const string& title)
{
    window_ = NULL;
    title_ = NULL;
    id_ = id;
    name_ = name;
    title_text_ = title;
}

// The controls go away with the window.
guipage::~guipage()
{
    if (window_ != NULL)
    {
        DestroyWindow(window_);
    }
}

// The page window is hidden until the shell shows it.
void guipage::build(HWND host)
{
    window_ = CreateWindowExA(WS_EX_CONTROLPARENT, pageclass, "", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0,
                              10, 10, host, NULL, GetModuleHandleA(NULL), NULL);
    SetWindowLongPtrA(window_, GWLP_USERDATA, (LONG_PTR)this);
    createcontrols();
    themeapplyfonts(window_);
}

// A page that has nothing special to do simply refreshes.
void guipage::onshow()
{
    refresh();
}

// By default a page handles no command.
bool guipage::command(int id, int code)
{
    (void)id;
    (void)code;
    return false;
}

// By default a page handles no notification.
bool guipage::notify(const NMHDR* header)
{
    (void)header;
    return false;
}

// By default a page has no primary button.
int guipage::defaultid() const
{
    return 0;
}

// The calm rule: one list.
int guipage::maxlists() const
{
    return 1;
}

// The calm rule: the primary button and two others.
int guipage::maxbuttons() const
{
    return 3;
}

// By default the page keeps its own sidebar item selected.
int guipage::navid() const
{
    return id_;
}

// By default a page has no list.
int guipage::rows() const
{
    return 0;
}

// The window of the page.
HWND guipage::window() const
{
    return window_;
}

// The number of the page.
int guipage::id() const
{
    return id_;
}

// The short name of the page.
const string& guipage::name() const
{
    return name_;
}

// The title text.
const string& guipage::title() const
{
    return title_text_;
}

// The title is a static with the role ROLE_TITLE.
HWND guipage::addtitle()
{
    title_ = makelabel(window_, title_text_, ROLE_TITLE);
    return title_;
}

// The title sits at the page margin (32 design pixels); the content starts a block gap (24) below it.
int guipage::layouttitle(guilayout& layout, int left, int width)
{
    int top = px(gaptop);
    int height = px(40);
    layout.place(title_, left, top, width, height);
    return top + height + px(pagemargin);
}

// A left aligned (or centred) static. Texts that are too long for the control wrap to the next line.
HWND makelabel(HWND parent, const string& text, int role, int id, bool centred)
{
    // SS_NOPREFIX: an ampersand in a name is shown as it is and never makes a keyboard shortcut
    DWORD style = WS_CHILD | WS_VISIBLE | SS_NOPREFIX | (centred ? SS_CENTER : SS_LEFT);
    HWND control = CreateWindowExA(0, "STATIC", text.c_str(), style, 0, 0, 10, 10, parent, (HMENU)(INT_PTR)id,
                                   GetModuleHandleA(NULL), NULL);
    themesetrole(control, role);
    return control;
}

// A push button that can be reached with Tab.
HWND makebutton(HWND parent, const string& text, int id, bool primary)
{
    DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | (primary ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON);
    return CreateWindowExA(0, "BUTTON", text.c_str(), style, 0, 0, 10, 10, parent, (HMENU)(INT_PTR)id,
                           GetModuleHandleA(NULL), NULL);
}

// A one-line text box with an edge.
HWND makeedit(HWND parent, int id, bool numberonly, int maxchars)
{
    DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | (numberonly ? ES_NUMBER : 0);
    HWND control = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", style, 0, 0, 10, 10, parent, (HMENU)(INT_PTR)id,
                                   GetModuleHandleA(NULL), NULL);
    SendMessageA(control, EM_LIMITTEXT, (WPARAM)maxchars, 0);
    return control;
}

// A drop-down list. The window is made tall, because the height of a combo box includes its list.
HWND makecombo(HWND parent, int id)
{
    return CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, 0, 0,
                           10, px(200), parent, (HMENU)(INT_PTR)id, GetModuleHandleA(NULL), NULL);
}

// An automatic check box.
HWND makecheck(HWND parent, const string& text, int id)
{
    return CreateWindowExA(0, "BUTTON", text.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 10, 10,
                           parent, (HMENU)(INT_PTR)id, GetModuleHandleA(NULL), NULL);
}

// A window property marks a text as data, so no subclassing is needed.
void markdata(HWND control)
{
    SetPropA(control, "cbdata", (HANDLE)1);
}

// A control without the property shows program text.
bool isdata(HWND control)
{
    return GetPropA(control, "cbdata") != NULL;
}

// Measures the text with the font of the control, wrapped at the width.
int measureheight(HWND control, int width)
{
    char text[1024];
    text[0] = '\0';
    GetWindowTextA(control, text, (int)sizeof(text));

    HDC dc = GetDC(control);
    HGDIOBJ old = SelectObject(dc, (HFONT)SendMessageA(control, WM_GETFONT, 0, 0));
    RECT area = {0, 0, width > 0 ? width : 1, 0};
    DrawTextA(dc, text, -1, &area, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    SelectObject(dc, old);
    ReleaseDC(control, dc);
    return area.bottom - area.top;
}

// Measures the text with the font of the control.
int textwidth(HWND control)
{
    char text[1024];
    text[0] = '\0';
    GetWindowTextA(control, text, (int)sizeof(text));

    HDC dc = GetDC(control);
    HGDIOBJ old = SelectObject(dc, (HFONT)SendMessageA(control, WM_GETFONT, 0, 0));
    SIZE size;
    GetTextExtentPoint32A(dc, text, (int)lstrlenA(text), &size);
    SelectObject(dc, old);
    ReleaseDC(control, dc);
    return size.cx;
}

// The caption with room at both sides.
int buttonwidth(HWND button)
{
    int width = textwidth(button) + px(40);
    return width < px(110) ? px(110) : width;
}

// Equal margins on both sides.
void contentbox(int pagewidth, int& left, int& width)
{
    left = px(pagemargin);
    width = pagewidth - 2 * left;

    if (width < 0)
    {
        width = 0;
    }
}

// Goes from the right edge to the left, so the last button made (the primary one) ends exactly at right.
int placebuttons(guilayout& layout, const vector<HWND>& buttons, int right, int bottom)
{
    int height = px(controlheight);
    int edge = right;

    for (int pos = (int)buttons.size() - 1; pos >= 0; pos--)
    {
        int wide = buttonwidth(buttons[pos]);
        edge = edge - wide;
        layout.place(buttons[pos], edge, bottom - height, wide, height);
        edge = edge - px(gapsmall);
    }

    return bottom - height;
}

// Uses the content box or a centred column.
void formcolumn(int pagewidth, int& left, int& width)
{
    if (formfullwidth)
    {
        contentbox(pagewidth, left, width);
        return;
    }

    centredcolumn(pagewidth, px(formwidth), px(pagemargin), left, width);
}

// The label sits a tiny gap above the field; the next field starts a field gap below it.
int placefield(guilayout& layout, HWND label, HWND field, HWND help, int left, int top, int width, int fieldwidth,
               int fieldheight)
{
    layout.place(label, left, top, width, px(textline));
    top = top + px(textline + gaptiny);
    layout.place(field, left, top, fieldwidth, fieldheight);

    if (help != NULL)
    {
        int helpleft = left + fieldwidth + px(gapfield);
        layout.place(help, helpleft, top + px(gaptiny), left + width - helpleft, px(textline));
    }

    return top + px(controlheight + gapfield);
}

// The text of a control with the spaces at both ends cut off.
string readtext(HWND control)
{
    char buffer[1024];
    buffer[0] = '\0';
    GetWindowTextA(control, buffer, (int)sizeof(buffer));

    string text = buffer;
    size_t first = 0;

    while (first < text.length() && (text[first] == ' ' || text[first] == '\t'))
    {
        first++;
    }

    size_t last = text.length();

    while (last > first && (text[last - 1] == ' ' || text[last - 1] == '\t'))
    {
        last--;
    }

    return text.substr(first, last - first);
}

// Accepts one to nine digits, so the number always fits in an int.
bool readnumber(HWND control, int& value)
{
    string text = readtext(control);

    if (text.empty() || text.length() > 9)
    {
        return false;
    }

    int number = 0;

    for (size_t pos = 0; pos < text.length(); pos++)
    {
        if (text[pos] < '0' || text[pos] > '9')
        {
            return false;
        }

        number = number * 10 + (text[pos] - '0');
    }

    value = number;
    return true;
}
