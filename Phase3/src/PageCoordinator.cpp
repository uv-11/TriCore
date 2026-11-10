// PageCoordinator.cpp: the pages of the coordinator. Every page reads the data through the backend and every
// action is one Workflow call; the pages hold no business logic. Every action has one home: matching and closing
// a donation are on Donations, withdrawing an offer is on Offers.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>

#include "AppState.h"
#include "Dashboard.h"
#include "Exceptions.h"
#include "GuiMessage.h"
#include "GuiShell.h"
#include "PageTable.h"
#include "Pages.h"
#include "Queries.h"
#include "Tables.h"
#include "TextTools.h"
#include "Theme.h"
#include "Workflow.h"
#include "resource.h"

#include <string>
#include <vector>

using namespace std;

// The most notes about the data that the Home page shows; the rest is counted in one more line.
static const int maxnotes = 6;

// A donation can be offered or closed while it is waiting or partly given.
static bool openstatus(DonationStatus status)
{
    return status == PENDING || status == PARTIAL;
}

// Home: five numbers and, only when the start-up found something, the notes about the data as plain lines.
class coordhome : public pagehome
{
public:
    // The base class gets the page number and the short name (a base-class call needs the initializer list).
    coordhome() : pagehome(PAGE_C_HOME, "home")
    {
        captions_.push_back("Open donations");
        captions_.push_back("Waiting for an offer");
        captions_.push_back("Offers pending");
        captions_.push_back("Food given out");
        captions_.push_back("Food wasted");
        notescaption_ = NULL;
        morenote_ = NULL;

        for (int pos = 0; pos < maxnotes; pos++)
        {
            notelabels_[pos] = NULL;
        }
    }

    // The lines come from the base page; the caption and the note lines are added after them.
    void createcontrols()
    {
        pagehome::createcontrols();
        notescaption_ = makelabel(window_, "Notes about the data", ROLE_MUTED);

        for (int pos = 0; pos < maxnotes; pos++)
        {
            notelabels_[pos] = makelabel(window_, "", ROLE_BANNER);
        }

        morenote_ = makelabel(window_, "", ROLE_TEXT);
    }

    // The numbers and the notes. Layout is done again because the number of notes can change.
    void refresh()
    {
        pagehome::refresh();
        notes_ = plainnotes(theapp->warnings, theapp->data);

        for (int pos = 0; pos < maxnotes; pos++)
        {
            SetWindowTextA(notelabels_[pos], pos < (int)notes_.size() ? notes_[pos].c_str() : "");
        }

        if (lastwidth_ > 0)
        {
            onsize(lastwidth_, lastheight_);
        }
    }

protected:
    // The numbers of the Dashboard.
    void values(vector<string>& out)
    {
        coordinatorstats stats = coordinatorstatsof(theapp->data, theapp->offers);
        out.push_back(numtext(stats.opendonations));
        out.push_back(numtext(stats.waiting));
        out.push_back(numtext(stats.pendingoffers));
        out.push_back(numtext(stats.givenout));
        out.push_back(numtext(stats.wasted));
    }

    // The notes sit under the lines. A note wraps onto more lines and is never cut off: when the next one does not fit
    // in the free room, the rest is counted in the line "and N more".
    void layoutextra(guilayout& layout, int left, int top, int width, int limit)
    {
        int total = (int)notes_.size();

        if (total == 0)
        {
            ShowWindow(notescaption_, SW_HIDE);
            ShowWindow(morenote_, SW_HIDE);

            for (int pos = 0; pos < maxnotes; pos++)
            {
                ShowWindow(notelabels_[pos], SW_HIDE);
            }

            return;
        }

        ShowWindow(notescaption_, SW_SHOWNA);
        layout.place(notescaption_, left, top, width, px(captionline));
        top = top + px(captionline + gaptiny);

        int shown = 0;
        int wanted = total < maxnotes ? total : maxnotes;

        for (int pos = 0; pos < maxnotes; pos++)
        {
            if (pos >= wanted)
            {
                ShowWindow(notelabels_[pos], SW_HIDE);
                continue;
            }

            int height = measureheight(notelabels_[pos], width) + px(gaptiny);
            int reserve = (total - (pos + 1) > 0) ? px(textline + gaptiny) : 0;

            if (shown > 0 && top + height + reserve > limit)
            {
                ShowWindow(notelabels_[pos], SW_HIDE);
                continue;
            }

            ShowWindow(notelabels_[pos], SW_SHOWNA);
            layout.place(notelabels_[pos], left, top, width, height);
            top = top + height + px(gapsmall);
            shown++;
        }

        if (total > shown)
        {
            SetWindowTextA(morenote_, ("and " + numtext(total - shown) + " more").c_str());
            ShowWindow(morenote_, SW_SHOWNA);
            layout.place(morenote_, left, top, width, px(textline));
        }
        else
        {
            ShowWindow(morenote_, SW_HIDE);
        }
    }

private:
    HWND notescaption_;           // "Notes about the data"
    HWND notelabels_[6];          // one wrapped line of text for each of the first notes
    HWND morenote_;               // "and N more"
    vector<string> notes_;        // every note, in plain words
};

