// GuiLayout.cpp: pixel scaling and batch positioning.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>

#include "GuiLayout.h"
#include "Theme.h"

#include <sstream>

using namespace std;

// Multiplies by the dpi of the window (the one that themesetdpi last got) and divides by 96.
int px(int design)
{
    return MulDiv(design, themedpi(), 96);
}

// Multiplies by the dpi that is given and divides by 96.
int px(int design, int dpi)
{
    return MulDiv(design, dpi, 96);
}

// Turns a whole number into text.
string numtext(int value)
{
    stringstream text;
    text << value;
    return text.str();
}

// The column is as wide as allowed and centred; a narrow window shrinks it to fit between the margins.
void centredcolumn(int totalwidth, int maxwidth, int margin, int& left, int& width)
{
    width = totalwidth - 2 * margin;

    if (width > maxwidth)
    {
        width = maxwidth;
    }

    if (width < 0)
    {
        width = 0;
    }

    left = (totalwidth - width) / 2;
}

// Nothing is placed yet.
guilayout::guilayout()
{
}

// Only remembers the rectangle.
void guilayout::place(HWND control, int left, int top, int width, int height)
{
    entry item;
    item.control = control;
    item.left = left;
    item.top = top;
    item.width = width;
    item.height = height;
    entries_.push_back(item);
}

// Uses BeginDeferWindowPos. If Windows refuses the batch, every control is moved one by one instead. SWP_NOCOPYBITS makes
// every moved control paint itself again: a centred text that grows would otherwise keep a copy of its old pixels.
void guilayout::apply()
{
    if (entries_.empty())
    {
        return;
    }

    HDWP batch = BeginDeferWindowPos((int)entries_.size());
    bool failed = (batch == NULL);

    for (size_t pos = 0; pos < entries_.size() && !failed; pos++)
    {
        const entry& item = entries_[pos];
        batch = DeferWindowPos(batch, item.control, NULL, item.left, item.top, item.width, item.height,
                               SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);

        if (batch == NULL)
        {
            failed = true;
        }
    }

    if (!failed)
    {
        EndDeferWindowPos(batch);
    }
    else
    {
        for (size_t pos = 0; pos < entries_.size(); pos++)
        {
            const entry& item = entries_[pos];
            SetWindowPos(item.control, NULL, item.left, item.top, item.width, item.height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
        }
    }

    entries_.clear();
}
