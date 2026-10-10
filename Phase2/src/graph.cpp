#include "graph.h"
#include "Exceptions.h"

using namespace std;

// Creates a graph with count nodes and no edges.
graph::graph(int count)
{
    if (count < 0)
    {
        throw StructureException("A graph cannot have a negative number of nodes");
    }

    nodes_ = count;
    head_ = new edgenode*[nodes_];

    for (int pos = 0; pos < nodes_; pos++)
    {
        head_[pos] = NULL;
    }
}

// Copies every list of the other graph.
graph::graph(const graph& other)
{
    copyfrom(other);
}

// Frees the old lists and then copies the other graph.
graph& graph::operator=(const graph& other)
{
    if (this != &other)
    {
        clear();
        copyfrom(other);
    }

    return *this;
}

// Frees every edge node and the array of list heads.
graph::~graph()
{
    clear();
}

// Puts one edge node at the front of each of the two lists.
graph& graph::addedge(int from, int to, int minutes)
{
    if (from < 0 || from >= nodes_ || to < 0 || to >= nodes_)
    {
        throw StructureException("addedge got a node outside the graph");
    }

    if (minutes < 1)
    {
        throw StructureException("An edge needs at least 1 minute");
    }

    edgenode* forward = new edgenode;
    forward->to = to;
    forward->minutes = minutes;
    forward->next = head_[from];
    head_[from] = forward;

    edgenode* backward = new edgenode;
    backward->to = from;
    backward->minutes = minutes;
    backward->next = head_[to];
    head_[to] = backward;

    return *this;
}

// Returns how many nodes the graph has.
int graph::nodecount() const
{
    return nodes_;
}

// Returns the first edge of the list of a node.
const edgenode* graph::neighbors(int node) const
{
    if (node < 0 || node >= nodes_)
    {
        throw StructureException("neighbors got a node outside the graph");
    }

    return head_[node];
}

// Allocates the same number of heads and copies each list in the same order.
void graph::copyfrom(const graph& other)
{
    nodes_ = other.nodes_;
    head_ = new edgenode*[nodes_];

    for (int pos = 0; pos < nodes_; pos++)
    {
        head_[pos] = NULL;
        edgenode* tail = NULL;
        const edgenode* current = other.head_[pos];

        while (current != NULL)
        {
            edgenode* fresh = new edgenode;
            fresh->to = current->to;
            fresh->minutes = current->minutes;
            fresh->next = NULL;

            if (tail == NULL)
            {
                head_[pos] = fresh;
            }
            else
            {
                tail->next = fresh;
            }

            tail = fresh;
            current = current->next;
        }
    }
}

// Deletes the edge nodes of every list and then the array of heads.
void graph::clear()
{
    for (int pos = 0; pos < nodes_; pos++)
    {
        edgenode* current = head_[pos];

        while (current != NULL)
        {
            edgenode* following = current->next;
            delete current;
            current = following;
        }
    }

    delete[] head_;
    head_ = NULL;
    nodes_ = 0;
}

// Gives a node and every node reachable from it the same component number.
static void visitnode(const graph& network, int node, int mark, vector<int>& label)
{
    label[node] = mark;

    for (const edgenode* edge = network.neighbors(node); edge != NULL; edge = edge->next)
    {
        if (label[edge->to] == -1)
        {
            visitnode(network, edge->to, mark, label);
        }
    }
}

// Starts a new search at every node that has no component yet.
int labelcomponents(const graph& network, vector<int>& label)
{
    label.assign(network.nodecount(), -1);
    int mark = 0;

    for (int node = 0; node < network.nodecount(); node++)
    {
        if (label[node] == -1)
        {
            visitnode(network, node, mark, label);
            mark++;
        }
    }

    return mark;
}