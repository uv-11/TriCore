// PageRecipient.cpp: the pages of a recipient. Every page reads the data through the backend and every action is one
// Workflow call; the pages hold no business logic. Every action has one home: answering offers is on Incoming offers,
// changing the details is on My details.

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
#include "RecipientUpdate.h"
#include "Tables.h"
#include "TextTools.h"
#include "Theme.h"
#include "Workflow.h"
#include "resource.h"

#include <string>
#include <vector>

using namespace std;

// The id of the recipient who is signed in.
static EntityId recipientid()
{
    return EntityId::parse(shellpersonid(), "R");
}

// True when a recipient is signed in (the pages are only shown then, but a test may refresh them earlier).
static bool recipientsignedin()
{
    return shellwho() == WHO_RECIPIENT && !shellpersonid().empty();
}

// The position of the signed-in recipient in the recipient list, or -1.
static int recipientposition()
{
    int pos = -1;

    if (!recipientsignedin() || !theapp->data.recipientindex.get(shellpersonid(), pos))
    {
        pos = -1;
    }

    return pos;
}

// Home: two numbers and nothing else (the profile lives on My details).
class recipienthome : public pagehome
{
public:
    // The base class gets the page number and the short name (a base-class call needs the initializer list).
    recipienthome() : pagehome(PAGE_R_HOME, "home")
    {
        captions_.push_back("Offers waiting");
        captions_.push_back("Food received");
    }

protected:
    // The numbers of the Dashboard.
    void values(vector<string>& out)
    {
        recipientstats stats;

        if (recipientsignedin())
        {
            stats = recipientstatsof(theapp->data, theapp->offers, recipientid());
        }

        out.push_back(numtext(stats.incoming));
        out.push_back(numtext(stats.received));
    }
};

// Incoming offers: the offers that wait for an answer. Decline and Accept act on the selected one.
class recipientincoming : public pagetable
{
public:
    // The base class gets the page number, the names and the empty message (a base-class call needs the initializer list).
    recipientincoming() : pagetable(PAGE_R_INCOMING, "incoming", "Incoming offers", "No offers are waiting for you")
    {
    }

protected:
    // The waiting offers of this recipient (the Recipient column would repeat the same name in every row).
    texttable maketable()
    {
        vector<int> mine;

        if (recipientsignedin())
        {
            mine = offersforrecipient(theapp->offers, recipientid(), true);
        }

        texttable table = offerstable(theapp->data, theapp->offers, mine);
        dropcolumn(table, "Recipient");
        return table;
    }

    // Decline offer, then the primary Accept offer.
    void addbuttons()
    {
        addbutton("Decline offer", IDC_R_DECLINE, false);
        addbutton("Accept offer", IDC_R_ACCEPT, true);
    }

    // Both buttons need a selected offer.
    void updatebuttons()
    {
        bool chosen = selectedposition() >= 0;
        EnableWindow(buttons_[0], chosen ? TRUE : FALSE);
        EnableWindow(buttons_[1], chosen ? TRUE : FALSE);
    }

