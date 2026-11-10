// test_text: the text tools (plaintext, plaintextkeepfiles, haspunct) and rule P1: no fixed text of the backend and
// no message of any Workflow function has a punctuation mark. Plain C++, no window.

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
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// Checks plaintext for one input.
void plaincase(const string& input, const string& expected)
{
    string got = plaintext(input);
    check(got == expected, "plaintext of [" + input + "] is [" + got + "] and should be [" + expected + "]");
}

// Checks haspunct for one input.
void punctcase(const string& input, bool expected)
{
    check(haspunct(input) == expected, string("haspunct of [") + input + "] is " + (expected ? "true" : "false"));
}

// Checks that every header of a table and every text in the given columns has no punctuation.
void tableplain(const texttable& table, const string& name, const vector<int>& fixedcolumns)
{
    for (size_t pos = 0; pos < table.headers.size(); pos++)
    {
        check(!haspunct(table.headers[pos]), name + " header [" + table.headers[pos] + "] has no punctuation");
    }

    for (size_t row = 0; row < table.rows.size(); row++)
    {
        for (size_t at = 0; at < fixedcolumns.size(); at++)
        {
            int column = fixedcolumns[at];

            if (column < (int)table.rows[row].size())
            {
                check(!haspunct(table.rows[row][column]), name + " text [" + table.rows[row][column] + "] has no punctuation");
            }
        }
    }
}

// One running copy of the data (rich fixture: five donations, seven recipients, six offers).
class Setup
{
public:
    // Loads the files like the real start-up and makes the travel time provider (the datamanager has no default
    // constructor, so it is built in the initializer list).
    Setup(const string& name) : files(string(CB_TESTDATA) + name + "/")
    {
        folder = string(CB_TESTDATA) + name + "/";
        testfolder(name);
        writerichfixture(folder);
        provider = NULL;
        files.loadall(data, warnings);
        transactionidissuer::seed(data.lasttransaction);
        donationidissuer::seed(EntityId("DON", 5));
        donoridissuer::seed(EntityId("DN", 2));
        recipientidissuer::seed(EntityId("R", 7));
        store.load(folder, warnings);
        provider = new dijkstratraveltime(data.network);
    }

    // Frees the provider.
    ~Setup()
    {
        delete provider;
    }

    string folder;                  // the test folder with a trailing slash
    datamanager files;              // reads and writes the csv files
    dataset data;                   // everything in memory
    list<string> warnings;          // start-up warnings
    offerstore store;               // all offers
    dijkstratraveltime* provider;   // travel times

private:
    // Copying is not allowed (the provider is owned by the object).
    Setup(const Setup& other);
    Setup& operator=(const Setup& other);
};

