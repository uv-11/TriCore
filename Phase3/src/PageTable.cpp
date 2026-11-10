// PageTable.cpp: the table page and the home page, the two page shapes that other pages build on.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>

#include "PageTable.h"
#include "Theme.h"

using namespace std;

// Takes one column out of a table (the column with this header). Used where every row would repeat the person who is signed in.
void dropcolumn(texttable& table, const string& header)
{
    size_t column = 0;

    while (column < table.headers.size() && table.headers[column] != header)
    {
        column++;
    }

    if (column >= table.headers.size() || column == 0)
    {
        return;
    }

    table.headers.erase(table.headers.begin() + column);

    for (size_t row = 0; row < table.rows.size(); row++)
    {
        if (column < table.rows[row].size())
        {
            table.rows[row].erase(table.rows[row].begin() + column);
        }
    }
}

// The base class gets the page number, the short name and the title (a base-class call needs the initializer list).
pagetable::pagetable(int id, const string& name, const string& title, const string& emptymessage)
    : guipage(id, name, title)
{
    primaryid_ = 0;
    emptymessage_ = emptymessage;
    width_ = 0;
    height_ = 0;
}

// The title first, then the list, then the buttons, which is also the order of the Tab key.
void pagetable::createcontrols()
{
    addtitle();
    list_.create(window_, IDC_LIST_MAIN, emptymessage_);
    addbuttons();
}

// A page has no buttons unless it makes them.
void pagetable::addbuttons()
{
}

// By default a list is as high as the page.
bool pagetable::huglist() const
{
    return false;
}

// By default nothing is under the list.
int pagetable::belowheight(int width)
{
    (void)width;
    return 0;
}

// By default there is nothing to place under the list.
void pagetable::layoutbelow(guilayout& layout, int left, int top, int width)
{
    (void)layout;
    (void)left;
    (void)top;
    (void)width;
}

// Nothing needs a selection unless the page says so.
void pagetable::updatebuttons()
{
}

// Nothing happens unless the page says so.
void pagetable::clicked(int id)
{
    (void)id;
}

// The button is remembered in the order it was made; the primary one is the default button.
HWND pagetable::addbutton(const string& text, int id, bool primary)
{
    HWND button = makebutton(window_, text, id, primary);
    buttons_.push_back(button);

    if (primary)
    {
        primaryid_ = id;
    }

    return button;
}

// Builds the table and puts it in the list. The selection stays when the row is still there.
void pagetable::refresh()
{
    list_.fill(maketable());
    updatebuttons();

    // the columns may need another width now, so the list is laid out again (the page was sized before)
    if (width_ > 0)
    {
        onsize(width_, height_);
    }
}

// The list fills the content (the page minus equal margins) and the free height; the buttons are one row at the bottom
// right of the content, the primary one rightmost.
void pagetable::onsize(int width, int height)
{
    width_ = width;
    height_ = height;
    int left = 0;
    int contentwidth = 0;
    contentbox(width, left, contentwidth);
    guilayout layout;
    int top = layouttitle(layout, left, contentwidth);
    int bottom = height - px(pagemargin);

    if (!buttons_.empty())
    {
        bottom = placebuttons(layout, buttons_, left + contentwidth, bottom) - px(gapbox);
    }

    int listheight = bottom - top - belowheight(contentwidth);

    if (listheight < 0)
    {
        listheight = 0;
    }

    // only whole rows are visible, so no row is cut by the bottom edge of the list; a short table with text under it is
    // only as high as its rows
    listheight = list_.snapheight(listheight);

    if (huglist() && list_.hugheight() < listheight)
    {
        listheight = list_.hugheight();
    }

    list_.place(layout, left, top, contentwidth, listheight);
    layoutbelow(layout, left, top + listheight, contentwidth);
    layout.apply();
    list_.fitcolumns();
}

// A button press goes to the page.
bool pagetable::command(int id, int code)
{
    if (code == BN_CLICKED)
    {
        clicked(id);
        return true;
    }

    return false;
}

// The selection of the list changed (this also happens while the list is filled).
bool pagetable::notify(const NMHDR* header)
{
    if (header->idFrom == (UINT_PTR)IDC_LIST_MAIN && header->code == LVN_ITEMCHANGED)
    {
        updatebuttons();
        return true;
    }

    return false;
}

// The number of rows.
int pagetable::rows() const
{
    return list_.rowcount();
}

// The primary button, if the page has one.
int pagetable::defaultid() const
{
    return primaryid_;
}

// The base class gets the page number, the short name and the title (a base-class call needs the initializer list).
pagehome::pagehome(int id, const string& name) : guipage(id, name, "Home")
{
    lastwidth_ = 0;
    lastheight_ = 0;
}

// One pair of statics for each caption: the caption on the left, the value on the right.
void pagehome::createcontrols()
{
    addtitle();

    for (size_t pos = 0; pos < captions_.size(); pos++)
    {
        captionwindows_.push_back(makelabel(window_, captions_[pos], ROLE_MUTED));
        HWND value = makelabel(window_, "", ROLE_VALUE);
        markdata(value);
        valuewindows_.push_back(value);
    }
}

// Writes the values.
void pagehome::refresh()
{
    vector<string> texts;
    values(texts);

    for (size_t pos = 0; pos < valuewindows_.size() && pos < texts.size(); pos++)
    {
        SetWindowTextA(valuewindows_[pos], texts[pos].c_str());
    }
}

// One stat row across the content: equal columns, each a caption (12 px, muted) above its value (17 px, semibold).
void pagehome::onsize(int width, int height)
{
    int left = 0;
    int contentwidth = 0;
    contentbox(width, left, contentwidth);
    guilayout layout;
    int top = layouttitle(layout, left, contentwidth);
    int count = (int)captionwindows_.size();
    int columnwidth = count > 0 ? contentwidth / count : contentwidth;
    int captionheight = px(gapbox + gaptiny);
    int valueheight = px(controlheight);

    for (int pos = 0; pos < count; pos++)
    {
        int x = left + pos * columnwidth;
        layout.place(captionwindows_[pos], x, top, columnwidth - px(gapbox), captionheight);
        layout.place(valuewindows_[pos], x, top + captionheight, columnwidth - px(gapbox), valueheight);
    }

    layoutextra(layout, left, top + captionheight + valueheight + px(pagemargin), contentwidth, height - px(pagemargin));
    layout.apply();
    lastwidth_ = width;
    lastheight_ = height;
}

// A plain home page has nothing below its lines.
void pagehome::layoutextra(guilayout& layout, int left, int top, int width, int limit)
{
    (void)layout;
    (void)left;
    (void)top;
    (void)width;
    (void)limit;
}

// A home page has no button.
int pagehome::maxbuttons() const
{
    return 0;
}
