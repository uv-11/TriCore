#ifndef THEME_H
#define THEME_H

// Theme.h: every colour and font of the program. No colour value and no font name may appear anywhere else.
// The look is calm and light: a soft page, a slightly different sidebar, near-black text, grey helper lines,
// a blue title and a red error line. Standard controls keep their normal themed look.
// Every size follows the dpi of the window: the program has one top-level window and all other windows are its
// children, so the dpi of the main window is the dpi of every window (see DECISIONS.md, S2).

#include <windows.h>
#include <commctrl.h>

// The kind of text a static control shows. A control without a role is normal text.
enum themerole
{
    ROLE_TEXT,       // normal text on the page
    ROLE_TITLE,      // the page title (accent colour, large, bold)
    ROLE_MUTED,      // captions, helper lines and empty-list messages (grey, small)
    ROLE_ERROR,      // the one validation or error line (red)
    ROLE_APPNAME,    // the program name at the top of the sidebar (accent colour, bold)
    ROLE_SIDETEXT,   // normal text on the sidebar
    ROLE_SIDEMUTED,  // a grey caption on the sidebar
    ROLE_WELCOME,    // the big title of the welcome page
    ROLE_VALUE,      // a number of a Home page (17 px semibold, under its caption)
    ROLE_LABEL,      // the label above a field (14 px medium)
    ROLE_BANNER      // a note about the data on the coordinator's Home page (warning text on the warning fill)
};

// The six fonts of the design guide, in design pixels (96 dpi), all in the Windows message font face.
enum themefontkind
{
    FONT_BODY,     // 14 px, normal
    FONT_TITLE,    // 24 px, semibold (the page title)
    FONT_CAPTION,  // 12 px, normal
    FONT_SECTION,  // 17 px, semibold (the program name and section headings)
    FONT_WELCOME,  // 35 px, normal (the title of the welcome page)
    FONT_LABEL,    // 14 px, medium (the label above a field)
    FONT_COUNT
};

// The colours of the palette (the tests read them to check contrast and the evenness of the sidebar).
enum themecolorkind
{
    COLOR_PAGE,     // the background of the pages, the lists and the window
    COLOR_SIDEBAR,  // the background of the sidebar
    COLOR_TEXT,     // normal text
    COLOR_MUTED,    // captions and helper lines
    COLOR_ACCENT,   // titles and the program name
    COLOR_ERROR,    // the error line
    COLOR_SELECT    // the selected line of the name list on the welcome page
};

// Reads the dpi of the screen and makes the fonts and brushes. Call once, before any window is made.
void themeinit();

// Returns the dpi that all sizes follow now (96 means 100 percent scaling).
int themedpi();

// Changes the dpi (a window moved to another monitor, the scale changed, or a test asks for it). The fonts are made
// again; the old ones are kept until themereleasefonts() so controls that still use them stay valid.
void themesetdpi(int dpi);

// Deletes the fonts that themesetdpi replaced. Call it after every control got the new fonts.
void themereleasefonts();

// Test modes only: makes themedpiforwindow() answer this dpi (0 switches the override off).
void themesetdpioverride(int dpi);

// Tells whether a test dpi is set.
bool themedpiisoverridden();

// The dpi of a window: the override, else GetDpiForWindow (loaded at run time, it needs Windows 10 1607), else the
// dpi that themeinit read.
int themedpiforwindow(HWND window);

// Returns one of the fonts. It lives until the next dpi change.
HFONT themefont(int kind);

// Returns the height in pixels that the body font has at a dpi (negative, as in LOGFONT).
int themebodyheight(int dpi);

// Returns a colour of the palette.
COLORREF themecolorof(int kind);

// Remembers the text role of a static control (it decides colour and font).
void themesetrole(HWND control, int role);

// Returns the role of a control (ROLE_TEXT when none was set).
int themegetrole(HWND control);

// Gives every control below the parent the font of its role.
void themeapplyfonts(HWND parent);

// Gives the sidebar list its look: the sidebar background, no header, rows of the sidebar item height and a
// little room in front of the text, and the light "Explorer" selection of Windows.
// The list view is a standard control; nothing is drawn by the program.
void themestylenav(HWND list);

// Gives a list its look: the page background, the text colours, rows of 32 design pixels (an empty image list of that
// height sets the row height) and the "Explorer" theme. Call it again after a dpi change.
void themestylelist(HWND list);

// Styles every list below the parent again (after a dpi change). The sidebar list gets its own style.
void themerestylelists(HWND parent);

// Gives the status bar the sidebar tone as its background: the classic look (so the colour message is obeyed).
void themestylestatus(HWND status);

// Custom drawing that the owner of this round allowed for three things only (decision D20): the header of a list, the status
// bar and the selected line of the name list on the welcome page. Every colour comes from the palette.

// Draws the header of a list in the page tone (answers NM_CUSTOMDRAW of a header; returns what the window procedure returns).
LRESULT themedrawheader(NMCUSTOMDRAW* draw);

// The height of a line of the owner drawn name list (32 design pixels).
int themelistboxheight();

// Draws one line of the owner drawn name list: the page tone, or the selection tone for the chosen line, and the text.
void themedrawlistbox(const DRAWITEMSTRUCT* item);

// Draws the text of the status bar on the sidebar tone.
void themedrawstatus(const DRAWITEMSTRUCT* item, const char* text);

// Answers the WM_CTLCOLOR messages of static texts, check boxes and radio buttons.
// Returns true when the message was handled; the window procedure then returns result.
bool themecolour(UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& result);

// The brush of the window and page background.
HBRUSH themepagebrush();

// The brush of the sidebar background.
HBRUSH themesidebarbrush();

#endif