// Runs every Workflow function with good and bad input and checks every message.
void messagechecks()
{
    Setup w("text_messages");
    vector<string> messages;
    string message = "";

    // sendoffer: a bad number, a bad quantity, too much, an unknown donation, and a good offer
    matchview match = buildmatch(w.data, w.store, *w.provider, 0);
    check(!match.candidates.empty(), "the match of DON001 has candidates");
    sendoffer(w.data, w.store, w.files, match, 0, 5, message);
    messages.push_back(message);
    sendoffer(w.data, w.store, w.files, match, 1, 0, message);
    messages.push_back(message);
    sendoffer(w.data, w.store, w.files, match, 1, 5000, message);
    messages.push_back(message);
    matchview wrong = match;
    wrong.donation = EntityId("DON", 99);
    sendoffer(w.data, w.store, w.files, wrong, 1, 1, message);
    messages.push_back(message);
    matchview closed = match;
    closed.donation = EntityId("DON", 3);
    sendoffer(w.data, w.store, w.files, closed, 1, 1, message);
    messages.push_back(message);
    int sentpos = sendoffer(w.data, w.store, w.files, match, 1, 5, message);
    check(sentpos >= 0 && message.compare(0, 6, "Offer ") == 0 && message.find(" sent to ") != string::npos, "a good offer says it was sent: " + message);
    messages.push_back(message);

    // accept, decline, withdraw: unknown, already answered, and a good one each
    acceptoffer(w.data, w.store, w.files, EntityId("OFF", 99), message);
    messages.push_back(message);
    acceptoffer(w.data, w.store, w.files, EntityId("OFF", 2), message);
    messages.push_back(message);
    bool accepted = acceptoffer(w.data, w.store, w.files, EntityId("OFF", 1), message);
    messages.push_back(message);
    check(accepted, "OFF001 can be accepted: " + message);
    declineoffer(w.store, w.files, EntityId("OFF", 99), message);
    messages.push_back(message);
    declineoffer(w.store, w.files, EntityId("OFF", 2), message);
    messages.push_back(message);
    bool declined = declineoffer(w.store, w.files, EntityId("OFF", 6), message);
    messages.push_back(message);
    check(declined, "OFF006 can be declined: " + message);
    withdrawoffer(w.store, w.files, EntityId("OFF", 99), message);
    messages.push_back(message);
    withdrawoffer(w.store, w.files, EntityId("OFF", 2), message);
    messages.push_back(message);
    bool withdrawn = withdrawoffer(w.store, w.files, EntityId("OFF", 5), message);
    messages.push_back(message);
    check(withdrawn, "OFF005 can be withdrawn: " + message);

    // a stale offer: pending on a donation that is already closed, then accepted (the program withdraws it)
    closedonationwithoffers(w.data, w.store, w.files, 99, message);
    messages.push_back(message);
    closedonationwithoffers(w.data, w.store, w.files, 3, message);
    messages.push_back(message);
    bool closedok = closedonationwithoffers(w.data, w.store, w.files, 0, message);
    messages.push_back(message);
    check(closedok, "DON001 can be closed: " + message);

    // donations
    vector<FoodType> foods;
    foods.push_back(COOKED);
    createdonation(w.data, w.files, EntityId("DN", 99), COOKED, 10, 10, message);
    messages.push_back(message);
    createdonation(w.data, w.files, EntityId("DN", 1), COOKED, 0, 10, message);
    messages.push_back(message);
    createdonation(w.data, w.files, EntityId("DN", 1), COOKED, 10, 0, message);
    messages.push_back(message);
    int added = createdonation(w.data, w.files, EntityId("DN", 1), COOKED, 10, 10, message);
    messages.push_back(message);
    check(added >= 0 && message.find("added") != string::npos, "a good donation says added: " + message);

    // recipients: every wrong input, a good one, and one on a location without a road to a donor (node 9)
    createrecipient(w.data, w.files, "", "pantry", foods, 10, true, true, 8, message);
    messages.push_back(message);
    createrecipient(w.data, w.files, "A, B", "pantry", foods, 10, true, true, 8, message);
    messages.push_back(message);
    createrecipient(w.data, w.files, "Name", "", foods, 10, true, true, 8, message);
    messages.push_back(message);
    createrecipient(w.data, w.files, "Name", "bad!", foods, 10, true, true, 8, message);
    messages.push_back(message);
    createrecipient(w.data, w.files, "Name", "pantry", vector<FoodType>(), 10, true, true, 8, message);
    messages.push_back(message);
    createrecipient(w.data, w.files, "Name", "pantry", foods, -1, true, true, 8, message);
    messages.push_back(message);
    createrecipient(w.data, w.files, "Name", "pantry", foods, 10, true, true, -1, message);
    messages.push_back(message);
    int rgood = createrecipient(w.data, w.files, "Test Kitchen", "soup kitchen", foods, 50, true, true, 8, message);
    messages.push_back(message);
    check(rgood >= 0, "a good recipient is added: " + message);
    int risolated = createrecipient(w.data, w.files, "Lonely Kitchen", "pantry", foods, 50, true, true, 9, message);
    messages.push_back(message);
    check(risolated >= 0 && message.find("no donor is connected") != string::npos, "the isolated recipient says so: " + message);

    // donors
    createdonor(w.data, w.files, "", "market", 2, message);
    messages.push_back(message);
    createdonor(w.data, w.files, "X", "market", 50, message);
    messages.push_back(message);
    int dgood = createdonor(w.data, w.files, "Corner Bakery", "bakery", 3, message);
    messages.push_back(message);
    check(dgood >= 0, "a good donor is added: " + message);
    int disolated = createdonor(w.data, w.files, "Lonely Market", "market", 9, message);
    messages.push_back(message);
    check(disolated >= 0, "an isolated donor is added: " + message);
    checkdonordetails(w.data, "", "market", 3, message);
    messages.push_back(message);
    checkdonordetails(w.data, "X", "market", 11, message);
    messages.push_back(message);
    createdonationwithnewdonor(w.data, w.files, "Other", "bakery", 3, BAKERY, 0, 30, message);
    messages.push_back(message);
    createdonationwithnewdonor(w.data, w.files, "Other", "bakery", 3, BAKERY, 5, 0, message);
    messages.push_back(message);
    int both = createdonationwithnewdonor(w.data, w.files, "Third Bakery", "bakery", 3, BAKERY, 5, 30, message);
    messages.push_back(message);
    check(both >= 0 && message.find("added for the new donor") != string::npos, "donation with a new donor: " + message);

    // changing the details of a recipient
    updaterecipient(w.data, w.files, EntityId("R", 99), 10, true, true, message);
    messages.push_back(message);
    updaterecipient(w.data, w.files, EntityId("R", 1), -5, true, true, message);
    messages.push_back(message);
    updaterecipient(w.data, w.files, EntityId("R", 1), 20, true, true, message);
    messages.push_back(message);

    // a failed save: the files are read-only for a moment
    setreadonly(w.folder + "donations.csv", true);
    createdonation(w.data, w.files, EntityId("DN", 1), COOKED, 10, 10, message);
    messages.push_back(message);
    setreadonly(w.folder + "donations.csv", false);
    setreadonly(w.folder + "offers.csv", true);
    matchview again = buildmatch(w.data, w.store, *w.provider, 1);

    if (!again.candidates.empty())
    {
        sendoffer(w.data, w.store, w.files, again, 1, 1, message);
        messages.push_back(message);
    }

    setreadonly(w.folder + "offers.csv", false);

    int plainmessages = 0;

    for (size_t pos = 0; pos < messages.size(); pos++)
    {
        stringstream number;
        number << pos + 1;
        check(!messages[pos].empty() && !haspunct(messages[pos]), "workflow message " + number.str() + " [" + messages[pos] + "] is plain");
        plainmessages++;
    }

    check(plainmessages > 40, "more than 40 workflow messages were checked");
}

