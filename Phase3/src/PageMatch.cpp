// PageMatch.cpp: the Match page. It is reached from the Donations page, shows the recipients that can take one donation
// and the ones that cannot, and sends an offer. Closing a donation is not done here, it has its home on Donations.
// It is a page of the one window, not a dialog.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>

#include "AppState.h"
#include "Exceptions.h"
#include "GuiList.h"
#include "GuiMessage.h"
#include "GuiShell.h"
#include "Match.h"
#include "PageTable.h"
#include "Pages.h"
#include "Tables.h"
#include "TextTools.h"
#include "Theme.h"
#include "Workflow.h"
#include "resource.h"

#include <stdlib.h>
#include <string>

using namespace std;

// The six facts about the donation, left to right: a grey caption above a value in normal text.
static const int factcount = 6;

// The page that matches one donation.
class pagematch : public guipage
{
public:
    // The base class gets the page number, the short name and the title (a base-class call needs the initializer list).
    pagematch() : guipage(PAGE_C_MATCH, "match", "Match a donation")
    {
        for (int pos = 0; pos < factcount; pos++)
        {
            factcaptions_[pos] = NULL;
            factvalues_[pos] = NULL;
        }

        cancaption_ = NULL;
        cannotcaption_ = NULL;
        quantitylabel_ = NULL;
        quantity_ = NULL;
        error_ = NULL;
        back_ = NULL;
        send_ = NULL;
        position_ = -1;
        width_ = 0;
        height_ = 0;
    }

    // Chooses the donation (by its position in the donation list) before the page is shown.
    void setdonation(int position)
    {
        position_ = position;
    }

    // The captions and values of the donation, the two lists with their captions, the quantity and the two buttons,
    // in the order of the Tab key.
    void createcontrols()
    {
        static const char* const captions[factcount] = {"Donation", "Donor", "Food", "Left", "Already offered", "Usable minutes"};

        addtitle();

        for (int pos = 0; pos < factcount; pos++)
        {
            factcaptions_[pos] = makelabel(window_, captions[pos], ROLE_MUTED);
        }

        for (int pos = 0; pos < factcount; pos++)
        {
            factvalues_[pos] = makelabel(window_, "", ROLE_TEXT);
            markdata(factvalues_[pos]);
        }

        cancaption_ = makelabel(window_, "Recipients that can take it", ROLE_MUTED);
        can_.create(window_, IDC_LIST_MAIN, "No recipient can take this donation right now");
        cannotcaption_ = makelabel(window_, "Recipients that cannot", ROLE_MUTED);
        cannot_.create(window_, IDC_LIST_SECOND, "Every recipient could take it");
        quantitylabel_ = makelabel(window_, "Quantity", ROLE_LABEL);
        quantity_ = makeedit(window_, IDC_M_QTY, true, 9);
        error_ = makelabel(window_, "", ROLE_ERROR, IDC_M_ERROR);
        back_ = makebutton(window_, "Back", IDC_M_BACK, false);
        send_ = makebutton(window_, "Send offer", IDC_M_SEND, true);
    }

    // The page starts clean every time it is shown.
    void onshow()
    {
        SetWindowTextA(quantity_, "");
        SetWindowTextA(error_, "");
        refresh();
    }

    // Builds the match again from the data and fills the values and both lists, then lays the page out again because
    // the lists may need another width now.
    void refresh()
    {
        fillpage();

        if (width_ > 0)
        {
            onsize(width_, height_);
        }
    }

