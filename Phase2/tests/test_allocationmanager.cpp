#include "Duration.h"
#include "EntityId.h"
#include "Exceptions.h"
#include "Models.h"
#include "allocationmanager.h"
#include <iostream>
#include <string>
#include <vector>

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

// Builds recipient R004 (COOKED and BAKERY, capacity 80, accepting, pickup).
Recipient makerecipient()
{
    vector<FoodType> types;
    types.push_back(COOKED);
    types.push_back(BAKERY);
    return Recipient(EntityId("R", 4), "Sunrise Care Home", "CARE_HOME", 80, types, true,
                     true, 8);
}

// Builds a COOKED donation of 100 portions with the given status and usable minutes 30.
Donation makedonation(DonationStatus status)
{
    int remaining = 100;

    if (status == COMPLETED)
    {
        remaining = 0;
    }

    return Donation(EntityId("DON", 7), EntityId("DN", 1), COOKED, 100, remaining, 30,
                    status);
}

// Main test runner. WHY: Executes all allocation and id issuer unit tests.
int main()
{
    cout << "--- test_allocationmanager ---\n";

    EntityId txn("T", 5);

    // A1: a valid allocation changes both objects and builds the transaction
    Recipient recipient = makerecipient();
    Donation donation = makedonation(PENDING);
    allocationresult done = allocate(donation, recipient, 50, Duration(12), txn);

    check(done.error == ALLOC_OK, "A1: a valid allocation returns ALLOC_OK");
    check(donation.remainingQuantity() == 50 && donation.status() == PARTIAL,
          "A1: remaining 50 and status PARTIAL");
    check(recipient.availableCapacity() == 30, "A1: capacity 80 becomes 30");
    check(done.transaction.toCsvRow() == "T005,DON007,R004,50,12",
          "A1: the transaction row is T005,DON007,R004,50,12");

    // A2 to A6: each wrong input gives its own code
    Recipient r2 = makerecipient();
    Donation d2 = makedonation(PENDING);
    check(allocate(d2, r2, 0, Duration(12), txn).error == BAD_QUANTITY,
          "A2: quantity 0 gives BAD_QUANTITY");
    check(allocate(d2, r2, -5, Duration(12), txn).error == BAD_QUANTITY,
          "A3: a negative quantity gives BAD_QUANTITY");
    check(allocate(d2, r2, 101, Duration(12), txn).error == EXCEEDS_REMAINING,
          "A4: quantity above the remaining gives EXCEEDS_REMAINING");
    check(allocate(d2, r2, 81, Duration(12), txn).error == EXCEEDS_CAPACITY,
          "A5: quantity above the capacity gives EXCEEDS_CAPACITY");

    Donation closed = makedonation(PENDING);
    closed.close();
    Donation completed = makedonation(COMPLETED);
    check(allocate(closed, r2, 10, Duration(12), txn).error == DONATION_NOT_OPEN,
          "A6: a closed donation gives DONATION_NOT_OPEN");
    check(allocate(completed, r2, 10, Duration(12), txn).error == DONATION_NOT_OPEN,
          "A6: a completed donation gives DONATION_NOT_OPEN");
    check(allocate(d2, r2, 10, Duration(31), txn).error == NOT_FEASIBLE,
          "A6: travel 31 with usable 30 gives NOT_FEASIBLE");

    // A7: quantity exactly equal to the capacity leaves capacity 0
    Recipient r7 = makerecipient();
    Donation d7 = makedonation(PENDING);
    check(allocate(d7, r7, 80, Duration(12), txn).error == ALLOC_OK &&
              r7.availableCapacity() == 0,
          "A7: quantity equal to the capacity leaves capacity 0");

    // A8: quantity exactly equal to the remaining completes the donation
    vector<FoodType> bigtypes;
    bigtypes.push_back(COOKED);
    Recipient big(EntityId("R", 2), "Big", "FOOD_BANK", 300, bigtypes, true, true, 6);
    Donation d8 = makedonation(PENDING);
    check(allocate(d8, big, 100, Duration(12), txn).error == ALLOC_OK &&
              d8.status() == COMPLETED && d8.remainingQuantity() == 0,
          "A8: quantity equal to the remaining gives COMPLETED");

    // A9: after any failure nothing has changed and no transaction was built
    Recipient r9 = makerecipient();
    Donation d9 = makedonation(PENDING);
    string donationbefore = d9.toCsvRow();
    string recipientbefore = r9.toCsvRow();
    int quantities[5] = {0, -1, 101, 81, 10};
    Duration travels[5] = {Duration(12), Duration(12), Duration(12), Duration(12),
                           Duration(31)};
    bool unchanged = true;
    bool notransaction = true;

    for (int pos = 0; pos < 5; pos++)
    {
        allocationresult failed = allocate(d9, r9, quantities[pos], travels[pos], txn);

        if (failed.error == ALLOC_OK || d9.toCsvRow() != donationbefore ||
            r9.toCsvRow() != recipientbefore)
        {
            unchanged = false;
        }

        if (failed.transaction.quantity() != 0)
        {
            notransaction = false;
        }
    }

    check(unchanged, "A9: donation and recipient rows are identical after every failure");
    check(notransaction, "A9: a failed allocation builds no transaction");

    // closedonation
    Donation toclose = makedonation(PENDING);
    check(closedonation(toclose) == ALLOC_OK && toclose.status() == CLOSED &&
              toclose.remainingQuantity() == 100,
          "close: an open donation is closed and keeps its leftover");
    check(closedonation(toclose) == DONATION_NOT_OPEN,
          "close: closing a closed donation gives DONATION_NOT_OPEN");

    // Messages
    check(allocerrormessage(NOT_FEASIBLE) != "" && allocerrormessage(ALLOC_OK) != "",
          "every allocerror has a message");

    // The id issuer: starts at T001, never repeats, and is seeded after a restart
    check(transactionidissuer::issuenext().toString() == "T001",
          "issuer: the first id is T001");
    check(transactionidissuer::issuenext().toString() == "T002",
          "issuer: the second id is T002");

    transactionidissuer::seed(EntityId("T", 41));
    check(transactionidissuer::issuenext().toString() == "T042",
          "issuer: after seed(T041) the next id is T042");
    check(transactionidissuer::issuenext().toString() == "T043",
          "issuer: ids keep counting up and are never reused");

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}