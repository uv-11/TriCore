// Theme.cpp: the one place that knows colours, fonts and brushes.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

#include "Theme.h"

#include <string.h>
#include <vector>

using namespace std;

// The property names under which a control remembers its role and that it is the sidebar list.
static const char* const roleproperty = "cbrole";
static const char* const navproperty = "cbnav";

// The design sizes (at 96 dpi) of the rows. A list row is 32 pixels high, a sidebar item 44.
static const int listrowdesign = 32;
static const int navrowdesign = 44;

static int dpivalue = 96;           // the dpi that every size follows now
static int dpioverride = 0;         // the dpi a test asked for (0 means none)
static HFONT fonts[FONT_COUNT] = {NULL, NULL, NULL, NULL, NULL, NULL};
static vector<HFONT> retired;       // fonts that were replaced and are deleted later
static HBRUSH pagebrush = NULL;     // window and page background
static HBRUSH sidebarbrush = NULL;  // sidebar background
static HBRUSH bannerbrush = NULL;   // the fill of a note banner
static char facename[LF_FACESIZE] = "Segoe UI"; // the face of the Windows message font
static BYTE facecharset = DEFAULT_CHARSET;

// The palette of docs/DESIGN_GUIDE.md (Astryx light theme; colours with transparency are already mixed over white).
// PAGE_STYLE 0 keeps the soft off-white page and the warm sidebar the user chose; 1 is the guide's white page and cool grey sidebar.
#define PAGE_STYLE 0
#if PAGE_STYLE == 1
static const COLORREF palettepage = RGB(255, 255, 255);     // --color-background-surface
static const COLORREF palettesidebar = RGB(241, 244, 247);  // --color-background-body
#else
static const COLORREF palettepage = RGB(250, 248, 244);     // soft warm paper: the window, the pages and the lists
static const COLORREF palettesidebar = RGB(240, 237, 230);  // a little deeper than the page
#endif
static const COLORREF palettetext = RGB(10, 19, 23);        // --color-text-primary
static const COLORREF palettemuted = RGB(78, 96, 111);      // --color-text-secondary
static const COLORREF paletteaccent = RGB(0, 100, 224);     // --color-accent (only the program name)
static const COLORREF paletteerror = RGB(123, 2, 16);       // --color-text-red
static const COLORREF palettesuccess = RGB(9, 68, 31);      // --color-text-green
static const COLORREF palettewarning = RGB(117, 63, 7);     // --color-text-yellow
static const COLORREF paletteinfo = RGB(4, 47, 151);        // --color-text-blue
static const COLORREF paletteerrorfill = RGB(249, 209, 216);   // --color-error-muted
static const COLORREF palettesuccessfill = RGB(206, 235, 210); // --color-success-muted
static const COLORREF palettewarningfill = RGB(249, 237, 204); // --color-warning-muted (the notes banner)
static const COLORREF paletteinfofill = RGB(204, 230, 254);    // --color-info-muted
static const COLORREF paletteselect = RGB(204, 230, 254);   // --color-accent-muted: a chosen line
// The window and page background.
static COLORREF colorpage()
{
    return palettepage;
}

// The sidebar background.
static COLORREF colorsidebar()
{
    return palettesidebar;
}

// All normal text.
static COLORREF colortext()
{
    return palettetext;
}

// Captions, helper lines and empty-list messages.
static COLORREF colormuted()
{
    return palettemuted;
}

// Only the program name in the sidebar.
static COLORREF coloraccent()
{
    return paletteaccent;
}

// Only the one validation or error line.
static COLORREF colorerror()
{
    return paletteerror;
}
// One colour of the palette by its kind.
COLORREF themecolorof(int kind)
{
    if (kind == COLOR_SIDEBAR)
    {
        return colorsidebar();
    }

    if (kind == COLOR_TEXT)
    {
        return colortext();
    }

    if (kind == COLOR_MUTED)
    {
        return colormuted();
    }

    if (kind == COLOR_ACCENT)
    {
        return coloraccent();
    }

    if (kind == COLOR_ERROR)
    {
        return colorerror();
    }

    if (kind == COLOR_SELECT)
    {
        return paletteselect;
    }

    return colorpage();
}

