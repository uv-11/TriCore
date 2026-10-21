#include "traveltime.h"
#include "Exceptions.h"
#include "intstack.h"
#include "priorityqueue.h"
#include <cmath>

using namespace std;

// Dijkstra on the project priorityqueue. Entries are (distance, node, node). An entry
// that is larger than the stored distance of its node is stale and is skipped.
void dijkstra(const graph& network, int source, vector<Duration>& distances,
              vector<int>& previous)
{
    int count = network.nodecount();

    if (source < 0 || source >= count)
    {
        throw StructureException("Dijkstra source node is not in the graph");
    }

    distances.assign(count, Duration::infinite());
    previous.assign(count, -1);
    distances[source] = Duration(0);

    priorityqueue frontier;
    heapitem start;
    start.key1 = 0;
    start.key2 = source;
    start.payload = source;
    frontier.push(start);

    while (!frontier.empty())
    {
        heapitem top = frontier.popmin();
        int node = top.payload;
        bool stale = top.key1 > distances[node].minutes();

        if (!stale && !distances[node].isInfinite())
        {
            for (const edgenode* edge = network.neighbors(node); edge != NULL;
                 edge = edge->next)
            {
                Duration through = distances[node] + Duration(edge->minutes);

                // Only a strictly better time changes the route, so the first route wins
                if (through < distances[edge->to])
                {
                    distances[edge->to] = through;
                    previous[edge->to] = node;

                    heapitem next;
                    next.key1 = through.minutes();
                    next.key2 = edge->to;
                    next.payload = edge->to;
                    frontier.push(next);
                }
            }
        }
    }
}

// Does nothing. The destructor is only virtual so derived objects are freed correctly.
traveltimeprovider::~traveltimeprovider()
{
}

// Copies the graph so the provider does not depend on the lifetime of the original.
dijkstratraveltime::dijkstratraveltime(const graph& network)
{
    network_ = network;
}

// Runs Dijkstra and returns only the distances.
vector<Duration> dijkstratraveltime::traveltimesfrom(int source) const
{
    vector<Duration> distances;
    vector<int> previous;
    dijkstra(network_, source, distances, previous);
    return distances;
}

// Returns the provider name.
string dijkstratraveltime::name() const
{
    return "dijkstra";
}

// Walks previous from the target back to the source and pushes each node on a stack.
// Popping the stack then gives the route from the source to the target.
vector<int> dijkstratraveltime::routeto(int source, int target) const
{
    if (target < 0 || target >= network_.nodecount())
    {
        throw StructureException("routeto target node is not in the graph");
    }

    vector<Duration> distances;
    vector<int> previous;
    dijkstra(network_, source, distances, previous);

    vector<int> route;

    if (distances[target].isInfinite())
    {
        return route;
    }

    intstack walk;
    int node = target;

    while (node != -1)
    {
        walk.push(node);
        node = previous[node];
    }

    while (!walk.empty())
    {
        route.push_back(walk.pop());
    }

    return route;
}

// Stores the coordinates and the speed.
straightlinetraveltime::straightlinetraveltime(const vector<double>& xs,
                                               const vector<double>& ys,
                                               double kmperminute)
{
    if (xs.size() != ys.size())
    {
        throw StructureException("Coordinate lists must have the same length");
    }

    if (kmperminute <= 0)
    {
        throw StructureException("Speed must be above zero");
    }

    xs_ = xs;
    ys_ = ys;
    kmperminute_ = kmperminute;
}

// Computes ceil(euclidean distance / speed) from the source to every node.
vector<Duration> straightlinetraveltime::traveltimesfrom(int source) const
{
    int count = (int)xs_.size();

    if (source < 0 || source >= count)
    {
        throw StructureException("Straight-line source node is not in the graph");
    }

    vector<Duration> times;

    for (int node = 0; node < count; node++)
    {
        double across = xs_[node] - xs_[source];
        double up = ys_[node] - ys_[source];
        double km = sqrt(across * across + up * up);
        times.push_back(Duration((int)ceil(km / kmperminute_)));
    }

    return times;
}

// Returns the provider name.
string straightlinetraveltime::name() const
{
    return "straight-line";
}

// Remembers the number of nodes.
notravelcheck::notravelcheck(int count)
{
    if (count < 0)
    {
        throw StructureException("Node count cannot be negative");
    }

    nodes_ = count;
}

// Returns zero minutes for every node, so nothing is ever too far.
vector<Duration> notravelcheck::traveltimesfrom(int source) const
{
    if (source < 0 || source >= nodes_)
    {
        throw StructureException("No-check source node is not in the graph");
    }

    return vector<Duration>(nodes_, Duration(0));
}

// Returns the provider name.
string notravelcheck::name() const
{
    return "no-travel-check";
}