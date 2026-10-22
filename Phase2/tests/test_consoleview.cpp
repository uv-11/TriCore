#include "Duration.h"
#include "EntityId.h"
#include "Exceptions.h"
#include "Models.h"
#include "allocationmanager.h"
#include "consoleview.h"
#include "csv_fixture.h"
#include "datamanager.h"
#include "intqueue.h"
#include "scripted.h"
#include "traveltime.h"
#include <iostream>
#include <list>
#include <sstream>
#include <string>

using namespace std;

int failcount = 0;

// All files of these tests start with this prefix, inside the tests/fixtures folder.
const string PREFIX = "Phase2/tests/fixtures/cv_";

// The header row of donations.csv.
const string DONATIONS_HEADER =
    "donation_id,donor_id,food_type,original_quantity,remaining_quantity,usable_minutes,"
    "status\n";

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

// Checks whether text contains needle.
bool contains(const string& text, const string& needle)
{
    return text.find(needle) != string::npos;
}

// Counts how many times needle appears in text.
int occurrences(const string& text, const string& needle)
{
    int total = 0;
    size_t spot = text.find(needle);

    while (spot != string::npos)
    {
        total++;
        spot = text.find(needle, spot + needle.length());
    }

    return total;
}

// Returns the part of the screen text between the Ranked and the Rejected headings of the
// first evaluation.
string rankedpart(const string& shown)
{
    size_t start = shown.find("Ranked candidates");
    size_t stop = shown.find("Rejected recipients");
    return shown.substr(start, stop - start);
}

// Returns the part of the first evaluation after the Rejected heading, up to the next blank
// line.
string rejectedpart(const string& shown)
{
    size_t start = shown.find("Rejected recipients");
    size_t stop = shown.find("\n\n", start);
    return shown.substr(start, stop - start);
}

// Returns the line of the rejected part that starts with the given recipient id.
string rejectedline(const string& shown, const string& id)
{
    string part = rejectedpart(shown);
    size_t start = part.find("  " + id);

    if (start == string::npos)
    {
        return "";
    }

    size_t stop = part.find("\n", start);
    return part.substr(start, stop - start);
}

// Checks that the recipient ids appear in the ranked part in the given order.
bool ranksin(const string& shown, const string& first, const string& second, const string& third)
{
    string part = rankedpart(shown);
    size_t one = part.find(first);
    size_t two = part.find(second);

    if (one == string::npos || two == string::npos || one > two)
    {
        return false;
    }

    if (third == "")
    {
        return true;
    }

    size_t three = part.find(third);
    return three != string::npos && two < three;
}

// Writes the standard fixture with the given donation rows, loads it and processes the
// first donation with the scripted answers. Returns the dataset after processing.
dataset runone(const string& rows, const string& script, string& shown, bool& finished)
{
    removefixture(PREFIX);
    writefixture(PREFIX);
    writefile(PREFIX + "donations.csv", DONATIONS_HEADER + rows);

    datamanager files(PREFIX);
    dataset data;
    list<string> warnings;
    files.loadall(data, warnings);
    transactionidissuer::seed(data.lasttransaction);

    dijkstratraveltime provider(data.network);
    istringstream in(script);
    ostringstream out;
    consoleview view(in, out);
    finished = view.processdonation(data, files, provider, 0);
    shown = out.str();
    return data;
}

// Loads the files again, as a restart would.
dataset reload()
{
    datamanager files(PREFIX);
    dataset data;
    list<string> warnings;
    files.loadall(data, warnings);
    return data;
}

// Compares the saved files with the state in memory.
bool filesmatch(const dataset& memory)
{
    dataset saved = reload();

    if (saved.donations.size() != memory.donations.size() ||
        saved.recipients.size() != memory.recipients.size() ||
        saved.transactions.size() != memory.transactions.size())
    {
        return false;
    }

    for (size_t pos = 0; pos < memory.donations.size(); pos++)
    {
        if (saved.donations[pos].toCsvRow() != memory.donations[pos].toCsvRow())
        {
            return false;
        }
    }

    for (size_t pos = 0; pos < memory.recipients.size(); pos++)
    {
        if (saved.recipients[pos].toCsvRow() != memory.recipients[pos].toCsvRow())
        {
            return false;
        }
    }

    for (size_t pos = 0; pos < memory.transactions.size(); pos++)
    {
        if (saved.transactions[pos].toCsvRow() != memory.transactions[pos].toCsvRow())
        {
            return false;
        }
    }

    return true;
}

