#ifndef GUIPAGE_H
#define GUIPAGE_H

// GuiPage.h: what every page of the window has in common, and small helpers that make controls.
// A page is a child window of the page host. It is made once, then only shown or hidden.

#include <windows.h>

#include "GuiLayout.h"

#include <string>

using namespace std;

// The number of every page. Pages of the same role are listed in the order of the sidebar.
enum pageid
{
    PAGE_NONE,
    PAGE_WELCOME,
    PAGE_C_HOME,
    PAGE_C_DONATIONS,
    PAGE_C_OFFERS,
    PAGE_C_RECIPIENTS,
    PAGE_C_TRANSACTIONS,
    PAGE_C_SUMMARY,
    PAGE_C_MATCH,
    PAGE_D_HOME,
    PAGE_D_DONATIONS,
    PAGE_D_OFFERS,
    PAGE_D_GIVEN,
    PAGE_D_ADD,
    PAGE_R_HOME,
    PAGE_R_INCOMING,
    PAGE_R_PAST,
    PAGE_R_RECEIVED,
    PAGE_R_EDIT,
    PAGE_COUNT
};

// The base of every page.
class guipage
{
public:
    // Remembers the number, the short name (used in test logs) and the title of the page.
    guipage(int id, const string& name, const string& title);

    // Destroys the page window.
    virtual ~guipage();

    // Makes the page window below the host, then lets the page make its controls and gives them their fonts.
    void build(HWND host);

    // Makes the controls of the page (called once, by build). The tab order is the order of creation.
    virtual void createcontrols() = 0;

    // Fills the page from the data again.
    virtual void refresh() = 0;

    // Called when the page is about to be shown. The default refreshes the page.
    virtual void onshow();

    // Moves every control for a page of this size (in pixels).
    virtual void onsize(int width, int height) = 0;

    // A control of the page was used. Returns true when the page handled it.
    virtual bool command(int id, int code);

    // A control of the page sent a notification (for example a list changed its selection).
    virtual bool notify(const NMHDR* header);

    // The id of the primary button, or 0 when the page has none (Enter presses it).
    virtual int defaultid() const;

    // The most lists the page may show (the calm rules say 1, the match page says 2).
    virtual int maxlists() const;

    // The most push buttons the page may show (the primary one and two others).
    virtual int maxbuttons() const;

    // The page whose sidebar item stays selected while this page is shown.
    virtual int navid() const;

    // The number of rows in the main list of the page (0 when the page has no list). The test log shows it.
    virtual int rows() const;

    // The window of the page.
    HWND window() const;

    // The number of the page.
    int id() const;

    // The short name of the page.
    const string& name() const;

    // The title of the page.
    const string& title() const;

protected:
    // Makes the title static (the first control of every page) and returns it.
    HWND addtitle();

    // Puts the title in the batch and returns the top of the content below it.
    int layouttitle(guilayout& layout, int left, int width);

    HWND window_;   // the page window
    HWND title_;    // the title static
    int id_;        // page number
    string name_;   // short name
    string title_text_; // the title shown at the top
};

// Registers the window class of the pages (call once).
void guiregisterpageclass(HINSTANCE instance);

// Makes a static text with a role (colour and font follow from the role).
HWND makelabel(HWND parent, const string& text, int role, int id = -1, bool centred = false);

// Makes a push button. The primary button is the default one (Enter presses it).
HWND makebutton(HWND parent, const string& text, int id, bool primary);

// Makes a text box. numberonly allows digits only.
HWND makeedit(HWND parent, int id, bool numberonly, int maxchars);

// Makes a drop-down list (the list part is as high as eight lines).
HWND makecombo(HWND parent, int id);

// Makes a check box.
HWND makecheck(HWND parent, const string& text, int id);

// The width of the text of a control on one line, in its font.
int textwidth(HWND control);

// The width a push button needs for its text with some room at both sides (never below 110 design pixels).
int buttonwidth(HWND button);

// The content of a page: the page width minus the same margin (24 design pixels) on the left and on the right.
void contentbox(int pagewidth, int& left, int& width);

// Places a row of buttons that ends at right and sits on bottom, the last button (the primary one) rightmost, all
// 32 design pixels high with 8 between them. Returns the top of the row.
int placebuttons(guilayout& layout, const vector<HWND>& buttons, int right, int bottom);

// The column of a form: centred and at most formwidth wide, or the full content width when formfullwidth is set.
void formcolumn(int pagewidth, int& left, int& width);

// Places a label above its field and a grey help text (or NULL) right of the field. Returns the top of the next field.
int placefield(guilayout& layout, HWND label, HWND field, HWND help, int left, int top, int width, int fieldwidth,
               int fieldheight);

// Marks a text as data (a name, an id, a value from the files). The punctuation check of the self-test skips such texts.
void markdata(HWND control);

// Tells whether a text was marked as data.
bool isdata(HWND control);

// The height that the text of a static control needs when it wraps at this width (in pixels).
int measureheight(HWND control, int width);

// Reads the text of a control without the spaces at both ends.
string readtext(HWND control);

// Reads a whole number of one to nine digits from a text box. Returns false when it is not one.
bool readnumber(HWND control, int& value);

#endif