// Makes one font of the message font face: a size in design pixels at the present dpi and a weight.
static HFONT makefont(int design, int weight)
{
    LOGFONTA font;
    ZeroMemory(&font, sizeof(font));
    strncpy(font.lfFaceName, facename, LF_FACESIZE - 1);
    font.lfCharSet = facecharset;
    font.lfHeight = -MulDiv(design, dpivalue, 96);
    font.lfWeight = weight;
    font.lfQuality = CLEARTYPE_QUALITY;
    return CreateFontIndirectA(&font);
}

// The Windows 10 function that reads the message font for a dpi (loaded by name, the old headers lack it).
typedef BOOL(WINAPI* infofordpi)(UINT, UINT, PVOID, UINT, UINT);

// The Windows 10 function that tells the dpi of a window.
typedef UINT(WINAPI* dpiforwindow)(HWND);

// The Windows 10 function that tells the dpi of the system.
typedef UINT(WINAPI* dpiforsystem)(void);

// The non-client metrics in the size that Windows Vista and later expect (the old header has the smaller one).
struct metricsbox
{
    NONCLIENTMETRICSW metrics;
    int padded;
};

// Reads the face of the Windows message font. At the dpi given the new function is used when Windows has it, which
// gives the font that Windows itself shows at that scale; otherwise the old call answers.
static void readmessagefont(int dpi)
{
    HMODULE user = GetModuleHandleA("user32.dll");
    infofordpi function = user == NULL ? NULL : (infofordpi)GetProcAddress(user, "SystemParametersInfoForDpi");

    if (function != NULL)
    {
        metricsbox box;
        ZeroMemory(&box, sizeof(box));
        box.metrics.cbSize = sizeof(box);

        if (function(SPI_GETNONCLIENTMETRICS, sizeof(box), &box, 0, (UINT)dpi))
        {
            char converted[LF_FACESIZE];
            converted[0] = '\0';
            WideCharToMultiByte(CP_ACP, 0, box.metrics.lfMessageFont.lfFaceName, -1, converted, LF_FACESIZE, NULL, NULL);

            if (converted[0] != '\0')
            {
                strncpy(facename, converted, LF_FACESIZE - 1);
                facecharset = box.metrics.lfMessageFont.lfCharSet;
                return;
            }
        }
    }

    NONCLIENTMETRICSA metrics;
    ZeroMemory(&metrics, sizeof(metrics));
    metrics.cbSize = sizeof(metrics);

    if (SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0) && metrics.lfMessageFont.lfFaceName[0] != '\0')
    {
        strncpy(facename, metrics.lfMessageFont.lfFaceName, LF_FACESIZE - 1);
        facecharset = metrics.lfMessageFont.lfCharSet;
    }
}

// Makes the six fonts for the present dpi (sizes of the design guide). The fonts that were there before are kept in the retired list.
static void makefonts()
{
    for (int kind = 0; kind < FONT_COUNT; kind++)
    {
        if (fonts[kind] != NULL)
        {
            retired.push_back(fonts[kind]);
        }
    }

    fonts[FONT_BODY] = makefont(14, FW_NORMAL);
    fonts[FONT_TITLE] = makefont(24, FW_SEMIBOLD);
    fonts[FONT_CAPTION] = makefont(12, FW_NORMAL);
    fonts[FONT_SECTION] = makefont(17, FW_SEMIBOLD);
    fonts[FONT_WELCOME] = makefont(35, FW_NORMAL);
    fonts[FONT_LABEL] = makefont(14, FW_MEDIUM);
}

// Reads the dpi of the system and the message font, then makes the fonts and the two brushes.
void themeinit()
{
    HMODULE user = GetModuleHandleA("user32.dll");
    dpiforsystem system = user == NULL ? NULL : (dpiforsystem)GetProcAddress(user, "GetDpiForSystem");

    if (system != NULL)
    {
        dpivalue = (int)system();
    }
    else
    {
        HDC screen = GetDC(NULL);
        dpivalue = GetDeviceCaps(screen, LOGPIXELSX);
        ReleaseDC(NULL, screen);
    }

    if (dpivalue < 96)
    {
        dpivalue = 96;
    }

    readmessagefont(dpivalue);
    makefonts();

    pagebrush = CreateSolidBrush(colorpage());
    sidebarbrush = CreateSolidBrush(colorsidebar());
    bannerbrush = CreateSolidBrush(palettewarningfill);
}

// The dpi that every size follows now.
int themedpi()
{
    return dpivalue;
}

