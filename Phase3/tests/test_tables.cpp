// test_tables: the text of every list the screens show.

#include "Datamanager.h"
#include "Exceptions.h"
#include "OfferStore.h"
#include "Tables.h"
#include "testkit.h"

#include <list>
#include <string>
#include <vector>

using namespace std;

// Joins the cells of a row with "|" so a whole row can be compared at once.
string rowtext(const texttable& table, size_t row)
{
    string text = "";

    if (row >= table.rows.size())
    {
        return "NO SUCH ROW";
    }

    for (size_t cell = 0; cell < table.rows[row].size(); cell++)
    {
        if (cell > 0)
        {
            text = text + "|";
        }

        text = text + table.rows[row][cell];
    }

    return text;
}

// Joins the headers with "|".
string headertext(const texttable& table)
{
    string text = "";

    for (size_t cell = 0; cell < table.headers.size(); cell++)
    {
        if (cell > 0)
        {
            text = text + "|";
        }

        text = text + table.headers[cell];
    }

    return text;
}

// Checks that every row has exactly one cell for each header.
bool rectangular(const texttable& table)
{
    for (size_t row = 0; row < table.rows.size(); row++)
    {
        if (table.rows[row].size() != table.headers.size())
        {
            return false;
        }
    }

    return true;
}

