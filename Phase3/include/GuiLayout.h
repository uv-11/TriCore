#ifndef GUILAYOUT_H
#define GUILAYOUT_H

// GuiLayout.h: scaling and positioning helpers. Every pixel number of the program goes through px().

#include <windows.h>

#include <string>
#include <vector>

using namespace std;

// The spacing tokens of docs/DESIGN_GUIDE.md in design pixels (96 dpi). Pages use these names, never a number.
static const int gaptiny = 4;          // label to its field
static const int gapsmall = 8;         // between buttons in a row
static const int gapfield = 12;        // between form fields
static const int gapbox = 16;          // above the button row, inside a group
static const int pagemargin = 24;      // left and right page margin, and between sections
static const int gaptop = 32;          // above the page title
static const int sidebarwidth = 240;   // the sidebar
static const int controlheight = 32;   // buttons, edit boxes, drop-down lists, list rows
static const int formwidth = 640;      // the widest form column
static const bool formfullwidth = false; // true makes the forms use the full content width
static const int textline = 24;        // the height of one line of body text
static const int captionline = 20;     // the height of one line of caption text
static const int welcometitle = 52;    // the height of the welcome title line
static const int rolebuttonwidth = 200; // the three role buttons of the welcome page
static const int rolebuttonheight = 56;
static const int welcomewidth = 560;   // the column of the welcome name list
static const int namelistheight = 320; // the welcome name list: tallest and lowest height
static const int namelistminimum = 120;
static const int numberwidth = 160;    // an edit box for a number
static const int choicewidth = 220;    // a drop-down list of a form
static const int droplistheight = 200; // a drop-down list with its open list

// Turns a size in design pixels (96 dpi) into real pixels for this screen.
int px(int design);

// Turns a size in design pixels into real pixels for an explicit dpi.
int px(int design, int dpi);

// Turns a whole number into text.
string numtext(int value);

// Gives the left edge and the width of a column of at most maxwidth that is centred in total width
// (with at least the margin on both sides).
void centredcolumn(int totalwidth, int maxwidth, int margin, int& left, int& width);

// Collects new positions for the children of one parent and applies them in one batch, so the
// window is redrawn once and nothing flickers.
class guilayout
{
public:
    // Starts an empty batch.
    guilayout();

    // Remembers where a control (a child of the parent) should go.
    void place(HWND control, int left, int top, int width, int height);

    // Moves every remembered control in one operation.
    void apply();

private:
    // The place of one control.
    struct entry
    {
        HWND control;
        int left;
        int top;
        int width;
        int height;
    };

    vector<entry> entries_; // everything placed so far
};

#endif