// A new dpi: the fonts are made again for it.
void themesetdpi(int dpi)
{
    if (dpi < 96)
    {
        dpi = 96;
    }

    dpivalue = dpi;
    readmessagefont(dpivalue);
    makefonts();
}

// The replaced fonts can go now: every control has a new one.
void themereleasefonts()
{
    for (size_t pos = 0; pos < retired.size(); pos++)
    {
        DeleteObject(retired[pos]);
    }

    retired.clear();
}

// A test asks for a dpi that is not the one of the screen.
void themesetdpioverride(int dpi)
{
    dpioverride = dpi;
}

// True when a test asked for a dpi.
bool themedpiisoverridden()
{
    return dpioverride > 0;
}

// The dpi of a window.
int themedpiforwindow(HWND window)
{
    if (dpioverride > 0)
    {
        return dpioverride;
    }

    HMODULE user = GetModuleHandleA("user32.dll");
    dpiforwindow function = user == NULL ? NULL : (dpiforwindow)GetProcAddress(user, "GetDpiForWindow");

    if (function != NULL && window != NULL)
    {
        UINT value = function(window);

        if (value >= 96)
        {
            return (int)value;
        }
    }

    return dpivalue;
}

// The height of the body font at a dpi: 14 design pixels, negative like LOGFONT.
int themebodyheight(int dpi)
{
    return -MulDiv(14, dpi, 96);
}

// One of the fonts.
HFONT themefont(int kind)
{
    return fonts[kind];
}

// The page brush.
HBRUSH themepagebrush()
{
    return pagebrush;
}

// The sidebar brush.
HBRUSH themesidebarbrush()
{
    return sidebarbrush;
}

// The role lives in a window property, so the controls need no subclassing.
void themesetrole(HWND control, int role)
{
    SetPropA(control, roleproperty, (HANDLE)(INT_PTR)role);
}

// A control without the property has the role ROLE_TEXT (0).
int themegetrole(HWND control)
{
    return (int)(INT_PTR)GetPropA(control, roleproperty);
}

// Chooses the font of a role.
static int fontofrole(int role)
{
    if (role == ROLE_TITLE)
    {
        return FONT_TITLE;
    }

    if (role == ROLE_WELCOME)
    {
        return FONT_WELCOME;
    }

    if (role == ROLE_MUTED || role == ROLE_SIDEMUTED)
    {
        return FONT_CAPTION;
    }

    if (role == ROLE_APPNAME || role == ROLE_VALUE)
    {
        return FONT_SECTION;
    }

    if (role == ROLE_LABEL)
    {
        return FONT_LABEL;
    }

    return FONT_BODY;
}

// Called once for every control below the parent.
static BOOL CALLBACK applyfontproc(HWND control, LPARAM unused)
{
    (void)unused;
    SendMessageA(control, WM_SETFONT, (WPARAM)themefont(fontofrole(themegetrole(control))), FALSE);
    return TRUE;
}

// Every control below the parent gets the font of its role.
void themeapplyfonts(HWND parent)
{
    EnumChildWindows(parent, applyfontproc, 0);
}

// The old Windows headers do not know this flag: the list paints into an off-screen bitmap, which avoids flicker.
#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER 0x00010000
#endif

// The sidebar list uses the light "Explorer" look of Windows (a clear grey selection, also when the list has no focus).
// The lists of the pages keep the plain default look: the "Explorer" look draws the column dividers down into the empty
// part of a list, which showed as stray lines (stage U3, see DECISIONS.md).
static void settheme(HWND list, bool explorer)
{
    SetWindowTheme(list, explorer ? L"Explorer" : NULL, NULL);
}

// Gives a list view an empty image list of this size. The row height of a report list follows the height of its
// image list, and the width is the room before the text. The old image list is deleted.
static void setspacer(HWND list, int width, int height)
{
    HIMAGELIST spacer = ImageList_Create(width, height, ILC_COLOR, 1, 1);
    HIMAGELIST old = (HIMAGELIST)SendMessageA(list, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)spacer);

    if (old != NULL)
    {
        ImageList_Destroy(old);
    }
}

