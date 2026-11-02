#include "EntityId.h"
#include "Exceptions.h"
#include "Models.h"
#include "allocationmanager.h"
#include "csv_fixture.h"
#include "Datamanager.h"
#include <iostream>
#include <list>
#include <string>
#include <vector>

using namespace std;

int failcount = 0;

// All files of these tests start with this prefix, inside the tests/fixtures folder.
const string PREFIX = "Phase2/tests/fixtures/da_";

// Checks a condition and prints PASS or FAIL.
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

// Loads the fixture files into a new dataset, as a restart would.
dataset reload()
{
    datamanager files(PREFIX);
    dataset data;
    list<string> warnings;
    files.loadall(data, warnings);
    return data;
}

// Returns true when the call throws a DataException.
bool throwsdata(datamanager& files, const dataset& data, const Recipient& recipient)
{
    try
    {
        files.appendrecipient(data, recipient);
    }
    catch (const DataException&)
    {
        return true;
    }

    return false;
}

// Returns true when the call throws a DataException.
bool throwsdata(datamanager& files, const dataset& data, const Donor& donor)
{
    try
    {
        files.appenddonor(data, donor);
    }
    catch (const DataException&)
    {
        return true;
    }

    return false;
}

int main()
{
    cout << "--- test_datamanager_add ---\n";

    vector<FoodType> types;
    types.push_back(COOKED);
    types.push_back(PACKAGED);

    // appendrecipient: the saved recipient comes back unchanged after a restart
    fresh();
    datamanager files(PREFIX);
    dataset data = reload();
    Recipient hope(EntityId("R", 8), "Hope Community Kitchen", "SHELTER", 120, types, true,
                   false, 3);
    files.appendrecipient(data, hope);

    dataset after = reload();
    int pos = -1;
    check(after.recipients.size() == data.recipients.size() + 1,
          "DA1: appendrecipient adds exactly one recipient");
    check(after.recipientindex.get("R008", pos) && pos == 7,
          "DA2: the new recipient is found by the recipient index at the end");

    const Recipient& back = after.recipients[after.recipients.size() - 1];
    check(back.id() == EntityId("R", 8) && back.name() == "Hope Community Kitchen" &&
              back.type() == "SHELTER" && back.availableCapacity() == 120 &&
              back.acceptedTypes() == types && back.accepting() && !back.canPickup() &&
              back.nodeId() == 3,
          "DA3: name, type, capacity, food types, flags and location are kept");

    // appenddonor: the same for a donor
    fresh();
    data = reload();
    Donor sunrise(EntityId("DN", 3), "Sunrise Caterers", "CATERER", 2);
    files.appenddonor(data, sunrise);

    after = reload();
    check(after.donors.size() == data.donors.size() + 1,
          "DA4: appenddonor adds exactly one donor");
    check(after.donorindex.get("DN003", pos) && pos == 2,
          "DA5: the new donor is found by the donor index at the end");
    check(after.donors[2].name() == "Sunrise Caterers" && after.donors[2].type() == "CATERER" &&
              after.donors[2].nodeId() == 2,
          "DA6: name, type and location of the donor are kept");

    // the same id twice does not add a second row
    files.appenddonor(after, sunrise);
    check(reload().donors.size() == after.donors.size(),
          "DA7: an id that is already in the table is not added again");

    // a comma or a line break stops the save and leaves the file unchanged
    fresh();
    data = reload();
    string recipientsbefore = readfile(PREFIX + "recipients.csv");
    string donorsbefore = readfile(PREFIX + "donors.csv");

    Recipient commaname(EntityId("R", 8), "Hope, Kitchen", "SHELTER", 120, types, true, true, 3);
    Recipient commatype(EntityId("R", 8), "Hope Kitchen", "SHE,LTER", 120, types, true, true, 3);
    Recipient breakname(EntityId("R", 8), "Hope\nKitchen", "SHELTER", 120, types, true, true, 3);
    Donor commadonor(EntityId("DN", 3), "Sunrise, Caterers", "CATERER", 2);
    Donor breaktype(EntityId("DN", 3), "Sunrise Caterers", "CATE\rRER", 2);

    check(throwsdata(files, data, commaname) && throwsdata(files, data, commatype) &&
              throwsdata(files, data, breakname),
          "DA8: a comma or line break in a recipient name or type throws DataException");
    check(throwsdata(files, data, commadonor) && throwsdata(files, data, breaktype),
          "DA9: a comma or line break in a donor name or type throws DataException");
    check(readfile(PREFIX + "recipients.csv") == recipientsbefore &&
              readfile(PREFIX + "donors.csv") == donorsbefore,
          "DA10: the files are unchanged after the refused saves");
    check(!exists(PREFIX + "recipients.csv.tmp") && !exists(PREFIX + "donors.csv.tmp"),
          "DA11: no temporary file is left behind");

    // the issuers continue from a seeded value and never repeat
    donoridissuer::seed(EntityId("DN", 12));
    EntityId first = donoridissuer::issuenext();
    EntityId second = donoridissuer::issuenext();
    check(first.toString() == "DN013" && second.toString() == "DN014",
          "DA12: after seeding DN012 the donor ids are DN013 then DN014");

    recipientidissuer::seed(EntityId("R", 26));
    EntityId third = recipientidissuer::issuenext();
    EntityId fourth = recipientidissuer::issuenext();
    check(third.toString() == "R027" && fourth.toString() == "R028",
          "DA13: after seeding R026 the recipient ids are R027 then R028");

    bool distinct = true;
    EntityId previous = recipientidissuer::issuenext();

    for (int count = 0; count < 20; count++)
    {
        EntityId next = recipientidissuer::issuenext();

        if (!(previous < next))
        {
            distinct = false;
        }

        previous = next;
    }

    check(distinct, "DA14: twenty more recipient ids are all different and rising");

    // seeding from the loaded data gives the next free id
    fresh();
    data = reload();
    recipientidissuer::seed(data.recipients[data.recipients.size() - 1].id());
    check(recipientidissuer::issuenext().toString() == "R008",
          "DA15: seeded from the fixture (last R007) the next recipient id is R008");

    removefixture(PREFIX);

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}
