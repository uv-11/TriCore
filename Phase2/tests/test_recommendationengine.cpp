#include "Duration.h"
#include "EntityId.h"
#include "Exceptions.h"
#include "Models.h"
#include "network_v1.h"
#include "recipients_v1.h"
#include "recommendationengine.h"
#include "sorting.h"
#include "traveltime.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
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

// Builds a recipient with one accepted food type, for the single-reason tests.
Recipient makerecipient(FoodType type, int capacity, bool accepting, bool pickup)
{
    vector<FoodType> types;
    types.push_back(type);
    return Recipient(EntityId("R", 1), "Test", "SHELTER", capacity, types, accepting,
                     pickup, 0);
}

// Builds a pending donation of COOKED food with the given usable minutes.
Donation makedonation(int usable)
{
    return Donation(EntityId("DON", 1), EntityId("DN", 1), COOKED, 100, 100, usable,
                    PENDING);
}

// Builds a candidate for recipient R<number>.
candidate makecandidate(int number, int accept, int travel, int unused)
{
    return candidate(EntityId("R", number), accept, Duration(travel), unused);
}

// Compares one candidate with the expected values.
bool sameentry(const candidate& entry, int number, int accept, int travel, int unused)
{
    return entry.recipientid() == EntityId("R", number) &&
           entry.potentialacceptance() == accept && entry.traveltime() == Duration(travel) &&
           entry.unusedcapacity() == unused;
}

// Builds a donation from DN001 or DN002 for the scenario tests.
Donation scenario(int donor, FoodType food, int quantity, int usable)
{
    return Donation(EntityId("DON", 101), EntityId("DN", donor), food, quantity, quantity,
                    usable, PENDING);
}

// Counts how many rejections have the given recipient number and reason.
int countrejection(const recommendationresult& found, int number, Reason why)
{
    int total = 0;

    for (size_t pos = 0; pos < found.rejections.size(); pos++)
    {
        if (found.rejections[pos].recipientid() == EntityId("R", number) &&
            found.rejections[pos].reason() == why)
        {
            total++;
        }
    }

    return total;
}

