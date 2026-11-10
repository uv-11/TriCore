// Workflow.cpp: the operations of the program. Every operation checks first, changes memory and
// files in a safe order, and puts everything back when a save fails (plain C++, no window code).

#include "Workflow.h"
#include "Exceptions.h"
#include "RecommendationEngine.h"
#include "TextTools.h"
#include "allocationmanager.h"

#include <sstream>
#include <vector>

using namespace std;

// The header row of transactions.csv, needed to rewrite that file after a failed accept.
static const string transactionsheaderline = "transaction_id,donation_id,recipient_id,quantity,travel_minutes";

// Turns a whole number into text.
static string numtext(int value)
{
    stringstream text;
    text << value;
    return text.str();
}

// Turns the first letter of a text into a small letter, so a message can continue a sentence.
static string lowerfirst(const string& text)
{
    string result = text;

    if (!result.empty() && result[0] >= 'A' && result[0] <= 'Z')
    {
        result[0] = (char)(result[0] - 'A' + 'a');
    }

    return result;
}

// A donation can receive offers while it is waiting or partly given.
static bool isopen(const Donation& donation)
{
    return donation.status() == PENDING || donation.status() == PARTIAL;
}

// Returns the name of a recipient, or the id when the recipient is not known.
static string recipientnamefor(const dataset& data, const EntityId& recipient)
{
    int pos = 0;

    if (data.recipientindex.get(recipient.toString(), pos))
    {
        return data.recipients[pos].name();
    }

    return recipient.toString();
}

// Lets the offer id issuer continue after the highest offer id that is really in the store.
static void reseedoffers(const offerstore& store)
{
    EntityId highest("OFF", 0);

    for (int pos = 0; pos < store.count(); pos++)
    {
        if (highest < store.at(pos).id())
        {
            highest = store.at(pos).id();
        }
    }

    offeridissuer::seed(highest);
}

// Lets the transaction id issuer continue after the highest transaction id that is really in memory.
static void reseedtransactions(const dataset& data)
{
    EntityId highest("T", 0);

    for (size_t pos = 0; pos < data.transactions.size(); pos++)
    {
        if (highest < data.transactions[pos].id())
        {
            highest = data.transactions[pos].id();
        }
    }

    transactionidissuer::seed(highest);
}

// Writes the route as the names of the locations joined with " -> ".
static string routetext(const dataset& data, const dijkstratraveltime& provider, int fromnode, int tonode)
{
    vector<int> path = provider.routeto(fromnode, tonode);
    string text = "";

    for (size_t pos = 0; pos < path.size(); pos++)
    {
        if (pos > 0)
        {
            text = text + " -> ";
        }

        int node = path[pos];

        if (node >= 0 && node < (int)data.nodelabels.size())
        {
            text = text + data.nodelabels[node];
        }
        else
        {
            text = text + numtext(node);
        }
    }

    return text;
}

// Food left in the donation minus the waiting offers. Never below 0, and 0 for a wrong position.
int effectiveremaining(const dataset& data, const offerstore& store, int donationPos)
{
    if (donationPos < 0 || donationPos >= (int)data.donations.size())
    {
        return 0;
    }

    const Donation& donation = data.donations[donationPos];
    int left = donation.remainingQuantity() - store.pendingquantity(donation.id());

    if (left < 0)
    {
        left = 0;
    }

    return left;
}

// Only the donations that can still be offered are put in the urgency queue.
int nexturgentdonation(const dataset& data, const offerstore& store)
{
    urgencyqueue waiting;

    for (size_t pos = 0; pos < data.donations.size(); pos++)
    {
        if (isopen(data.donations[pos]) && effectiveremaining(data, store, (int)pos) > 0)
        {
            waiting.push(data.donations[pos], (int)pos);
        }
    }

    if (waiting.empty())
    {
        return -1;
    }

    return waiting.popmosturgent();
}