// The sidebar list: the sidebar colours, rows of 44 design pixels and 8 design pixels of room before the text.
void themestylenav(HWND list)
{
    SetPropA(list, navproperty, (HANDLE)1);
    SendMessageA(list, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER,
                 LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    SendMessageA(list, LVM_SETBKCOLOR, 0, (LPARAM)colorsidebar());
    SendMessageA(list, LVM_SETTEXTBKCOLOR, 0, (LPARAM)colorsidebar());
    SendMessageA(list, LVM_SETTEXTCOLOR, 0, (LPARAM)colortext());
    // the "ItemsView" look gives a soft selection that stays visible without the focus; the "Explorer" look would draw
    // a column edge line down the sidebar, and the plain look leaves the left edge of the selection and the unfocused
    // selection almost in the sidebar colour
    SetWindowTheme(list, L"ItemsView", NULL);
    setspacer(list, 1, MulDiv(navrowdesign, dpivalue, 96));
}

// A list of a page: the page colours and rows of 32 design pixels.
void themestylelist(HWND list)
{
    SendMessageA(list, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER,
                 LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    SendMessageA(list, LVM_SETBKCOLOR, 0, (LPARAM)colorpage());
    SendMessageA(list, LVM_SETTEXTBKCOLOR, 0, (LPARAM)colorpage());
    SendMessageA(list, LVM_SETTEXTCOLOR, 0, (LPARAM)colortext());
    settheme(list, false);
    setspacer(list, 1, MulDiv(listrowdesign, dpivalue, 96));
}

// Called once for every window below the parent: a list view is styled again.
static BOOL CALLBACK restyleproc(HWND control, LPARAM unused)
{
    (void)unused;
    char name[64];
    name[0] = '\0';
    GetClassNameA(control, name, (int)sizeof(name));

    if (strcmp(name, "ListBox") == 0 && (GetWindowLongA(control, GWL_STYLE) & LBS_OWNERDRAWFIXED) != 0)
    {
        SendMessageA(control, LB_SETITEMHEIGHT, 0, (LPARAM)themelistboxheight());
    }

    if (strcmp(name, WC_LISTVIEWA) == 0)
    {
        if (GetPropA(control, navproperty) != NULL)
        {
            themestylenav(control);
        }
        else
        {
            themestylelist(control);
        }
    }

    return TRUE;
}

// Every list below the parent is styled again for the present dpi.
void themerestylelists(HWND parent)
{
    EnumChildWindows(parent, restyleproc, 0);
}

// The status bar gets the sidebar tone as its background (the message SB_SETBKCOLOR, a standard message).
void themestylestatus(HWND status)
{
    // the themed status bar ignores the colour message, so the bar gets the classic look; its one part is drawn by the program
    SetWindowTheme(status, L"", L"");
    SendMessageA(status, SB_SETBKCOLOR, 0, (LPARAM)colorsidebar());
}

// The status text is drawn on the sidebar tone with the normal text colour, 12 design pixels from the left edge.
void themedrawstatus(const DRAWITEMSTRUCT* item, const char* text)
{
    HBRUSH brush = CreateSolidBrush(colorsidebar());
    FillRect(item->hDC, &item->rcItem, brush);
    DeleteObject(brush);

    if (text == NULL || text[0] == '\0')
    {
        return;
    }

    RECT area = item->rcItem;
    area.left = area.left + MulDiv(12, dpivalue, 96);
    HFONT old = (HFONT)SelectObject(item->hDC, (HFONT)SendMessageA(item->hwndItem, WM_GETFONT, 0, 0));
    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, colortext());
    DrawTextA(item->hDC, text, -1, &area, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
    SelectObject(item->hDC, old);
}
// The header of a list: the page tone, the title in the normal text colour 8 design pixels from the left of its cell and a
// thin line under it in the sidebar tone. The control draws the cells one by one; the part to the right of the last cell is
// painted again at the end, because the control paints that part itself.
LRESULT themedrawheader(NMCUSTOMDRAW* draw)
{
    HWND header = draw->hdr.hwndFrom;

    if (draw->dwDrawStage == CDDS_PREPAINT)
    {
        RECT all;
        GetClientRect(header, &all);
        FillRect(draw->hdc, &all, pagebrush);
        return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
    }

    if (draw->dwDrawStage == CDDS_ITEMPREPAINT)
    {
        char text[128];
        text[0] = '\0';
        HDITEMA item;
        ZeroMemory(&item, sizeof(item));
        item.mask = HDI_TEXT;
        item.pszText = text;
        item.cchTextMax = (int)sizeof(text);
        SendMessageA(header, HDM_GETITEMA, (WPARAM)draw->dwItemSpec, (LPARAM)&item);

        FillRect(draw->hdc, &draw->rc, pagebrush);
        RECT area = draw->rc;
        area.left = area.left + MulDiv(8, dpivalue, 96);
        HFONT old = (HFONT)SelectObject(draw->hdc, (HFONT)SendMessageA(header, WM_GETFONT, 0, 0));
        SetBkMode(draw->hdc, TRANSPARENT);
        SetTextColor(draw->hdc, colortext());
        DrawTextA(draw->hdc, text, -1, &area, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
        SelectObject(draw->hdc, old);
        return CDRF_SKIPDEFAULT;
    }

    if (draw->dwDrawStage == CDDS_POSTPAINT)
    {
        RECT all;
        GetClientRect(header, &all);
        int count = (int)SendMessageA(header, HDM_GETITEMCOUNT, 0, 0);
        RECT last;
        ZeroMemory(&last, sizeof(last));

        if (count > 0)
        {
            SendMessageA(header, HDM_GETITEMRECT, (WPARAM)(count - 1), (LPARAM)&last);
        }

        RECT rest = all;
        rest.left = last.right;

        if (rest.left < rest.right)
        {
            FillRect(draw->hdc, &rest, pagebrush);
        }

        RECT line = all;
        line.top = line.bottom - 1;
        FillRect(draw->hdc, &line, sidebarbrush);
        return CDRF_DODEFAULT;
    }

    return CDRF_DODEFAULT;
}

// A line of the name list is 32 design pixels high.
int themelistboxheight()
{
    return MulDiv(listrowdesign, dpivalue, 96);
}

// One line of the name list: the chosen line has the selection tone, the others the page tone; no focus rectangle.
void themedrawlistbox(const DRAWITEMSTRUCT* item)
{
    if (item->itemID == (UINT)-1)
    {
        return;
    }

    bool chosen = (item->itemState & ODS_SELECTED) != 0;
    HBRUSH brush = CreateSolidBrush(chosen ? paletteselect : colorpage());
    FillRect(item->hDC, &item->rcItem, brush);
    DeleteObject(brush);

    char text[256];
    text[0] = '\0';
    SendMessageA(item->hwndItem, LB_GETTEXT, (WPARAM)item->itemID, (LPARAM)text);
    RECT area = item->rcItem;
    area.left = area.left + MulDiv(8, dpivalue, 96);
    HFONT old = (HFONT)SelectObject(item->hDC, (HFONT)SendMessageA(item->hwndItem, WM_GETFONT, 0, 0));
    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, colortext());
    DrawTextA(item->hDC, text, -1, &area, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
    SelectObject(item->hDC, old);
}
// Picks the text colour and the background brush of a control from its role.
bool themecolour(UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& result)
{
    // a list box (the name list of the welcome page) has the page colours as well; its selection is the one of the system
    if (msg == WM_CTLCOLORLISTBOX)
    {
        SetTextColor((HDC)wparam, colortext());
        SetBkColor((HDC)wparam, colorpage());
        result = (LRESULT)pagebrush;
        return true;
    }

    if (msg != WM_CTLCOLORSTATIC && msg != WM_CTLCOLORBTN)
    {
        return false;
    }

    HDC dc = (HDC)wparam;
    int role = themegetrole((HWND)lparam);
    bool sidebar = (role == ROLE_SIDETEXT || role == ROLE_SIDEMUTED || role == ROLE_APPNAME);
    COLORREF text = colortext();

    if (role == ROLE_APPNAME)
    {
        text = coloraccent();
    }
    else if (role == ROLE_MUTED || role == ROLE_SIDEMUTED)
    {
        text = colormuted();
    }
    else if (role == ROLE_ERROR)
    {
        text = colorerror();
    }

    if (role == ROLE_BANNER)
    {
        SetTextColor(dc, palettewarning);
        SetBkColor(dc, palettewarningfill);
        result = (LRESULT)bannerbrush;
        return true;
    }

    SetTextColor(dc, text);
    SetBkColor(dc, sidebar ? colorsidebar() : colorpage());
    result = (LRESULT)(sidebar ? sidebarbrush : pagebrush);
    return true;
}
