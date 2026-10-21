#ifndef NETWORK_V1_H
#define NETWORK_V1_H

#include "graph.h"

// Builds the 11-node road network from Spec 6.2 / graph_edges.csv for tests.
inline graph buildnetwork()
{
    graph g(11);
    g.addedge(0, 2, 8);
    g.addedge(2, 5, 17);
    g.addedge(0, 4, 6);
    g.addedge(4, 6, 9);
    g.addedge(0, 8, 12);
    g.addedge(2, 7, 14);
    g.addedge(6, 3, 5);
    g.addedge(3, 7, 7);
    g.addedge(0, 3, 25);
    g.addedge(1, 3, 4);
    g.addedge(1, 7, 9);
    g.addedge(4, 10, 8);
    g.addedge(2, 10, 6);
    return g;
}

#endif // NETWORK_V1_H
