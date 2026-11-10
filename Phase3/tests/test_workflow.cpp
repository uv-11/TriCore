// test_workflow: the scenarios S1 to S8 and the extra checks of Workflow.h, with no window at all.
//
// The standard fixture (testkit.h): donor DN002 is at location 1, and from there the travel times
// are R001 40, R002 9, R003 9, R004 36, R005 unreachable, R006 26, R007 36 minutes. DON001 is 80
// cooked food, usable 60 minutes: the ranked candidates are R003 (9), R004 (36), R001 (40).
// R005 is not connected to any donor, so every start-up of the fixture has exactly that one warning.

#include "Datamanager.h"
#include "Exceptions.h"
#include "OfferStore.h"
#include "Queries.h"
#include "RecipientUpdate.h"
#include "Tables.h"
#include "TextTools.h"
#include "Workflow.h"
#include "allocationmanager.h"
#include "testkit.h"
#include "traveltime.h"

#include <list>
#include <string>
#include <vector>

using namespace std;

// One running copy of the program's data, started like the real start-up does.
class World
{
public:
    // Starts from a folder. With fresh the six standard files are written first, otherwise the
    // files that are already there are loaded (a restart).
    World(const string& name, bool fresh, int richness);

    // Frees the travel time provider.
    ~World();

    string folder;                  // the test folder with a trailing slash
    datamanager files;              // reads and writes the csv files
    dataset data;                   // everything in memory
    list<string> warnings;          // start-up warnings
    offerstore store;               // all offers
    dijkstratraveltime* provider;   // travel times, created after loadall

private:
    // Copying is not allowed (the provider is owned by the object).
    World(const World& other);
    World& operator=(const World& other);
};

// The datamanager has no default constructor, so it is built in the initializer list.
World::World(const string& name, bool fresh, int richness) : files(string(CB_TESTDATA) + name + "/")
{
    folder = string(CB_TESTDATA) + name + "/";

    if (fresh)
    {
        testfolder(name);

        if (richness == 0)
        {
            writefixture(folder);
        }
        else
        {
            writerichfixture(folder);
        }
    }

    provider = NULL;
    files.loadall(data, warnings);
    transactionidissuer::seed(data.lasttransaction);

    EntityId highestdonation("DON", 0);

    for (size_t pos = 0; pos < data.donations.size(); pos++)
    {
        if (highestdonation < data.donations[pos].id())
        {
            highestdonation = data.donations[pos].id();
        }
    }

    donationidissuer::seed(highestdonation);
    donoridissuer::seed(EntityId("DN", 2));
    recipientidissuer::seed(EntityId("R", 7));
    store.load(folder, warnings);

    // like the real start-up: offers that do not fit the data become warnings
    vector<string> mismatches = store.reconcile(data);

    for (size_t pos = 0; pos < mismatches.size(); pos++)
    {
        warnings.push_back(mismatches[pos]);
    }

    provider = new dijkstratraveltime(data.network);
}

// Frees the provider.
World::~World()
{
    delete provider;
}

// Returns all rows of donations, recipients, transactions and offers as one text.
string statetext(const World& world)
{
    string text = "";

    for (size_t pos = 0; pos < world.data.donations.size(); pos++)
    {
        text = text + world.data.donations[pos].toCsvRow() + "\n";
    }

    for (size_t pos = 0; pos < world.data.recipients.size(); pos++)
    {
        text = text + world.data.recipients[pos].toCsvRow() + "\n";
    }

    for (size_t pos = 0; pos < world.data.transactions.size(); pos++)
    {
        text = text + world.data.transactions[pos].toCsvRow() + "\n";
    }

    for (int pos = 0; pos < world.store.count(); pos++)
    {
        text = text + world.store.at(pos).toCsvRow() + "\n";
    }

    return text;
}

// Returns the text of the seven csv files of a folder (an absent file gives nothing).
string filestate(const string& folder)
{
    string text = "";
    const char* const* names = knownfiles();

    for (int pos = 0; names[pos] != NULL; pos++)
    {
        text = text + "[" + names[pos] + "]\n" + readfile(folder + names[pos]);
    }

    return text;
}

// Checks that a message is plain language: not empty, no developer words and no punctuation (rule P1).
bool plain(const string& message)
{
    return !message.empty() && message.find("Exception") == string::npos &&
           message.find("exception") == string::npos && message.find("ALLOC") == string::npos && !haspunct(message);
}

// Checks whether a text contains a part.
bool contains(const string& text, const string& part)
{
    return text.find(part) != string::npos;
}

// Returns the 1-based number of a recipient in the candidates, or 0 when it is not there.
int numberof(const matchview& match, const string& recipient)
{
    for (size_t pos = 0; pos < match.candidates.size(); pos++)
    {
        if (match.candidates[pos].recipientId().toString() == recipient)
        {
            return (int)pos + 1;
        }
    }

    return 0;
}

// Returns true when the recipient is in the declined list.
bool isdeclined(const matchview& match, const string& recipient)
{
    for (size_t pos = 0; pos < match.declined.size(); pos++)
    {
        if (match.declined[pos].toString() == recipient)
        {
            return true;
        }
    }

    return false;
}

// Returns the position of a donation id.
int donationpos(const World& world, const string& id)
{
    int pos = -1;
    world.data.donationindex.get(id, pos);
    return pos;
}

// Returns the position of a recipient id.
int recipientpos(const World& world, const string& id)
{
    int pos = -1;
    world.data.recipientindex.get(id, pos);
    return pos;
}

// Returns the position of an offer id.
int offerpos(const World& world, const string& id)
{
    int pos = -1;
    world.store.find(EntityId::parse(id, "OFF"), pos);
    return pos;
}