// Runs recommend on a copy of the donation that has the effective remaining as its quantity, so the
// numbers already promised in waiting offers are not offered again.
matchview buildmatch(const dataset& data, const offerstore& store, const dijkstratraveltime& provider,
                     int donationPos)
{
    if (donationPos < 0 || donationPos >= (int)data.donations.size())
    {
        throw StructureException("There is no donation at this position");
    }

    const Donation& donation = data.donations[donationPos];
    matchview match;
    match.donation = donation.id();
    match.pendingoffers = store.pendingquantity(donation.id());

    // a donation that is completed or closed has nothing left to offer
    if (isopen(donation))
    {
        match.effectiveremaining = effectiveremaining(data, store, donationPos);
    }

    int donorpos = 0;

    if (!data.donorindex.get(donation.donorId().toString(), donorpos))
    {
        throw StructureException("The donor of this donation is not in the files");
    }

    int donornode = data.donors[donorpos].nodeId();
    Donation copy(donation.id(), donation.donorId(), donation.foodType(), donation.originalQuantity(),
                  match.effectiveremaining, donation.usableMinutes(), donation.status());
    vector<Duration> travel = provider.traveltimesfrom(donornode);
    RecommendationResult result = recommend(copy, data.recipients, travel);

    match.rejected = result.rejected();

    for (size_t pos = 0; pos < result.candidates().size(); pos++)
    {
        const Candidate& candidate = result.candidates()[pos];

        // nothing left to offer gives candidates that could take 0, they are not shown
        if (candidate.potentialAcceptance() < 1)
        {
            continue;
        }

        if (store.wasdeclined(donation.id(), candidate.recipientId()))
        {
            match.declined.push_back(candidate.recipientId());
            continue;
        }

        int recipientpos = 0;
        string route = "";

        if (data.recipientindex.get(candidate.recipientId().toString(), recipientpos))
        {
            route = routetext(data, provider, donornode, data.recipients[recipientpos].nodeId());
        }

        match.candidates.push_back(candidate);
        match.routes.push_back(route);
    }

    return match;
}

// Checks the choice, adds the offer and saves offers.csv. A copy of the store is kept to undo the add.
int sendoffer(dataset& data, offerstore& store, datamanager& files, const matchview& match,
              int candidateNumber, int qty, string& message)
{
    if (candidateNumber < 1 || candidateNumber > (int)match.candidates.size())
    {
        message = "Please choose one of the recipients in the list";
        return -1;
    }

    const Candidate& candidate = match.candidates[candidateNumber - 1];

    if (qty < 1 || qty > candidate.potentialAcceptance())
    {
        message = "The quantity must be from 1 to " + numtext(candidate.potentialAcceptance());
        return -1;
    }

    int donationpos = 0;

    if (!data.donationindex.get(match.donation.toString(), donationpos))
    {
        message = "That donation could not be found";
        return -1;
    }

    if (!isopen(data.donations[donationpos]))
    {
        message = "This donation is no longer open";
        return -1;
    }

    int left = effectiveremaining(data, store, donationpos);

    if (qty > left)
    {
        message = "Only " + numtext(left) + " is left to offer for this donation";
        return -1;
    }

    offerstore backup = store;
    int pos = store.add(match.donation, candidate.recipientId(), qty, candidate.travel().minutes());

    try
    {
        store.save(files);
    }
    catch (const AppException&)
    {
        store = backup;
        reseedoffers(store);
        message = "The offer could not be saved so nothing was changed and you can try again";
        return -1;
    }

    message = "Offer " + store.at(pos).id().toString() + " sent to " +
              recipientnamefor(data, candidate.recipientId());
    return pos;
}

// Rewrites the files after a failed accept, as well as possible, so they match the memory again.
static void healfiles(dataset& data, datamanager& files, const offerstore& store, int recipientPos)
{
    vector<const CsvRecord*> records;

    for (size_t pos = 0; pos < data.transactions.size(); pos++)
    {
        records.push_back(&data.transactions[pos]);
    }

    try
    {
        files.savetable("transactions.csv", transactionsheaderline, records);
    }
    catch (const AppException&)
    {
    }

    try
    {
        files.commitdonations(data);
    }
    catch (const AppException&)
    {
    }

    try
    {
        files.appendrecipient(data, data.recipients[recipientPos]);
    }
    catch (const AppException&)
    {
    }

    try
    {
        store.save(files);
    }
    catch (const AppException&)
    {
    }
}