// Main test runner. WHY: Executes all recommendation engine unit tests.
int main()
{
    cout << "--- test_recommendationengine ---\n";

    // F1 to F6: one case per reason, F-OK for a recipient that passes everything
    Duration closeby(10);
    Donation donation = makedonation(30);

    check(checkfeasibility(makerecipient(COOKED, 50, false, true), donation, closeby) ==
              NOT_ACCEPTING,
          "F1: not accepting gives NOT_ACCEPTING");
    check(checkfeasibility(makerecipient(PACKAGED, 50, true, true), donation, closeby) ==
              FOOD_INCOMPATIBLE,
          "F2: food not in the accepted types gives FOOD_INCOMPATIBLE");
    check(checkfeasibility(makerecipient(COOKED, 0, true, true), donation, closeby) ==
              NO_CAPACITY,
          "F3: capacity 0 gives NO_CAPACITY");
    check(checkfeasibility(makerecipient(COOKED, 50, true, false), donation, closeby) ==
              NO_PICKUP,
          "F4: no pickup gives NO_PICKUP");
    check(checkfeasibility(makerecipient(COOKED, 50, true, true), donation,
                           Duration::infinite()) == UNREACHABLE,
          "F5: INF travel gives UNREACHABLE");
    check(checkfeasibility(makerecipient(COOKED, 50, true, true), donation, Duration(31)) ==
              TOO_FAR,
          "F6: travel 31 against usable 30 gives TOO_FAR");
    check(checkfeasibility(makerecipient(COOKED, 50, true, true), donation, closeby) == OK,
          "F6: a recipient that passes everything gives OK");

    // F7: precedence. Not accepting AND wrong food reports NOT_ACCEPTING
    check(checkfeasibility(makerecipient(PACKAGED, 0, false, false), donation,
                           Duration::infinite()) == NOT_ACCEPTING,
          "F7: not accepting and wrong food gives NOT_ACCEPTING");
    check(checkfeasibility(makerecipient(PACKAGED, 0, true, false), donation,
                           Duration::infinite()) == FOOD_INCOMPATIBLE,
          "F7: wrong food, no capacity, no pickup, INF gives FOOD_INCOMPATIBLE");
    check(checkfeasibility(makerecipient(COOKED, 0, true, false), donation,
                           Duration::infinite()) == NO_CAPACITY,
          "F7: no capacity, no pickup, INF gives NO_CAPACITY");
    check(checkfeasibility(makerecipient(COOKED, 50, true, false), donation,
                           Duration::infinite()) == NO_PICKUP,
          "F7: no pickup and INF gives NO_PICKUP");

    // F8: the boundary is inclusive
    check(checkfeasibility(makerecipient(COOKED, 50, true, true), donation, Duration(30)) ==
              OK,
          "F8: travel 30 equal to usable 30 gives OK");
    check(checkfeasibility(makerecipient(COOKED, 50, true, true), donation, Duration(31)) ==
              TOO_FAR,
          "F8: travel 31 with usable 30 gives TOO_FAR");

    // R1 to R4: each criterion decides alone
    check(makecandidate(2, 100, 50, 90) < makecandidate(1, 80, 1, 0),
          "R1: higher acceptance ranks first even with a longer travel");
    check(!(makecandidate(1, 80, 1, 0) < makecandidate(2, 100, 50, 90)),
          "R1: the reverse comparison is false");
    check(makecandidate(2, 100, 10, 50) < makecandidate(1, 100, 20, 0),
          "R2: same acceptance, shorter travel ranks first");
    check(makecandidate(2, 100, 10, 5) < makecandidate(1, 100, 10, 9),
          "R3: same acceptance and travel, smaller unused capacity ranks first");
    check(makecandidate(2, 100, 10, 5) < makecandidate(10, 100, 10, 5),
          "R4: everything equal, smaller recipient number ranks first");
    check(!(makecandidate(10, 100, 10, 5) < makecandidate(2, 100, 10, 5)),
          "R4: the reverse comparison is false");

    // R5: numeric comparison of recipient numbers
    check(makecandidate(999, 100, 10, 5) < makecandidate(1000, 100, 10, 5),
          "R5: R999 ranks before R1000");

    // R6: a candidate is never before itself, and shuffled input gives the same output
    candidate same = makecandidate(3, 60, 22, 60);
    check(!(same < same), "R6: a candidate is not before itself");

    candidate base[4];
    base[0] = makecandidate(2, 100, 10, 5);
    base[1] = makecandidate(1, 100, 10, 9);
    base[2] = makecandidate(3, 100, 20, 0);
    base[3] = makecandidate(4, 80, 1, 0);

    srand(12345);
    bool shuffled = true;

    for (int round = 0; round < 100; round++)
    {
        candidate items[4];

        for (int pos = 0; pos < 4; pos++)
        {
            items[pos] = base[pos];
        }

        for (int pos = 3; pos > 0; pos--)
        {
            int other = rand() % (pos + 1);
            candidate temp = items[pos];
            items[pos] = items[other];
            items[other] = temp;
        }

        mergesort(items, 4);

        // Expected order: R2 (travel 10, unused 5), R1 (travel 10, unused 9), R3, R4
        if (!sameentry(items[0], 2, 100, 10, 5) || !sameentry(items[1], 1, 100, 10, 9) ||
            !sameentry(items[2], 3, 100, 20, 0) || !sameentry(items[3], 4, 80, 1, 0))
        {
            shuffled = false;
        }
    }

    check(shuffled, "R6: 100 shuffled inputs always give the same ranking");

    // SO3: the candidate sets of S4, S5 and S6 in the order of Spec 7.2, shuffled 100 times
    candidate s4[3] = {makecandidate(4, 60, 12, 20), makecandidate(3, 60, 22, 60),
                       makecandidate(1, 60, 25, 90)};
    candidate s5[3] = {makecandidate(3, 100, 22, 20), makecandidate(1, 100, 25, 50),
                       makecandidate(4, 80, 12, 0)};
    candidate s6[2] = {makecandidate(2, 100, 15, 200), makecandidate(1, 100, 25, 50)};
    candidate* sets[3] = {s4, s5, s6};
    int sizes[3] = {3, 3, 2};

    for (int which = 0; which < 3; which++)
    {
        int count = sizes[which];
        bool alwayssame = true;

        for (int round = 0; round < 100; round++)
        {
            candidate items[3];

            for (int pos = 0; pos < count; pos++)
            {
                items[pos] = sets[which][pos];
            }

            for (int pos = count - 1; pos > 0; pos--)
            {
                int other = rand() % (pos + 1);
                candidate temp = items[pos];
                items[pos] = items[other];
                items[other] = temp;
            }

            mergesort(items, count);

            for (int pos = 0; pos < count; pos++)
            {
                if (!sameentry(items[pos], sets[which][pos].recipientid().number(),
                               sets[which][pos].potentialacceptance(),
                               sets[which][pos].traveltime().minutes(),
                               sets[which][pos].unusedcapacity()))
                {
                    alwayssame = false;
                }
            }
        }

        stringstream label;
        label << "SO3: S" << (which + 4) << " candidates always rank in the Spec 7.2 order";
        check(alwayssame, label.str());
    }

    // Printing
    stringstream shown;
    shown << makecandidate(4, 80, 12, 0);
    check(shown.str() == "R004 (80 / 12 / 0)", "candidate prints as R004 (80 / 12 / 0)");

    stringstream shownfar;
    shownfar << rejection(EntityId("R", 1), TOO_FAR, Duration(25));
    check(shownfar.str() == "R001 TOO_FAR (25)", "rejection TOO_FAR prints the travel time");

    stringstream shownother;
    shownother << rejection(EntityId("R", 5), UNREACHABLE, Duration::infinite());
    check(shownother.str() == "R005 UNREACHABLE", "other rejections print only the reason");

    // recommend with the real network and recipients (scenarios S1 to S6 of Spec 11.2)
    dijkstratraveltime provider(buildnetwork());
    vector<Recipient> recipients = buildrecipients();
    vector<Duration> from1 = provider.traveltimesfrom(0);

    // S1: DN001, COOKED 100, usable 20
    recommendationresult s1 = recommend(scenario(1, COOKED, 100, 20), recipients, from1);
    check(s1.candidates.size() == 1 && sameentry(s1.candidates[0], 4, 80, 12, 0),
          "S1: only R004 (80 / 12 / 0) is ranked");
    check(s1.rejections.size() == 6 && countrejection(s1, 1, TOO_FAR) == 1 &&
              countrejection(s1, 2, FOOD_INCOMPATIBLE) == 1 &&
              countrejection(s1, 3, TOO_FAR) == 1 && countrejection(s1, 5, UNREACHABLE) == 1 &&
              countrejection(s1, 6, FOOD_INCOMPATIBLE) == 1 &&
              countrejection(s1, 7, NOT_ACCEPTING) == 1,
          "S1: the six rejection reasons match Spec 11.2");

    // S2: boundary. usable 12 keeps R004, usable 11 leaves no candidate
    recommendationresult s2a = recommend(scenario(1, COOKED, 50, 12), recipients, from1);
    recommendationresult s2b = recommend(scenario(1, COOKED, 50, 11), recipients, from1);
    check(s2a.candidates.size() == 1 && sameentry(s2a.candidates[0], 4, 50, 12, 30),
          "S2: usable 12 keeps R004 (travel 12 <= 12)");
    check(s2b.candidates.empty(), "S2: usable 11 leaves no candidates");

    // S3: DN001, PACKAGED 400, usable 30
    recommendationresult s3 = recommend(scenario(1, PACKAGED, 400, 30), recipients, from1);
    check(s3.candidates.size() == 2 && sameentry(s3.candidates[0], 2, 300, 15, 0) &&
              sameentry(s3.candidates[1], 1, 150, 25, 0),
          "S3: R002 (300 / 15 / 0) then R001 (150 / 25 / 0)");
    check(s3.rejections.size() == 5 && countrejection(s3, 3, FOOD_INCOMPATIBLE) == 1 &&
              countrejection(s3, 4, FOOD_INCOMPATIBLE) == 1 &&
              countrejection(s3, 5, UNREACHABLE) == 1 && countrejection(s3, 6, NO_PICKUP) == 1 &&
              countrejection(s3, 7, NOT_ACCEPTING) == 1,
          "S3: the five rejection reasons match Spec 11.2");

    // S4: DN001, COOKED 60, usable 40
    recommendationresult s4r = recommend(scenario(1, COOKED, 60, 40), recipients, from1);
    check(s4r.candidates.size() == 3 && sameentry(s4r.candidates[0], 4, 60, 12, 20) &&
              sameentry(s4r.candidates[1], 3, 60, 22, 60) &&
              sameentry(s4r.candidates[2], 1, 60, 25, 90),
          "S4: R004, R003, R001 in that order");

    // S5: DN001, COOKED 100, usable 30
    recommendationresult s5r = recommend(scenario(1, COOKED, 100, 30), recipients, from1);
    check(s5r.candidates.size() == 3 && sameentry(s5r.candidates[0], 3, 100, 22, 20) &&
              sameentry(s5r.candidates[1], 1, 100, 25, 50) &&
              sameentry(s5r.candidates[2], 4, 80, 12, 0),
          "S5: R003, R001, R004 in that order");

    // S6: DN001, PACKAGED 100, usable 30
    recommendationresult s6r = recommend(scenario(1, PACKAGED, 100, 30), recipients, from1);
    check(s6r.candidates.size() == 2 && sameentry(s6r.candidates[0], 2, 100, 15, 200) &&
              sameentry(s6r.candidates[1], 1, 100, 25, 50),
          "S6: R002 (100 / 15 / 200) then R001 (100 / 25 / 50)");

    // SC2 in small: no ranked candidate fails checkfeasibility
    bool allfeasible = true;
    recommendationresult* runs[5] = {&s3, &s4r, &s5r, &s6r, &s1};
    Donation rundonations[5] = {scenario(1, PACKAGED, 400, 30), scenario(1, COOKED, 60, 40),
                                scenario(1, COOKED, 100, 30), scenario(1, PACKAGED, 100, 30),
                                scenario(1, COOKED, 100, 20)};

    for (int run = 0; run < 5; run++)
    {
        for (size_t pos = 0; pos < runs[run]->candidates.size(); pos++)
        {
            int number = runs[run]->candidates[pos].recipientid().number();
            const Recipient& chosen = recipients[number - 1];

            if (checkfeasibility(chosen, rundonations[run], from1[chosen.nodeId()]) != OK)
            {
                allfeasible = false;
            }
        }
    }

    check(allfeasible, "SC2: no ranked candidate fails checkfeasibility in S1, S3 to S6");

    // A recipient with a node that has no travel time is a structure mistake
    bool threw = false;

    try
    {
        vector<Duration> tooshort(3, Duration(1));
        recommend(scenario(1, COOKED, 10, 20), recipients, tooshort);
    }
    catch (const StructureException&)
    {
        threw = true;
    }

    check(threw, "recommend with too few travel times throws StructureException");

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}