// Adds up the quantities of all transactions.
int totalplaced(const dataset& data)
{
    int total = 0;

    for (size_t pos = 0; pos < data.transactions.size(); pos++)
    {
        total = total + data.transactions[pos].quantity();
    }

    return total;
}

// Adds up the leftover of all closed donations.
int totalwasted(const dataset& data)
{
    int total = 0;

    for (size_t pos = 0; pos < data.donations.size(); pos++)
    {
        if (data.donations[pos].status() == CLOSED)
        {
            total = total + data.donations[pos].remainingQuantity();
        }
    }

    return total;
}

// Main test runner. WHY: Executes the console tests and the scenarios S1 to S8.
int main()
{
    cout << "--- test_consoleview ---\n";

    string shown;
    bool finished = false;

    // coordinatorchoice: what is accepted and what is not
    string lines[8] = {"1 40", "c", "C", "abc", "1", "1 2 3", "0 -5", "  2   15  "};
    bool valids[8] = {true, true, true, false, false, false, false, true};

    for (int pos = 0; pos < 8; pos++)
    {
        istringstream input(lines[pos] + "\n");
        coordinatorchoice choice;
        input >> choice;

        if (choice.valid() != valids[pos])
        {
            check(false, "choice: the line [" + lines[pos] + "] is read wrongly");
        }
    }

    istringstream pair("2   15\n");
    coordinatorchoice parsed;
    pair >> parsed;
    check(parsed.valid() && !parsed.isclose() && parsed.candidatenumber() == 2 &&
              parsed.quantity() == 15,
          "choice: 2   15 gives candidate 2 and quantity 15");

    istringstream closing("c\n");
    coordinatorchoice closechoice;
    closing >> closechoice;
    check(closechoice.valid() && closechoice.isclose(), "choice: c means close");

    istringstream ended("");
    coordinatorchoice nothing;
    check(!(ended >> nothing), "choice: the stream fails when the input has ended");

    // S1: DN001, COOKED 100, usable 20. The coordinator takes candidate 1 for 80.
    dataset s1 = runone("DON101,DN001,COOKED,100,100,20,PENDING\n", "1 80\n", shown, finished);

    check(finished, "S1: the donation was processed to the end");
    check(contains(rankedpart(shown), "R004") && !contains(rankedpart(shown), "R001") &&
              contains(rankedpart(shown), "0 -> 8"),
          "S1: only R004 is ranked and its route is 0 -> 8");
    check(contains(shown, "TOO_FAR (25 min)") && contains(shown, "TOO_FAR (22 min)") &&
              occurrences(rejectedpart(shown), "FOOD_INCOMPATIBLE") == 2 &&
              contains(shown, "UNREACHABLE") && contains(shown, "NOT_ACCEPTING"),
          "S1: the rejection reasons are shown");
    check(s1.transactions.size() == 1 && s1.transactions[0].toCsvRow() == "T001,DON101,R004,80,12",
          "S1: transaction T001,DON101,R004,80,12");
    check(s1.donations[0].status() == CLOSED && s1.donations[0].remainingQuantity() == 20 &&
              contains(shown, "recorded as wasted"),
          "S1: nobody is left, so the leftover 20 is closed as wasted");
    check(filesmatch(s1), "S1: the saved files match the state in memory");

    // S2: the boundary. Usable 12 keeps R004, usable 11 leaves no candidate.
    dataset s2a = runone("DON201,DN001,COOKED,50,50,12,PENDING\n", "1 50\n", shown, finished);
    check(s2a.donations[0].status() == COMPLETED && s2a.transactions.size() == 1,
          "S2: usable 12 allows R004 and completes the donation");

    dataset s2b = runone("DON202,DN001,COOKED,50,50,11,PENDING\n", "", shown, finished);
    check(finished && s2b.donations[0].status() == CLOSED &&
              s2b.donations[0].remainingQuantity() == 50 && s2b.transactions.empty(),
          "S2: usable 11 has no candidate, so it closes with 50 wasted without any input");
    check(contains(shown, "none") && contains(shown, "recorded as wasted"),
          "S2: the screen says there is no candidate");

    // S3: partial allocation in two steps and the id after a restart
    dataset s3 = runone("DON301,DN001,PACKAGED,400,400,30,PENDING\n", "1 300\n1 100\n", shown,
                        finished);

    check(finished && s3.donations[0].status() == COMPLETED, "S3: the donation is COMPLETED");
    check(s3.transactions.size() == 2 && s3.transactions[0].toCsvRow() == "T001,DON301,R002,300,15" &&
              s3.transactions[1].toCsvRow() == "T002,DON301,R001,100,25",
          "S3: transactions T001 (R002, 300, 15) and T002 (R001, 100, 25)");
    check(s3.recipients[1].availableCapacity() == 0 && s3.recipients[0].availableCapacity() == 50,
          "S3: R002 capacity 0 and R001 capacity 50");
    check(contains(shown, "NO_CAPACITY"), "S3: after the first step R002 is shown as NO_CAPACITY");
    check(filesmatch(s3), "S3: the saved files match the state in memory");

    dataset restarted = reload();
    transactionidissuer::seed(restarted.lasttransaction);
    check(transactionidissuer::issuenext().toString() == "T003",
          "S3: after a restart the next id is T003");

    // S4, S5, S6: the ranking order. The coordinator closes the donation with c.
    runone("DON401,DN001,COOKED,60,60,40,PENDING\n", "c\n", shown, finished);
    check(ranksin(shown, "R004", "R003", "R001"), "S4: R004, R003, R001 in that order");

    runone("DON501,DN001,COOKED,100,100,30,PENDING\n", "c\n", shown, finished);
    check(ranksin(shown, "R003", "R001", "R004"), "S5: R003, R001, R004 in that order");

    dataset s6 = runone("DON601,DN001,PACKAGED,100,100,30,PENDING\n", "c\n", shown, finished);
    check(ranksin(shown, "R002", "R001", "") && contains(rankedpart(shown), "200") &&
              contains(rankedpart(shown), "0 -> 4 -> 6"),
          "S6: R002 then R001, with unused capacity 200 and route 0 -> 4 -> 6");
    check(s6.donations[0].status() == CLOSED && s6.donations[0].remainingQuantity() == 100 &&
              s6.transactions.empty() && contains(shown, "The donation is closed"),
          "S6: c closes the donation and nothing is allocated without a choice");

    // S7: every non-candidate of S3 has exactly one correct reason
    runone("DON301,DN001,PACKAGED,400,400,30,PENDING\n", "c\n", shown, finished);
    string reasons[5] = {"FOOD_INCOMPATIBLE", "FOOD_INCOMPATIBLE", "UNREACHABLE", "NO_PICKUP",
                         "NOT_ACCEPTING"};
    string ids[5] = {"R003", "R004", "R005", "R006", "R007"};
    bool onereason = true;
    string allcodes[7] = {"NOT_ACCEPTING", "FOOD_INCOMPATIBLE", "NO_CAPACITY", "NO_PICKUP",
                          "UNREACHABLE", "TOO_FAR", "OK "};

    for (int pos = 0; pos < 5; pos++)
    {
        string line = rejectedline(shown, ids[pos]);
        int codes = 0;

        for (int code = 0; code < 6; code++)
        {
            codes = codes + occurrences(line, allcodes[code]);
        }

        if (!contains(line, reasons[pos]) || codes != 1)
        {
            onereason = false;
        }
    }

    check(onereason, "S7: R003 to R007 each show exactly one correct reason");

    // S8: shared capacity. Arrival order and urgency order through the real console flow.
    removefixture(PREFIX);
    writefixture(PREFIX);
    datamanager files(PREFIX);
    dataset arrival;
    list<string> warnings;
    files.loadall(arrival, warnings);
    transactionidissuer::seed(arrival.lasttransaction);
    dijkstratraveltime provider(arrival.network);
    dataset urgency = arrival;

    istringstream arrivalin("1 80\n1 40\n");
    ostringstream arrivalout;
    consoleview arrivalview(arrivalin, arrivalout);
    intqueue order;
    order.enqueue(0);
    order.enqueue(1);

    while (!order.empty())
    {
        arrivalview.processdonation(arrival, files, provider, order.dequeue());
    }

    check(totalplaced(arrival) == 120 && totalwasted(arrival) == 40,
          "S8: arrival order places 120 and wastes 40");

    removefixture(PREFIX);
    writefixture(PREFIX);
    istringstream urgencyin("1 80\n1 80\n");
    ostringstream urgencyout;
    consoleview urgencyview(urgencyin, urgencyout);
    urgencyqueue waiting;
    fillurgencyqueue(urgency.donations, waiting);

    while (!waiting.empty())
    {
        urgencyview.processdonation(urgency, files, provider, waiting.popmosturgent());
    }

    check(totalplaced(urgency) == 160 && totalwasted(urgency) == 0,
          "S8: urgency order places 160 and wastes 0");
    check(urgency.transactions[0].donationId().toString() == "DON002" &&
              urgency.transactions[1].recipientId().toString() == "R004",
          "S8: DON002 goes first to R003 and then DON001 goes to R004");

    // The scripted coordinator (used by E3) gives the same totals
    removefixture(PREFIX);
    writefixture(PREFIX);
    dataset scriptedarrival = reload();
    dataset scriptedurgency = reload();
    check(runscripted(scriptedarrival, false) == 120 && runscripted(scriptedurgency, true) == 160,
          "S8: the scripted coordinator also gives 120 and 160");

    // Input mistakes are answered with a message and a new prompt
    dataset mistakes = runone("DON201,DN001,COOKED,50,50,12,PENDING\n",
                              "abc\n9 10\n1 999\n0 5\n1 50\n", shown, finished);
    check(contains(shown, "Please type two whole numbers") &&
              contains(shown, "There is no candidate with that number") &&
              contains(shown, "The quantity must be between 1 and 50"),
          "input: each wrong answer gets its own message");
    check(finished && mistakes.donations[0].status() == COMPLETED,
          "input: after the mistakes a valid answer completes the donation");

    // The end of the input leaves the donation open and changes nothing
    dataset cut = runone("DON201,DN001,COOKED,50,50,12,PENDING\n", "", shown, finished);
    check(!finished && cut.donations[0].status() == PENDING && cut.transactions.empty() &&
              contains(shown, "The input ended"),
          "input: the end of the input leaves the donation open");
    check(filesmatch(cut), "input: no file was changed when the input ended");

    // Menu, about and the tables
    removefixture(PREFIX);
    writefixture(PREFIX);
    dataset tables = reload();

    istringstream menuin("abc\n7\n3\n");
    ostringstream menuout;
    consoleview menuview(menuin, menuout);
    check(menuview.readmenu() == 3 && occurrences(menuout.str(), "Please type a number from 0 to 6") == 2,
          "menu: two wrong answers are refused, then 3 is read");

    istringstream endin("");
    ostringstream endout;
    consoleview endview(endin, endout);
    check(endview.readmenu() == -1, "menu: the end of the input returns -1");

    istringstream noin("");
    ostringstream aboutout;
    consoleview aboutview(noin, aboutout);
    aboutview.showabout();
    aboutview.showmenu();
    aboutview.showdonations(tables);
    aboutview.showrecipients(tables);
    aboutview.showtransactions(tables);
    string screen = aboutout.str();

    check(contains(screen, "location-aware travel-time feasibility") && contains(screen, "SIMULATED"),
          "about: names the feature and says the edge weights are simulated");
    check(contains(screen, "Hope Shelter") && contains(screen, "DON002") &&
              contains(screen, "No transactions yet"),
          "tables: recipients, donations and the empty transaction log are shown");

    list<string> some;
    some.push_back("R005 is far away");
    ostringstream warnout;
    consoleview warnview(noin, warnout);
    warnview.showwarnings(some);
    check(contains(warnout.str(), "R005 is far away"), "warnings: each warning is printed");

    // Adding a donation: wrong answers get a message, the right ones add DON003
    istringstream addin("DN009\nDN1x\nDN001\nPIZZA\nCOOKED\n0\nabc\n50\n0\n25\n");
    ostringstream addout;
    consoleview addview(addin, addout);
    int added = addview.adddonation(tables, files);

    check(added == 2 && tables.donations[2].toCsvRow() == "DON003,DN001,COOKED,50,50,25,PENDING",
          "add: the new donation is DON003 with 50 portions and 25 minutes");
    check(contains(addout.str(), "There is no donor with that id") &&
              contains(addout.str(), "looks like DN001") &&
              contains(addout.str(), "not one of the four food types") &&
              occurrences(addout.str(), "Please type a whole number of at least 1") == 3,
          "add: each wrong answer gets its message");
    check(reload().donations.size() == 3, "add: the new donation is saved");

    istringstream cancelin("q\n");
    ostringstream cancelout;
    consoleview cancelview(cancelin, cancelout);
    check(cancelview.adddonation(tables, files) == -1 && tables.donations.size() == 3,
          "add: q cancels and nothing is added");

    removefixture(PREFIX);

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}