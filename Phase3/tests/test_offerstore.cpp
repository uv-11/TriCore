// test_offerstore: loading, checking and saving offers.csv, counts, reconcile and a failed save.

#include "Datamanager.h"
#include "Exceptions.h"
#include "OfferStore.h"
#include "testkit.h"

#include <list>
#include <string>
#include <vector>

using namespace std;

// The header row of offers.csv.
const string HEADER = "offer_id,donation_id,recipient_id,quantity,travel_minutes,status,txn_id\n";

// Checks whether a text starts with a prefix.
bool startswith(const string& text, const string& prefix)
{
    return text.substr(0, prefix.length()) == prefix;
}

// Checks whether a text contains a part.
bool contains(const string& text, const string& part)
{
    return text.find(part) != string::npos;
}

// Writes the content as offers.csv, loads it and returns the DataException text, or "LOADED".
string loadresult(const string& folder, const string& content)
{
    clearfolder(folder);
    writefile(folder + "offers.csv", content);

    offerstore store;
    list<string> warnings;

    try
    {
        store.load(folder, warnings);
    }
    catch (const DataException& problem)
    {
        return problem.what();
    }

    return "LOADED";
}

// Checks that a bad file gives a DataException that names the file and the expected line.
void checkbad(const string& folder, const string& content, const string& line, const string& name)
{
    string result = loadresult(folder, content);
    check(startswith(result, "offers.csv line " + line + ": "), name + " -> " + result);
}

// Returns the rows of a store as one text, to compare two stores.
string rowsof(const offerstore& store)
{
    string text = "";

    for (int pos = 0; pos < store.count(); pos++)
    {
        text = text + store.at(pos).toCsvRow() + "\n";
    }

    return text;
}

// Returns true when save throws a DataException.
bool savethrows(const offerstore& store, datamanager& files)
{
    try
    {
        store.save(files);
    }
    catch (const DataException&)
    {
        return true;
    }

    return false;
}

// Loading a missing file, saving and loading again.
void testbasics()
{
    string folder = testfolder("offerstore_basics");
    offerstore empty;
    list<string> warnings;
    empty.load(folder, warnings);
    check(empty.count() == 0, "missing offers.csv means zero offers");
    check(!exists(folder + "offers.csv"), "loading a missing file creates no file");
    check(warnings.empty(), "missing offers.csv gives no warning");

    offeridissuer::seed(EntityId("OFF", 0));
    offerstore store;
    store.add(EntityId("DON", 1), EntityId("R", 1), 30, 10);
    store.add(EntityId("DON", 1), EntityId("R", 3), 20, 9);
    int third = store.add(EntityId("DON", 2), EntityId("R", 3), 10, 9);
    check(third == 2, "add returns the position");
    check(store.count() == 3, "three offers added");
    check(store.at(0).id().toString() == "OFF001" && store.at(2).id().toString() == "OFF003", "ids are OFF001 to OFF003");
    check(store.at(1).status() == OFFER_PENDING, "a new offer is pending");
    check(!exists(folder + "offers.csv"), "add does not save");

    datamanager files(folder);
    store.save(files);
    check(exists(folder + "offers.csv"), "save creates offers.csv");
    check(readfile(folder + "offers.csv") ==
              HEADER + "OFF001,DON001,R001,30,10,PENDING,\nOFF002,DON001,R003,20,9,PENDING,\nOFF003,DON002,R003,10,9,PENDING,\n",
          "saved file text");

    offerstore again;
    list<string> more;
    again.load(folder, more);
    check(again.count() == 3, "three offers after a fresh load");
    check(rowsof(again) == rowsof(store), "rows equal after save and load");
    check(more.empty(), "no warnings for a good file");

    // find and at
    int pos = -1;
    check(again.find(EntityId("OFF", 2), pos) && pos == 1, "find an existing offer");
    check(!again.find(EntityId("OFF", 9), pos), "find a missing offer");

    bool thrown = false;

    try
    {
        again.at(3);
    }
    catch (const StructureException&)
    {
        thrown = true;
    }

    check(thrown, "at with a wrong position throws StructureException");

    bool zero = false;

    try
    {
        store.add(EntityId("DON", 1), EntityId("R", 1), 0, 5);
    }
    catch (const StructureException&)
    {
        zero = true;
    }

    check(zero, "add with quantity 0 throws StructureException");
    check(store.count() == 3, "a refused add changes nothing");
}

