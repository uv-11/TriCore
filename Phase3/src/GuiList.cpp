// GuiList.cpp: the list view used by every page.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

#include "GuiList.h"
#include "Theme.h"

#include <string.h>

using namespace std;

// The old Windows headers do not know this flag: the list paints into an off-screen bitmap, which avoids flicker.
#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER 0x00010000
#endif

// The id of the message text of a list is the list id plus this number (the two never clash with page controls).
static const int messageoffset = 5000;

// Starts with no windows.
guilistview::guilistview()
{
    list_ = NULL;
    message_ = NULL;
    id_ = 0;
    columns_ = 0;
}

// The list has no sorting and shows the selection even without focus. The message is a centred grey static.
void guilistview::create(HWND parent, int id, const string& emptymessage)
{
    id_ = id;

    // the list has no frame: a plain list on the page tone is calmer than a black rectangle (stage U3, DECISIONS.md S7)
    list_ = CreateWindowExA(0, WC_LISTVIEWA, "",
                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL |
                                LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
                            0, 0, 10, 10, parent, (HMENU)(INT_PTR)id, GetModuleHandleA(NULL), NULL);
    themestylelist(list_);

    message_ = CreateWindowExA(0, "STATIC", emptymessage.c_str(),
                               WS_CHILD | SS_CENTER | SS_CENTERIMAGE, 0, 0, 10, 10, parent,
                               (HMENU)(INT_PTR)(id + messageoffset), GetModuleHandleA(NULL), NULL);
    themesetrole(message_, ROLE_MUTED);
}

// Measures the header and a row of a list (the real ones, not assumed): the header height, and the height of the first
// row when the list has rows (else 32 design pixels).
static void measurelist(HWND list, int rows, int& headerheight, int& rowheight)
{
    headerheight = px(32);
    HWND header = (HWND)SendMessageA(list, LVM_GETHEADER, 0, 0);

    if (header != NULL)
    {
        RECT place;
        GetWindowRect(header, &place);

        if (place.bottom - place.top > 0)
        {
            headerheight = place.bottom - place.top;
        }
    }

    rowheight = px(32);
    RECT first;
    ZeroMemory(&first, sizeof(first));
    first.left = LVIR_BOUNDS;

    if (rows > 0 && SendMessageA(list, LVM_GETITEMRECT, 0, (LPARAM)&first) && first.bottom - first.top > 0)
    {
        rowheight = first.bottom - first.top;
    }
}

// The tallest height up to the available one that shows whole rows only: the header and a whole number of rows. A partly
// visible last row would look cut. The list has no frame, so nothing is left over for a border.
int guilistview::snapheight(int available) const
{
    int headerheight = 0;
    int rowheight = 0;
    measurelist(list_, rowcount(), headerheight, rowheight);
    int body = available - headerheight;

    if (body < rowheight)
    {
        return available;
    }

    return headerheight + (body / rowheight) * rowheight;
}

// The height the list needs for all its rows (the header and every row, at least 160 design pixels).
int guilistview::hugheight() const
{
    int headerheight = 0;
    int rowheight = 0;
    measurelist(list_, rowcount(), headerheight, rowheight);
    int wanted = headerheight + rowcount() * rowheight;
    return wanted < px(160) ? px(160) : wanted;
}
// The width the list needs to show every column in full: the columns, the room of a vertical scroll bar and 8 design pixels.
int guilistview::neededwidth() const
{
    int total = 0;

    for (size_t column = 0; column < natural_.size(); column++)
    {
        total = total + natural_[column];
    }

    return total + px(17) + px(8);
}

// Returns the control id.
int guilistview::id() const
{
    return id_;
}

// Returns the list window.
HWND guilistview::window() const
{
    return list_;
}