// Donations: every donation, with the two buttons that act on the selected one. The most urgent donation is
// selected when the page is shown and nothing else is selected.
class coorddonations : public pagetable
{
public:
    // The base class gets the page number, the names and the empty message (a base-class call needs the initializer list).
    coorddonations() : pagetable(PAGE_C_DONATIONS, "donations", "Donations", "No donations yet")
    {
    }

    // Fills the list, then selects the most urgent donation when no row is selected.
    void refresh()
    {
        pagetable::refresh();

        if (list_.selectedid().empty())
        {
            int urgent = nexturgentdonation(theapp->data, theapp->offers);

            if (urgent >= 0)
            {
                list_.selectid(theapp->data.donations[urgent].id().toString());
            }
        }

        updatebuttons();
    }

protected:
    // Every donation.
    texttable maketable()
    {
        return donationstable(theapp->data, theapp->offers, allpositions((int)theapp->data.donations.size()));
    }

    // Close donation, then the primary Process donation.
    void addbuttons()
    {
        addbutton("Close donation", IDC_C_CLOSE_SEL, false);
        addbutton("Process donation", IDC_C_PROCESS_SEL, true);
    }

    // Both buttons need a donation that can still be offered.
    void updatebuttons()
    {
        int pos = selectedposition();
        bool open = pos >= 0 && openstatus(theapp->data.donations[pos].status());
        EnableWindow(buttons_[0], open ? TRUE : FALSE);
        EnableWindow(buttons_[1], open ? TRUE : FALSE);
    }

    // Process opens the match page; Close asks first.
    void clicked(int id)
    {
        int pos = selectedposition();

        if (pos < 0)
        {
            return;
        }

        if (id == IDC_C_PROCESS_SEL)
        {
            openmatchpage(pos);
        }
        else if (id == IDC_C_CLOSE_SEL)
        {
            if (!guiaskyes(window_, "Close this donation and record the food that is left as wasted?"))
            {
                return;
            }

            string message = "";
            bool closed = closedonationwithoffers(theapp->data, theapp->offers, *theapp->files, pos, message);
            refresh();

            if (closed)
            {
                shellstatus(message);
            }
            else
            {
                guishowerror(window_, message);
            }
        }
    }

private:
    // The position of the selected donation in the donation list, or -1.
    int selectedposition() const
    {
        int pos = -1;
        string id = list_.selectedid();

        if (!id.empty() && !theapp->data.donationindex.get(id, pos))
        {
            pos = -1;
        }

        return pos;
    }
};

// Offers: every offer; a waiting offer can be withdrawn.
class coordoffers : public pagetable
{
public:
    // The base class gets the page number, the names and the empty message (a base-class call needs the initializer list).
    coordoffers() : pagetable(PAGE_C_OFFERS, "offers", "Offers", "No offers yet")
    {
    }

protected:
    // Every offer.
    texttable maketable()
    {
        return offerstable(theapp->data, theapp->offers, allpositions(theapp->offers.count()));
    }

    // The only button.
    void addbuttons()
    {
        addbutton("Withdraw offer", IDC_C_WITHDRAW, true);
    }

    // Only a waiting offer can be withdrawn.
    void updatebuttons()
    {
        int pos = selectedposition();
        bool pending = pos >= 0 && theapp->offers.at(pos).status() == OFFER_PENDING;
        EnableWindow(buttons_[0], pending ? TRUE : FALSE);
    }

