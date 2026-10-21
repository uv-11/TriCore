#ifndef TRAVELTIME_H
#define TRAVELTIME_H

#include "Duration.h"
#include "graph.h"
#include <string>
#include <vector>

using namespace std;

// Finds the shortest travel time from source to every node and the route that gets
// there. distances holds a Duration per node (INF if unreachable). previous holds the
// node before it on the best route (-1 for the source and for unreachable nodes).
// Throws StructureException if source is not a node of the graph.
void dijkstra(const graph& network, int source, vector<Duration>& distances,
              vector<int>& previous);

// Abstract source of travel times. The same pipeline runs with any provider, so swapping
// the object is the only change between the real model and the two baselines.
class traveltimeprovider
{
public:
    // Virtual because providers are deleted through a base pointer.
    virtual ~traveltimeprovider();

    // Returns the travel time from the source node to every node.
    virtual vector<Duration> traveltimesfrom(int source) const = 0;

    // Returns a short name for reports.
    virtual string name() const = 0;
};

// Travel times from Dijkstra's algorithm on the road graph (the real model).
class dijkstratraveltime : public traveltimeprovider
{
public:
    // Keeps its own copy of the graph.
    dijkstratraveltime(const graph& network);

    // Runs Dijkstra from the source node.
    vector<Duration> traveltimesfrom(int source) const;

    // Returns the provider name.
    string name() const;

    // Returns the nodes of the best route from source to target in forward order.
    // The result is empty when the target cannot be reached.
    vector<int> routeto(int source, int target) const;

private:
    // The road graph
    graph network_;
};

// Baseline B1: straight-line distance divided by an assumed speed, rounded up.
class straightlinetraveltime : public traveltimeprovider
{
public:
    // Stores the x and y coordinate (in km) of every node and the assumed speed.
    straightlinetraveltime(const vector<double>& xs, const vector<double>& ys,
                           double kmperminute);

    // Returns ceil(distance in km / kmperminute) for every node.
    vector<Duration> traveltimesfrom(int source) const;

    // Returns the provider name.
    string name() const;

private:
    // x coordinate of every node in km
    vector<double> xs_;
    // y coordinate of every node in km
    vector<double> ys_;
    // Assumed speed in km per minute
    double kmperminute_;
};

// Baseline B0: no travel check, every travel time is zero.
class notravelcheck : public traveltimeprovider
{
public:
    // Remembers how many nodes there are.
    notravelcheck(int count);

    // Returns a zero Duration for every node.
    vector<Duration> traveltimesfrom(int source) const;

    // Returns the provider name.
    string name() const;

private:
    // Number of nodes
    int nodes_;
};

#endif