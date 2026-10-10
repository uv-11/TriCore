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

// Compares distances with expected minutes. The value -1 in expected means INF.
bool matches(const vector<Duration>& distances, const int* expected, int count)
{
    if ((int)distances.size() != count)
    {
        return false;
    }

    for (int pos = 0; pos < count; pos++)
    {
        if (expected[pos] == -1)
        {
            if (!distances[pos].isInfinite())
            {
                return false;
            }
        }
        else if (distances[pos].isInfinite() || distances[pos].minutes() != expected[pos])
        {
            return false;
        }
    }

    return true;
}

// Main test runner. WHY: Executes all Dijkstra unit tests.
int main()
{
    cout << "--- test_dijkstra ---\n";

    graph network = buildnetwork();
    vector<Duration> from0;
    vector<int> previous0;
    dijkstra(network, 0, from0, previous0);

    // G1: direct route
    check(from0[2] == Duration(8), "G1: 0 to 2 takes 8");

    // G2: a shorter indirect route beats the direct edge
    check(from0[3] == Duration(20), "G2: 0 to 3 takes 20, not 25");

    // G3: two equal shortest paths, first one found wins (A19)
    check(from0[10] == Duration(14), "G3: 0 to 10 takes 14");
    check(previous0[10] == 4, "G3: route to 10 comes through node 4, not node 2");

    // G4: unreachable
    check(from0[9] == Duration::infinite(), "G4: 0 to 9 is INF");
    check(previous0[9] == -1, "G4: unreachable node 9 has no previous node");

    // G5: source to itself
    check(from0[0] == Duration(0), "G5: 0 to 0 takes 0");
    check(previous0[0] == -1, "G5: the source has no previous node");

    // G6: full tables from node 0 and node 1 (Spec 6.2)
    int row0[11] = {0, 24, 8, 20, 6, 25, 15, 22, 12, -1, 14};
    int row1[11] = {24, 0, 23, 4, 18, 40, 9, 9, 36, -1, 26};
    vector<Duration> from1;
    vector<int> previous1;
    dijkstra(network, 1, from1, previous1);

    check(matches(from0, row0, 11), "G6: full table from node 0 matches Spec 6.2");
    check(matches(from1, row1, 11), "G6: full table from node 1 matches Spec 6.2");

    // The network is undirected, so the 11 by 11 table must be symmetric
    Duration table[11][11];

    for (int source = 0; source < 11; source++)
    {
        vector<Duration> row;
        vector<int> unused;
        dijkstra(network, source, row, unused);

        for (int target = 0; target < 11; target++)
        {
            table[source][target] = row[target];
        }
    }

    bool symmetric = true;

    for (int first = 0; first < 11; first++)
    {
        for (int second = 0; second < 11; second++)
        {
            if (!(table[first][second] == table[second][first]))
            {
                symmetric = false;
            }
        }
    }

    check(symmetric, "G6: the 11 by 11 distance table is symmetric");

    // G7: a single-node graph, and invalid sources
    graph single(1);
    vector<Duration> alone;
    vector<int> aloneprevious;
    dijkstra(single, 0, alone, aloneprevious);

    check(alone.size() == 1 && alone[0] == Duration(0), "G7: single-node graph gives {0}");

    bool threwlow = false;
    bool threwhigh = false;

    try
    {
        dijkstra(network, -1, from0, previous0);
    }
    catch (const StructureException&)
    {
        threwlow = true;
    }

    try
    {
        dijkstra(network, 11, from0, previous0);
    }
    catch (const StructureException&)
    {
        threwhigh = true;
    }

    check(threwlow, "G7: source -1 throws StructureException");
    check(threwhigh, "G7: source 11 throws StructureException");

    // G8: no wrap-around. Two very long edges add up to more than INF, which must stay INF
    graph heavy(4);
    heavy.addedge(0, 1, 999999999).addedge(1, 2, 999999999);
    vector<Duration> heavydist;
    vector<int> heavyprevious;
    dijkstra(heavy, 0, heavydist, heavyprevious);

    check(heavydist[1] == Duration(999999999), "G8: first long edge is exact");
    check(heavydist[2] == Duration::infinite(), "G8: 999999999 + 999999999 stays INF");
    check(heavydist[3] == Duration::infinite(), "G8: node without edges stays INF");

    bool neverbelow = true;

    for (int node = 0; node < 11; node++)
    {
        if (from0[node].minutes() < 0)
        {
            neverbelow = false;
        }
    }

    check(neverbelow, "G8: no distance in network_v1 is negative after Dijkstra");

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}