// Sends an offer to a recipient for a donation position. Returns the offer position, or -1.
int sendto(World& world, int dpos, const string& recipient, int qty, string& message)
{
    matchview match = buildmatch(world.data, world.store, *world.provider, dpos);
    int number = numberof(match, recipient);

    if (number == 0)
    {
        message = "recipient is not a candidate";
        return -1;
    }

    return sendoffer(world.data, world.store, world.files, match, number, qty, message);
}

// Accepts an offer by its text id.
bool acceptby(World& world, const string& offer, string& message)
{
    return acceptoffer(world.data, world.store, world.files, EntityId::parse(offer, "OFF"), message);
}

// S1: the happy path from a new donation to an accepted offer.
void scenario1()
{
    World world("wf_s1", true, 0);
    string message = "";
    check(world.warnings.size() == 1 && contains(world.warnings.front(), "R005"), "S1 start with the one warning about R005");
    check(world.store.reconcile(world.data).empty(), "S1 no offer warnings at the start");

    int dpos = createdonation(world.data, world.files, EntityId("DN", 2), COOKED, 50, 60, message);
    check(dpos == 2, "S1 createdonation returns position 2");
    check(message == "Donation DON003 added", "S1 message: " + message);
    check(world.data.donations[dpos].id().toString() == "DON003", "S1 new donation is DON003");
    check(world.data.donations[dpos].status() == PENDING && world.data.donations[dpos].remainingQuantity() == 50, "S1 new donation is waiting with 50");

    matchview match = buildmatch(world.data, world.store, *world.provider, dpos);
    check(match.donation.toString() == "DON003", "S1 match is for DON003");
    check(match.effectiveremaining == 50 && match.pendingoffers == 0, "S1 effective remaining 50, nothing offered");
    check(match.candidates.size() == 3, "S1 three candidates");
    check(match.candidates.size() == 3 && match.candidates[0].recipientId().toString() == "R003" &&
              match.candidates[1].recipientId().toString() == "R004" && match.candidates[2].recipientId().toString() == "R001",
          "S1 ranking R003, R004, R001");
    check(match.routes.size() == match.candidates.size(), "S1 one route per candidate");
    check(match.routes.size() == 3 && match.routes[0] == "DN002 Metro Hotel Kitchen -> R003 Green Community Kitchen", "S1 route text uses location names: " + (match.routes.empty() ? string("none") : match.routes[0]));
    check(match.rejected.size() == 4, "S1 four rejected recipients");

    string before = statetext(world);
    int unusedcheck = effectiveremaining(world.data, world.store, dpos);
    check(unusedcheck == 50, "S1 effectiveremaining is 50");
    check(statetext(world) == before, "S1 buildmatch changed nothing");

    int opos = sendoffer(world.data, world.store, world.files, match, 1, 50, message);
    check(opos == 0, "S1 sendoffer returns position 0");
    check(plain(message), "S1 send message is plain: " + message);
    check(world.store.at(0).status() == OFFER_PENDING && world.store.at(0).recipientId().toString() == "R003", "S1 offer is pending for R003");
    check(world.store.at(0).travelMinutes() == 9, "S1 offer remembers 9 minutes");
    check(readfile(world.folder + "offers.csv").find("OFF001,DON003,R003,50,9,PENDING,") != string::npos, "S1 offers.csv holds the pending offer");
    check(world.data.donations[dpos].remainingQuantity() == 50, "S1 nothing is reserved yet");
    check(world.data.recipients[recipientpos(world, "R003")].availableCapacity() == 120, "S1 capacity unchanged before accept");
    check(effectiveremaining(world.data, world.store, dpos) == 0, "S1 effective remaining is 0 after the offer");

    bool accepted = acceptby(world, "OFF001", message);
    check(accepted, "S1 acceptoffer returns true: " + message);
    check(plain(message), "S1 accept message is plain");
    check(world.data.donations[dpos].remainingQuantity() == 0 && world.data.donations[dpos].status() == COMPLETED, "S1 donation is completed with 0 left");
    check(world.data.recipients[recipientpos(world, "R003")].availableCapacity() == 70, "S1 R003 capacity is 70");
    check(world.data.transactions.size() == 1, "S1 one transaction");
    check(world.data.transactions.size() == 1 && world.data.transactions[0].toCsvRow() == "T001,DON003,R003,50,9", "S1 transaction row");
    check(world.store.at(0).status() == OFFER_ACCEPTED && world.store.at(0).transactionId().toString() == "T001", "S1 offer is accepted with T001");

    check(world.files.checkinvariants(world.data).empty(), "S1 invariants are clean");

    World again("wf_s1", false, 0);
    check(statetext(again) == statetext(world), "S1 everything is the same after a fresh load");
    check(readfile(world.folder + "transactions.csv").find("T001,DON003,R003,50,9") != string::npos, "S1 transactions.csv has the row");
    check(readfile(world.folder + "recipients.csv").find("R003,Green Community Kitchen,COMMUNITY_KITCHEN,70,COOKED,1,1,7") != string::npos, "S1 recipients.csv has capacity 70");
    check(readfile(world.folder + "donations.csv").find("DON003,DN002,COOKED,50,0,60,COMPLETED") != string::npos, "S1 donations.csv shows completed");
    check(readfile(world.folder + "offers.csv").find("OFF001,DON003,R003,50,9,ACCEPTED,T001") != string::npos, "S1 offers.csv shows accepted with T001");
    check(again.files.checkinvariants(again.data).empty(), "S1 invariants clean after reload");
    check(again.warnings.size() == 1, "S1 after reload still only the R005 warning (reconcile is clean)");
    check(offeridissuer::issuenext().toString() == "OFF002", "S1 the next offer id is OFF002");
}

