#ifndef TABLES_H
#define TABLES_H

// Tables.h: the text of every list the screens show. The GUI only copies this text into a list.
// The first column of a row is always its id text or number, so the GUI can read it back.

#include "Datamanager.h"
#include "Match.h"
#include "OfferStore.h"
#include "Queries.h"

#include <string>
#include <vector>

using namespace std;

// A table of text: one header per column and one vector of cells per row.
struct texttable
{
    vector<string> headers;       // column titles
    vector<vector<string> > rows; // the cells of every row
};

// Says in plain words why a recipient can or cannot take a donation.
string reasontext(Reason reason);

// Donations at the given positions: ID, Donor, Food, Original, Left, Offered, Usable (min), Status.
texttable donationstable(const dataset& data, const offerstore& store, const vector<int>& positions);

// Recipients at the given positions: ID, Name, Type, Capacity, Food types, Accepting, Can collect.
texttable recipientstable(const dataset& data, const vector<int>& positions);

// Transactions at the given positions: ID, Donation, Donor, Food, Recipient, Quantity, Travel (min).
texttable transactionstable(const dataset& data, const vector<int>& positions);

// Offers at the given offer positions: ID, Donation, Donor, Food, Recipient, Quantity, Travel (min), Status.
texttable offerstable(const dataset& data, const offerstore& store, const vector<int>& offerpositions);

// The recipients that can take the donation: No., Recipient, Can take, Travel (min), Spare room, Route.
texttable candidatestable(const dataset& data, const matchview& match);

// The recipients that cannot: Recipient, Why not, Travel (min). Recipients that declined earlier are added.
texttable rejectedtable(const dataset& data, const matchview& match);

// The summary: Item, Quantity. Only the totals; the lines about the program are in aboutlines().
texttable summarytable(const totals& sums);

// The three plain lines about the program that the summary page shows as text under its list.
vector<string> aboutlines();

#endif