// Runs every check of this test.
int main()
{
    // plaintext
    plaincase("", "");
    plaincase("...", "");
    plaincase("?", "");
    plaincase("Hello, world.", "Hello world");
    plaincase("a - b", "a b");
    plaincase("A-B", "A B");
    plaincase("don't", "dont");
    plaincase("Are you sure?", "Are you sure?");
    plaincase("Really??", "Really?");
    plaincase("What?!", "What");
    plaincase("Is (this) right?", "Is this right?");
    plaincase("  two   spaces  ", "two spaces");
    plaincase("a;b:c", "a b c");
    plaincase("end.", "end");
    plaincase("tabs\tand\nlines", "tabs and lines");
    plaincase("(node 16)", "node 16");
    plaincase("C++ code", "C code");
    plaincase("100% [done]!", "100 done");
    plaincase("already plain", "already plain");
    plaincase("Donation DON047 added", "Donation DON047 added");

    // plaintext is stable: a second run changes nothing
    check(plaintext(plaintext("Hello, (world); it's fine?")) == plaintext("Hello, (world); it's fine?"), "plaintext twice is the same as once");

    // haspunct
    punctcase("", false);
    punctcase("plain words", false);
    punctcase("Are you sure?", false);
    punctcase("Are you sure??", true);
    punctcase("What? Really", true);
    punctcase("end.", true);
    punctcase("a,b", true);
    punctcase("a-b", true);
    punctcase("(x)", true);
    punctcase("DON001", false);
    punctcase("  spaces only around  ", false);
    punctcase("?", false);
    punctcase("a_b", true);

    // plaintextkeepfiles keeps file names and paths
    check(plaintextkeepfiles("offers.csv line 3: the id OFF001 is used twice") == "offers.csv line 3 the id OFF001 is used twice", "keepfiles keeps the file name and drops the colon");
    check(plaintextkeepfiles("Data folder C:\\Users\\Me\\data\\") == "Data folder C:\\Users\\Me\\data\\", "keepfiles keeps a path");
    check(plaintextkeepfiles("It failed, sadly.") == "It failed sadly", "keepfiles cleans ordinary words");
    check(plaintextkeepfiles("") == "", "keepfiles of nothing");

    // every fixed text of every table
    Setup w("text_tables");
    vector<int> allpos = allpositions((int)w.data.donations.size());
    vector<int> none;
    tableplain(donationstable(w.data, w.store, allpos), "donations", vector<int>(1, 7));
    vector<int> recipientcolumns;
    recipientcolumns.push_back(2);
    recipientcolumns.push_back(4);
    recipientcolumns.push_back(5);
    recipientcolumns.push_back(6);
    tableplain(recipientstable(w.data, allpositions((int)w.data.recipients.size())), "recipients", recipientcolumns);
    tableplain(transactionstable(w.data, allpositions((int)w.data.transactions.size())), "transactions", vector<int>());
    vector<int> fixedtwo;
    fixedtwo.push_back(7);
    tableplain(offerstable(w.data, w.store, allpositions(w.store.count())), "offers", fixedtwo);
    matchview match = buildmatch(w.data, w.store, *w.provider, 0);
    vector<int> forone(1, 3);
    tableplain(candidatestable(w.data, match), "candidates", forone);
    vector<int> fortwo;
    fortwo.push_back(1);
    fortwo.push_back(2);
    tableplain(rejectedtable(w.data, match), "rejected", fortwo);
    tableplain(summarytable(computetotals(w.data)), "summary", vector<int>(1, 0));
    vector<string> about = aboutlines();

    for (size_t line = 0; line < about.size(); line++)
    {
        check(!haspunct(about[line]), "the about line " + about[line] + " is plain");
    }

    // the rejected table of a donation that has an unreachable recipient and a recipient that declined
    matchview second = buildmatch(w.data, w.store, *w.provider, 1);
    tableplain(rejectedtable(w.data, second), "rejected of DON002", fortwo);

    // reasontext for every reason, and the statuses
    for (int reason = 0; reason <= 6; reason++)
    {
        string text = reasontext((Reason)reason);
        check(!text.empty() && !haspunct(text), "reasontext " + text + " is plain");
    }

    // the unreachable travel time shows the word none
    bool sawnone = false;
    matchview everything = buildmatch(w.data, w.store, *w.provider, 1);

    for (size_t pos = 0; pos < rejectedtable(w.data, everything).rows.size(); pos++)
    {
        if (rejectedtable(w.data, everything).rows[pos][2] == "none")
        {
            sawnone = true;
        }
    }

    check(sawnone, "an unreachable recipient shows none as its travel time");

    // every message of every Workflow function
    messagechecks();

    return finish();
}