// S2: the recipient declines.
void scenario2()
{
    World world("wf_s2", true, 0);
    string message = "";
    int opos = sendto(world, 0, "R003", 30, message);
    check(opos == 0, "S2 offer sent to R003");

    bool declined = declineoffer(world.store, world.files, EntityId("OFF", 1), message);
    check(declined, "S2 declineoffer returns true");
    check(plain(message), "S2 decline message is plain: " + message);
    check(world.store.at(0).status() == OFFER_DECLINED, "S2 offer is declined");
    check(world.data.recipients[recipientpos(world, "R003")].availableCapacity() == 120, "S2 capacity unchanged");
    check(world.data.donations[0].status() == PENDING && world.data.donations[0].remainingQuantity() == 80, "S2 donation still open with 80");
    check(world.data.transactions.empty(), "S2 no transaction");

    matchview match = buildmatch(world.data, world.store, *world.provider, 0);
    check(numberof(match, "R003") == 0, "S2 R003 is no longer a candidate");
    check(isdeclined(match, "R003"), "S2 R003 is in the declined list");
    check(match.candidates.size() == 2, "S2 two candidates remain");
    check(match.effectiveremaining == 80, "S2 effective remaining is 80 again");

    texttable rejected = rejectedtable(world.data, match);
    check(rejected.rows.size() == match.rejected.size() + 1, "S2 the rejected table lists the declined recipient");
    check(rejected.rows.size() > 0 && rejected.rows.back()[0] == "Green Community Kitchen" && rejected.rows.back()[1] == "Declined this donation earlier", "S2 declined row text");

    check(declineoffer(world.store, world.files, EntityId("OFF", 1), message) == false, "S2 declining twice is refused");
    check(plain(message), "S2 second decline message is plain: " + message);
    check(declineoffer(world.store, world.files, EntityId("OFF", 9), message) == false, "S2 declining an unknown offer is refused");

    World again("wf_s2", false, 0);
    check(statetext(again) == statetext(world), "S2 same after a fresh load");
    check(again.store.wasdeclined(EntityId("DON", 1), EntityId("R", 3)), "S2 the decline survives a restart");
}

// S3: one donation is shared by two recipients: PENDING to PARTIAL to COMPLETED.
void scenario3()
{
    World world("wf_s3", true, 0);
    string message = "";
    int dpos = createdonation(world.data, world.files, EntityId("DN", 2), COOKED, 200, 60, message);
    matchview first = buildmatch(world.data, world.store, *world.provider, dpos);
    check(first.candidates.size() == 3, "S3 three candidates");
    check(first.candidates.size() == 3 && first.candidates[0].recipientId().toString() == "R001" && first.candidates[0].potentialAcceptance() == 150, "S3 R001 first, can take 150");
    check(first.candidates.size() == 3 && first.candidates[1].potentialAcceptance() == 120 && first.candidates[2].potentialAcceptance() == 80, "S3 then 120 and 80");

    int a = sendoffer(world.data, world.store, world.files, first, 1, 150, message);
    check(a == 0, "S3 first offer of 150 sent");
    check(effectiveremaining(world.data, world.store, dpos) == 50, "S3 50 left to offer");

    matchview second = buildmatch(world.data, world.store, *world.provider, dpos);
    check(second.effectiveremaining == 50 && second.pendingoffers == 150, "S3 match shows 50 left and 150 offered");
    check(second.candidates.size() == 3 && second.candidates[0].potentialAcceptance() == 50, "S3 everybody can take 50 now");
    int b = sendto(world, dpos, "R003", 50, message);
    check(b == 1, "S3 second offer of 50 sent to R003");
    check(effectiveremaining(world.data, world.store, dpos) == 0, "S3 nothing left to offer");
    matchview third = buildmatch(world.data, world.store, *world.provider, dpos);
    check(third.candidates.empty(), "S3 no candidates when everything is offered");
    check(world.data.donations[dpos].status() == PENDING, "S3 still PENDING before any accept");

    check(acceptby(world, "OFF001", message), "S3 first accept: " + message);
    check(world.data.donations[dpos].status() == PARTIAL && world.data.donations[dpos].remainingQuantity() == 50, "S3 PARTIAL with 50 left");
    check(world.data.recipients[recipientpos(world, "R001")].availableCapacity() == 0, "S3 R001 has no room left");

    check(acceptby(world, "OFF002", message), "S3 second accept: " + message);
    check(world.data.donations[dpos].status() == COMPLETED && world.data.donations[dpos].remainingQuantity() == 0, "S3 COMPLETED with 0 left");
    check(world.data.transactions.size() == 2 && world.data.transactions[0].quantity() + world.data.transactions[1].quantity() == 200, "S3 quantities add up to 200");
    check(world.files.checkinvariants(world.data).empty(), "S3 invariants clean");

    World again("wf_s3", false, 0);
    check(statetext(again) == statetext(world), "S3 same after a fresh load");
}