// The accept is done on copies first. The offers file is saved before the other files, and every
// step is undone when a later step fails.
bool acceptoffer(dataset& data, offerstore& store, datamanager& files, const EntityId& offerid,
                 string& message)
{
    int offerpos = 0;

    if (!store.find(offerid, offerpos))
    {
        message = "That offer could not be found";
        return false;
    }

    if (store.at(offerpos).status() != OFFER_PENDING)
    {
        message = "This offer has already been answered";
        return false;
    }

    int donationpos = 0;
    int recipientpos = 0;

    if (!data.donationindex.get(store.at(offerpos).donationId().toString(), donationpos) ||
        !data.recipientindex.get(store.at(offerpos).recipientId().toString(), recipientpos))
    {
        message = "The donation or the recipient of this offer could not be found";
        return false;
    }

    int qty = store.at(offerpos).quantity();
    int travel = store.at(offerpos).travelMinutes();
    Donation donation = data.donations[donationpos];
    Recipient recipient = data.recipients[recipientpos];
    Donation olddonation = data.donations[donationpos];
    Recipient oldrecipient = data.recipients[recipientpos];
    offerstore backup = store;

    AllocError code = allocate(donation, recipient, qty, Duration(travel), EntityId());

    if (code != ALLOC_OK)
    {
        message = plaintext(allocerrormessage(code));

        // an offer for a donation that is closed or completed can never be accepted
        if (code == DONATION_NOT_OPEN)
        {
            store.at(offerpos).markwithdrawn();

            try
            {
                store.save(files);
                message = message + " so the offer was withdrawn";
            }
            catch (const AppException&)
            {
                store = backup;
            }
        }
        else
        {
        }

        return false;
    }

    // only now is a transaction id used up
    EntityId txn = transactionidissuer::issuenext();
    bool started = false;

    try
    {
        store.at(offerpos).markaccepted(txn);
        store.save(files);

        started = true;
        data.donations[donationpos] = donation;
        data.recipients[recipientpos] = recipient;
        data.transactions.push_back(Transaction(txn, donation.id(), recipient.id(), qty, travel));
        files.commitallocation(data, data.transactions.back());
    }
    catch (const AppException&)
    {
        data.donations[donationpos] = olddonation;
        data.recipients[recipientpos] = oldrecipient;

        if (data.transactions.size() > 0 && data.transactions.back().id() == txn)
        {
            data.transactions.pop_back();
        }

        store = backup;
        reseedtransactions(data);

        if (started)
        {
            healfiles(data, files, store, recipientpos);
        }
        else
        {
            try
            {
                store.save(files);
            }
            catch (const AppException&)
            {
            }
        }

        message = "The offer could not be accepted because the files could not be saved so nothing was changed";
        return false;
    }

    message = "The offer was accepted and recorded as " + txn.toString();
    return true;
}

// Declines a waiting offer and saves. A copy of the store is kept to undo the change.
bool declineoffer(offerstore& store, datamanager& files, const EntityId& offerid, string& message)
{
    int pos = 0;

    if (!store.find(offerid, pos))
    {
        message = "That offer could not be found";
        return false;
    }

    if (store.at(pos).status() != OFFER_PENDING)
    {
        message = "This offer has already been answered";
        return false;
    }

    offerstore backup = store;
    store.at(pos).markdeclined();

    try
    {
        store.save(files);
    }
    catch (const AppException&)
    {
        store = backup;
        message = "The answer could not be saved so nothing was changed and you can try again";
        return false;
    }

    message = "The offer was declined";
    return true;
}

// Withdraws a waiting offer and saves. A copy of the store is kept to undo the change.
bool withdrawoffer(offerstore& store, datamanager& files, const EntityId& offerid, string& message)
{
    int pos = 0;

    if (!store.find(offerid, pos))
    {
        message = "That offer could not be found";
        return false;
    }

    if (store.at(pos).status() != OFFER_PENDING)
    {
        message = "Only an offer that is still waiting can be withdrawn";
        return false;
    }

    offerstore backup = store;
    store.at(pos).markwithdrawn();

    try
    {
        store.save(files);
    }
    catch (const AppException&)
    {
        store = backup;
        message = "The offer could not be withdrawn so nothing was changed and you can try again";
        return false;
    }

    message = "The offer was withdrawn";
    return true;
}