    // Fills the values and both lists from the data.
    void fillpage()
    {
        for (int pos = 0; pos < factcount; pos++)
        {
            SetWindowTextA(factvalues_[pos], "");
        }

        if (position_ < 0 || position_ >= (int)theapp->data.donations.size())
        {
            can_.fill(texttable());
            cannot_.fill(texttable());
            EnableWindow(send_, FALSE);
            return;
        }

        try
        {
            match_ = buildmatch(theapp->data, theapp->offers, *theapp->provider, position_);
        }
        catch (const AppException& problem)
        {
            SetWindowTextA(error_, ("This donation cannot be matched because " + plaintext(problem.what())).c_str());
            can_.fill(texttable());
            cannot_.fill(texttable());
            EnableWindow(send_, FALSE);
            return;
        }

        can_.fill(candidatestable(theapp->data, match_));
        cannot_.fill(rejectedtable(theapp->data, match_));

        // nothing can be sent when no recipient can take the donation
        EnableWindow(send_, can_.rowcount() > 0 ? TRUE : FALSE);

        const Donation& donation = theapp->data.donations[position_];
        string donor = donation.donorId().toString();
        int donorpos = 0;

        if (theapp->data.donorindex.get(donor, donorpos))
        {
            donor = theapp->data.donors[donorpos].name();
        }

        SetWindowTextA(factvalues_[0], donation.id().toString().c_str());
        SetWindowTextA(factvalues_[1], donor.c_str());
        SetWindowTextA(factvalues_[2], foodToStr(donation.foodType()).c_str());
        SetWindowTextA(factvalues_[3], numtext(donation.remainingQuantity()).c_str());
        SetWindowTextA(factvalues_[4], numtext(match_.pendingoffers).c_str());
        SetWindowTextA(factvalues_[5], numtext(donation.usableMinutes()).c_str());
    }

    // The facts in equal columns, two lists across the content width, and a bottom row with the quantity (its label
    // above it) on the left, the red line next to it and the two buttons on the right.
    void onsize(int width, int height)
    {
        width_ = width;
        height_ = height;
        int left = 0;
        int contentwidth = 0;
        contentbox(width, left, contentwidth);
        guilayout layout;

        // this page has the most on it, so the gap under the title is the small gap
        int top = layouttitle(layout, left, contentwidth) - px(pagemargin - gapsmall);
        // one row of facts in equal columns when every fact fits its share; else every column is as wide as its caption or
        // value and the donor name takes the rest and may wrap
        int gap = px(gapbox);
        int share = (contentwidth - (factcount - 1) * gap) / factcount;
        int widths[factcount];
        int used = (factcount - 1) * gap;
        bool equal = true;

        for (int pos = 0; pos < factcount; pos++)
        {
            int caption = textwidth(factcaptions_[pos]);
            int value = textwidth(factvalues_[pos]);
            widths[pos] = caption > value ? caption : value;
            used = used + (pos == 1 ? 0 : widths[pos]);
            equal = equal && widths[pos] <= share;
        }

        widths[1] = contentwidth - used;

        for (int pos = 0; equal && pos < factcount; pos++)
        {
            widths[pos] = share;
        }
        int x = left;

        for (int pos = 0; pos < factcount; pos++)
        {
            layout.place(factcaptions_[pos], x, top, widths[pos], px(captionline));
            layout.place(factvalues_[pos], x, top + px(captionline + gaptiny), widths[pos], 2 * px(textline));
            x = x + widths[pos] + gap;
        }

        top = top + px(captionline + gaptiny + 2 * textline + gapsmall);

        vector<HWND> buttons;
        buttons.push_back(back_);
        buttons.push_back(send_);
        int rowtop = placebuttons(layout, buttons, left + contentwidth, height - px(pagemargin));
        int labeltop = rowtop - px(captionline + gaptiny);
        layout.place(quantitylabel_, left, labeltop, px(numberwidth), px(captionline));
        layout.place(quantity_, left, rowtop, px(numberwidth), px(controlheight));
        int errorleft = left + px(numberwidth + gapbox);
        int buttonsleft = left + contentwidth - buttonwidth(send_) - px(gapsmall) - buttonwidth(back_);
        int errorwidth = buttonsleft - px(gapbox) - errorleft;
        layout.place(error_, errorleft, labeltop, errorwidth < 0 ? 0 : errorwidth, rowtop + px(controlheight) - labeltop);

        // the room between the facts and the quantity label: the first list takes the rows it needs (at most half), the
        // second list the rest
        int room = labeltop - px(gapbox) - top - 2 * px(captionline + gaptiny) - px(gapsmall);
        room = room < 0 ? 0 : room;
        int first = can_.hugheight() < room / 2 ? can_.hugheight() : room / 2;
        layout.place(cancaption_, left, top, contentwidth, px(captionline));
        top = top + px(captionline + gaptiny);
        can_.place(layout, left, top, contentwidth, first);
        top = top + first + px(gapsmall);
        layout.place(cannotcaption_, left, top, contentwidth, px(captionline));
        top = top + px(captionline + gaptiny);
        cannot_.place(layout, left, top, contentwidth, room - first);
        layout.apply();
        can_.fitcolumns();
        cannot_.fitcolumns();
    }