// Every rule of the file format.
void testbadfiles()
{
    string folder = testfolder("offerstore_bad");
    checkbad(folder, "offer_id,donation_id\nOFF001,DON001\n", "1", "wrong header");
    checkbad(folder, HEADER + "XXX001,DON001,R001,5,5,PENDING,\n", "2", "wrong offer prefix");
    checkbad(folder, HEADER + "OFF1,DON001,R001,5,5,PENDING,\n", "2", "offer id with fewer than three digits");
    checkbad(folder, HEADER + "OFF001,DN001,R001,5,5,PENDING,\n", "2", "wrong donation prefix");
    checkbad(folder, HEADER + "OFF001,DON1,R001,5,5,PENDING,\n", "2", "donation id with fewer than three digits");
    checkbad(folder, HEADER + "OFF001,DON001,DN001,5,5,PENDING,\n", "2", "wrong recipient prefix");
    checkbad(folder, HEADER + "OFF001,DON001,R1,5,5,PENDING,\n", "2", "recipient id with fewer than three digits");
    checkbad(folder, HEADER + "OFF001,DON001,R001,5,5,PENDING,\nOFF001,DON002,R002,5,5,PENDING,\n", "3", "duplicate offer id");
    checkbad(folder, HEADER + "OFF001,DON001,R001,0,5,PENDING,\n", "2", "quantity 0");
    checkbad(folder, HEADER + "OFF001,DON001,R001,abc,5,PENDING,\n", "2", "quantity not a number");
    checkbad(folder, HEADER + "OFF001,DON001,R001,5,-1,PENDING,\n", "2", "negative travel");
    checkbad(folder, HEADER + "OFF001,DON001,R001,5,xx,PENDING,\n", "2", "travel not a number");
    checkbad(folder, HEADER + "OFF001,DON001,R001,5,5,MAYBE,\n", "2", "unknown status");
    checkbad(folder, HEADER + "OFF001,DON001,R001,5,5,ACCEPTED,\n", "2", "accepted without txn id");
    checkbad(folder, HEADER + "OFF001,DON001,R001,5,5,ACCEPTED,X001\n", "2", "accepted with a wrong txn prefix");
    checkbad(folder, HEADER + "OFF001,DON001,R001,5,5,PENDING,T001\n", "2", "pending with a txn id");
    checkbad(folder, HEADER + "OFF001,DON001,R001,5,5,DECLINED,T001\n", "2", "declined with a txn id");
    checkbad(folder, HEADER + "OFF001,DON001,R001,5,5,WITHDRAWN,T001\n", "2", "withdrawn with a txn id");
    checkbad(folder, HEADER + "OFF001,DON001,R001,5,5,PENDING\n", "2", "six fields");
    checkbad(folder, HEADER + "OFF001,DON001,R001,5,5,PENDING,,\n", "2", "eight fields");
    checkbad(folder, HEADER + "OFF001,DON001,R001,5,5,PENDING,\nOFF002,DON001,R001,5,5,PENDING\n", "3", "bad row on line 3");

    string empty = loadresult(folder, "");
    check(startswith(empty, "offers.csv") && contains(empty, "empty"), "empty file -> " + empty);

    string good = loadresult(folder, HEADER + "OFF001,DON001,R001,5,5,ACCEPTED,T007\nOFF002,DON001,R002,5,0,DECLINED,\n");
    check(good == "LOADED", "good file with an accepted and a declined offer loads");

    // a bad file leaves an already loaded store unchanged
    clearfolder(folder);
    writefile(folder + "offers.csv", HEADER + "OFF001,DON001,R001,5,5,PENDING,\n");
    offerstore store;
    list<string> warnings;
    store.load(folder, warnings);
    writefile(folder + "offers.csv", HEADER + "OFF001,DON001,R001,5,5,PENDING,\nbroken row\n");
    bool thrown = false;

    try
    {
        store.load(folder, warnings);
    }
    catch (const DataException&)
    {
        thrown = true;
    }

    check(thrown, "a bad file throws DataException on a second load");
    check(store.count() == 1, "the store is unchanged after a failed load");
}

