#include "Exceptions.h"
#include "graph.h"
#include "network_v1.h"
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

// Checks whether the list of node from has an edge to node to with the given minutes.
bool hasedge(const graph& network, int from, int to, int minutes)
{
    for (const edgenode* edge = network.neighbors(from); edge != NULL; edge = edge->next)
    {
        if (edge->to == to && edge->minutes == minutes)
        {
            return true;
        }
    }

    return false;
}

// Counts the edges in the list of a node.
int countedges(const graph& network, int node)
{
    int total = 0;

    for (const edgenode* edge = network.neighbors(node); edge != NULL; edge = edge->next)
    {
        total++;
    }

    return total;
}

// Main test runner. WHY: Executes all graph and connectivity unit tests.
int main()
{
    cout << "--- test_graph ---\n";

    // GR1: addedge adds the road in both directions
    graph small(3);
    small.addedge(0, 2, 8);

    check(hasedge(small, 0, 2, 8), "GR1: list of node 0 has the edge to 2");
    check(hasedge(small, 2, 0, 8), "GR1: list of node 2 has the edge back to 0");
    check(countedges(small, 1) == 0, "GR1: node 1 has no edges");
    check(small.nodecount() == 3, "GR1: nodecount is 3");

    // GR2: a copy is independent of the original
    graph original(3);
    original.addedge(0, 1, 5);

    graph copy(original);
    copy.addedge(1, 2, 7);

    check(countedges(original, 1) == 1, "GR2: original node 1 still has one edge");
    check(countedges(original, 2) == 0, "GR2: original node 2 has no edges");
    check(hasedge(copy, 0, 1, 5), "GR2: copy has the original edge");
    check(hasedge(copy, 2, 1, 7), "GR2: copy has the new edge");

    graph assigned(1);
    assigned = original;
    assigned.addedge(0, 2, 9);

    check(countedges(original, 0) == 1, "GR2: original unchanged after changing an operator= copy");
    check(assigned.nodecount() == 3 && countedges(assigned, 0) == 2,
          "GR2: operator= copy has its own lists");

    assigned = assigned;
    check(countedges(assigned, 0) == 2, "GR2: self-assignment keeps the edges");

    // GR3: calls can be chained because addedge returns the graph
    graph chained(4);
    chained.addedge(0, 1, 1).addedge(1, 2, 2).addedge(2, 3, 3);

    check(hasedge(chained, 0, 1, 1) && hasedge(chained, 1, 2, 2) && hasedge(chained, 3, 2, 3),
          "GR3: chained addedge calls all took effect");

    // addedge and neighbors reject bad arguments
    bool threwnode = false;
    bool threwminutes = false;
    bool threwneighbors = false;

    try
    {
        chained.addedge(0, 4, 5);
    }
    catch (const StructureException&)
    {
        threwnode = true;
    }

    try
    {
        chained.addedge(0, 1, 0);
    }
    catch (const StructureException&)
    {
        threwminutes = true;
    }

    try
    {
        chained.neighbors(-1);
    }
    catch (const StructureException&)
    {
        threwneighbors = true;
    }

    check(threwnode, "GR3: addedge with a node outside the graph throws StructureException");
    check(threwminutes, "GR3: addedge with 0 minutes throws StructureException");
    check(threwneighbors, "GR3: neighbors of a node outside the graph throws StructureException");

    // CN1: the depth-first search finds two components in network_v1
    graph network = buildnetwork();
    vector<int> label;
    int parts = labelcomponents(network, label);

    check(parts == 2, "CN1: network_v1 has 2 components");

    bool connected = true;

    for (int node = 0; node < 11; node++)
    {
        if (node != 9 && label[node] != label[0])
        {
            connected = false;
        }
    }

    check(connected, "CN1: nodes 0 to 8 and 10 are in one component");
    check(label[9] != label[0], "CN1: node 9 (R005) is in a component of its own");

    graph empty(0);
    check(labelcomponents(empty, label) == 0, "CN1: an empty graph has 0 components");

    cout << "Total FAIL: " << failcount << "\n";

    if (failcount > 0)
    {
        return 1;
    }

    return 0;
}