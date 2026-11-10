#ifndef MATCH_H
#define MATCH_H

// Match.h: the result of looking for recipients for one donation. It is kept in its own small
// header because both Tables and Workflow use it.

#include "EntityId.h"
#include "RecommendationEngine.h"

#include <string>
#include <vector>

using namespace std;

// Everything the match screen shows for one donation. Building one never changes any data.
struct matchview
{
    // Creates an empty match view.
    matchview();

    EntityId donation;               // the donation that is matched
    vector<Candidate> candidates;    // recipients that can take it, best first
    vector<string> routes;           // routes[i] is the route text of candidates[i]
    vector<Rejection> rejected;      // recipients that cannot take it, with the reason
    vector<EntityId> declined;       // feasible recipients that declined this donation earlier
    int effectiveremaining;          // food left that is not already offered
    int pendingoffers;               // quantity in offers that are still waiting
};

#endif
