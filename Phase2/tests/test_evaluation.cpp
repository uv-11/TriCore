#include "Duration.h"
#include "EntityId.h"
#include "Models.h"
#include "allocationmanager.h"
#include "csv_fixture.h"
#include "datamanager.h"
#include "recommendationengine.h"
#include "scripted.h"
#include "sorting.h"
#include "traveltime.h"
#include <iomanip>
#include <iostream>
#include <list>
#include <string>
#include <vector>

using namespace std;

int failcount = 0;

// All files of this test start with this prefix, inside the tests/fixtures folder.
const string PREFIX = "Phase2/tests/fixtures/ev_";

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

// Returns the recipient number of the best candidate, or 0 if there is none.
int toppick(const recommendationresult& found)
{
    if (found.candidates.empty())
    {
        return 0;
    }

    return found.candidates[0].recipientid().number();
}

// Ranks the candidates again as if every travel time were 0, and returns the best one.
int toppickwithouttravel(const recommendationresult& found)
{
    int count = (int)found.candidates.size();

    if (count == 0)
    {
        return 0;
    }

    candidate* items = new candidate[count];

    for (int pos = 0; pos < count; pos++)
    {
        items[pos] = candidate(found.candidates[pos].recipientid(),
                               found.candidates[pos].potentialacceptance(), Duration(0),
                               found.candidates[pos].unusedcapacity());
    }

    mergesort(items, count);
    int best = items[0].recipientid().number();
    delete[] items;
    return best;
}

