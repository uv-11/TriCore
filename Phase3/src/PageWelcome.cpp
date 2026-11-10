// PageWelcome.cpp: the first page, in two steps inside the same window. Step 1 has three equal buttons, one for each
// role. The coordinator signs in at once; a donor or a recipient goes to step 2 to pick a name.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>

#include "AppState.h"
#include "GuiShell.h"
#include "Pages.h"
#include "Theme.h"
#include "resource.h"

#include <string>
#include <vector>

using namespace std;

// The welcome page.
class pagewelcome : public guipage
{
public:
    // The base class gets the page number, the short name and the title (a base-class call needs the initializer list).
    pagewelcome() : guipage(PAGE_WELCOME, "welcome", "CommunityBridge")
    {
        tagline_ = NULL;
        coord_ = NULL;
        donor_ = NULL;
        recip_ = NULL;
        pick_ = NULL;
        names_ = NULL;
        back_ = NULL;
        continue_ = NULL;
        who_ = WHO_NONE;
        step_ = 1;
        width_ = 0;
        height_ = 0;
    }

    // Makes the controls in the order of the tab key: the three role buttons, the names, Back and Continue.
    void createcontrols()
    {
        title_ = makelabel(window_, title_text_, ROLE_WELCOME, -1, true);
        tagline_ = makelabel(window_, "Send surplus food to the recipients who can take it", ROLE_MUTED, -1, true);
        coord_ = makebutton(window_, "Coordinator", IDC_W_COORD, false);
        donor_ = makebutton(window_, "Donor", IDC_W_DONOR, false);
        recip_ = makebutton(window_, "Recipient", IDC_W_RECIP, false);
        pick_ = makelabel(window_, "Pick your name", ROLE_MUTED);
        names_ = CreateWindowExA(0, "LISTBOX", "", WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
                                 0, 0, 10, 10, window_, (HMENU)(INT_PTR)IDC_W_NAMES, GetModuleHandleA(NULL), NULL);
        back_ = makebutton(window_, "Back", IDC_W_BACK, false);
        continue_ = makebutton(window_, "Continue", IDC_W_CONTINUE, true);
    }

    // Every time the page is shown (start, Switch user) it begins at step 1.
    void refresh()
    {
        who_ = WHO_NONE;
        SendMessageA(names_, LB_RESETCONTENT, 0, 0);
        ids_.clear();
        names_text_.clear();
        gostep(1);
    }

    // Step 1: one block (the title, one grey line and the three role buttons) centred in the window, its middle
    // at 42 percent of the height. Step 2: the title, a caption, the name list and two buttons, again one centred block.
    void onsize(int width, int height)
    {
        width_ = width;
        height_ = height;

        guilayout layout;
        int margin = px(pagemargin);
        int titleheight = px(welcometitle);
        int taglineheight = px(textline);

        // the title and the grey line are the same on both steps
        int fixedtop = titleheight + px(gapfield) + taglineheight + px(pagemargin);

        // step 1: three equal buttons of 200 by 56 design pixels with a gap of 20 in one centred row
        int rolewidth = px(rolebuttonwidth);
        int buttonheight = px(rolebuttonheight);
        int buttongap = px(gapbox);
        int rowwidth = 3 * rolewidth + 2 * buttongap;
        int rowleft = (width - rowwidth) / 2;
        int blockheight = fixedtop + buttonheight;
        int top = blocktop(height, blockheight, margin);
        int rowtop = top + fixedtop;
        layout.place(coord_, rowleft, rowtop, rolewidth, buttonheight);
        layout.place(donor_, rowleft + rolewidth + buttongap, rowtop, rolewidth, buttonheight);
        layout.place(recip_, rowleft + 2 * (rolewidth + buttongap), rowtop, rolewidth, buttonheight);

        // step 2: the caption and the list in a centred column of at most 560 design pixels, Back and Continue at the
        // bottom right of the list; the list is at most 320 design pixels high and at least 120
        int left = 0;
        int columnwidth = 0;
        centredcolumn(width, px(welcomewidth), margin, left, columnwidth);
        int stepbuttonheight = px(controlheight);
        int captionheight = px(captionline);
        int listheight = px(namelistheight);
        int around = fixedtop + captionheight + px(gaptiny) + px(gapbox) + stepbuttonheight;

        // the whole block must fit between the margin and its mirror image around 42 percent of the height, so its
        // middle stays where the first step has it
        int mostblock = 2 * ((height * 42) / 100 - margin);

        if (around + listheight > mostblock)
        {
            listheight = mostblock - around;
        }

        if (listheight < px(namelistminimum))
        {
            listheight = px(namelistminimum);
        }

        int step2top = blocktop(height, around + listheight, margin);
        int pickstop = step2top + fixedtop;
        int liststop = pickstop + captionheight + px(gaptiny);
        int buttonstop = liststop + listheight + px(gapbox);
        layout.place(pick_, left, pickstop, columnwidth, captionheight);
        layout.place(names_, left, liststop, columnwidth, listheight);

        int continuewidth = buttonwidth(continue_);
        int backwidth = buttonwidth(back_);
        layout.place(continue_, left + columnwidth - continuewidth, buttonstop, continuewidth, stepbuttonheight);
        layout.place(back_, left + columnwidth - continuewidth - px(gapsmall) - backwidth, buttonstop, backwidth, stepbuttonheight);

        // the title and the grey line belong to the step that is shown
        int titletop = (step_ == 2) ? step2top : top;
        layout.place(title_, margin, titletop, width - 2 * margin, titleheight);
        layout.place(tagline_, margin, titletop + titleheight + px(gapfield), width - 2 * margin, taglineheight);
        layout.apply();
    }

