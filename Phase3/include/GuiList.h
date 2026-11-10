#ifndef GUILIST_H
#define GUILIST_H

// GuiList.h: a report-style list for one texttable. When the table has no rows the list is hidden and a
// grey message takes its place, so two visible controls never overlap.

#include <windows.h>

#include "GuiLayout.h"
#include "Tables.h"

#include <string>
#include <vector>

using namespace std;

// One list plus its "nothing here" message.
class guilistview
{
public:
    // Creates an object that has no windows yet.
    guilistview();

    // Makes the list and the message as children of the parent. id is the control id of the list.
    void create(HWND parent, int id, const string& emptymessage);

    // Fills the list from a table. The selected row stays selected when its first-column text still exists.
    void fill(const texttable& table);

    // Puts the list (and the message in the same place) in the batch and remembers the size.
    void place(guilayout& layout, int left, int top, int width, int height);

    // Returns the number of the selected row, or -1.
    int selectedindex() const;

    // Returns the first-column text of the selected row, or an empty text.
    string selectedid() const;

    // Returns the number of rows.
    int rowcount() const;

    // Hides the list and its message (a page that has nothing to show there).
    void hide();

    // Makes the columns fit the width the list has now (call it after the page has moved the list).
    void fitcolumns();

    // The height up to the available one that shows whole rows only (no cut row at the bottom edge).
    int snapheight(int available) const;

    // The height the list needs to show all its rows (at least 160 design pixels).
    int hugheight() const;

    // The width the list needs to show every column in full (valid after fill).
    int neededwidth() const;

    // Returns the control id of the list.
    int id() const;

    // Returns the window of the list.
    HWND window() const;

    // Selects the row whose first column equals the text. Returns false when there is none.
    bool selectid(const string& text);

private:
    // Shows the list or the message, depending on the row count.
    void showpart();

    HWND list_;       // the SysListView32
    HWND message_;    // the grey text shown instead of an empty list
    int id_;          // the control id of the list
    int columns_;     // number of columns of the last fill
    vector<int> natural_; // the width each column needs for its longest text
};

#endif