// Checks the tables that come from the data files.
void testdatatables()
{
    string folder = testfolder("tables");
    writerichfixture(folder);
    datamanager files(folder);
    dataset data;
    offerstore store;
    list<string> warnings;
    files.loadall(data, warnings);
    store.load(folder, warnings);

    // donations
    texttable donations = donationstable(data, store, allpositions((int)data.donations.size()));
    check(headertext(donations) == "ID|Donor|Food|Original|Left|Offered|Usable minutes|Status", "donations headers");
    check(donations.rows.size() == 5 && rectangular(donations), "donations rows");
    check(rowtext(donations, 0) == "DON001|Metro Hotel Kitchen|COOKED|80|80|40|60|Waiting", "donation DON001");
    check(rowtext(donations, 1) == "DON002|Metro Hotel Kitchen|COOKED|80|50|0|9|Partly given", "donation DON002");
    check(rowtext(donations, 2) == "DON003|City Banquet Hall|PACKAGED|40|40|0|30|Closed", "donation DON003");
    check(rowtext(donations, 3) == "DON004|City Banquet Hall|PRODUCE|100|0|0|90|All given", "donation DON004");
    check(rowtext(donations, 4) == "DON005|City Banquet Hall|BAKERY|50|50|20|20|Waiting", "donation DON005");

    vector<int> picked;
    picked.push_back(4);
    picked.push_back(99);
    picked.push_back(-1);
    picked.push_back(0);
    texttable some = donationstable(data, store, picked);
    check(some.rows.size() == 2 && some.rows[0][0] == "DON005" && some.rows[1][0] == "DON001", "positions are kept in order and bad ones skipped");

    texttable none = donationstable(data, store, vector<int>());
    check(none.rows.empty() && none.headers.size() == 8, "an empty list has headers and no rows");

    // recipients
    texttable recipients = recipientstable(data, allpositions((int)data.recipients.size()));
    check(headertext(recipients) == "ID|Name|Type|Capacity|Food types|Accepting|Can collect", "recipients headers");
    check(recipients.rows.size() == 7 && rectangular(recipients), "recipients rows");
    check(rowtext(recipients, 0) == "R001|Hope Shelter|Shelter|150|COOKED  PACKAGED|yes|yes", "recipient R001");
    check(rowtext(recipients, 2) == "R003|Green Community Kitchen|Community kitchen|120|COOKED|yes|yes", "recipient R003");
    check(rowtext(recipients, 5) == "R006|Bridge Pantry|Pantry|100|PACKAGED  PRODUCE|yes|no", "recipient R006 cannot collect");
    check(rowtext(recipients, 6) == "R007|Sevak Kitchen|Community kitchen|90|COOKED|no|yes", "recipient R007 is not accepting");
    check(recipientstable(data, vector<int>()).rows.empty(), "no recipients");

    // transactions
    texttable transactions = transactionstable(data, allpositions((int)data.transactions.size()));
    check(headertext(transactions) == "ID|Donation|Donor|Food|Recipient|Quantity|Travel minutes", "transactions headers");
    check(transactions.rows.size() == 3 && rectangular(transactions), "transactions rows");
    check(rowtext(transactions, 0) == "T001|DON002|Metro Hotel Kitchen|COOKED|Hope Shelter|30|5", "transaction T001");
    check(rowtext(transactions, 2) == "T003|DON004|City Banquet Hall|PRODUCE|City Food Bank|40|9", "transaction T003");

    // offers
    texttable offers = offerstable(data, store, allpositions(store.count()));
    check(headertext(offers) == "ID|Donation|Donor|Food|Recipient|Quantity|Travel minutes|Status", "offers headers");
    check(offers.rows.size() == 6 && rectangular(offers), "offers rows");
    check(rowtext(offers, 0) == "OFF001|DON001|Metro Hotel Kitchen|COOKED|Hope Shelter|30|5|Waiting for answer", "offer OFF001");
    check(rowtext(offers, 1) == "OFF002|DON002|Metro Hotel Kitchen|COOKED|Hope Shelter|30|5|Accepted", "offer OFF002");
    check(rowtext(offers, 2) == "OFF003|DON003|City Banquet Hall|PACKAGED|City Food Bank|5|5|Withdrawn", "offer OFF003");
    check(rowtext(offers, 3) == "OFF004|DON004|City Banquet Hall|PRODUCE|City Food Bank|10|9|Declined", "offer OFF004");

    // an offer or transaction that names something unknown shows the id instead of a name
    offerstore odd;
    odd.add(EntityId("DON", 77), EntityId("R", 88), 3, 4);
    texttable oddtable = offerstable(data, odd, allpositions(odd.count()));
    check(rowtext(oddtable, 0).find("DON077|DON077|Unknown|R088|3|4") != string::npos, "an unknown donation and recipient show ids: " + rowtext(oddtable, 0));

    // summary
    totals sums = computetotals(data);
    texttable summary = summarytable(sums);
    check(headertext(summary) == "Item|Quantity", "summary headers");
    check(summary.rows.size() == 8 && rectangular(summary), "summary rows");
    check(rowtext(summary, 0) == "Offered|350", "summary offered");
    check(rowtext(summary, 1) == "Given out|130", "summary given out");
    check(rowtext(summary, 2) == "Wasted|40", "summary wasted");
    check(rowtext(summary, 3) == "Still open|180", "summary still open");
    check(rowtext(summary, 4) == "Cooked given out|30", "summary cooked");
    check(rowtext(summary, 5) == "Packaged given out|0", "summary packaged");
    check(rowtext(summary, 6) == "Produce given out|100", "summary produce");
    check(rowtext(summary, 7) == "Bakery given out|0", "summary bakery");
    vector<string> about = aboutlines();
    check(about.size() == 3, "three about lines");
    check(about.size() == 3 && about[0] == "CommunityBridge helps decide where surplus food should go", "about line 1");
    check(about.size() == 3 && about[1] == "All travel times are simulated minutes and not real road data", "about line 2");
    check(about.size() == 3 && about[2] == "Recipients are ranked by food accepted then travel time then spare room", "about line 3");
}

