#include "Datamanager.h"
#include "Exceptions.h"
#include "fixtures/invariants_edge_cases_fixture.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

static int failcount = 0;

static void check(bool condition, const string& name)
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

static bool containsProblem(const vector<string>& problems, const string& needle)
{
    for (size_t i = 0; i < problems.size(); ++i)
    {
        if (problems[i].find(needle) != string::npos)
        {
            return true;
        }
    }
    return false;
}

static const string PREFIX = "Phase2/tests/fixtures/temp_dm_inv_";

int main()
{
    cout << "--- test_datamanager_invariants ---\n";

    // 1. Clean dataset has 0 invariant problems
    {
        setup_invariants_fixture(PREFIX);
        datamanager dm(PREFIX);
        dataset data;
        list<string> warnings;
        dm.loadall(data, warnings);

        vector<string> problems = dm.checkinvariants(data);
        check(problems.empty(), "INV1: clean dataset produces zero invariant violations");
        teardown_invariants_fixture(PREFIX);
    }

    // 2. Invariant I3: transaction sum mismatch
    {
        setup_invariants_fixture(PREFIX);
        datamanager dm(PREFIX);
        dataset data;
        list<string> warnings;
        dm.loadall(data, warnings);

        // Tamper with memory: claim remaining is 0 (so 80 placed), but transactions add up to 0
        data.donations[0] = Donation(data.donations[0].id(), data.donations[0].donorId(),
                                     data.donations[0].foodType(), 80, 0, 60, COMPLETED);

        vector<string> problems = dm.checkinvariants(data);
        check(containsProblem(problems, "Data problem: donation"),
              "INV2: I3 detected when transactions sum does not match original minus remaining");
        teardown_invariants_fixture(PREFIX);
    }

    // 3. Invariant I5: transaction uses unknown donation or recipient
    {
        setup_invariants_fixture(PREFIX);
        datamanager dm(PREFIX);
        dataset data;
        list<string> warnings;
        dm.loadall(data, warnings);

        data.transactions.push_back(Transaction(EntityId("T", 1), EntityId("DON", 999),
                                                data.recipients[0].id(), 10, 15));
        vector<string> problems = dm.checkinvariants(data);
        check(containsProblem(problems, "Data problem: transaction"),
              "INV3: I5 detected when transaction references unknown donation");
        teardown_invariants_fixture(PREFIX);
    }

    // 4. Invariant I6: transactions out of order
    {
        setup_invariants_fixture(PREFIX);
        datamanager dm(PREFIX);
        dataset data;
        list<string> warnings;
        dm.loadall(data, warnings);

        // Push T002 then T001
        data.transactions.push_back(Transaction(EntityId("T", 2), data.donations[0].id(),
                                                data.recipients[0].id(), 10, 15));
        data.transactions.push_back(Transaction(EntityId("T", 1), data.donations[0].id(),
                                                data.recipients[0].id(), 10, 15));
        vector<string> problems = dm.checkinvariants(data);
        check(containsProblem(problems, "Data problem: transaction"),
              "INV4: I6 detected when transactions are out of order");
        teardown_invariants_fixture(PREFIX);
    }

    // 5. Invariant I7: recipient available capacity below zero
    {
        setup_invariants_fixture(PREFIX);
        datamanager dm(PREFIX);
        dataset data;
        list<string> warnings;
        dm.loadall(data, warnings);

        data.recipients[0] = Recipient(data.recipients[0].id(), data.recipients[0].name(),
                                       data.recipients[0].type(), -5,
                                       data.recipients[0].acceptedTypes(), true, true,
                                       data.recipients[0].nodeId());
        vector<string> problems = dm.checkinvariants(data);
        check(containsProblem(problems, "has a capacity below 0"),
              "INV5: detected when recipient capacity is negative");
        teardown_invariants_fixture(PREFIX);
    }

    cout << "Total FAIL: " << failcount << "\n";
    return failcount > 0 ? 1 : 0;
}
