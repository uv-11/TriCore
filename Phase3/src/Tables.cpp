// Tables.cpp: builds the text of every list. No window code here, so everything can be tested.

#include "Tables.h"

#include <sstream>

using namespace std;

// Turns a whole number into text.
static string numtext(int value)
{
    stringstream text;
    text << value;
    return text.str();
}

// Shows a travel time, or the word none when the place cannot be reached.
static string travelcell(const Duration& travel)
{
    if (travel.isInfinite())
    {
        return "none";
    }

    return numtext(travel.minutes());
}

// Says yes or no.
static string yesno(bool value)
{
    if (value)
    {
        return "yes";
    }

    return "no";
}

// Returns the name of a donor, or the id when the donor is not known.
static string donorname(const dataset& data, const EntityId& donor)
{
    int pos = 0;

    if (data.donorindex.get(donor.toString(), pos))
    {
        return data.donors[pos].name();
    }

    return donor.toString();
}

// Returns the name of a recipient, or the id when the recipient is not known.
static string recipientname(const dataset& data, const EntityId& recipient)
{
    int pos = 0;

    if (data.recipientindex.get(recipient.toString(), pos))
    {
        return data.recipients[pos].name();
    }

    return recipient.toString();
}

// Writes a stored type such as COMMUNITY_KITCHEN as "Community kitchen".
static string plaintype(const string& text)
{
    string result = text;

    for (size_t pos = 0; pos < result.length(); pos++)
    {
        if (result[pos] == '_')
        {
            result[pos] = ' ';
        }
        else if (pos > 0 && result[pos] >= 'A' && result[pos] <= 'Z')
        {
            result[pos] = (char)(result[pos] - 'A' + 'a');
        }
    }

    return result;
}

// Joins the food types with two spaces, so the text has no punctuation.
static string foodlist(const vector<FoodType>& types)
{
    string text = "";

    for (size_t pos = 0; pos < types.size(); pos++)
    {
        if (pos > 0)
        {
            text = text + "  ";
        }

        text = text + foodToStr(types[pos]);
    }

    return text;
}

// Says the state of a donation in plain words.
static string donationstatustext(DonationStatus status)
{
    if (status == PENDING)
    {
        return "Waiting";
    }

    if (status == PARTIAL)
    {
        return "Partly given";
    }

    if (status == COMPLETED)
    {
        return "All given";
    }

    return "Closed";
}

// Says the state of an offer in plain words.
static string offerstatustext(OfferStatus status)
{
    if (status == OFFER_PENDING)
    {
        return "Waiting for answer";
    }

    if (status == OFFER_ACCEPTED)
    {
        return "Accepted";
    }

    if (status == OFFER_DECLINED)
    {
        return "Declined";
    }

    return "Withdrawn";
}

// Plain-language reason for each rule of the feasibility check.
string reasontext(Reason reason)
{
    if (reason == OK)
    {
        return "Can take it";
    }

    if (reason == NOT_ACCEPTING)
    {
        return "Not accepting donations right now";
    }

    if (reason == FOOD_INCOMPATIBLE)
    {
        return "Does not take this kind of food";
    }

    if (reason == NO_CAPACITY)
    {
        return "No room left";
    }

    if (reason == NO_PICKUP)
    {
        return "Cannot collect the food";
    }

    if (reason == UNREACHABLE)
    {
        return "No road connects it to the donor";
    }

    return "Too far for the usable time";
}

