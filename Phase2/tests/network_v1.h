#ifndef NETWORK_V1_H
#define NETWORK_V1_H

#include "graph.h"
#include <vector>

using namespace std;

// Builds the eleven-node, thirteen-edge test network from Spec section 6.1.
inline graph buildnetwork()
{
    graph network(11);
    network.addedge(0, 2, 8).addedge(2, 5, 17).addedge(0, 4, 6).addedge(4, 6, 9);
    network.addedge(0, 8, 12).addedge(2, 7, 14).addedge(6, 3, 5).addedge(3, 7, 7);
    network.addedge(0, 3, 25).addedge(1, 3, 4).addedge(1, 7, 9).addedge(4, 10, 8);
    network.addedge(2, 10, 6);
    return network;
}

// The x coordinate in km of nodes 0 to 10.
inline vector<double> networkxs()
{
    double values[11] = {0, 9, 2, 6, 4, 2, 4, 7, -3, 10, 3};
    return vector<double>(values, values + 11);
}

// The y coordinate in km of nodes 0 to 10.
inline vector<double> networkys()
{
    double values[11] = {0, 1, 4, 1, -3, 0, 0, 4, -1, 6, -1};
    return vector<double>(values, values + 11);
}

#endif