// S4: nobody can take the food, the coordinator closes the donation.
void scenario4()
{
    World world("wf_s4", true, 0);
    string message = "";
    int dpos = createdonation(world.data, world.files, EntityId("DN", 2), COOKED, 30, 1, message);
    matchview match = buildmatch(world.data, world.store, *world.provider, dpos);
    check(match.candidates.empty(), "S4 no candidate");
    check(match.rejected.size() == 7, "S4 every recipient is rejected with a reason");

    bool toofar = false;
    bool unreachable = false;

    for (size_t pos = 0; pos < match.rejected.size(); pos++)
    {
        if (match.rejected[pos].reason() == TOO_FAR)
        {
            toofar = true;
        }

        if (match.rejected[pos].reason() == UNREACHABLE)
        {
            unreachable = true;
        }
    }

    check(toofar && unreachable, "S4 reasons include too far and unreachable");

    bool closed = closedonationwithoffers(world.data, world.store, world.files, dpos, message);
    check(closed, "S4 closedonationwithoffers returns true");
    check(contains(message, "30"), "S4 message names the wasted quantity: " + message);
    check(plain(message), "S4 close message is plain");
    check(world.data.donations[dpos].status() == CLOSED && world.data.donations[dpos].remainingQuantity() == 30, "S4 donation is closed and 30 stays as wasted");
    check(computetotals(world.data).wasted == 30, "S4 the summary counts 30 as wasted");
    check(readfile(world.folder + "donations.csv").find("DON003,DN002,COOKED,30,30,1,CLOSED") != string::npos, "S4 donations.csv shows closed");
    check(world.files.checkinvariants(world.data).empty(), "S4 invariants clean");

    check(closedonationwithoffers(world.data, world.store, world.files, dpos, message) == false, "S4 closing again is refused");
    check(plain(message), "S4 second close message is plain: " + message);
    check(closedonationwithoffers(world.data, world.store, world.files, 99, message) == false, "S4 closing an unknown donation is refused");

    matchview closedmatch = buildmatch(world.data, world.store, *world.provider, dpos);
    check(closedmatch.candidates.empty() && closedmatch.effectiveremaining == 0, "S4 a closed donation has nothing left to offer");

    World again("wf_s4", false, 0);
    check(statetext(again) == statetext(world), "S4 same after a fresh load");
}

// S5: the recipient stops accepting after an offer was sent.
void scenario5()
{
    World world("wf_s5", true, 0);
    string message = "";
    check(sendto(world, 0, "R003", 30, message) == 0, "S5 offer sent");
    check(updaterecipient(world.data, world.files, EntityId("R", 3), 120, false, true, message), "S5 R003 stops accepting");

    string filesbefore = filestate(world.folder);
    string memorybefore = statetext(world);
    check(acceptby(world, "OFF001", message) == false, "S5 accept fails");
    check(plain(message) && contains(message, "no longer"), "S5 plain message: " + message);
    check(world.store.at(0).status() == OFFER_PENDING, "S5 the offer stays pending");
    check(filestate(world.folder) == filesbefore, "S5 no file changed");
    check(statetext(world) == memorybefore, "S5 memory unchanged");
    check(world.data.transactions.empty(), "S5 no transaction");
}

// S6: a donation is closed while an offer is pending, and a stale pending offer on a closed donation.
void scenario6()
{
    World world("wf_s6", true, 0);
    string message = "";
    check(sendto(world, 0, "R003", 40, message) == 0, "S6 first offer sent");
    check(sendto(world, 0, "R001", 20, message) == 1, "S6 second offer sent");
    check(closedonationwithoffers(world.data, world.store, world.files, 0, message), "S6 donation closed");
    check(world.store.at(0).status() == OFFER_WITHDRAWN && world.store.at(1).status() == OFFER_WITHDRAWN, "S6 both pending offers are withdrawn");
    check(readfile(world.folder + "offers.csv").find("OFF001,DON001,R003,40,9,WITHDRAWN,") != string::npos, "S6 offers.csv shows withdrawn");
    check(world.data.donations[0].status() == CLOSED, "S6 donation is closed");
    check(acceptby(world, "OFF001", message) == false, "S6 a withdrawn offer cannot be accepted");
    check(plain(message), "S6 message is plain: " + message);

    // a hand-made stale offer: pending on a donation that is already closed
    World stale("wf_s6b", true, 0);
    writefile(stale.folder + "donations.csv",
              "donation_id,donor_id,food_type,original_quantity,remaining_quantity,usable_minutes,status\n"
              "DON001,DN002,COOKED,80,80,60,PENDING\n"
              "DON002,DN002,COOKED,80,80,9,CLOSED\n");
    writefile(stale.folder + "offers.csv", offersheadertext() + "OFF001,DON002,R003,20,9,PENDING,\n");
    World loaded("wf_s6b", false, 0);
    check(loaded.warnings.size() == 2 && contains(loaded.warnings.back(), "OFF001"), "S6 start-up warns about the stale offer");
    string others = readfile(loaded.folder + "donations.csv") + readfile(loaded.folder + "recipients.csv") + readfile(loaded.folder + "transactions.csv");
    bool accepted = acceptby(loaded, "OFF001", message);
    check(!accepted, "S6 accepting the stale offer fails");
    check(plain(message) && contains(message, "not open"), "S6 plain message: " + message);
    check(loaded.store.at(0).status() == OFFER_WITHDRAWN, "S6 the stale offer becomes withdrawn");
    check(readfile(loaded.folder + "offers.csv").find("OFF001,DON002,R003,20,9,WITHDRAWN,") != string::npos, "S6 the withdrawal is saved");
    check(readfile(loaded.folder + "donations.csv") + readfile(loaded.folder + "recipients.csv") + readfile(loaded.folder + "transactions.csv") == others, "S6 no other file changed");
    check(loaded.data.transactions.empty(), "S6 no transaction");
}

