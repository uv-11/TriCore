#include "Exceptions.h"
#include "Models.h"
#include "allocationmanager.h"
#include <iostream>
#include <string>

using namespace std;

int failcount = 0;

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

// Builds a pending donation with the given number and usable minutes.
Donation makedonation(int number, int usable)
{
    return Donation(EntityId("DON", number), EntityId("DN", 1), COOKED, 80, 80, usable,
                    PENDING);
}

// Main test runner. WHY: Executes all urgency queue unit tests.
int main()
{
    cout << "--- test_urgencyqueue ---\n";

    // Q1: smaller usable minutes come out first. The index is the array position.
    urgencyqueue byusable;
    byusable.push(makedonation(1, 60), 0);
    byusable.push(makedonation(2, 9), 1);
    byusable.push(makedonation(3, 30), 2);

    check(byusable.popmosturgent() == 1, "Q1: usable 9 comes first");
    check(byusable.popmosturgent() == 2, "Q1: usable 30 comes second");
    check(byusable.popmosturgent() == 0, "Q1: usable 60 comes last");
    check(byusable.empty(), "Q1: the queue is empty afterwards");

    // Q2: equal usable minutes, the smaller donation number (earlier arrival) comes first
    urgencyqueue byarrival;
    byarrival.push(makedonation(12, 20), 0);
    byarrival.push(makedonation(3, 20), 1);
    byarrival.push(makedonation(7, 20), 2);

    check(byarrival.popmosturgent() == 1, "Q2: DON003 comes before DON007 and DON012");
    check(byarrival.popmosturgent() == 2, "Q2: DON007 comes second");
    check(byarrival.popmosturgent() == 0, "Q2: DON012 comes last");

    // Numeric comparison: DON999 is more urgent than DON1000 when the time is equal
    urgencyqueue numeric;
    numeric.push(makedonation(1000, 20), 0);
    numeric.push(makedonation(999, 20), 1);
    check(numeric.popmosturgent() == 1, "Q2: DON999 comes before DON1000");

    // Q3: popping an empty queue is guarded
    urgencyqueue emptyqueue;
    bool threw = false;

    try
    {
        emptyqueue.popmosturgent();
    }
    catch (const StructureException&)
    {
        threw = true;
    }

    check(emptyqueue.empty(), "Q3: a new queue is empty");
    check(threw, "Q3: popmosturgent on an empty queue throws StructureException");

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}