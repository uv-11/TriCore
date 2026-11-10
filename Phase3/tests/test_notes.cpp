// test_notes: plainnotes turns the warnings of the start-up into plain lines. The exact Phase 2 warning for an isolated
// recipient is checked, then the other shapes (an unknown id, a donor, the offers, a duplicate road, a recovered file).

#include "Datamanager.h"
#include "TextTools.h"
#include "testkit.h"

#include <list>
#include <string>
#include <vector>

using namespace std;

// Checks that plainnotes gives exactly this one line for this one warning.
void notecase(const list<string>& none, const dataset& data, const string& warning, const string& expected)
{
    (void)none;
    list<string> warnings;
    warnings.push_back(warning);
    vector<string> lines = plainnotes(warnings, data);
    check(lines.size() == 1 && lines[0] == expected, "note for [" + warning + "] is [" + (lines.empty() ? string("nothing") : lines[0]) + "] and should be [" + expected + "]");

    if (!lines.empty())
    {
        check(!haspunct(lines[0]) && lines[0].find("node") == string::npos, "the note [" + lines[0] + "] has no punctuation and no node");
    }
}

// Runs every check of this test.
int main()
{
    // the standard fixture: R005 sits on a location without a road, so the start-up really gives one warning
    string folder = testfolder("notes_standard");
    writefixture(folder);
    datamanager files(folder);
    dataset data;
    list<string> warnings;
    files.loadall(data, warnings);
    check(warnings.size() == 1, "the standard fixture gives exactly one warning");
    vector<string> real = plainnotes(warnings, data);
    check(real.size() == 1 && real[0] == "Riverside School Meals has no road to any donor so it can never be recommended",
          "the real warning of Phase 2 becomes a plain line: " + (real.empty() ? string("nothing") : real[0]));

    // R024 and R025 on two locations without a road (a small data set of its own)
    string folder2 = testfolder("notes_r024");
    writefixture(folder2);
    writefile(folder2 + "graph_nodes.csv", nodestext() + "11,R025 Patparganj Isolated Food Bank,12,8\n");
    writefile(folder2 + "recipients.csv",
              "recipient_id,name,type,available_capacity,accepted_types,accepting,can_pickup,node_id\n"
              "R001,Hope Shelter,SHELTER,150,COOKED;PACKAGED,1,1,5\n"
              "R024,Mayur Vihar Isolated Shelter,SHELTER,120,COOKED;PACKAGED,1,1,9\n"
              "R025,Patparganj Isolated Food Bank,FOOD_BANK,150,PACKAGED;PRODUCE,1,1,11\n");
    datamanager files2(folder2);
    dataset data2;
    list<string> warnings2;
    files2.loadall(data2, warnings2);
    check(warnings2.size() == 2, "two isolated recipients give two warnings: " + string(warnings2.empty() ? "none" : warnings2.front()));
    vector<string> lines2 = plainnotes(warnings2, data2);
    check(lines2.size() == 2, "two plain lines");

    if (lines2.size() == 2)
    {
        check(lines2[0] == "Mayur Vihar Isolated Shelter has no road to any donor so it can never be recommended", "line for R024: " + lines2[0]);
        check(lines2[1] == "Patparganj Isolated Food Bank has no road to any donor so it can never be recommended", "line for R025: " + lines2[1]);
    }

    // the exact text of Phase 2, given directly
    list<string> none;
    notecase(none, data2, "R024 (node 16) is not connected to any donor; it can never be recommended.",
             "Mayur Vihar Isolated Shelter has no road to any donor so it can never be recommended");
    notecase(none, data2, "R025 (node 17) is not connected to any donor; it can never be recommended.",
             "Patparganj Isolated Food Bank has no road to any donor so it can never be recommended");

    // an unknown id keeps the id
    notecase(none, data2, "R999 (node 3) is not connected to any donor; it can never be recommended.",
             "R999 has no road to any donor so it can never be recommended");

    // another warning about a donor or a recipient: the id and the name
    notecase(none, data2, "DN001 (node 0) is not connected to any recipient; it can never be recommended.",
             "DN001 City Banquet Hall is not connected to any recipient it can never be recommended");
    notecase(none, data2, "R001 has a problem.", "R001 Hope Shelter has a problem");

    // warnings that do not start with an id
    notecase(none, data2, "Offer OFF005 is for donation DON099, which is not in the donations file.",
             "Offer OFF005 is for donation DON099 which is not in the donations file");
    notecase(none, data2, "graph_edges.csv line 5: duplicate edge 3-5, the smaller minutes are kept",
             "graph edges file line 5 duplicate edge 3 5 the smaller minutes are kept");
    notecase(none, data2, "donations.csv was recovered from a .tmp file.", "donations file was recovered from a tmp file");
    notecase(none, data2, "A stale .tmp file for offers.csv was discarded.", "A stale tmp file for offers file was discarded");

    // nothing and empty texts
    check(plainnotes(list<string>(), data2).empty(), "no warnings give no lines");
    list<string> blank;
    blank.push_back("");
    blank.push_back("   ");
    blank.push_back("...");
    check(plainnotes(blank, data2).empty(), "empty warnings give no lines");

    // the order of the lines follows the order of the warnings
    list<string> two;
    two.push_back("R025 (node 17) is not connected to any donor; it can never be recommended.");
    two.push_back("R024 (node 16) is not connected to any donor; it can never be recommended.");
    vector<string> ordered = plainnotes(two, data2);
    check(ordered.size() == 2 && ordered[0].find("Patparganj") == 0 && ordered[1].find("Mayur") == 0, "the lines keep the order of the warnings");

    return finish();
}