// S7: many steps, then a restart with everything loaded again.
void scenario7()
{
    World world("wf_s7", true, 1);
    string message = "";
    check(world.warnings.size() == 1, "S7 the rich data starts with only the R005 warning");

    int d1 = createdonation(world.data, world.files, EntityId("DN", 2), COOKED, 60, 60, message);
    check(d1 == 5, "S7 new donation at position 5");
    int o1 = sendto(world, d1, "R003", 30, message);
    int o2 = sendto(world, d1, "R001", 30, message);
    check(o1 >= 0 && o2 >= 0, "S7 two offers sent");
    check(acceptoffer(world.data, world.store, world.files, world.store.at(o1).id(), message), "S7 first offer accepted");
    check(declineoffer(world.store, world.files, world.store.at(o2).id(), message), "S7 second offer declined");
    int o3 = sendto(world, d1, "R004", 30, message);
    check(o3 >= 0, "S7 third offer sent");
    check(withdrawoffer(world.store, world.files, world.store.at(o3).id(), message), "S7 third offer withdrawn");
    check(closedonationwithoffers(world.data, world.store, world.files, d1, message), "S7 donation closed");
    check(updaterecipient(world.data, world.files, EntityId("R", 2), 55, true, true, message), "S7 R002 changes its capacity");

    World again("wf_s7", false, 0);
    check(statetext(again) == statetext(world), "S7 all rows equal after a restart");
    check(again.files.checkinvariants(again.data).empty(), "S7 invariants clean after a restart");
    check(again.warnings.size() == 1, "S7 only the R005 warning after a restart");
    check(again.store.count() == world.store.count(), "S7 same number of offers");

    string newmessage = "";
    int d2 = createdonation(again.data, again.files, EntityId("DN", 1), PACKAGED, 10, 20, newmessage);
    check(again.data.donations[d2].id().toString() == "DON007", "S7 the next donation id continues: " + again.data.donations[d2].id().toString());
    int o4 = sendto(again, d2, "R002", 10, newmessage);
    check(o4 >= 0 && again.store.at(o4).id().toString() == "OFF010", "S7 the next offer id continues: " + (o4 >= 0 ? again.store.at(o4).id().toString() : string("none")));
}

// S8: bad data and failing saves.
void scenario8()
{
    // a corrupt offers.csv stops the loading with a clear message
    World good("wf_s8", true, 0);
    string message = "";
    check(sendto(good, 0, "R003", 30, message) == 0, "S8 offer sent");
    writefile(good.folder + "offers.csv", offersheadertext() + "OFF001,DON001,R003,30,9,PENDING,\nOFF002,DON001,R003,zero,9,PENDING,\n");
    string problem = "";

    try
    {
        offerstore broken;
        list<string> warnings;
        broken.load(good.folder, warnings);
    }
    catch (const DataException& caught)
    {
        problem = caught.what();
    }

    check(problem.find("offers.csv line 3: ") == 0, "S8 corrupt line gives a clear message: " + problem);
    writefile(good.folder + "offers.csv", offersheadertext() + "OFF001,DON001,R003,30,9,PENDING,\n");

    // a read-only offers.csv: sendoffer fails and changes nothing
    string rowsbefore = statetext(good);
    string filesbefore = filestate(good.folder);
    setreadonly(good.folder + "offers.csv", true);
    int opos = sendto(good, 0, "R001", 20, message);
    setreadonly(good.folder + "offers.csv", false);
    check(opos == -1, "S8 sendoffer returns -1 on a failed save");
    check(plain(message), "S8 message is plain: " + message);
    check(statetext(good) == rowsbefore, "S8 the store is unchanged after the failed send");
    remove((good.folder + "offers.csv.tmp").c_str());
    check(filestate(good.folder) == filesbefore, "S8 the files are unchanged after the failed send");
    check(sendto(good, 0, "R001", 20, message) == 1, "S8 sending works again");
    check(good.store.at(1).id().toString() == "OFF002", "S8 the failed send did not use up an offer number");

    // a failing offers save inside acceptoffer
    string memory2 = statetext(good);
    string files2 = filestate(good.folder);
    setreadonly(good.folder + "offers.csv", true);
    bool accepted = acceptby(good, "OFF001", message);
    setreadonly(good.folder + "offers.csv", false);
    remove((good.folder + "offers.csv.tmp").c_str());
    check(!accepted, "S8 accept fails when offers.csv cannot be saved");
    check(plain(message), "S8 accept message is plain: " + message);
    check(statetext(good) == memory2, "S8 donation, recipient, transactions and offer unchanged in memory (offers file)");
    check(filestate(good.folder) == files2, "S8 files unchanged (offers file)");
    check(good.store.at(0).status() == OFFER_PENDING, "S8 offer is pending again");

    // a failing donations save inside acceptoffer (after the offers file and the transaction were written)
    setreadonly(good.folder + "donations.csv", true);
    accepted = acceptby(good, "OFF001", message);
    setreadonly(good.folder + "donations.csv", false);
    remove((good.folder + "donations.csv.tmp").c_str());
    check(!accepted, "S8 accept fails when donations.csv cannot be saved");
    check(plain(message), "S8 second accept message is plain: " + message);
    check(statetext(good) == memory2, "S8 everything unchanged in memory (donations file)");
    check(filestate(good.folder) == files2, "S8 files are healed back (donations file): the transaction line is gone, the offer is pending");
    check(good.data.transactions.empty() && good.store.at(0).status() == OFFER_PENDING, "S8 no transaction and the offer is pending");

    World again("wf_s8", false, 0);
    check(statetext(again) == statetext(good), "S8 a fresh load agrees with memory");
    check(again.files.checkinvariants(again.data).empty(), "S8 invariants clean");

    check(acceptby(good, "OFF001", message), "S8 the accept works once the files are writable: " + message);
    check(good.data.transactions.size() == 1 && good.data.transactions[0].id().toString() == "T001", "S8 the transaction id was not used up by the failed tries");
}

