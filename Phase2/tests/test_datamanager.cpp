#include "EntityId.h"
#include "Exceptions.h"
#include "Models.h"
#include "allocationmanager.h"
#include "csv_fixture.h"
#include "datamanager.h"
#include <iostream>
#include <list>
#include <string>
#include <vector>

using namespace std;

int failcount = 0;

// All files of these tests start with this prefix, inside the tests/fixtures folder.
const string PREFIX = "Phase2/tests/fixtures/dm_";

// Checks a condition and prints PASS or FAIL. WHY: Helper for tests.
void check(bool condition, const string& name)
{
    if (condition)
    {
        cout << "PASS " << name << "\n";
    }
    else
    {
        cout << "FAIL " << name << "\n";
        failcount++;
    }
}

// Starts every test with a fresh copy of the standard fixture files.
void fresh()
{
    removefixture(PREFIX);
    writefixture(PREFIX);
}

// Checks whether text contains needle.
bool contains(const string& text, const string& needle)
{
    return text.find(needle) != string::npos;
}

// Replaces the first occurrence of from in text.
string replaceonce(const string& text, const string& from, const string& to)
{
    size_t spot = text.find(from);

    if (spot == string::npos)
    {
        return text;
    }

    return text.substr(0, spot) + to + text.substr(spot + from.length());
}

// Turns every line ending of the text into CR LF.
string crlf(const string& text)
{
    string changed = "";

    for (size_t pos = 0; pos < text.length(); pos++)
    {
        if (text[pos] == '\n')
        {
            changed = changed + "\r\n";
        }
        else
        {
            changed = changed + text[pos];
        }
    }

    return changed;
}

// Loads the fixture and returns the DataException message, or "NO ERROR" if it loaded.
string loaderror()
{
    datamanager manager(PREFIX);
    dataset data;
    list<string> warnings;

    try
    {
        manager.loadall(data, warnings);
    }
    catch (const DataException& problem)
    {
        return problem.what();
    }

    return "NO ERROR";
}

// Checks whether a list of warnings has a line that contains needle.
bool haswarning(const list<string>& warnings, const string& needle)
{
    for (list<string>::const_iterator spot = warnings.begin(); spot != warnings.end(); spot++)
    {
        if (contains(*spot, needle))
        {
            return true;
        }
    }

    return false;
}

// Compares the printed rows of two datasets table by table.
bool samerows(const dataset& first, const dataset& second)
{
    if (first.donors.size() != second.donors.size() ||
        first.recipients.size() != second.recipients.size() ||
        first.donations.size() != second.donations.size() ||
        first.transactions.size() != second.transactions.size())
    {
        return false;
    }

    for (size_t pos = 0; pos < first.donors.size(); pos++)
    {
        if (first.donors[pos].toCsvRow() != second.donors[pos].toCsvRow())
        {
            return false;
        }
    }

    for (size_t pos = 0; pos < first.recipients.size(); pos++)
    {
        if (first.recipients[pos].toCsvRow() != second.recipients[pos].toCsvRow())
        {
            return false;
        }
    }

    for (size_t pos = 0; pos < first.donations.size(); pos++)
    {
        if (first.donations[pos].toCsvRow() != second.donations[pos].toCsvRow())
        {
            return false;
        }
    }

    for (size_t pos = 0; pos < first.transactions.size(); pos++)
    {
        if (first.transactions[pos].toCsvRow() != second.transactions[pos].toCsvRow())
        {
            return false;
        }
    }

    return true;
}

// Counts the edges in the list of a node.
int countedges(const graph& network, int node)
{
    int total = 0;

    for (const edgenode* edge = network.neighbors(node); edge != NULL; edge = edge->next)
    {
        total++;
    }

    return total;
}