// Number of the selected row.
int guilistview::selectedindex() const
{
    return (int)SendMessageA(list_, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
}

// Number of rows.
int guilistview::rowcount() const
{
    return (int)SendMessageA(list_, LVM_GETITEMCOUNT, 0, 0);
}

// The first column holds the id of the row.
string guilistview::selectedid() const
{
    int row = selectedindex();

    if (row < 0)
    {
        return "";
    }

    char buffer[512];
    buffer[0] = '\0';
    LVITEMA item;
    ZeroMemory(&item, sizeof(item));
    item.iSubItem = 0;
    item.pszText = buffer;
    item.cchTextMax = (int)sizeof(buffer);
    SendMessageA(list_, LVM_GETITEMTEXTA, (WPARAM)row, (LPARAM)&item);
    return buffer;
}

// Selects the row of an id and scrolls to it.
bool guilistview::selectid(const string& text)
{
    int count = rowcount();

    for (int row = 0; row < count; row++)
    {
        char buffer[512];
        buffer[0] = '\0';
        LVITEMA item;
        ZeroMemory(&item, sizeof(item));
        item.iSubItem = 0;
        item.pszText = buffer;
        item.cchTextMax = (int)sizeof(buffer);
        SendMessageA(list_, LVM_GETITEMTEXTA, (WPARAM)row, (LPARAM)&item);

        if (text == buffer)
        {
            LVITEMA state;
            ZeroMemory(&state, sizeof(state));
            state.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
            state.state = LVIS_SELECTED | LVIS_FOCUSED;
            SendMessageA(list_, LVM_SETITEMSTATE, (WPARAM)row, (LPARAM)&state);
            SendMessageA(list_, LVM_ENSUREVISIBLE, (WPARAM)row, FALSE);
            return true;
        }
    }

    return false;
}

// Fits the columns into the width the list has now. When the natural widths are too wide, the widest columns
// are made narrower first (their text then ends with "..."), but never below 70 pixels. When there is room left,
// the columns share it in proportion to their width.
void guilistview::fitcolumns()
{
    if (list_ == NULL || columns_ <= 0 || (int)natural_.size() != columns_)
    {
        return;
    }

    RECT area;
    GetClientRect(list_, &area);
    int available = area.right - area.left;

    if (available <= 0)
    {
        return;
    }

    vector<int> widths = natural_;
    int total = 0;

    for (int column = 0; column < columns_; column++)
    {
        total = total + widths[column];
    }

    int minimum = px(70);

    while (total > available)
    {
        int widest = 0;

        for (int column = 1; column < columns_; column++)
        {
            if (widths[column] > widths[widest])
            {
                widest = column;
            }
        }

        if (widths[widest] <= minimum)
        {
            break;
        }

        int step = widths[widest] - minimum;

        if (step > px(10))
        {
            step = px(10);
        }

        if (step > total - available)
        {
            step = total - available;
        }

        widths[widest] = widths[widest] - step;
        total = total - step;
    }

    // room that is left is shared by the columns in proportion to their width, so a short table is not a narrow text column
    // next to one very wide column; a column grows to at most 1.6 times its own width, and what is left goes to the last column
    if (total < available && total > 0)
    {
        int spare = available - total;
        int given = 0;

        for (int column = 0; column < columns_; column++)
        {
            int part = (int)(((long long)spare * widths[column]) / total);

            if (part > widths[column] * 6 / 10)
            {
                part = widths[column] * 6 / 10;
            }

            widths[column] = widths[column] + part;
            given = given + part;
        }

        widths[columns_ - 1] = widths[columns_ - 1] + (spare - given);
    }

    for (int column = 0; column < columns_; column++)
    {
        SendMessageA(list_, LVM_SETCOLUMNWIDTH, (WPARAM)column, MAKELPARAM(widths[column], 0));
    }
}

// Hides both windows.
void guilistview::hide()
{
    ShowWindow(list_, SW_HIDE);
    ShowWindow(message_, SW_HIDE);
}

// An empty table hides the list and shows the message.
void guilistview::showpart()
{
    bool empty = (rowcount() == 0);
    ShowWindow(list_, empty ? SW_HIDE : SW_SHOWNA);
    ShowWindow(message_, empty ? SW_SHOWNA : SW_HIDE);
}

// Redraw is off while the list is rebuilt and the list is repainted once at the end.
void guilistview::fill(const texttable& table)
{
    string keep = selectedid();

    SendMessageA(list_, WM_SETREDRAW, FALSE, 0);
    SendMessageA(list_, LVM_DELETEALLITEMS, 0, 0);

    while (SendMessageA(list_, LVM_DELETECOLUMN, 0, 0))
    {
    }

    columns_ = (int)table.headers.size();

    for (size_t column = 0; column < table.headers.size(); column++)
    {
        LVCOLUMNA header;
        ZeroMemory(&header, sizeof(header));
        header.mask = LVCF_TEXT | LVCF_WIDTH;
        header.cx = px(80);
        header.pszText = (char*)table.headers[column].c_str();
        SendMessageA(list_, LVM_INSERTCOLUMNA, (WPARAM)column, (LPARAM)&header);
    }

    SendMessageA(list_, LVM_SETITEMCOUNT, (WPARAM)table.rows.size(), 0);

    for (size_t row = 0; row < table.rows.size(); row++)
    {
        const vector<string>& cells = table.rows[row];

        if (cells.empty())
        {
            continue;
        }

        LVITEMA item;
        ZeroMemory(&item, sizeof(item));
        item.mask = LVIF_TEXT;
        item.iItem = (int)row;
        item.iSubItem = 0;
        item.pszText = (char*)cells[0].c_str();
        SendMessageA(list_, LVM_INSERTITEMA, 0, (LPARAM)&item);

        for (size_t column = 1; column < cells.size() && column < table.headers.size(); column++)
        {
            item.iSubItem = (int)column;
            item.pszText = (char*)cells[column].c_str();
            SendMessageA(list_, LVM_SETITEMTEXTA, (WPARAM)row, (LPARAM)&item);
        }
    }

    // each column is as wide as its longest text or its title, with a little air (measured once, here)
    natural_.clear();

    for (size_t column = 0; column < table.headers.size(); column++)
    {
        SendMessageA(list_, LVM_SETCOLUMNWIDTH, (WPARAM)column, MAKELPARAM(LVSCW_AUTOSIZE, 0));
        int content = (int)SendMessageA(list_, LVM_GETCOLUMNWIDTH, (WPARAM)column, 0);

        // the width of the title is measured directly: for the last column "use header" would mean "fill the rest"
        int header = (int)SendMessageA(list_, LVM_GETSTRINGWIDTHA, 0, (LPARAM)table.headers[column].c_str()) + px(14);
        int width = content > header ? content : header;

        if (width > px(520))
        {
            width = px(520);
        }

        natural_.push_back(width + px(4));
    }

    fitcolumns();

    if (!keep.empty())
    {
        selectid(keep);
    }

    SendMessageA(list_, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list_, NULL, TRUE);
    showpart();
}

// Both windows get the same rectangle; only one of them is visible at a time.
void guilistview::place(guilayout& layout, int left, int top, int width, int height)
{
    layout.place(list_, left, top, width, height);
    layout.place(message_, left, top, width, height);
}
