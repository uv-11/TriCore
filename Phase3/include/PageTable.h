#ifndef PAGETABLE_H
#define PAGETABLE_H

// PageTable.h: two page shapes that many pages share. A table page has a title, one list and up to three buttons
// at the bottom right. A home page has a title and a few lines "caption ... value" and nothing else.

#include "GuiList.h"
#include "GuiPage.h"

#include <string>
#include <vector>

using namespace std;

// The control id of the list on a table page.
#define IDC_LIST_MAIN 2050

// The control id of the second list on a page that has two (the match page).
#define IDC_LIST_SECOND 2051

// Takes the column with this header out of a table (never the first column, which holds the ids).
void dropcolumn(texttable& table, const string& header);

// A page with one list. A derived page says which table to show and which buttons it has.
class pagetable : public guipage
{
public:
    // Remembers the empty-list message, which the list shows when it has no rows.
    pagetable(int id, const string& name, const string& title, const string& emptymessage);

    // Makes the title, the list and (through addbuttons) the buttons.
    void createcontrols();

    // Fills the list from the data and updates the buttons.
    void refresh();

    // The list fills the free space; the buttons sit at the bottom right, the primary one rightmost.
    void onsize(int width, int height);

    // A button was pressed: clicked() decides what it means.
    bool command(int id, int code);

    // The selected row changed: the buttons are updated.
    bool notify(const NMHDR* header);

    // The number of rows in the list.
    int rows() const;

    // Enter presses the primary button (0 when the page has none).
    int defaultid() const;

protected:
    // The table to show (the page builds it from the backend, never from a window).
    virtual texttable maketable() = 0;

    // Makes the buttons (the page calls addbutton for each). The default page has none.
    virtual void addbuttons();

    // Enables or disables the buttons that need a selected row. The default does nothing.
    virtual void updatebuttons();

    // A button was pressed.
    virtual void clicked(int id);

    // Tells whether the list is only as high as its rows (a short table that has text under it) and not as high as the page.
    virtual bool huglist() const;

    // The height that the page needs under the list for extra text, including the gap above it (0 for most pages).
    virtual int belowheight(int width);

    // Places the extra text under the list: left, top and width of the column.
    virtual void layoutbelow(guilayout& layout, int left, int top, int width);

    // Makes a button below the list. Buttons are made left to right, the primary one last.
    HWND addbutton(const string& text, int id, bool primary);

    guilistview list_;        // the list
    vector<HWND> buttons_;    // the buttons, left to right
    int primaryid_;           // the id of the primary button (0 when there is none)
    string emptymessage_;     // shown when the list has no rows
    int width_;               // the width of the page at the last layout (0 before the first)
    int height_;              // the height of the page at the last layout
};

// A home page: a title and a few lines of caption and value. It has no button, because every action has its own page.
class pagehome : public guipage
{
public:
    // Remembers the short name of the page.
    pagehome(int id, const string& name);

    // Makes the title and the lines (one pair of texts for each caption).
    void createcontrols();

    // Writes the values next to the captions.
    void refresh();

    // The lines sit below the title.
    void onsize(int width, int height);

    // A home page has no button.
    int maxbuttons() const;

protected:
    // Gives the value of each line, in the order of the captions.
    virtual void values(vector<string>& out) = 0;

    // Places the extra controls of a derived page in the free space below the lines: left, top and width of the
    // column, and the limit, the lowest y the extras may reach. The default page has no extras.
    virtual void layoutextra(guilayout& layout, int left, int top, int width, int limit);

    int lastwidth_;             // the width of the page at the last layout (0 before the first)
    int lastheight_;            // the height of the page at the last layout
    vector<string> captions_;   // the left texts, set by the derived page in its constructor
    vector<HWND> captionwindows_; // the statics with the captions
    vector<HWND> valuewindows_;   // the statics with the values
};

#endif