    // Accept and Decline ask first.
    void clicked(int id)
    {
        int pos = selectedposition();

        if (pos < 0)
        {
            return;
        }

        if (id == IDC_R_ACCEPT)
        {
            accept(pos);
        }
        else if (id == IDC_R_DECLINE)
        {
            decline(pos);
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

    // Asks "Accept <qty> of <food> from <donor>?" and then lets the Workflow accept the offer.
    void accept(int pos)
    {
        // everything the question needs is copied first, because accepting can replace the offers in memory
        EntityId offerid = theapp->offers.at(pos).id();
        string quantity = numtext(theapp->offers.at(pos).quantity());
        string food = "food";
        string donor = "the donor";
        int donationpos = 0;

        if (theapp->data.donationindex.get(theapp->offers.at(pos).donationId().toString(), donationpos))
        {
            const Donation& donation = theapp->data.donations[donationpos];
            food = foodToStr(donation.foodType());
            int donorpos = 0;

            if (theapp->data.donorindex.get(donation.donorId().toString(), donorpos))
            {
                donor = theapp->data.donors[donorpos].name();
            }
        }

        if (!guiaskyes(window_, "Accept " + quantity + " of " + food + " from " + donor + "?"))
        {
            return;
        }

        string message = "";
        bool done = false;

        try
        {
            done = acceptoffer(theapp->data, theapp->offers, *theapp->files, offerid, message);
        }
        catch (const AppException& failure)
        {
            message = "The offer could not be accepted because " + plaintext(failure.what());
        }

        refresh();

        if (done)
        {
            shellstatus(message);
        }
        else
        {
            guishowerror(window_, message);
        }
    }

    // Asks "Decline this offer?" and then lets the Workflow decline it.
    void decline(int pos)
    {
        EntityId offerid = theapp->offers.at(pos).id();

        if (!guiaskyes(window_, "Decline this offer?"))
        {
            return;
        }

        string message = "";
        bool done = false;

        try
        {
            done = declineoffer(theapp->offers, *theapp->files, offerid, message);
        }
        catch (const AppException& failure)
        {
            message = "The answer could not be saved because " + plaintext(failure.what());
        }

        refresh();

        if (done)
        {
            shellstatus("Offer " + offerid.toString() + " declined");
        }
        else
        {
            guishowerror(window_, message);
        }
    }
};

// Past offers: the offers that were answered (accepted, declined or withdrawn).
class recipientpast : public pagetable
{
public:
    // The base class gets the page number, the names and the empty message (a base-class call needs the initializer list).
    recipientpast() : pagetable(PAGE_R_PAST, "past", "Past offers", "No past offers yet")
    {
    }

protected:
    // The finished offers of this recipient.
    texttable maketable()
    {
        vector<int> mine;

        if (recipientsignedin())
        {
            mine = pastoffersforrecipient(theapp->offers, recipientid());
        }

        texttable table = offerstable(theapp->data, theapp->offers, mine);
        dropcolumn(table, "Recipient");
        return table;
    }
};

// Received food: the transactions of this recipient.
class recipientreceived : public pagetable
{
public:
    // The base class gets the page number, the names and the empty message (a base-class call needs the initializer list).
    recipientreceived() : pagetable(PAGE_R_RECEIVED, "received", "Received food", "No food received yet")
    {
    }

protected:
    // The transactions of this recipient.
    texttable maketable()
    {
        vector<int> mine;

        if (recipientsignedin())
        {
            mine = transactionsofrecipient(theapp->data, recipientid());
        }

        texttable table = transactionstable(theapp->data, mine);
        dropcolumn(table, "Recipient");
        return table;
    }
};

// My details: shows and changes the capacity and the two yes or no settings, and shows the food types. The only button
// is Save. The saved values are shown every time the page is shown, so leaving the page drops unsaved changes.
class recipientdetails : public guipage
{
public:
    // The base class gets the page number, the short name and the title (a base-class call needs the initializer list).
    recipientdetails() : guipage(PAGE_R_EDIT, "details", "My details")
    {
        capacitylabel_ = NULL;
        capacity_ = NULL;
        accepting_ = NULL;
        pickup_ = NULL;
        foodcaption_ = NULL;
        foodvalue_ = NULL;
        error_ = NULL;
        save_ = NULL;
    }

    // The fields in the order of the Tab key: capacity, the two check boxes, Save.
    void createcontrols()
    {
        addtitle();
        capacitylabel_ = makelabel(window_, "Capacity", ROLE_LABEL);
        capacity_ = makeedit(window_, IDC_R_CAPACITY, true, 9);
        accepting_ = makecheck(window_, "Accepting donations", IDC_R_ACCEPTING);
        pickup_ = makecheck(window_, "Can collect food", IDC_R_PICKUP);
        foodcaption_ = makelabel(window_, "Food types", ROLE_MUTED);
        foodvalue_ = makelabel(window_, "", ROLE_TEXT);
        markdata(foodvalue_);
        error_ = makelabel(window_, "", ROLE_ERROR, IDC_R_ERROR);
        save_ = makebutton(window_, "Save", IDC_R_SAVE, true);
    }

    // The form has nothing else to refresh; the values are loaded when the page is shown.
    void refresh()
    {
    }

    // Every time the page is shown it holds the details that are saved now.
    void onshow()
    {
        SetWindowTextA(error_, "");
        loadvalues();
    }

    // The form column: the capacity with its label above, the two check boxes, the food types, the error line and Save.
    void onsize(int width, int height)
    {
        int left = 0;
        int columnwidth = 0;
        formcolumn(width, left, columnwidth);
        guilayout layout;
        int top = layouttitle(layout, left, columnwidth);
        top = placefield(layout, capacitylabel_, capacity_, NULL, left, top, columnwidth, px(numberwidth),
                         px(controlheight));
        layout.place(accepting_, left, top, columnwidth, px(controlheight));
        top = top + px(controlheight + gapsmall);
        layout.place(pickup_, left, top, columnwidth, px(controlheight));
        top = top + px(controlheight + pagemargin);
        layout.place(foodcaption_, left, top, columnwidth, px(captionline));
        top = top + px(captionline + gaptiny);
        layout.place(foodvalue_, left, top, columnwidth, 2 * px(textline));
        top = top + 2 * px(textline) + px(gapfield);
        layout.place(error_, left, top, columnwidth, 2 * px(textline));
        vector<HWND> buttons(1, save_);
        placebuttons(layout, buttons, left + columnwidth, height - px(pagemargin));
        layout.apply();
    }

    // Save.
    bool command(int id, int code)
    {
        if (code == BN_CLICKED && id == IDC_R_SAVE)
        {
            save();
            return true;
        }

        return false;
    }

    // Enter saves.
    int defaultid() const
    {
        return IDC_R_SAVE;
    }

    // Only the Save button.
    int maxbuttons() const
    {
        return 1;
    }

private:
    // Shows the values that are saved: capacity, the two settings and the food types (two spaces between them).
    void loadvalues()
    {
        int pos = recipientposition();

        if (pos < 0)
        {
            SetWindowTextA(capacity_, "");
            SetWindowTextA(foodvalue_, "");
            return;
        }

        const Recipient& recipient = theapp->data.recipients[pos];
        SetWindowTextA(capacity_, numtext(recipient.availableCapacity()).c_str());
        SendMessageA(accepting_, BM_SETCHECK, recipient.accepting() ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageA(pickup_, BM_SETCHECK, recipient.canPickup() ? BST_CHECKED : BST_UNCHECKED, 0);

        string foods = "";

        for (size_t kind = 0; kind < recipient.acceptedTypes().size(); kind++)
        {
            foods = foods + (kind > 0 ? "  " : "") + foodToStr(recipient.acceptedTypes()[kind]);
        }

        SetWindowTextA(foodvalue_, foods.c_str());
    }

    // Checks the capacity, calls updaterecipient and says so in the status line. The page stays.
    void save()
    {
        int capacity = 0;

        if (!readnumber(capacity_, capacity))
        {
            SetWindowTextA(error_, "Please type the capacity as a whole number 0 or more");
            SetFocus(capacity_);
            return;
        }

        bool accepting = SendMessageA(accepting_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        bool pickup = SendMessageA(pickup_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        string message = "";
        bool saved = false;

        try
        {
            saved = updaterecipient(theapp->data, *theapp->files, recipientid(), capacity, accepting, pickup, message);
        }
        catch (const AppException& failure)
        {
            message = "Your details could not be saved because " + plaintext(failure.what());
        }

        if (!saved)
        {
            SetWindowTextA(error_, message.c_str());
            return;
        }

        SetWindowTextA(error_, "");
        loadvalues();
        shellstatus("Details saved");
    }

    HWND capacitylabel_;   // "Capacity"
    HWND capacity_;        // the capacity box (digits only)
    HWND accepting_;       // check box "Accepting donations"
    HWND pickup_;          // check box "Can collect food"
    HWND foodcaption_;     // "Food types"
    HWND foodvalue_;       // the food types that this recipient takes
    HWND error_;           // the one red problem line
    HWND save_;            // Save (the only button)
};

// Makes the recipient pages.
guipage* newpagerecipienthome()
{
    return new recipienthome();
}

guipage* newpagerecipientincoming()
{
    return new recipientincoming();
}

guipage* newpagerecipientpast()
{
    return new recipientpast();
}

guipage* newpagerecipientreceived()
{
    return new recipientreceived();
}

guipage* newpagerecipientedit()
{
    return new recipientdetails();
}