// Main test runner. WHY: Reproduces the reference values of Spec section 12.
int main()
{
    cout << "--- test_evaluation (Spec section 12) ---\n";
    cout << "Travel times are SIMULATED minutes on the network_v1 test graph.\n";
    cout << "The results show that the mechanism works, not how often it matters in real\n";
    cout << "cities.\n\n";

    removefixture(PREFIX);
    writefixture(PREFIX);
    datamanager files(PREFIX);
    dataset base;
    list<string> warnings;
    files.loadall(base, warnings);
    removefixture(PREFIX);

    dijkstratraveltime real(base.network);
    straightlinetraveltime line(base.nodexs, base.nodeys, 0.5);
    int donornodes[2] = {0, 1};

    // E1: 98 verdicts, travel only. 2 donors x 7 recipients x 7 usable values.
    int usables[7] = {10, 15, 20, 25, 30, 40, 60};
    int agree = 0;
    int wrongyes = 0;
    int wrongno = 0;

    for (int donor = 0; donor < 2; donor++)
    {
        vector<Duration> truth = real.traveltimesfrom(donornodes[donor]);
        vector<Duration> guess = line.traveltimesfrom(donornodes[donor]);

        for (size_t pos = 0; pos < base.recipients.size(); pos++)
        {
            int node = base.recipients[pos].nodeId();

            for (int which = 0; which < 7; which++)
            {
                bool truefeasible = !truth[node].isInfinite() && truth[node] <= Duration(usables[which]);
                bool guessfeasible = guess[node] <= Duration(usables[which]);

                if (truefeasible == guessfeasible)
                {
                    agree++;
                }
                else if (guessfeasible)
                {
                    wrongyes++;
                }
                else
                {
                    wrongno++;
                }
            }
        }
    }

    cout << "E1: feasibility verdicts, straight-line (B1) against Dijkstra, 98 cases\n";
    cout << "  agree " << agree << ", B1 feasible but truly not " << wrongyes
         << ", B1 infeasible but truly feasible " << wrongno << "\n\n";
    check(agree == 68 && wrongyes == 29 && wrongno == 1,
          "E1: 68 agree, 29 false feasible, 1 false infeasible (Spec 12)");

    // E2: 192 scenarios. 2 donors x 4 food types x 4 quantities x 6 usable values.
    FoodType foods[4] = {COOKED, PACKAGED, PRODUCE, BAKERY};
    int quantities[4] = {50, 100, 200, 400};
    int usablegrid[6] = {10, 15, 20, 30, 45, 60};
    int scenarios = 0;
    int nofeasible = 0;
    int somefeasible = 0;
    int travelchanges = 0;
    int b1hastop = 0;
    int b1unreachable = 0;
    int b1nothing = 0;
    int b1differs = 0;
    bool constraintsheld = true;

    for (int donor = 0; donor < 2; donor++)
    {
        vector<Duration> truth = real.traveltimesfrom(donornodes[donor]);
        vector<Duration> guess = line.traveltimesfrom(donornodes[donor]);

        for (int food = 0; food < 4; food++)
        {
            for (int amount = 0; amount < 4; amount++)
            {
                for (int which = 0; which < 6; which++)
                {
                    Donation donation(EntityId("DON", 1), EntityId("DN", donor + 1), foods[food],
                                      quantities[amount], quantities[amount], usablegrid[which],
                                      PENDING);
                    recommendationresult full = recommend(donation, base.recipients, truth);
                    recommendationresult baseline = recommend(donation, base.recipients, guess);
                    scenarios++;

                    for (size_t pos = 0; pos < full.candidates.size(); pos++)
                    {
                        int number = full.candidates[pos].recipientid().number();
                        const Recipient& chosen = base.recipients[number - 1];

                        if (checkfeasibility(chosen, donation, truth[chosen.nodeId()]) != OK)
                        {
                            constraintsheld = false;
                        }
                    }

                    if (full.candidates.empty())
                    {
                        nofeasible++;
                    }
                    else
                    {
                        somefeasible++;

                        if (toppick(full) != toppickwithouttravel(full))
                        {
                            travelchanges++;
                        }
                    }

                    if (!baseline.candidates.empty())
                    {
                        b1hastop++;
                        int number = toppick(baseline);
                        int node = base.recipients[number - 1].nodeId();

                        if (truth[node].isInfinite() || Duration(usablegrid[which]) < truth[node])
                        {
                            b1unreachable++;
                        }

                        if (!full.candidates.empty() && toppick(full) != number)
                        {
                            b1differs++;
                        }
                    }
                    else if (!full.candidates.empty())
                    {
                        b1nothing++;
                    }
                }
            }
        }
    }

    cout << "E2: recommendation grid, " << scenarios << " scenarios\n";
    cout << "  no feasible recipient (full system)          " << nofeasible << "\n";
    cout << "  at least one feasible recipient              " << somefeasible << "\n";
    cout << "  travel criterion changes the rank-1 pick     " << travelchanges << " of " << somefeasible << "\n";
    cout << "  scenarios where B1 produces a top pick       " << b1hastop << "\n";
    cout << "  B1 top pick cannot be reached in time        " << b1unreachable << " of " << b1hastop << "\n";
    cout << "  B1 finds nobody but a feasible one exists    " << b1nothing << "\n";
    cout << "  B1 top pick differs from the full system     " << b1differs << "\n\n";

    check(scenarios == 192, "E2: 192 scenarios");
    check(constraintsheld, "SC2: no ranked candidate fails checkfeasibility in any of the 192 scenarios");
    check(nofeasible == 16 && somefeasible == 176, "E2: 16 scenarios without a feasible recipient, 176 with");
    check(travelchanges == 14, "E2: the travel criterion changes the rank-1 pick in 14 of 176");
    check(b1hastop == 180 && b1unreachable == 59, "E2: B1 gives a top pick in 180 and 59 of them cannot be reached in time");
    check(b1nothing == 12, "E2: B1 finds nobody in 12 scenarios where a feasible recipient exists");
    check(b1differs == 55, "E2: B1 and the full system pick differently in 55 scenarios");

    // E3: urgency ordering sweep. Donor DN002, DON001 COOKED 80 usable 60 arrives first.
    int capacities[3] = {80, 120, 160};
    int usabletimes[4] = {9, 30, 36, 45};
    int expectedarrival[3][4] = {{80, 80, 160, 160}, {120, 120, 160, 160}, {160, 160, 160, 160}};
    int expectedurgency[3][4] = {{160, 160, 160, 160}, {160, 160, 160, 160}, {160, 160, 160, 160}};
    bool matches = true;

    cout << "E3: total allocated, arrival order / urgency order\n";
    cout << "  R003 capacity   usable 9    usable 30   usable 36   usable 45\n";

    for (int row = 0; row < 3; row++)
    {
        cout << "  " << setw(13) << capacities[row];

        for (int column = 0; column < 4; column++)
        {
            dataset arrival = base;
            vector<FoodType> cooked;
            cooked.push_back(COOKED);
            arrival.recipients[2] = Recipient(EntityId("R", 3), "Green Community Kitchen",
                                              "COMMUNITY_KITCHEN", capacities[row], cooked, true,
                                              true, 7);
            arrival.donations[1] = Donation(EntityId("DON", 2), EntityId("DN", 2), COOKED, 80, 80,
                                            usabletimes[column], PENDING);
            dataset urgency = arrival;

            int byarrival = runscripted(arrival, false);
            int byurgency = runscripted(urgency, true);

            cout << "   " << setw(4) << byarrival << " / " << setw(3) << byurgency;

            if (byarrival != expectedarrival[row][column] ||
                byurgency != expectedurgency[row][column])
            {
                matches = false;
            }
        }

        cout << "\n";
    }

    cout << "\n";
    check(matches, "E3: all 12 cells match the Spec table");
    cout << "Reading: urgency ordering helps only when R003 cannot hold both donations and\n";
    cout << "DON002 is too urgent to reach R004 (36 minutes away). Otherwise both orders tie.\n";
    cout << "E4 (searching and sorting timings) is optional in the Spec and is not included.\n\n";

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}