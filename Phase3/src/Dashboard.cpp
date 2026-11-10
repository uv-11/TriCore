// Dashboard.cpp: the numbers of the Home pages. Everything here only reads the data.

#include "Dashboard.h"
#include "Queries.h"
#include "Workflow.h"

using namespace std;

// Creates stats that are all zero.
coordinatorstats::coordinatorstats()
{
    opendonations = 0;
    waiting = 0;
    pendingoffers = 0;
    givenout = 0;
    wasted = 0;
}

// Creates stats that are all zero.
donorstats::donorstats()
{
    donations = 0;
    opendonations = 0;
    pendingoffers = 0;
    givenout = 0;
}

// Creates stats that are all zero.
recipientstats::recipientstats()
{
    incoming = 0;
    received = 0;
}

// A donation can receive offers while it is waiting or partly given.
static bool isopen(const Donation& donation)
{
    return donation.status() == PENDING || donation.status() == PARTIAL;
}

// Adds up the quantities of the transactions at these positions.
static int quantityof(const dataset& data, const vector<int>& positions)
{
    int sum = 0;

    for (size_t pos = 0; pos < positions.size(); pos++)
    {
        sum = sum + data.transactions[positions[pos]].quantity();
    }

    return sum;
}

// Looks at every donation, offer and transaction once.
coordinatorstats coordinatorstatsof(const dataset& data, const offerstore& store)
{
    coordinatorstats stats;

    for (size_t pos = 0; pos < data.donations.size(); pos++)
    {
        const Donation& donation = data.donations[pos];

        if (isopen(donation))
        {
            stats.opendonations++;

            if (effectiveremaining(data, store, (int)pos) > 0)
            {
                stats.waiting++;
            }
        }

        if (donation.status() == CLOSED)
        {
            stats.wasted = stats.wasted + donation.remainingQuantity();
        }
    }

    for (int pos = 0; pos < store.count(); pos++)
    {
        if (store.at(pos).status() == OFFER_PENDING)
        {
            stats.pendingoffers++;
        }
    }

    stats.givenout = quantityof(data, allpositions((int)data.transactions.size()));
    return stats;
}

// Uses the Queries of the donor pages, so the Home numbers always agree with the lists.
donorstats donorstatsof(const dataset& data, const offerstore& store, const EntityId& donor)
{
    donorstats stats;
    vector<int> mine = donationsofdonor(data, donor);
    stats.donations = (int)mine.size();

    for (size_t pos = 0; pos < mine.size(); pos++)
    {
        if (isopen(data.donations[mine[pos]]))
        {
            stats.opendonations++;
        }
    }

    vector<int> offers = offersfordonor(data, store, donor);

    for (size_t pos = 0; pos < offers.size(); pos++)
    {
        if (store.at(offers[pos]).status() == OFFER_PENDING)
        {
            stats.pendingoffers++;
        }
    }

    stats.givenout = quantityof(data, transactionsofdonor(data, donor));
    return stats;
}

// Uses the Queries of the recipient pages, so the Home numbers always agree with the lists.
recipientstats recipientstatsof(const dataset& data, const offerstore& store, const EntityId& recipient)
{
    recipientstats stats;
    stats.incoming = (int)offersforrecipient(store, recipient, true).size();
    stats.received = quantityof(data, transactionsofrecipient(data, recipient));
    return stats;
}