// Main test runner. WHY: Executes all datamanager unit tests.
int main()
{
    cout << "--- test_datamanager ---\n";

    datamanager manager(PREFIX);

    // Load the standard fixture
    fresh();
    dataset data;
    list<string> warnings;
    manager.loadall(data, warnings);

    check(data.donors.size() == 2 && data.recipients.size() == 7 &&
              data.donations.size() == 2 && data.transactions.empty(),
          "load: 2 donors, 7 recipients, 2 donations, no transactions");
    check(data.network.nodecount() == 11 && countedges(data.network, 0) == 4,
          "load: the graph has 11 nodes and node 0 has 4 roads");
    check(data.nodelabels.size() == 11 && data.nodexs[8] == -3 && data.nodeys[9] == 6,
          "load: node labels and coordinates are stored");
    check(data.lasttransaction.toString() == "T000", "load: with no transactions the last id is T000");

    int where = -1;
    check(data.recipientindex.get("R004", where) && where == 3 &&
              data.donorindex.get("DN002", where) && where == 1 &&
              data.donationindex.get("DON002", where) && where == 1,
          "load: the hash indexes hold the positions");

    // CN1 warning of Spec 6.6
    check(haswarning(warnings, "R005 (node 9) is not connected to any donor; it can never "
                               "be recommended."),
          "CN1: the warning names R005");
    check(warnings.size() == 1, "load: no other warning for the standard fixture");

    // A copied dataset is independent
    dataset copy = data;
    copy.recipients[0].reduceCapacity(10);
    copy.network.addedge(0, 1, 3);

    check(data.recipients[0].availableCapacity() == 150 &&
              copy.recipients[0].availableCapacity() == 140,
          "copy: changing the copy leaves the original recipient unchanged");
    check(countedges(data.network, 0) == 4 && countedges(copy.network, 0) == 5,
          "copy: changing the copy leaves the original graph unchanged");

    int found = -1;
    copy.donorindex.put("DN009", 9);
    check(!data.donorindex.get("DN009", found), "copy: the original index does not see the new key");

    // P1: save then load gives the same state
    fresh();
    dataset first;
    warnings.clear();
    manager.loadall(first, warnings);

    first.donations[0].allocateQuantity(80);
    first.recipients[2].reduceCapacity(80);
    Transaction entry(EntityId("T", 1), first.donations[0].id(), first.recipients[2].id(), 80, 9);
    first.transactions.push_back(entry);
    manager.commitallocation(first, entry);

    dataset second;
    warnings.clear();
    manager.loadall(second, warnings);

    check(samerows(first, second), "P1: a saved state loads back with identical rows");
    check(second.lasttransaction.toString() == "T001", "P1: the last transaction id is T001");
    check(second.donations[0].status() == COMPLETED && second.recipients[2].availableCapacity() == 40,
          "P1: donation completed and capacity 40 after the reload");
    check(!exists(PREFIX + "donations.csv.tmp") && !exists(PREFIX + "recipients.csv.tmp"),
          "P1: no .tmp file is left after a commit");

    // Closing a donation saves donations.csv only
    first.donations[1].close();
    manager.commitdonations(first);
    dataset third;
    warnings.clear();
    manager.loadall(third, warnings);
    check(third.donations[1].status() == CLOSED && third.donations[1].remainingQuantity() == 80,
          "P1: a closed donation keeps its leftover after the reload");

    // P2: tampered quantities are reported as I3
    writefile(PREFIX + "donations.csv",
              "donation_id,donor_id,food_type,original_quantity,remaining_quantity,"
              "usable_minutes,status\n"
              "DON001,DN002,COOKED,80,70,60,PARTIAL\n"
              "DON002,DN002,COOKED,80,80,9,PENDING\n");
    string tampered = loaderror();
    check(contains(tampered, "Invariant I3"), "P2: tampered quantities are reported as I3");
    check(contains(tampered, "DON001"), "P2: the message names the donation");

    // P3: the next transaction id continues after a restart
    fresh();
    writefile(PREFIX + "donations.csv",
              "donation_id,donor_id,food_type,original_quantity,remaining_quantity,"
              "usable_minutes,status\n"
              "DON301,DN001,PACKAGED,400,0,30,COMPLETED\n");
    writefile(PREFIX + "recipients.csv",
              replaceonce(replaceonce(recipientstext(), "R002,City Food Bank,FOOD_BANK,300",
                                      "R002,City Food Bank,FOOD_BANK,0"),
                          "R001,Hope Shelter,SHELTER,150", "R001,Hope Shelter,SHELTER,50"));
    writefile(PREFIX + "transactions.csv",
              transactionstext() + "T001,DON301,R002,300,15\nT002,DON301,R001,100,25\n");
    dataset restarted;
    warnings.clear();
    manager.loadall(restarted, warnings);
    transactionidissuer::seed(restarted.lasttransaction);

    check(restarted.transactions.size() == 2 && restarted.lasttransaction.toString() == "T002",
          "P3: the last id read from transactions.csv is T002");
    check(transactionidissuer::issuenext().toString() == "T003",
          "P3: the next id after the restart is T003, no id is reused");

    // P4: CRLF line endings and a UTF-8 byte order mark
    fresh();
    string bom = "";
    bom = bom + (char)0xEF + (char)0xBB + (char)0xBF;
    writefile(PREFIX + "donors.csv", bom + crlf(donorstext()));
    writefile(PREFIX + "donations.csv", crlf(donationstext()));
    writefile(PREFIX + "recipients.csv", crlf(recipientstext()));
    dataset windows;
    warnings.clear();
    manager.loadall(windows, warnings);

    check(windows.donors.size() == 2 && windows.donors[0].toCsvRow() == "DN001,City Banquet Hall,CATERER,0",
          "P4: CRLF and BOM are parsed correctly");
    check(windows.donations.size() == 2 && windows.recipients.size() == 7,
          "P4: CRLF files give the same row counts");

    // P5 and EX1: a bad field gives a DataException with the file and the line
    fresh();
    writefile(PREFIX + "donors.csv",
              replaceonce(donorstext(), "HOTEL,1", "HOTEL,notanumber"));
    string badfield = loaderror();

    check(contains(badfield, "donors.csv line 3"), "P5: the message has the file name and line 3");
    check(contains(badfield, "notanumber"), "P5: the message has the bad text");
    check(contains(badfield, "Not a whole number"),
          "EX1: the FormatException reason is carried into the DataException");

    bool wrongtype = false;

    try
    {
        datamanager again(PREFIX);
        dataset unused;
        list<string> ignored;
        again.loadall(unused, ignored);
    }
    catch (const FormatException&)
    {
        wrongtype = true;
    }
    catch (const DataException&)
    {
    }

    check(!wrongtype, "EX1: the loader rethrows a DataException, not the FormatException");

    // EX2: catching by the base reference catches all three kinds
    int caught = 0;

    try
    {
        throw FormatException("a");
    }
    catch (const AppException&)
    {
        caught++;
    }

    try
    {
        throw DataException("b");
    }
    catch (const AppException&)
    {
        caught++;
    }

    try
    {
        throw StructureException("c");
    }
    catch (const AppException&)
    {
        caught++;
    }

    check(caught == 3, "EX2: catch by AppException& catches all three kinds");

    // P6: a stale .tmp file next to the real file is ignored and deleted
    fresh();
    writefile(PREFIX + "donations.csv.tmp", "garbage");
    dataset stale;
    warnings.clear();
    manager.loadall(stale, warnings);

    check(stale.donations.size() == 2, "P6: the real file is used");
    check(!exists(PREFIX + "donations.csv.tmp"), "P6: the stale .tmp file is deleted");
    check(haswarning(warnings, "stale"), "P6: a warning reports the stale file");

    // P7: safe replace when the target exists
    fresh();
    vector<const CsvRecord*> records;
    Donation extra(EntityId("DON", 9), EntityId("DN", 1), BAKERY, 10, 10, 20, PENDING);
    records.push_back(&extra);
    manager.savetable("donations.csv", "donation_id,donor_id,food_type,original_quantity,"
                                       "remaining_quantity,usable_minutes,status",
                      records);

    check(readfile(PREFIX + "donations.csv") ==
              "donation_id,donor_id,food_type,original_quantity,remaining_quantity,"
              "usable_minutes,status\nDON009,DN001,BAKERY,10,10,20,PENDING\n",
          "P7: the new content is in place");
    check(!exists(PREFIX + "donations.csv.tmp"), "P7: no .tmp file is left");

    // P8: the file is missing and the .tmp file is present, so it is recovered at startup
    fresh();
    writefile(PREFIX + "donations.csv.tmp", donationstext());
    remove((PREFIX + "donations.csv").c_str());
    dataset recovered;
    warnings.clear();
    manager.loadall(recovered, warnings);

    check(recovered.donations.size() == 2, "P8: the donations were recovered from the .tmp file");
    check(!exists(PREFIX + "donations.csv.tmp") && exists(PREFIX + "donations.csv"),
          "P8: the .tmp file was renamed into place");
    check(haswarning(warnings, "recovered"), "P8: a warning reports the recovery");

    // appendtransaction adds a row, and creates the file with its header when it is missing
    fresh();
    remove((PREFIX + "transactions.csv").c_str());
    manager.appendtransaction(entry);
    manager.appendtransaction(Transaction(EntityId("T", 2), EntityId("DON", 1), EntityId("R", 3), 5, 9));

    check(readfile(PREFIX + "transactions.csv") ==
              transactionstext() + "T001,DON001,R003,80,9\nT002,DON001,R003,5,9\n",
          "append: the header is created and both rows are added");

    // append: a file whose last line has no line break does not get its rows joined
    fresh();
    writefile(PREFIX + "transactions.csv", "transaction_id,donation_id,recipient_id,quantity,travel_minutes");
    manager.appendtransaction(entry);

    check(readfile(PREFIX + "transactions.csv") == transactionstext() + "T001,DON001,R003,80,9\n",
          "append: a missing final line break is added before the new row");

    // V1 to V8: each rule reports file, line and rule number
    fresh();
    writefile(PREFIX + "donors.csv", donorstext() + "DN002,Copy,HOTEL,1\n");
    check(contains(loaderror(), "donors.csv line 4") && contains(loaderror(), "(V1)"),
          "V1: a duplicate donor id is reported");

    fresh();
    writefile(PREFIX + "recipients.csv", replaceonce(recipientstext(), "R001,", "R1,"));
    check(contains(loaderror(), "at least three digits"), "V1: R1 is rejected, an id needs three digits");

    fresh();
    writefile(PREFIX + "donations.csv", replaceonce(donationstext(), "DN002", "DN009"));
    check(contains(loaderror(), "(V2)") && contains(loaderror(), "DN009"),
          "V2: a donation with an unknown donor is reported");

    fresh();
    writefile(PREFIX + "recipients.csv", replaceonce(recipientstext(), ",1,1,5", ",1,1,99"));
    check(contains(loaderror(), "recipients.csv line 2") && contains(loaderror(), "(V2)"),
          "V2: a recipient on a node outside the graph is reported");

    fresh();
    writefile(PREFIX + "donations.csv", replaceonce(donationstext(), "80,80,60", "80,90,60"));
    check(contains(loaderror(), "(V3)"), "V3: remaining above original is reported");

    fresh();
    writefile(PREFIX + "donations.csv", replaceonce(donationstext(), "80,80,60", "0,0,60"));
    check(contains(loaderror(), "(V3)"), "V3: original quantity 0 is reported");

    fresh();
    writefile(PREFIX + "donations.csv", replaceonce(donationstext(), "80,80,60", "80,80,0"));
    check(contains(loaderror(), "(V4)"), "V4: usable minutes 0 is reported");

    fresh();
    writefile(PREFIX + "recipients.csv", replaceonce(recipientstext(), "SHELTER,150", "SHELTER,-5"));
    check(contains(loaderror(), "(V4)"), "V4: a negative capacity is reported");

    fresh();
    writefile(PREFIX + "donations.csv", replaceonce(donationstext(), "COOKED,80,80,60", "PIZZA,80,80,60"));
    check(contains(loaderror(), "Invalid FoodType") && contains(loaderror(), "donations.csv line 2"),
          "V5: an unknown food type is reported");

    fresh();
    writefile(PREFIX + "recipients.csv", replaceonce(recipientstext(), "PACKAGED,1,1,5", "PACKAGED,2,1,5"));
    check(contains(loaderror(), "Expected 0 or 1"), "V5: a flag that is not 0 or 1 is reported");

    fresh();
    writefile(PREFIX + "recipients.csv", replaceonce(recipientstext(), "COOKED;PACKAGED", "COOKED;SOUP"));
    check(contains(loaderror(), "Invalid FoodType: SOUP"), "V5: an unknown accepted type is reported");

    fresh();
    writefile(PREFIX + "donations.csv", replaceonce(donationstext(), "80,80,60,PENDING", "80,70,60,PENDING"));
    check(contains(loaderror(), "(V6)"), "V6: a status that does not match the quantities is reported");

    fresh();
    writefile(PREFIX + "graph_nodes.csv", replaceonce(nodestext(), "\n1,DN002", "\n2,DN002"));
    check(contains(loaderror(), "(V7)"), "V7: node ids that are not 0, 1, 2 ... are reported");

    fresh();
    writefile(PREFIX + "graph_edges.csv", edgestext() + "3,3,5\n");
    check(contains(loaderror(), "(V7)") && contains(loaderror(), "itself"), "V7: a self-loop is reported");

    fresh();
    writefile(PREFIX + "graph_edges.csv", edgestext() + "0,99,5\n");
    check(contains(loaderror(), "(V7)"), "V7: an edge to a missing node is reported");

    fresh();
    writefile(PREFIX + "graph_edges.csv", edgestext() + "0,1,0\n");
    check(contains(loaderror(), "(V7)") && contains(loaderror(), "at least 1 minute"),
          "V7: an edge with 0 minutes is reported");

    fresh();
    writefile(PREFIX + "graph_edges.csv", edgestext() + "2,0,5\n");
    dataset duplicated;
    warnings.clear();
    manager.loadall(duplicated, warnings);
    bool keptsmaller = false;

    for (const edgenode* edge = duplicated.network.neighbors(0); edge != NULL; edge = edge->next)
    {
        if (edge->to == 2 && edge->minutes == 5)
        {
            keptsmaller = true;
        }
    }

    check(keptsmaller && countedges(duplicated.network, 0) == 4,
          "V7: a duplicate edge keeps the smaller minutes (5, not 8)");
    check(haswarning(warnings, "duplicate edge 0-2"), "V7: a duplicate edge adds a warning");

    fresh();
    writefile(PREFIX + "transactions.csv", transactionstext() + "T001,DON777,R001,5,9\n");
    check(contains(loaderror(), "(V8)"), "V8: a transaction with an unknown donation is reported");

    // Boundary cases: empty, header-only and missing files
    fresh();
    writefile(PREFIX + "donations.csv", "");
    check(contains(loaderror(), "the file is empty"), "boundary: an empty file is reported");

    fresh();
    writefile(PREFIX + "donations.csv", "donation_id,donor_id,food_type,original_quantity,"
                                        "remaining_quantity,usable_minutes,status\n");
    dataset headeronly;
    warnings.clear();
    manager.loadall(headeronly, warnings);
    check(headeronly.donations.empty(), "boundary: a header-only file loads with no rows");

    fresh();
    remove((PREFIX + "donors.csv").c_str());
    check(contains(loaderror(), "Cannot open file"), "boundary: a missing file is reported");

    fresh();
    writefile(PREFIX + "donors.csv", replaceonce(donorstext(), "name", "wrong"));
    check(contains(loaderror(), "unexpected header"), "boundary: a wrong header is reported");

    fresh();
    writefile(PREFIX + "donors.csv", donorstext() + "DN003,Short,CATERER\n");
    check(contains(loaderror(), "expected 4 fields but found 3"),
          "boundary: a row with a missing field is reported");

    // checkinvariants on a dataset in memory
    fresh();
    dataset memory;
    warnings.clear();
    manager.loadall(memory, warnings);
    check(manager.checkinvariants(memory).empty(), "invariants: a clean dataset has no problems");

    dataset broken = memory;
    broken.transactions.push_back(Transaction(EntityId("T", 2), EntityId("DON", 1), EntityId("R", 3), 0, 9));
    broken.transactions.push_back(Transaction(EntityId("T", 1), EntityId("DON", 1), EntityId("R", 3), 0, 9));
    vector<string> sixth = manager.checkinvariants(broken);
    check(sixth.size() == 1 && contains(sixth[0], "I6"), "invariants: ids that do not increase are I6");

    dataset orphan = memory;
    orphan.transactions.push_back(Transaction(EntityId("T", 1), EntityId("DON", 99), EntityId("R", 3), 0, 9));
    vector<string> fifth = manager.checkinvariants(orphan);
    check(!fifth.empty() && contains(fifth[0], "I5"), "invariants: an unknown donation is I5");

    dataset spent = memory;
    spent.donations[0].allocateQuantity(10);
    vector<string> third2 = manager.checkinvariants(spent);
    check(third2.size() == 1 && contains(third2[0], "I3"),
          "invariants: quantity gone without a transaction is I3");

    removefixture(PREFIX);

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}