// Extra checks of the math and the refusals.
void extras()
{
    World world("wf_extra", true, 0);
    string message = "";

    // effectiveremaining
    check(effectiveremaining(world.data, world.store, 0) == 80, "extra effective remaining of a fresh donation is 80");
    check(effectiveremaining(world.data, world.store, -1) == 0 && effectiveremaining(world.data, world.store, 9) == 0, "extra a wrong position gives 0");
    writefile(world.folder + "offers.csv",
              offersheadertext() + "OFF001,DON001,R001,30,5,PENDING,\nOFF002,DON001,R002,20,5,PENDING,\nOFF003,DON001,R003,99,5,DECLINED,\n");
    World two("wf_extra", false, 0);
    check(effectiveremaining(two.data, two.store, 0) == 30, "extra 80 - 30 - 20 = 30 (declined offers do not count)");
    writefile(world.folder + "offers.csv", offersheadertext() + "OFF001,DON001,R001,60,5,PENDING,\nOFF002,DON001,R002,60,5,PENDING,\n");
    World over("wf_extra", false, 0);
    check(effectiveremaining(over.data, over.store, 0) == 0, "extra the effective remaining is never below 0");
    clearfolder(world.folder);
    writefixture(world.folder);

    // sendoffer refusals
    World w("wf_extra2", true, 0);
    matchview match = buildmatch(w.data, w.store, *w.provider, 0);
    check(sendoffer(w.data, w.store, w.files, match, 1, 81, message) == -1, "extra a quantity above the acceptance is refused");
    check(plain(message) && contains(message, "80"), "extra message names the limit: " + message);
    check(sendoffer(w.data, w.store, w.files, match, 1, 0, message) == -1, "extra quantity 0 is refused");
    check(sendoffer(w.data, w.store, w.files, match, 0, 10, message) == -1, "extra candidate number 0 is refused");
    check(sendoffer(w.data, w.store, w.files, match, 4, 10, message) == -1, "extra candidate number 4 of 3 is refused");
    check(plain(message), "extra candidate message is plain: " + message);
    check(w.store.count() == 0 && !exists(w.folder + "offers.csv"), "extra refusals add no offer and create no file");

    // the effective remaining stops a second offer from over-offering
    check(sendoffer(w.data, w.store, w.files, match, 1, 60, message) == 0, "extra first offer of 60 is fine");
    check(sendoffer(w.data, w.store, w.files, match, 2, 30, message) == -1, "extra an old match cannot over-offer (60 + 30 > 80)");
    check(plain(message) && contains(message, "20"), "extra message names what is left: " + message);
    matchview rebuilt = buildmatch(w.data, w.store, *w.provider, 0);
    check(rebuilt.candidates.size() == 3 && rebuilt.candidates[0].potentialAcceptance() == 20, "extra the new match can take only 20");
    check(sendoffer(w.data, w.store, w.files, rebuilt, 1, 21, message) == -1, "extra 21 is refused");
    check(sendoffer(w.data, w.store, w.files, rebuilt, 1, 20, message) == 1, "extra 20 is accepted");
    check(buildmatch(w.data, w.store, *w.provider, 0).candidates.empty(), "extra nothing left after 60 + 20");

    // withdrawing
    check(withdrawoffer(w.store, w.files, EntityId("OFF", 2), message), "extra withdrawing a waiting offer works");
    check(w.store.at(1).status() == OFFER_WITHDRAWN, "extra the offer is withdrawn");
    check(effectiveremaining(w.data, w.store, 0) == 20, "extra the withdrawn quantity can be offered again");
    check(!withdrawoffer(w.store, w.files, EntityId("OFF", 2), message), "extra withdrawing twice is refused");
    check(plain(message), "extra message is plain: " + message);
    check(!withdrawoffer(w.store, w.files, EntityId("OFF", 9), message), "extra withdrawing an unknown offer is refused");
    check(acceptby(w, "OFF002", message) == false, "extra accepting a withdrawn offer is refused");
    check(acceptby(w, "OFF009", message) == false, "extra accepting an unknown offer is refused");

    // nexturgentdonation: DON002 (9 minutes) before DON001 (60 minutes)
    World n("wf_extra3", true, 0);
    check(nexturgentdonation(n.data, n.store) == 1, "extra the most urgent donation is DON002");
    check(sendto(n, 1, "R003", 80, message) == 0, "extra the whole of DON002 is offered");
    check(nexturgentdonation(n.data, n.store) == 0, "extra a fully offered donation is skipped");
    check(sendto(n, 0, "R001", 80, message) == 1, "extra the whole of DON001 is offered");
    check(nexturgentdonation(n.data, n.store) == -1, "extra no donation is left to offer");
    World m("wf_extra4", true, 1);
    check(nexturgentdonation(m.data, m.store) == 1, "extra completed and closed donations are skipped (rich data)");

    // two waiting offers to one recipient that together are above its capacity
    World c("wf_extra5", true, 0);
    int extra = createdonation(c.data, c.files, EntityId("DN", 2), COOKED, 60, 60, message);
    check(sendto(c, 0, "R004", 50, message) == 0, "extra R004 gets 50 of DON001");
    check(sendto(c, extra, "R004", 60, message) == 1, "extra R004 gets 60 of DON003");
    check(acceptby(c, "OFF001", message), "extra the first accept works");
    check(c.data.recipients[recipientpos(c, "R004")].availableCapacity() == 30, "extra R004 has 30 left");
    string before = filestate(c.folder);
    check(acceptby(c, "OFF002", message) == false, "extra the second accept fails");
    check(contains(message, "capacity") && plain(message), "extra capacity message: " + message);
    check(c.store.at(1).status() == OFFER_PENDING, "extra the second offer stays pending");
    check(filestate(c.folder) == before, "extra no file changed by the failed accept");

    // createdonation validation
    World v("wf_extra6", true, 0);
    string donationsbefore = readfile(v.folder + "donations.csv");
    check(createdonation(v.data, v.files, EntityId("DN", 99), COOKED, 10, 10, message) == -1, "extra an unknown donor is refused");
    check(plain(message), "extra unknown donor message: " + message);
    check(createdonation(v.data, v.files, EntityId("DN", 1), COOKED, 0, 10, message) == -1, "extra quantity 0 is refused");
    check(plain(message), "extra quantity message: " + message);
    check(createdonation(v.data, v.files, EntityId("DN", 1), COOKED, 10, 0, message) == -1, "extra usable 0 is refused");
    check(plain(message), "extra usable message: " + message);
    check(createdonation(v.data, v.files, EntityId("DN", 1), COOKED, -4, 10, message) == -1, "extra a negative quantity is refused");
    check(v.data.donations.size() == 2 && readfile(v.folder + "donations.csv") == donationsbefore, "extra refused donations change nothing");
    int one = createdonation(v.data, v.files, EntityId("DN", 1), BAKERY, 10, 10, message);
    int two2 = createdonation(v.data, v.files, EntityId("DN", 1), BAKERY, 10, 10, message);
    check(one == 2 && two2 == 3 && v.data.donations[3].id().toString() == "DON004", "extra donation ids continue DON003, DON004");
    int found = -1;
    check(v.data.donationindex.get("DON004", found) && found == 3, "extra the donation index knows the new donation");
    setreadonly(v.folder + "donations.csv", true);
    check(createdonation(v.data, v.files, EntityId("DN", 1), BAKERY, 10, 10, message) == -1, "extra a failed save adds nothing");
    setreadonly(v.folder + "donations.csv", false);
    remove((v.folder + "donations.csv.tmp").c_str());
    check(plain(message) && v.data.donations.size() == 4, "extra failed save message is plain and memory unchanged: " + message);
    check(createdonation(v.data, v.files, EntityId("DN", 1), BAKERY, 10, 10, message) == 4 && v.data.donations[4].id().toString() == "DON005", "extra the failed save did not use up a donation number");

    // a failed close changes nothing
    World f("wf_extra7", true, 0);
    check(sendto(f, 0, "R003", 30, message) == 0, "extra an offer before the failed close");
    string memorybefore = statetext(f);
    string filesbefore = filestate(f.folder);
    setreadonly(f.folder + "offers.csv", true);
    bool closed = closedonationwithoffers(f.data, f.store, f.files, 0, message);
    setreadonly(f.folder + "offers.csv", false);
    remove((f.folder + "offers.csv.tmp").c_str());
    check(!closed && plain(message), "extra a failed close is refused with a plain message: " + message);
    check(statetext(f) == memorybefore && filestate(f.folder) == filesbefore, "extra a failed close changes nothing");
}