// One row per donation. A position that does not exist is skipped.
texttable donationstable(const dataset& data, const offerstore& store, const vector<int>& positions)
{
    texttable table;
    table.headers.push_back("ID");
    table.headers.push_back("Donor");
    table.headers.push_back("Food");
    table.headers.push_back("Original");
    table.headers.push_back("Left");
    table.headers.push_back("Offered");
    table.headers.push_back("Usable minutes");
    table.headers.push_back("Status");

    for (size_t index = 0; index < positions.size(); index++)
    {
        int pos = positions[index];

        if (pos < 0 || pos >= (int)data.donations.size())
        {
            continue;
        }

        const Donation& donation = data.donations[pos];
        vector<string> cells;
        cells.push_back(donation.id().toString());
        cells.push_back(donorname(data, donation.donorId()));
        cells.push_back(foodToStr(donation.foodType()));
        cells.push_back(numtext(donation.originalQuantity()));
        cells.push_back(numtext(donation.remainingQuantity()));
        cells.push_back(numtext(store.pendingquantity(donation.id())));
        cells.push_back(numtext(donation.usableMinutes()));
        cells.push_back(donationstatustext(donation.status()));
        table.rows.push_back(cells);
    }

    return table;
}

// One row per recipient.
texttable recipientstable(const dataset& data, const vector<int>& positions)
{
    texttable table;
    table.headers.push_back("ID");
    table.headers.push_back("Name");
    table.headers.push_back("Type");
    table.headers.push_back("Capacity");
    table.headers.push_back("Food types");
    table.headers.push_back("Accepting");
    table.headers.push_back("Can collect");

    for (size_t index = 0; index < positions.size(); index++)
    {
        int pos = positions[index];

        if (pos < 0 || pos >= (int)data.recipients.size())
        {
            continue;
        }

        const Recipient& recipient = data.recipients[pos];
        vector<string> cells;
        cells.push_back(recipient.id().toString());
        cells.push_back(recipient.name());
        cells.push_back(plaintype(recipient.type()));
        cells.push_back(numtext(recipient.availableCapacity()));
        cells.push_back(foodlist(recipient.acceptedTypes()));
        cells.push_back(yesno(recipient.accepting()));
        cells.push_back(yesno(recipient.canPickup()));
        table.rows.push_back(cells);
    }

    return table;
}

// One row per transaction. Names are looked up, and the id is shown when a name is missing.
texttable transactionstable(const dataset& data, const vector<int>& positions)
{
    texttable table;
    table.headers.push_back("ID");
    table.headers.push_back("Donation");
    table.headers.push_back("Donor");
    table.headers.push_back("Food");
    table.headers.push_back("Recipient");
    table.headers.push_back("Quantity");
    table.headers.push_back("Travel minutes");

    for (size_t index = 0; index < positions.size(); index++)
    {
        int pos = positions[index];

        if (pos < 0 || pos >= (int)data.transactions.size())
        {
            continue;
        }

        const Transaction& entry = data.transactions[pos];
        string donor = entry.donationId().toString();
        string food = "Unknown";
        int donationpos = 0;

        if (data.donationindex.get(entry.donationId().toString(), donationpos))
        {
            donor = donorname(data, data.donations[donationpos].donorId());
            food = foodToStr(data.donations[donationpos].foodType());
        }

        vector<string> cells;
        cells.push_back(entry.id().toString());
        cells.push_back(entry.donationId().toString());
        cells.push_back(donor);
        cells.push_back(food);
        cells.push_back(recipientname(data, entry.recipientId()));
        cells.push_back(numtext(entry.quantity()));
        cells.push_back(numtext(entry.travelMinutes()));
        table.rows.push_back(cells);
    }

    return table;
}

// One row per offer.
texttable offerstable(const dataset& data, const offerstore& store, const vector<int>& offerpositions)
{
    texttable table;
    table.headers.push_back("ID");
    table.headers.push_back("Donation");
    table.headers.push_back("Donor");
    table.headers.push_back("Food");
    table.headers.push_back("Recipient");
    table.headers.push_back("Quantity");
    table.headers.push_back("Travel minutes");
    table.headers.push_back("Status");

    for (size_t index = 0; index < offerpositions.size(); index++)
    {
        int pos = offerpositions[index];

        if (pos < 0 || pos >= store.count())
        {
            continue;
        }

        const Offer& offer = store.at(pos);
        string donor = offer.donationId().toString();
        string food = "Unknown";
        int donationpos = 0;

        if (data.donationindex.get(offer.donationId().toString(), donationpos))
        {
            donor = donorname(data, data.donations[donationpos].donorId());
            food = foodToStr(data.donations[donationpos].foodType());
        }

        vector<string> cells;
        cells.push_back(offer.id().toString());
        cells.push_back(offer.donationId().toString());
        cells.push_back(donor);
        cells.push_back(food);
        cells.push_back(recipientname(data, offer.recipientId()));
        cells.push_back(numtext(offer.quantity()));
        cells.push_back(numtext(offer.travelMinutes()));
        cells.push_back(offerstatustext(offer.status()));
        table.rows.push_back(cells);
    }

    return table;
}