// First the waiting offers are withdrawn and saved, then the donation is closed and saved.
bool closedonationwithoffers(dataset& data, offerstore& store, datamanager& files, int donationPos,
                             string& message)
{
    if (donationPos < 0 || donationPos >= (int)data.donations.size())
    {
        message = "That donation could not be found";
        return false;
    }

    Donation old = data.donations[donationPos];

    if (!isopen(old))
    {
        message = "This donation is already finished so there is nothing to close";
        return false;
    }

    offerstore backup = store;
    int withdrawn = 0;

    try
    {
        withdrawn = store.withdrawfordonation(old.id());

        if (withdrawn > 0)
        {
            store.save(files);
        }

        closedonation(data.donations[donationPos]);
        files.commitdonations(data);
    }
    catch (const AppException&)
    {
        data.donations[donationPos] = old;
        store = backup;

        if (withdrawn > 0)
        {
            try
            {
                store.save(files);
            }
            catch (const AppException&)
            {
            }
        }

        message = "The donation could not be closed because the files could not be saved so nothing was changed";
        return false;
    }

    message = "The donation was closed and " + numtext(old.remainingQuantity()) + " of the food is recorded as wasted";
    return true;
}

// Checks the values, issues the next donation id, writes the file first and only then changes memory.
int createdonation(dataset& data, datamanager& files, const EntityId& donor, FoodType food, int qty,
                   int usable, string& message)
{
    int donorpos = 0;

    if (!data.donorindex.get(donor.toString(), donorpos))
    {
        message = "That donor could not be found";
        return -1;
    }

    if (qty < 1)
    {
        message = "The quantity must be at least 1";
        return -1;
    }

    if (usable < 1)
    {
        message = "The usable minutes must be at least 1";
        return -1;
    }

    EntityId highest("DON", 0);

    for (size_t pos = 0; pos < data.donations.size(); pos++)
    {
        if (highest < data.donations[pos].id())
        {
            highest = data.donations[pos].id();
        }
    }

    donationidissuer::seed(highest);
    EntityId newid = donationidissuer::issuenext();
    Donation entry(newid, donor, food, qty, qty, usable, PENDING);

    try
    {
        files.appenddonation(data, entry);
    }
    catch (const AppException&)
    {
        donationidissuer::seed(highest);
        message = "The donation could not be saved so nothing was added and you can try again";
        return -1;
    }

    int newpos = (int)data.donations.size();
    data.donations.push_back(entry);
    data.donationindex.put(newid.toString(), newpos);
    message = "Donation " + newid.toString() + " added";
    return newpos;
}

// Checks a name of a donor or recipient: 1 to 60 characters and no comma or line break.
static bool goodname(const string& name, string& message)
{
    if (name.empty() || name.length() > 60 || name.find(',') != string::npos ||
        name.find('\n') != string::npos || name.find('\r') != string::npos)
    {
        message = "The name must be 1 to 60 characters and cannot contain a comma";
        return false;
    }

    return true;
}

// Checks a type (1 to 30 letters, digits, spaces or underscores) and writes it in capitals with the
// spaces turned into underscores, like the console program does.
static bool goodtype(const string& text, string& result, string& message)
{
    if (text.empty() || text.length() > 30)
    {
        message = "The type must be 1 to 30 characters long";
        return false;
    }

    result = text;

    for (size_t pos = 0; pos < result.length(); pos++)
    {
        char letter = result[pos];

        if (letter >= 'a' && letter <= 'z')
        {
            result[pos] = (char)(letter - 'a' + 'A');
        }
        else if (letter == ' ')
        {
            result[pos] = '_';
        }
        else if (!((letter >= 'A' && letter <= 'Z') || (letter >= '0' && letter <= '9') || letter == '_'))
        {
            message = "The type can only have letters and digits and spaces and underscores";
            return false;
        }
    }

    return true;
}

// Checks that a location number is on the map.
static bool goodnode(const dataset& data, int node, string& message)
{
    if (node < 0 || node >= data.network.nodecount())
    {
        message = "Please choose a location from the map";
        return false;
    }

    return true;
}

// Tells whether a donor (or a recipient, when donors is false) is at a location that is connected to the node.
static bool connectedto(const dataset& data, int node, bool donors)
{
    vector<int> label;
    labelcomponents(data.network, label);

    if (donors)
    {
        for (size_t pos = 0; pos < data.donors.size(); pos++)
        {
            if (label[data.donors[pos].nodeId()] == label[node])
            {
                return true;
            }
        }
    }
    else
    {
        for (size_t pos = 0; pos < data.recipients.size(); pos++)
        {
            if (label[data.recipients[pos].nodeId()] == label[node])
            {
                return true;
            }
        }
    }

    return false;
}