// Version 1.1: createrecipient, createdonor and createdonationwithnewdonor.
void newrecords()
{
    World w("wf_v11", true, 0);
    string message = "";
    vector<FoodType> foods;
    foods.push_back(COOKED);
    foods.push_back(BAKERY);

    // createrecipient: a good one on location 8 (connected to a donor)
    int pos = createrecipient(w.data, w.files, "Test Kitchen", "soup kitchen", foods, 50, true, true, 8, message);
    check(pos == 7, "v11 createrecipient returns position 7");
    check(message == "Recipient R008 added", "v11 message: " + message);
    check(pos == 7 && w.data.recipients[pos].toCsvRow() == "R008,Test Kitchen,SOUP_KITCHEN,50,COOKED;BAKERY,1,1,8", "v11 the type is stored in capitals with an underscore");
    int found = -1;
    check(w.data.recipientindex.get("R008", found) && found == 7, "v11 the recipient index knows R008");
    World again("wf_v11", false, 0);
    check(again.data.recipients.size() == 8 && again.data.recipients[7].toCsvRow() == w.data.recipients[7].toCsvRow(), "v11 R008 is in recipients.csv after a fresh load");
    check(again.files.checkinvariants(again.data).empty(), "v11 invariants clean");

    // the new recipient takes part in the matching (DON001: cooked, DN002 at location 1, 36 minutes to location 8)
    matchview match = buildmatch(w.data, w.store, *w.provider, 0);
    check(numberof(match, "R008") > 0, "v11 the new recipient is a candidate for DON001");

    // a location that no road reaches gives a note
    int second = createrecipient(w.data, w.files, "Lonely Pantry", "pantry", foods, 10, true, true, 9, message);
    check(second == 8 && contains(message, "Recipient R009 added") && contains(message, "no donor is connected"), "v11 note for an isolated location: " + message);

    // refusals: nothing is added and no file changes
    string recipientsfile = readfile(w.folder + "recipients.csv");
    size_t count = w.data.recipients.size();
    check(createrecipient(w.data, w.files, "", "pantry", foods, 10, true, true, 8, message) == -1 && plain(message), "v11 empty name refused: " + message);
    check(createrecipient(w.data, w.files, "A, B", "pantry", foods, 10, true, true, 8, message) == -1 && plain(message), "v11 comma in the name refused: " + message);
    check(createrecipient(w.data, w.files, string(61, 'x'), "pantry", foods, 10, true, true, 8, message) == -1, "v11 name of 61 characters refused");
    check(createrecipient(w.data, w.files, "Name", "", foods, 10, true, true, 8, message) == -1 && plain(message), "v11 empty type refused: " + message);
    check(createrecipient(w.data, w.files, "Name", "bad!", foods, 10, true, true, 8, message) == -1 && plain(message), "v11 bad type characters refused: " + message);
    check(createrecipient(w.data, w.files, "Name", string(31, 'x'), foods, 10, true, true, 8, message) == -1, "v11 type of 31 characters refused");
    check(createrecipient(w.data, w.files, "Name", "pantry", vector<FoodType>(), 10, true, true, 8, message) == -1 && plain(message), "v11 no food kind refused: " + message);
    check(createrecipient(w.data, w.files, "Name", "pantry", foods, -1, true, true, 8, message) == -1 && plain(message), "v11 negative capacity refused: " + message);
    check(createrecipient(w.data, w.files, "Name", "pantry", foods, 10, true, true, -1, message) == -1 && plain(message), "v11 location -1 refused: " + message);
    check(createrecipient(w.data, w.files, "Name", "pantry", foods, 10, true, true, 99, message) == -1, "v11 location 99 refused");
    check(w.data.recipients.size() == count && readfile(w.folder + "recipients.csv") == recipientsfile, "v11 refusals changed nothing");

    // a failed save adds nothing and does not use up an id
    setreadonly(w.folder + "recipients.csv", true);
    check(createrecipient(w.data, w.files, "Name", "pantry", foods, 10, true, true, 8, message) == -1, "v11 a failed save returns -1");
    setreadonly(w.folder + "recipients.csv", false);
    remove((w.folder + "recipients.csv.tmp").c_str());
    check(plain(message) && w.data.recipients.size() == count && readfile(w.folder + "recipients.csv") == recipientsfile, "v11 failed save changed nothing: " + message);
    check(createrecipient(w.data, w.files, "Name", "pantry", foods, 10, true, true, 8, message) == 9 && w.data.recipients[9].id().toString() == "R010", "v11 the failed save did not use up R010");

    // createdonor
    int donor = createdonor(w.data, w.files, "Fresh Foods", "market", 2, message);
    check(donor == 2 && message == "Donor DN003 added", "v11 createdonor: " + message);
    check(donor == 2 && w.data.donors[donor].toCsvRow() == "DN003,Fresh Foods,MARKET,2", "v11 donor row");
    World donors("wf_v11", false, 0);
    check(donors.data.donors.size() == 3 && donors.data.donors[2].toCsvRow() == "DN003,Fresh Foods,MARKET,2", "v11 DN003 is in donors.csv after a fresh load");
    string donorsfile = readfile(w.folder + "donors.csv");
    check(createdonor(w.data, w.files, "", "market", 2, message) == -1 && plain(message), "v11 donor without a name refused: " + message);
    check(createdonor(w.data, w.files, "X", "bad!", 2, message) == -1, "v11 donor with a bad type refused");
    check(createdonor(w.data, w.files, "X", "market", 50, message) == -1 && plain(message), "v11 donor on a location off the map refused: " + message);
    check(readfile(w.folder + "donors.csv") == donorsfile && w.data.donors.size() == 3, "v11 refused donors changed nothing");

    // checkdonordetails saves nothing and gives the same plain messages
    check(checkdonordetails(w.data, "Good Name", "market", 3, message) && w.data.donors.size() == 3, "v11 checkdonordetails accepts good details and saves nothing");
    check(!checkdonordetails(w.data, "", "market", 3, message) && plain(message), "v11 checkdonordetails refuses an empty name: " + message);
    check(!checkdonordetails(w.data, "X", "market", 11, message) && plain(message), "v11 checkdonordetails refuses a location off the map: " + message);

    // createdonationwithnewdonor
    int donation =createdonationwithnewdonor(w.data, w.files, "Corner Bakery", "bakery", 3, BAKERY, 20, 30, message);
    check(donation == 2, "v11 donation for a new donor returns position 2");
    check(message == "Donation DON003 added for the new donor Corner Bakery with id DN004", "v11 message: " + message);
    check(donation == 2 && w.data.donations[donation].donorId().toString() == "DN004", "v11 the donation belongs to the new donor");
    World both("wf_v11", false, 0);
    check(both.data.donors.size() == 4 && both.data.donations.size() == 3 && both.files.checkinvariants(both.data).empty(), "v11 donor and donation are both on file and consistent");

    // refusals save nothing, not even the donor
    string donorsfile2 = readfile(w.folder + "donors.csv");
    string donationsfile2 = readfile(w.folder + "donations.csv");
    check(createdonationwithnewdonor(w.data, w.files, "Other", "bakery", 3, BAKERY, 0, 30, message) == -1 && plain(message), "v11 quantity 0 refused: " + message);
    check(createdonationwithnewdonor(w.data, w.files, "Other", "bakery", 3, BAKERY, 5, 0, message) == -1 && plain(message), "v11 usable 0 refused: " + message);
    check(createdonationwithnewdonor(w.data, w.files, "", "bakery", 3, BAKERY, 5, 5, message) == -1, "v11 empty donor name refused");
    check(createdonationwithnewdonor(w.data, w.files, "Other", "bakery", 77, BAKERY, 5, 5, message) == -1, "v11 location off the map refused");
    check(readfile(w.folder + "donors.csv") == donorsfile2 && readfile(w.folder + "donations.csv") == donationsfile2 && w.data.donors.size() == 4, "v11 no donor was saved for a bad input");

    // the donation cannot be saved: the donor was already saved (and says so)
    setreadonly(w.folder + "donations.csv", true);
    check(createdonationwithnewdonor(w.data, w.files, "Late Donor", "bakery", 3, BAKERY, 5, 5, message) == -1, "v11 donation save failure returns -1");
    setreadonly(w.folder + "donations.csv", false);
    remove((w.folder + "donations.csv.tmp").c_str());
    check(contains(message, "was saved but the donation could not be saved") && plain(message), "v11 message says the donor was saved: " + message);
    check(w.data.donors.size() == 5 && w.data.donations.size() == 3 && readfile(w.folder + "donations.csv") == donationsfile2, "v11 the donor is kept, the donation is not");
    World last("wf_v11", false, 0);
    check(last.data.donors.size() == 5 && last.files.checkinvariants(last.data).empty(), "v11 the files still load cleanly");
}

// Runs every scenario.
int main()
{
    newrecords();
    scenario1();
    scenario2();
    scenario3();
    scenario4();
    scenario5();
    scenario6();
    scenario7();
    scenario8();
    extras();
    return finish();
}