    // Withdraw the selected offer.
    void clicked(int id)
    {
        int pos = selectedposition();

        if (id != IDC_C_WITHDRAW || pos < 0)
        {
            return;
        }

        EntityId offerid = theapp->offers.at(pos).id();
        string message = "";
        bool done = withdrawoffer(theapp->offers, *theapp->files, offerid, message);
        refresh();

        if (done)
        {
            shellstatus("Offer " + offerid.toString() + " withdrawn");
        }
        else
        {
            guishowerror(window_, message);
        }
    }

private:
    // The position of the selected offer in the offer store, or -1.
    int selectedposition() const
    {
        int pos = -1;
        string id = list_.selectedid();

        if (!id.empty() && !theapp->offers.find(EntityId::parse(id, "OFF"), pos))
        {
            pos = -1;
        }

        return pos;
    }
};

// Recipients: every recipient. No buttons.
class coordrecipients : public pagetable
{
public:
    // The base class gets the page number, the names and the empty message (a base-class call needs the initializer list).
    coordrecipients() : pagetable(PAGE_C_RECIPIENTS, "recipients", "Recipients", "No recipients yet")
    {
    }

protected:
    // Every recipient.
    texttable maketable()
    {
        return recipientstable(theapp->data, allpositions((int)theapp->data.recipients.size()));
    }
};

// Transactions: all the food that was given out. No buttons.
class coordtransactions : public pagetable
{
public:
    // The base class gets the page number, the names and the empty message (a base-class call needs the initializer list).
    coordtransactions() : pagetable(PAGE_C_TRANSACTIONS, "transactions", "Transactions", "No food has been given out yet")
    {
    }

protected:
    // Every transaction.
    texttable maketable()
    {
        return transactionstable(theapp->data, allpositions((int)theapp->data.transactions.size()));
    }
};

// Summary: the totals as a short list and the three lines about the program as plain grey text under it. No buttons.
class coordsummary : public pagetable
{
public:
    // The base class gets the page number, the names and the empty message (a base-class call needs the initializer list).
    coordsummary() : pagetable(PAGE_C_SUMMARY, "summary", "Summary", "Nothing to summarise yet")
    {
        for (int pos = 0; pos < 3; pos++)
        {
            about_[pos] = NULL;
        }
    }

    // The title and the list, then the three lines of text (grey, small). They are statics, so they take no part in the Tab order.
    void createcontrols()
    {
        pagetable::createcontrols();
        vector<string> lines = aboutlines();

        for (int pos = 0; pos < 3 && pos < (int)lines.size(); pos++)
        {
            about_[pos] = makelabel(window_, lines[pos], ROLE_MUTED);
        }
    }

protected:
    // The totals.
    texttable maketable()
    {
        return summarytable(computetotals(theapp->data));
    }

    // The table is short, so the list is only as high as its rows and the text follows right under it.
    bool huglist() const
    {
        return true;
    }

    // The three lines, each as high as its text needs at this width, with a gap of 24 design pixels above them.
    int belowheight(int width)
    {
        int total = px(textline);

        for (int pos = 0; pos < 3; pos++)
        {
            total = total + measureheight(about_[pos], width) + px(gaptiny);
        }

        return total;
    }

    // Puts the three lines under the list.
    void layoutbelow(guilayout& layout, int left, int top, int width)
    {
        int y = top + px(textline);

        for (int pos = 0; pos < 3; pos++)
        {
            int height = measureheight(about_[pos], width);
            layout.place(about_[pos], left, y, width, height);
            y = y + height + px(gaptiny);
        }
    }

private:
    HWND about_[3];   // the three grey lines about the program
};

// Makes the coordinator pages.
guipage* newpagecoordhome()
{
    return new coordhome();
}

guipage* newpagecoorddonations()
{
    return new coorddonations();
}

guipage* newpagecoordoffers()
{
    return new coordoffers();
}

guipage* newpagecoordrecipients()
{
    return new coordrecipients();
}

guipage* newpagecoordtransactions()
{
    return new coordtransactions();
}

guipage* newpagecoordsummary()
{
    return new coordsummary();
}