    // Send offer and Back.
    bool command(int id, int code)
    {
        if (code != BN_CLICKED)
        {
            return false;
        }

        if (id == IDC_M_SEND)
        {
            sendoffer();
            return true;
        }

        if (id == IDC_M_BACK)
        {
            shellshow(PAGE_C_DONATIONS, true);
            return true;
        }

        return false;
    }

    // Enter sends the offer.
    int defaultid() const
    {
        return IDC_M_SEND;
    }

    // Two lists, the only page that has two.
    int maxlists() const
    {
        return 2;
    }

    // Back and Send offer.
    int maxbuttons() const
    {
        return 2;
    }

    // The sidebar keeps Donations selected while this page is shown.
    int navid() const
    {
        return PAGE_C_DONATIONS;
    }

    // The rows of the first list (the recipients that can take the donation).
    int rows() const
    {
        return can_.rowcount();
    }

private:
    // Shows one plain problem line.
    void problem(const string& text)
    {
        SetWindowTextA(error_, text.c_str());
    }

    // Needs a chosen recipient and a quantity, then calls the Workflow. After an offer the match is built again.
    void sendoffer()
    {
        string chosen = can_.selectedid();

        if (chosen.empty())
        {
            problem("Please choose a recipient from the first list");
            return;
        }

        int quantity = 0;

        if (!readnumber(quantity_, quantity))
        {
            problem("Please type the quantity as a whole number 1 or more");
            SetFocus(quantity_);
            return;
        }

        int number = atoi(chosen.c_str());
        string name = "the recipient";

        if (number >= 1 && number <= (int)match_.candidates.size())
        {
            int recipientpos = 0;

            if (theapp->data.recipientindex.get(match_.candidates[number - 1].recipientId().toString(), recipientpos))
            {
                name = theapp->data.recipients[recipientpos].name();
            }
        }

        string message = "";
        int offerpos = -1;

        try
        {
            offerpos = ::sendoffer(theapp->data, theapp->offers, *theapp->files, match_, number, quantity, message);
        }
        catch (const AppException& failure)
        {
            message = "The offer could not be sent because " + plaintext(failure.what());
        }

        if (offerpos < 0)
        {
            problem(message);
            return;
        }

        string sent = "Offer " + theapp->offers.at(offerpos).id().toString() + " sent to " + name;
        SetWindowTextA(quantity_, "");
        SetWindowTextA(error_, "");
        refresh();

        if (match_.effectiveremaining == 0)
        {
            shellshow(PAGE_C_DONATIONS, true);
        }

        shellstatus(sent);
    }

    HWND factcaptions_[6];  // the grey captions of the facts
    HWND factvalues_[6];    // the values of the facts
    HWND cancaption_;       // "Recipients that can take it"
    guilistview can_;       // the recipients that can take it
    HWND cannotcaption_;    // "Recipients that cannot"
    guilistview cannot_;    // the recipients that cannot
    HWND quantitylabel_;    // "Quantity"
    HWND quantity_;         // the quantity box
    HWND error_;            // the one red problem line
    HWND back_;             // Back
    HWND send_;             // Send offer (the primary button)
    int position_;          // the position of the donation in the donation list
    int width_;             // the width of the page at the last layout (0 before the first)
    int height_;            // the height of the page at the last layout
    matchview match_;       // the latest match
};

// Makes the match page.
guipage* newpagematch()
{
    return new pagematch();
}

// Chooses the donation and shows the page. The page is found by its number.
void openmatchpage(int donationpos)
{
    pagematch* page = static_cast<pagematch*>(shellpage(PAGE_C_MATCH));

    if (page == NULL)
    {
        return;
    }

    page->setdonation(donationpos);
    shellshow(PAGE_C_MATCH, true);
}