// A leftover .tmp file is handled like Phase 2 does.
void testtmprecovery()
{
    string folder = testfolder("offerstore_tmp");

    // main missing, .tmp present
    writefile(folder + "offers.csv.tmp", HEADER + "OFF001,DON001,R001,5,5,PENDING,\n");
    offerstore first;
    list<string> warnings;
    first.load(folder, warnings);
    check(first.count() == 1, "the .tmp file is used when offers.csv is missing");
    check(warnings.size() == 1, "one warning for the recovered file");
    check(exists(folder + "offers.csv") && !exists(folder + "offers.csv.tmp"), "the .tmp was renamed");

    // both present
    writefile(folder + "offers.csv.tmp", HEADER + "OFF009,DON001,R001,9,9,PENDING,\n");
    offerstore second;
    list<string> more;
    second.load(folder, more);
    check(second.count() == 1 && second.at(0).id().toString() == "OFF001", "the main file wins when both exist");
    check(more.size() == 1, "one warning for the discarded .tmp");
    check(!exists(folder + "offers.csv.tmp"), "the stale .tmp was deleted");
}

// The id issuer continues after the highest id on file.
void testissuer()
{
    string folder = testfolder("offerstore_issuer");
    writefile(folder + "offers.csv", HEADER + "OFF003,DON001,R001,5,5,PENDING,\nOFF007,DON001,R002,5,5,DECLINED,\n");
    offerstore store;
    list<string> warnings;
    store.load(folder, warnings);
    int pos = store.add(EntityId("DON", 1), EntityId("R", 1), 1, 1);
    check(store.at(pos).id().toString() == "OFF008", "after loading OFF007 the next id is OFF008");

    // a missing file starts again at OFF001
    string empty = testfolder("offerstore_issuer2");
    offerstore fresh;
    fresh.load(empty, warnings);
    pos = fresh.add(EntityId("DON", 1), EntityId("R", 1), 1, 1);
    check(fresh.at(pos).id().toString() == "OFF001", "with no file the first id is OFF001");
}

// pendingquantity, wasdeclined and withdrawfordonation.
void testqueries()
{
    string folder = testfolder("offerstore_queries");
    writefile(folder + "offers.csv",
              HEADER + "OFF001,DON001,R001,30,5,PENDING,\n"
                       "OFF002,DON001,R002,20,5,PENDING,\n"
                       "OFF003,DON001,R003,10,5,DECLINED,\n"
                       "OFF004,DON001,R004,15,5,ACCEPTED,T001\n"
                       "OFF005,DON002,R001,7,5,PENDING,\n"
                       "OFF006,DON001,R005,4,5,WITHDRAWN,\n");
    offerstore store;
    list<string> warnings;
    store.load(folder, warnings);

    check(store.pendingquantity(EntityId("DON", 1)) == 50, "pending quantity of DON001 is 50");
    check(store.pendingquantity(EntityId("DON", 2)) == 7, "pending quantity of DON002 is 7");
    check(store.pendingquantity(EntityId("DON", 3)) == 0, "pending quantity of DON003 is 0");

    check(store.wasdeclined(EntityId("DON", 1), EntityId("R", 3)), "R003 declined DON001");
    check(!store.wasdeclined(EntityId("DON", 1), EntityId("R", 1)), "R001 did not decline DON001");
    check(!store.wasdeclined(EntityId("DON", 2), EntityId("R", 3)), "R003 did not decline DON002");
    check(!store.wasdeclined(EntityId("DON", 1), EntityId("R", 5)), "a withdrawn offer is not a decline");

    check(store.withdrawfordonation(EntityId("DON", 1)) == 2, "two offers withdrawn for DON001");
    check(store.at(0).status() == OFFER_WITHDRAWN && store.at(1).status() == OFFER_WITHDRAWN, "the two pending offers are withdrawn");
    check(store.at(3).status() == OFFER_ACCEPTED && store.at(2).status() == OFFER_DECLINED, "other statuses are untouched");
    check(store.at(4).status() == OFFER_PENDING, "the offer of another donation is untouched");
    check(store.pendingquantity(EntityId("DON", 1)) == 0, "no pending quantity left for DON001");
    check(store.withdrawfordonation(EntityId("DON", 1)) == 0, "nothing more to withdraw");
}