// One row per candidate, numbered from 1 in the order of the ranking.
texttable candidatestable(const dataset& data, const matchview& match)
{
    texttable table;
    table.headers.push_back("Number");
    table.headers.push_back("Recipient");
    table.headers.push_back("Can take");
    table.headers.push_back("Travel minutes");
    table.headers.push_back("Spare room");
    table.headers.push_back("Route");

    for (size_t pos = 0; pos < match.candidates.size(); pos++)
    {
        const Candidate& candidate = match.candidates[pos];
        string route = "";

        if (pos < match.routes.size())
        {
            route = match.routes[pos];
        }

        vector<string> cells;
        cells.push_back(numtext((int)pos + 1));
        cells.push_back(recipientname(data, candidate.recipientId()));
        cells.push_back(numtext(candidate.potentialAcceptance()));
        cells.push_back(travelcell(candidate.travel()));
        cells.push_back(numtext(candidate.unusedCapacity()));
        cells.push_back(route);
        table.rows.push_back(cells);
    }

    return table;
}

// The rejected recipients first, then the recipients that declined this donation earlier.
texttable rejectedtable(const dataset& data, const matchview& match)
{
    texttable table;
    table.headers.push_back("Recipient");
    table.headers.push_back("Why not");
    table.headers.push_back("Travel minutes");

    for (size_t pos = 0; pos < match.rejected.size(); pos++)
    {
        const Rejection& rejection = match.rejected[pos];
        vector<string> cells;
        cells.push_back(recipientname(data, rejection.recipientId()));
        cells.push_back(reasontext(rejection.reason()));
        cells.push_back(travelcell(rejection.travel()));
        table.rows.push_back(cells);
    }

    for (size_t pos = 0; pos < match.declined.size(); pos++)
    {
        vector<string> cells;
        cells.push_back(recipientname(data, match.declined[pos]));
        cells.push_back("Declined this donation earlier");
        cells.push_back("none");
        table.rows.push_back(cells);
    }

    return table;
}

// Adds one line of the summary.
static void addsummaryrow(texttable& table, const string& item, const string& quantity)
{
    vector<string> cells;
    cells.push_back(item);
    cells.push_back(quantity);
    table.rows.push_back(cells);
}

// The totals and the food given out per kind.
texttable summarytable(const totals& sums)
{
    texttable table;
    table.headers.push_back("Item");
    table.headers.push_back("Quantity");

    addsummaryrow(table, "Offered", numtext(sums.offered));
    addsummaryrow(table, "Given out", numtext(sums.allocated));
    addsummaryrow(table, "Wasted", numtext(sums.wasted));
    addsummaryrow(table, "Still open", numtext(sums.stillopen));
    addsummaryrow(table, "Cooked given out", numtext(sums.byfood[COOKED]));
    addsummaryrow(table, "Packaged given out", numtext(sums.byfood[PACKAGED]));
    addsummaryrow(table, "Produce given out", numtext(sums.byfood[PRODUCE]));
    addsummaryrow(table, "Bakery given out", numtext(sums.byfood[BAKERY]));

    return table;
}

// The three About lines of the console program (they are not rows of a table: a sentence does not belong in a list column).
vector<string> aboutlines()
{
    vector<string> lines;
    lines.push_back("CommunityBridge helps decide where surplus food should go");
    lines.push_back("All travel times are simulated minutes and not real road data");
    lines.push_back("Recipients are ranked by food accepted then travel time then spare room");
    return lines;
}