    // A role was chosen, a name was chosen, Back or Continue was pressed.
    bool command(int id, int code)
    {
        if (code == BN_CLICKED && id == IDC_W_COORD)
        {
            shellsignin(WHO_COORDINATOR, "", "");
            return true;
        }

        if (code == BN_CLICKED && (id == IDC_W_DONOR || id == IDC_W_RECIP))
        {
            choosewho(id == IDC_W_DONOR ? WHO_DONOR : WHO_RECIPIENT);
            return true;
        }

        if (code == BN_CLICKED && id == IDC_W_BACK)
        {
            gostep(1);
            SetFocus(coord_);
            return true;
        }

        if (id == IDC_W_NAMES && code == LBN_SELCHANGE)
        {
            updatecontinue();
            return true;
        }

        if (id == IDC_W_NAMES && code == LBN_DBLCLK)
        {
            proceed();
            return true;
        }

        if (code == BN_CLICKED && id == IDC_W_CONTINUE)
        {
            proceed();
            return true;
        }

        return false;
    }

    // Enter presses Continue, but only on step 2.
    int defaultid() const
    {
        return step_ == 2 ? IDC_W_CONTINUE : 0;
    }

    // Step 1 has three buttons; step 2 has two.
    int maxbuttons() const
    {
        return 3;
    }

    // The number of names in the list (step 2).
    int rows() const
    {
        return (int)SendMessageA(names_, LB_GETCOUNT, 0, 0);
    }

private:
    // The top of a block of this height whose middle is at 42 percent of the page height; never above the margin and
    // never so low that the block leaves the page.
    int blocktop(int pageheight, int blockheight, int margin)
    {
        int top = (pageheight * 42) / 100 - blockheight / 2;

        if (top > pageheight - margin - blockheight)
        {
            top = pageheight - margin - blockheight;
        }

        if (top < margin)
        {
            top = margin;
        }

        return top;
    }

    // Shows the controls of one step and hides the others. The two steps lay the title out in different places.
    void gostep(int step)
    {
        step_ = step;
        relayout();
        int first = (step == 1) ? SW_SHOWNA : SW_HIDE;
        int second = (step == 2) ? SW_SHOWNA : SW_HIDE;
        ShowWindow(coord_, first);
        ShowWindow(donor_, first);
        ShowWindow(recip_, first);
        ShowWindow(pick_, second);
        ShowWindow(names_, second);
        ShowWindow(back_, second);
        ShowWindow(continue_, second);
        updatecontinue();
    }

    // Lays the page out again for the size it has (the title moves between the two steps).
    void relayout()
    {
        if (width_ > 0)
        {
            onsize(width_, height_);
        }
    }

    // Fills the name list for a donor or a recipient and goes to step 2.
    void choosewho(int who)
    {
        who_ = who;
        SendMessageA(names_, LB_RESETCONTENT, 0, 0);
        ids_.clear();
        names_text_.clear();

        if (who == WHO_DONOR)
        {
            for (size_t pos = 0; pos < theapp->data.donors.size(); pos++)
            {
                addname(theapp->data.donors[pos].id().toString(), theapp->data.donors[pos].name());
            }
        }
        else
        {
            for (size_t pos = 0; pos < theapp->data.recipients.size(); pos++)
            {
                addname(theapp->data.recipients[pos].id().toString(), theapp->data.recipients[pos].name());
            }
        }

        gostep(2);
        SetFocus(names_);
    }

    // Adds one line "DN001  Name" to the list.
    void addname(const string& id, const string& name)
    {
        ids_.push_back(id);
        names_text_.push_back(name);
        string line = id + "  " + name;
        SendMessageA(names_, LB_ADDSTRING, 0, (LPARAM)line.c_str());
    }

    // Continue needs a chosen name.
    void updatecontinue()
    {
        bool ready = step_ == 2 && SendMessageA(names_, LB_GETCURSEL, 0, 0) != LB_ERR;
        EnableWindow(continue_, ready ? TRUE : FALSE);
    }

    // Signs in with the chosen name and leaves the welcome page.
    void proceed()
    {
        int chosen = (int)SendMessageA(names_, LB_GETCURSEL, 0, 0);

        if (step_ == 2 && chosen != LB_ERR && chosen < (int)ids_.size())
        {
            shellsignin(who_, ids_[chosen], names_text_[chosen]);
        }
    }

    HWND tagline_;        // the grey line under the title
    HWND coord_;          // button Coordinator
    HWND donor_;          // button Donor
    HWND recip_;          // button Recipient
    HWND pick_;           // the caption "Pick your name"
    HWND names_;          // the list of donors or recipients
    HWND back_;           // Back (step 2)
    HWND continue_;       // Continue (step 2)
    int who_;             // the role of step 2
    int step_;            // 1 or 2
    int width_;           // the width of the page when it was last laid out
    int height_;          // the height of the page when it was last laid out
    vector<string> ids_;          // the ids of the names in the list
    vector<string> names_text_;   // the names in the list, without the id
};

// Makes the welcome page.
guipage* newpagewelcome()
{
    return new pagewelcome();
}