// reconcile finds three kinds of problem and nothing on clean data.
void testreconcile()
{
    string folder = testfolder("offerstore_reconcile");
    writefixture(folder);
    writefile(folder + "donations.csv",
              "donation_id,donor_id,food_type,original_quantity,remaining_quantity,usable_minutes,status\n"
              "DON001,DN002,COOKED,80,80,60,PENDING\n"
              "DON002,DN002,COOKED,80,50,9,PARTIAL\n"
              "DON003,DN001,PACKAGED,40,40,30,CLOSED\n");
    writefile(folder + "transactions.csv", transactionstext() + "T001,DON002,R001,30,5\n");

    datamanager files(folder);
    dataset data;
    list<string> warnings;
    files.loadall(data, warnings);

    // clean data
    writefile(folder + "offers.csv", HEADER + "OFF001,DON001,R001,10,5,PENDING,\nOFF002,DON002,R001,30,5,ACCEPTED,T001\nOFF003,DON003,R002,5,5,WITHDRAWN,\n");
    offerstore clean;
    clean.load(folder, warnings);
    check(clean.reconcile(data).empty(), "reconcile finds nothing on clean data");

    // the three kinds of problem
    writefile(folder + "offers.csv",
              HEADER + "OFF001,DON002,R001,30,5,ACCEPTED,T009\n"
                       "OFF002,DON003,R002,5,5,PENDING,\n"
                       "OFF003,DON099,R001,5,5,PENDING,\n"
                       "OFF004,DON001,R099,5,5,PENDING,\n");
    offerstore bad;
    bad.load(folder, warnings);
    vector<string> found = bad.reconcile(data);
    check(found.size() == 4, "reconcile finds four problems");

    string all = "";

    for (size_t pos = 0; pos < found.size(); pos++)
    {
        all = all + found[pos] + "\n";
    }

    check(contains(all, "T009"), "warning for an accepted offer without its transaction");
    check(contains(all, "DON003") && contains(all, "finished"), "warning for a pending offer on a finished donation");
    check(contains(all, "DON099"), "warning for an unknown donation");
    check(contains(all, "R099"), "warning for an unknown recipient");
    check(bad.count() == 4 && bad.at(1).status() == OFFER_PENDING, "reconcile changes nothing");
}

// A save that fails (read-only offers.csv) throws and leaves the store and the file as they were.
void testfailedsave()
{
    string folder = testfolder("offerstore_failedsave");
    offeridissuer::seed(EntityId("OFF", 0));
    offerstore store;
    store.add(EntityId("DON", 1), EntityId("R", 1), 30, 10);
    datamanager files(folder);
    store.save(files);
    string before = readfile(folder + "offers.csv");

    store.add(EntityId("DON", 1), EntityId("R", 2), 5, 10);
    string rowsbefore = rowsof(store);
    setreadonly(folder + "offers.csv", true);
    bool thrown = savethrows(store, files);
    setreadonly(folder + "offers.csv", false);

    check(thrown, "saving to a read-only offers.csv throws DataException");
    check(store.count() == 2 && rowsof(store) == rowsbefore, "the store is unchanged after the failed save");
    check(readfile(folder + "offers.csv") == before, "the file on disk is unchanged after the failed save");

    remove((folder + "offers.csv.tmp").c_str());
    check(!savethrows(store, files), "saving works again once the file is writable");
    check(readfile(folder + "offers.csv") != before, "the new file holds the second offer");
}

// Runs every group of checks.
int main()
{
    testbasics();
    testbadfiles();
    testtmprecovery();
    testissuer();
    testqueries();
    testreconcile();
    testfailedsave();
    return finish();
}
