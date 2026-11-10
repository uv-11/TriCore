#ifndef PAGES_H
#define PAGES_H

// Pages.h: the functions that make the pages. The shell calls them once, when the window is made.

#include "GuiPage.h"

#include <string>

using namespace std;

// The welcome page: choose a role and a name, then continue.
guipage* newpagewelcome();

// The donor pages: Home, My donations, Offers, Given out and the Add donation form.
guipage* newpagedonorhome();
guipage* newpagedonordonations();
guipage* newpagedonoroffers();
guipage* newpagedonorgiven();
guipage* newpagedonoradd();

// The coordinator pages: Home, Donations, Offers, Recipients, Transactions and Summary.
guipage* newpagecoordhome();
guipage* newpagecoorddonations();
guipage* newpagecoordoffers();
guipage* newpagecoordrecipients();
guipage* newpagecoordtransactions();
guipage* newpagecoordsummary();

// The recipient pages: Home, Incoming offers, Past offers, Received food and the My details form.
guipage* newpagerecipienthome();
guipage* newpagerecipientincoming();
guipage* newpagerecipientpast();
guipage* newpagerecipientreceived();
guipage* newpagerecipientedit();

// The match page, and the call that opens it for the donation at a position in the donation list.
guipage* newpagematch();
void openmatchpage(int donationpos);

#endif
