#ifndef GRAPH_H
#define GRAPH_H

#include <vector>

using namespace std;

// One road in the adjacency list of a node (a singly linked list node).
struct edgenode
{
    int to;
    int minutes;
    edgenode* next;
};

// Undirected weighted graph. Each node has a linked list of the roads that leave it.
// The weights are simulated minutes, not real road data.
class graph
{
public:
    // Creates a graph with count nodes and no edges.
    graph(int count = 0);

    // Makes a deep copy of every list.
    graph(const graph& other);

    // Replaces this graph with a deep copy of another one.
    graph& operator=(const graph& other);

    // Frees every edge node and the array of list heads.
    ~graph();

    // Adds the road in both directions and returns the graph so calls can be chained.
    // Throws StructureException for a node outside the graph or minutes below 1.
    graph& addedge(int from, int to, int minutes);

    // Returns how many nodes the graph has.
    int nodecount() const;

    // Returns the first road that leaves a node, or NULL if it has none.
    const edgenode* neighbors(int node) const;

private:
    // Array with the first edge of every node
    edgenode** head_;
    // Number of nodes
    int nodes_;

    // Copies every list of another graph into this one.
    void copyfrom(const graph& other);

    // Frees every edge node and the array of list heads.
    void clear();
};

// Labels every node with the number of its connected component (0, 1, 2, ...) using a
// recursive depth-first search. Returns how many components there are.
int labelcomponents(const graph& network, vector<int>& label);

#endif