// Checks every value, issues the next recipient id, writes the file first and only then changes memory.
int createrecipient(dataset& data, datamanager& files, const string& name, const string& type,
                    const vector<FoodType>& foods, int capacity, bool accepting, bool pickup, int node,
                    string& message)
{
    string kind = "";

    if (!goodname(name, message) || !goodtype(type, kind, message))
    {
        return -1;
    }

    if (foods.empty())
    {
        message = "Please choose at least one kind of food";
        return -1;
    }

    if (capacity < 0)
    {
        message = "The capacity cannot be below 0";
        return -1;
    }

    if (!goodnode(data, node, message))
    {
        return -1;
    }

    EntityId highest("R", 0);

    for (size_t pos = 0; pos < data.recipients.size(); pos++)
    {
        if (highest < data.recipients[pos].id())
        {
            highest = data.recipients[pos].id();
        }
    }

    recipientidissuer::seed(highest);
    EntityId newid = recipientidissuer::issuenext();
    Recipient entry(newid, name, kind, capacity, foods, accepting, pickup, node);

    try
    {
        files.appendrecipient(data, entry);
    }
    catch (const AppException&)
    {
        recipientidissuer::seed(highest);
        message = "The recipient could not be saved so nothing was added and you can try again";
        return -1;
    }

    int newpos = (int)data.recipients.size();
    data.recipients.push_back(entry);
    data.recipientindex.put(newid.toString(), newpos);
    message = "Recipient " + newid.toString() + " added";

    if (!connectedto(data, node, true))
    {
        message = message + " and no donor is connected to this location so it cannot be recommended until a donor is placed in the same part of the map";
    }

    return newpos;
}

// Checks every value, issues the next donor id, writes the file first and only then changes memory.
int createdonor(dataset& data, datamanager& files, const string& name, const string& type, int node,
                string& message)
{
    string kind = "";

    if (!goodname(name, message) || !goodtype(type, kind, message) || !goodnode(data, node, message))
    {
        return -1;
    }

    EntityId highest("DN", 0);

    for (size_t pos = 0; pos < data.donors.size(); pos++)
    {
        if (highest < data.donors[pos].id())
        {
            highest = data.donors[pos].id();
        }
    }

    donoridissuer::seed(highest);
    EntityId newid = donoridissuer::issuenext();
    Donor entry(newid, name, kind, node);

    try
    {
        files.appenddonor(data, entry);
    }
    catch (const AppException&)
    {
        donoridissuer::seed(highest);
        message = "The donor could not be saved so nothing was added and you can try again";
        return -1;
    }

    int newpos = (int)data.donors.size();
    data.donors.push_back(entry);
    data.donorindex.put(newid.toString(), newpos);
    message = "Donor " + newid.toString() + " added";

    if (!connectedto(data, node, false))
    {
        message = message + " and no recipient is connected to this location so donations from this donor will have no recipient";
    }

    return newpos;
}

// Uses the same checks as createdonor, but saves nothing.
bool checkdonordetails(const dataset& data, const string& name, const string& type, int node, string& message)
{
    string kind = "";
    return goodname(name, message) && goodtype(type, kind, message) && goodnode(data, node, message);
}

// The donation values and the donor values are checked before anything is saved. The donor is saved
// first: a donation whose donor is missing from donors.csv would stop the next start.
int createdonationwithnewdonor(dataset& data, datamanager& files, const string& donorname,
                               const string& donortype, int node, FoodType food, int qty, int usable,
                               string& message)
{
    string kind = "";

    if (!goodname(donorname, message) || !goodtype(donortype, kind, message) || !goodnode(data, node, message))
    {
        return -1;
    }

    if (qty < 1)
    {
        message = "The quantity must be at least 1";
        return -1;
    }

    if (usable < 1)
    {
        message = "The usable minutes must be at least 1";
        return -1;
    }

    string donormessage = "";
    int donorpos = createdonor(data, files, donorname, donortype, node, donormessage);

    if (donorpos < 0)
    {
        message = donormessage;
        return -1;
    }

    EntityId donorid = data.donors[donorpos].id();
    string donationmessage = "";
    int donationpos = createdonation(data, files, donorid, food, qty, usable, donationmessage);

    if (donationpos < 0)
    {
        message = "The donor " + donorid.toString() + " was saved but the donation could not be saved because " + lowerfirst(donationmessage);
        return -1;
    }

    message = "Donation " + data.donations[donationpos].id().toString() + " added for the new donor " + donorname +
              " with id " + donorid.toString();
    return donationpos;
}