// Checks the match tables with a match view that is built by hand.
void testmatchtables()
{
    string folder = testfolder("tables_match");
    writefixture(folder);
    datamanager files(folder);
    dataset data;
    list<string> warnings;
    files.loadall(data, warnings);

    matchview match;
    match.donation = EntityId("DON", 1);
    match.candidates.push_back(Candidate(EntityId("R", 3), 60, Duration(9), 60));
    match.candidates.push_back(Candidate(EntityId("R", 1), 50, Duration(40), 100));
    match.routes.push_back("A -> B -> C");
    match.routes.push_back("A -> D");
    match.rejected.push_back(Rejection(EntityId("R", 5), UNREACHABLE, Duration::infinite()));
    match.rejected.push_back(Rejection(EntityId("R", 2), FOOD_INCOMPATIBLE, Duration(9)));
    match.rejected.push_back(Rejection(EntityId("R", 4), TOO_FAR, Duration(61)));
    match.declined.push_back(EntityId("R", 6));

    texttable candidates = candidatestable(data, match);
    check(headertext(candidates) == "Number|Recipient|Can take|Travel minutes|Spare room|Route", "candidates headers");
    check(candidates.rows.size() == 2 && rectangular(candidates), "candidates rows");
    check(rowtext(candidates, 0) == "1|Green Community Kitchen|60|9|60|A -> B -> C", "candidate 1");
    check(rowtext(candidates, 1) == "2|Hope Shelter|50|40|100|A -> D", "candidate 2");

    texttable rejected = rejectedtable(data, match);
    check(headertext(rejected) == "Recipient|Why not|Travel minutes", "rejected headers");
    check(rejected.rows.size() == 4 && rectangular(rejected), "rejected rows include the declined one");
    check(rowtext(rejected, 0) == "Riverside School Meals|No road connects it to the donor|none", "unreachable shows none");
    check(rowtext(rejected, 1) == "City Food Bank|Does not take this kind of food|9", "food incompatible");
    check(rowtext(rejected, 2) == "Sunrise Care Home|Too far for the usable time|61", "too far shows the minutes");
    check(rowtext(rejected, 3) == "Bridge Pantry|Declined this donation earlier|none", "declined recipient");

    matchview empty;
    check(candidatestable(data, empty).rows.empty() && rejectedtable(data, empty).rows.empty(), "an empty match gives empty tables");

    // a candidate without a route text still has a row
    matchview short_;
    short_.candidates.push_back(Candidate(EntityId("R", 3), 5, Duration(1), 0));
    check(rowtext(candidatestable(data, short_), 0) == "1|Green Community Kitchen|5|1|0|", "a missing route is an empty cell");
}

// Checks long names and big numbers.
void testbigvalues()
{
    string folder = testfolder("tables_big");
    writefixture(folder);
    string longname = "Extraordinarily Long Named Community Kitchen And Pantry Hall";
    writefile(folder + "recipients.csv",
              "recipient_id,name,type,available_capacity,accepted_types,accepting,can_pickup,node_id\n"
              "R001," + longname + ",COMMUNITY_CENTRE,999999999,COOKED;PACKAGED;PRODUCE;BAKERY,1,1,5\n");
    writefile(folder + "donations.csv",
              "donation_id,donor_id,food_type,original_quantity,remaining_quantity,usable_minutes,status\n"
              "DON001,DN002,COOKED,50000,50000,60,PENDING\n");

    datamanager files(folder);
    dataset data;
    offerstore store;
    list<string> warnings;
    files.loadall(data, warnings);

    texttable recipients = recipientstable(data, allpositions(1));
    check(rowtext(recipients, 0) == "R001|" + longname + "|Community centre|999999999|COOKED  PACKAGED  PRODUCE  BAKERY|yes|yes", "long name and 999999999");
    texttable donations = donationstable(data, store, allpositions(1));
    check(rowtext(donations, 0) == "DON001|Metro Hotel Kitchen|COOKED|50000|50000|0|60|Waiting", "quantity 50000");
}

// Checks the reason text for every reason.
void testreasons()
{
    check(reasontext(OK) == "Can take it", "reason OK");
    check(reasontext(NOT_ACCEPTING) == "Not accepting donations right now", "reason NOT_ACCEPTING");
    check(reasontext(FOOD_INCOMPATIBLE) == "Does not take this kind of food", "reason FOOD_INCOMPATIBLE");
    check(reasontext(NO_CAPACITY) == "No room left", "reason NO_CAPACITY");
    check(reasontext(NO_PICKUP) == "Cannot collect the food", "reason NO_PICKUP");
    check(reasontext(UNREACHABLE) == "No road connects it to the donor", "reason UNREACHABLE");
    check(reasontext(TOO_FAR) == "Too far for the usable time", "reason TOO_FAR");
}

// Runs every group of checks.
int main()
{
    testdatatables();
    testmatchtables();
    testbigvalues();
    testreasons();
    return finish();
}
