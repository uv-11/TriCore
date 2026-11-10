#ifndef DASHBOARD_H
#define DASHBOARD_H

// Dashboard.h: the few numbers shown on the Home pages, in plain C++ (no window code).

#include "Datamanager.h"
#include "EntityId.h"
#include "OfferStore.h"

using namespace std;

// What the coordinator Home page shows.
struct coordinatorstats
{
    // Creates stats that are all zero.
    coordinatorstats();

    int opendonations;  // donations that are PENDING or PARTIAL
    int waiting;        // open donations that still have food nobody was offered
    int pendingoffers;  // offers that wait for an answer
    int givenout;       // food in all transactions
    int wasted;         // food left in closed donations
};

// What the Home page of a donor shows.
struct donorstats
{
    // Creates stats that are all zero.
    donorstats();

    int donations;      // donations of this donor
    int opendonations;  // of those, PENDING or PARTIAL
    int pendingoffers;  // offers for those donations that wait for an answer
    int givenout;       // food of this donor in transactions
};

// What the Home page of a recipient shows.
struct recipientstats
{
    // Creates stats that are all zero.
    recipientstats();

    int incoming;  // offers that wait for this recipient's answer
    int received;  // food this recipient received in transactions
};

// Counts for the whole program.
coordinatorstats coordinatorstatsof(const dataset& data, const offerstore& store);

// Counts for one donor. An unknown donor gives zeros.
donorstats donorstatsof(const dataset& data, const offerstore& store, const EntityId& donor);

// Counts for one recipient. An unknown recipient gives zeros.
recipientstats recipientstatsof(const dataset& data, const offerstore& store, const EntityId& recipient);

#endif
