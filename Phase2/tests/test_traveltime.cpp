#include "Duration.h"
#include "Exceptions.h"
#include "graph.h"
#include "network_v1.h"
#include "traveltime.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int failcount = 0;

// A provider that sets a flag when it is destroyed. Used to see the virtual destructor work.
class watcher : public traveltimeprovider
{
public:
    // Remembers the flag to set.
    watcher(bool* flag)
    {
        flag_ = flag;
    }

    // Sets the flag so the test can see that this destructor ran.
    virtual ~watcher()
    {
        *flag_ = true;
    }

    // Not used by the test.
    vector<Duration> traveltimesfrom(int) const
    {
        return vector<Duration>();
    }

    // Not used by the test.
    string name() const
    {
        return "watcher";
    }

private:
    // The flag to set
    bool* flag_;
};

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

// Checks whether a route equals the given nodes.
bool sameroute(const vector<int>& route, const int* nodes, int count)
{
    if ((int)route.size() != count)
    {
        return false;
    }

    for (int pos = 0; pos < count; pos++)
    {
        if (route[pos] != nodes[pos])
        {
            return false;
        }
    }

    return true;
}

// Main test runner. WHY: Executes all travel time provider unit tests.
int main()
{
    cout << "--- test_traveltime ---\n";

    graph network = buildnetwork();

    // TT1: straight-line baseline from node 0 (Spec 6.2, ceil(km / 0.5))
    straightlinetraveltime straight(networkxs(), networkys(), 0.5);
    vector<Duration> line = straight.traveltimesfrom(0);

    // Recipient nodes 5, 6, 7, 8, 9, 10 are R001, R002, R003, R004, R005, R006
    int nodes[6] = {5, 6, 7, 8, 9, 10};
    int expected[6] = {4, 8, 17, 7, 24, 7};
    bool baseline = (line.size() == 11) && (line[0] == Duration(0));

    for (int pos = 0; pos < 6; pos++)
    {
        if (!(line[nodes[pos]] == Duration(expected[pos])))
        {
            baseline = false;
        }
    }

    check(baseline, "TT1: straight-line times from node 0 match the baseline row of Spec 6.2");
    check(straight.name() == "straight-line", "TT1: straight-line provider name");

    // TT2: no travel check gives zero everywhere
    notravelcheck nocheck(11);
    vector<Duration> zeros = nocheck.traveltimesfrom(1);
    bool allzero = (zeros.size() == 11);

    for (int node = 0; node < 11; node++)
    {
        if (!(zeros[node] == Duration(0)))
        {
            allzero = false;
        }
    }

    check(allzero, "TT2: notravelcheck gives 0 for all 11 nodes");

    // TT3: routes for the Spec 6.5 table
    dijkstratraveltime provider(network);

    int from0to5[3] = {0, 2, 5};
    int from0to6[3] = {0, 4, 6};
    int from0to7[3] = {0, 2, 7};
    int from0to8[2] = {0, 8};
    int from0to10[3] = {0, 4, 10};
    int from1to5[4] = {1, 7, 2, 5};
    int from1to6[3] = {1, 3, 6};
    int from1to7[2] = {1, 7};
    int from1to8[6] = {1, 3, 6, 4, 0, 8};
    int from1to10[5] = {1, 3, 6, 4, 10};

    check(sameroute(provider.routeto(0, 5), from0to5, 3), "TT3: DN001 to R001 is 0 2 5");
    check(sameroute(provider.routeto(0, 6), from0to6, 3), "TT3: DN001 to R002 is 0 4 6");
    check(sameroute(provider.routeto(0, 7), from0to7, 3), "TT3: DN001 to R003 is 0 2 7");
    check(sameroute(provider.routeto(0, 8), from0to8, 2), "TT3: DN001 to R004 is 0 8");
    check(sameroute(provider.routeto(0, 10), from0to10, 3), "TT3: DN001 to R006 is 0 4 10");
    check(provider.routeto(0, 9).empty(), "TT3: DN001 to R005 has no route");
    check(sameroute(provider.routeto(1, 5), from1to5, 4), "TT3: DN002 to R001 is 1 7 2 5");
    check(sameroute(provider.routeto(1, 6), from1to6, 3), "TT3: DN002 to R002 is 1 3 6");
    check(sameroute(provider.routeto(1, 7), from1to7, 2), "TT3: DN002 to R003 is 1 7");
    check(sameroute(provider.routeto(1, 8), from1to8, 6), "TT3: DN002 to R004 is 1 3 6 4 0 8");
    check(sameroute(provider.routeto(1, 10), from1to10, 5), "TT3: DN002 to R006 is 1 3 6 4 10");
    check(provider.routeto(1, 9).empty(), "TT3: DN002 to R005 has no route");

    int selfroute[1] = {3};
    check(sameroute(provider.routeto(3, 3), selfroute, 1), "TT3: a route from a node to itself is that node");

    vector<Duration> times = provider.traveltimesfrom(0);
    check(times.size() == 11 && times[8] == Duration(12) && times[9].isInfinite(),
          "TT3: dijkstra provider times from node 0 are correct");
    check(provider.name() == "dijkstra", "TT3: dijkstra provider name");

    bool threwtarget = false;

    try
    {
        provider.routeto(0, 11);
    }
    catch (const StructureException&)
    {
        threwtarget = true;
    }

    check(threwtarget, "TT3: routeto with a target outside the graph throws StructureException");

    // TT4: deleting through a base pointer runs the derived destructor
    bool destroyed = false;
    traveltimeprovider* base = new watcher(&destroyed);
    delete base;

    check(destroyed, "TT4: delete through a base pointer runs the derived destructor");

    // TT4: the three real providers can be used through a base pointer
    traveltimeprovider* providers[3];
    providers[0] = new dijkstratraveltime(network);
    providers[1] = new straightlinetraveltime(networkxs(), networkys(), 0.5);
    providers[2] = new notravelcheck(11);

    bool polymorphic = (providers[0]->traveltimesfrom(0)[8] == Duration(12)) &&
                       (providers[1]->traveltimesfrom(0)[8] == Duration(7)) &&
                       (providers[2]->traveltimesfrom(0)[8] == Duration(0));

    for (int pos = 0; pos < 3; pos++)
    {
        delete providers[pos];
    }

    check(polymorphic, "TT4: node 8 from node 0 is 12, 7 and 0 through the base pointer");

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}