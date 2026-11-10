#ifndef QUERIES_H
#define QUERIES_H

// Queries.h: read-only questions about the data. Each answer is a list of positions.

#include "Datamanager.h"
#include "EntityId.h"
#include "OfferStore.h"

#include <vector>

using namespace std;

// Positions of the donations that belong to a donor.
vector<int> donationsofdonor(const dataset& data, const EntityId& donor);

// Positions of the transactions whose donation belongs to a donor.
vector<int> transactionsofdonor(const dataset& data, const EntityId& donor);

// Positions of the transactions that went to a recipient.
vector<int> transactionsofrecipient(const dataset& data, const EntityId& recipient);

// Offer positions of the offers made for the donations of a donor.
vector<int> offersfordonor(const dataset& data, const offerstore& store, const EntityId& donor);

// Offer positions of the offers a recipient got. With pendingonly the list holds only the offers
// that are still waiting, otherwise it holds every offer of that recipient.
vector<int> offersforrecipient(const offerstore& store, const EntityId& recipient, bool pendingonly);

// Offer positions of the offers a recipient got that are no longer waiting (accepted, declined or withdrawn).
vector<int> pastoffersforrecipient(const offerstore& store, const EntityId& recipient);

// The positions 0 up to count - 1.
vector<int> allpositions(int count);

// The totals of the summary. byfood is indexed by the FoodType value (COOKED is 0).
struct totals
{
    // Creates totals that are all zero.
    totals();

    int offered;    // sum of the original quantities of all donations
    int allocated;  // sum of the quantities of all transactions
    int wasted;     // food left in closed donations
    int stillopen;  // food left in waiting and partly given donations
    int byfood[4];  // quantity given out for each kind of food
};

// Adds up the quantities, with the same meaning as the console summary.
totals computetotals(const dataset& data);

#endif
