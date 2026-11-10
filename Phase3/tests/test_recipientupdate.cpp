// test_recipientupdate: a recipient edits its own capacity and flags, in memory and in recipients.csv.

#include "Datamanager.h"
#include "Exceptions.h"
#include "RecipientUpdate.h"
#include "TextTools.h"
#include "testkit.h"

#include <list>
#include <string>
#include <vector>

using namespace std;

// Returns every recipient row of a data set as one text.
string recipientrows(const dataset& data)
{
    string text = "";

    for (size_t pos = 0; pos < data.recipients.size(); pos++)
    {
        text = text + data.recipients[pos].toCsvRow() + "\n";
    }

    return text;
}

// Loads the folder into a new data set, as a restart would.
dataset reload(const string& folder)
{
    datamanager files(folder);
    dataset data;
    list<string> warnings;
    files.loadall(data, warnings);
    return data;
}

// Checks that a message is plain language (not empty, no developer words and no punctuation, rule P1).
bool plain(const string& message)
{
    return !message.empty() && message.find("Exception") == string::npos && message.find("exception") == string::npos && !haspunct(message);
}

// Runs every check of this test.
int main()
{
    string folder = testfolder("recipientupdate");
    writefixture(folder);
    datamanager files(folder);
    dataset data;
    list<string> warnings;
    files.loadall(data, warnings);
    string message = "";

    // a good update: R003 gets capacity 77, stops accepting and cannot collect
    string before = recipientrows(data);
    bool done = updaterecipient(data, files, EntityId("R", 3), 77, false, false, message);
    check(done, "a good update returns true");
    check(plain(message), "the message is plain: " + message);

    int pos = 0;
    data.recipientindex.get("R003", pos);
    check(data.recipients[pos].availableCapacity() == 77, "capacity is 77 in memory");
    check(!data.recipients[pos].accepting(), "accepting is off in memory");
    check(!data.recipients[pos].canPickup(), "pickup is off in memory");
    check(data.recipients[pos].name() == "Green Community Kitchen", "the name is unchanged");
    check(data.recipients[pos].type() == "COMMUNITY_KITCHEN", "the type is unchanged");
    check(data.recipients[pos].nodeId() == 7, "the location is unchanged");
    check(data.recipients[pos].acceptedTypes().size() == 1 && data.recipients[pos].acceptedTypes()[0] == COOKED,
          "the food types are unchanged");

    // the same values must come back after a fresh load
    dataset again = reload(folder);
    check(again.recipients.size() == 7, "seven recipients after a fresh load");
    int pos2 = 0;
    again.recipientindex.get("R003", pos2);
    check(again.recipients[pos2].toCsvRow() == "R003,Green Community Kitchen,COMMUNITY_KITCHEN,77,COOKED,0,0,7",
          "recipients.csv holds the new values");
    check(recipientrows(again) == recipientrows(data), "memory and file agree");

    // every other recipient is exactly as before
    string expected = before;
    size_t start = expected.find("R003,");
    size_t end = expected.find("\n", start);
    expected = expected.substr(0, start) + "R003,Green Community Kitchen,COMMUNITY_KITCHEN,77,COOKED,0,0,7" + expected.substr(end);
    check(recipientrows(again) == expected, "only the R003 row changed");

    // other files are untouched
    check(readfile(folder + "donors.csv") == donorstext(), "donors.csv is untouched");
    check(readfile(folder + "donations.csv") == donationstext(), "donations.csv is untouched");

    // switching back on works too
    check(updaterecipient(data, files, EntityId("R", 3), 0, true, true, message), "capacity 0 is allowed");
    dataset third = reload(folder);
    int pos3 = 0;
    third.recipientindex.get("R003", pos3);
    check(third.recipients[pos3].toCsvRow() == "R003,Green Community Kitchen,COMMUNITY_KITCHEN,0,COOKED,1,1,7",
          "capacity 0 with both flags on is saved");

    // a negative capacity is refused and nothing changes
    string rowsbefore = recipientrows(data);
    string filebefore = readfile(folder + "recipients.csv");
    check(!updaterecipient(data, files, EntityId("R", 3), -5, true, true, message), "a negative capacity is refused");
    check(plain(message), "the message for a negative capacity is plain: " + message);
    check(recipientrows(data) == rowsbefore, "memory unchanged after the refused update");
    check(readfile(folder + "recipients.csv") == filebefore, "file unchanged after the refused update");

    // an unknown recipient is refused
    check(!updaterecipient(data, files, EntityId("R", 99), 10, true, true, message), "an unknown recipient is refused");
    check(plain(message), "the message for an unknown recipient is plain: " + message);
    check(recipientrows(data) == rowsbefore, "memory unchanged after the unknown recipient");

    // a failed save (read-only recipients.csv) puts the old values back
    setreadonly(folder + "recipients.csv", true);
    bool failed = updaterecipient(data, files, EntityId("R", 1), 5, false, false, message);
    setreadonly(folder + "recipients.csv", false);
    check(!failed, "a failed save returns false");
    check(plain(message), "the message for a failed save is plain: " + message);
    check(recipientrows(data) == rowsbefore, "the old values are back in memory after the failed save");
    check(readfile(folder + "recipients.csv") == filebefore, "the file is unchanged after the failed save");
    remove((folder + "recipients.csv.tmp").c_str());

    // and after the file is writable again the same update works
    check(updaterecipient(data, files, EntityId("R", 1), 5, false, false, message), "the update works once the file is writable");
    dataset last = reload(folder);
    int pos4 = 0;
    last.recipientindex.get("R001", pos4);
    check(last.recipients[pos4].toCsvRow() == "R001,Hope Shelter,SHELTER,5,COOKED;PACKAGED,0,0,5", "R001 saved with the new values");

    return finish();
}
