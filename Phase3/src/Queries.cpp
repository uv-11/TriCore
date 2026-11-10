// Queries.cpp: read-only questions about donations, transactions, offers and the totals.

#include "Queries.h"

using namespace std;

// Creates totals that are all zero.
totals::totals()
{
    offered = 0;
    allocated = 0;
    wasted = 0;
    stillopen = 0;

    for (int kind = 0; kind < 4; kind++)
    {
        byfood[kind] = 0;
    }
}

// Looks at every donation once.
vector<int> donationsofdonor(const dataset& data, const EntityId& donor)
{
    vector<int> positions;

    for (size_t pos = 0; pos < data.donations.size(); pos++)
    {
        if (data.donations[pos].donorId() == donor)
        {
            positions.push_back((int)pos);
        }
    }

    return positions;
}

// A transaction belongs to the donor of its donation.
vector<int> transactionsofdonor(const dataset& data, const EntityId& donor)
{
    vector<int> positions;

    for (size_t pos = 0; pos < data.transactions.size(); pos++)
    {
        int donationpos = 0;

        if (data.donationindex.get(data.transactions[pos].donationId().toString(), donationpos) &&
            data.donations[donationpos].donorId() == donor)
        {
            positions.push_back((int)pos);
        }
    }

    return positions;
}

// Looks at every transaction once.
vector<int> transactionsofrecipient(const dataset& data, const EntityId& recipient)
{
    vector<int> positions;

    for (size_t pos = 0; pos < data.transactions.size(); pos++)
    {
        if (data.transactions[pos].recipientId() == recipient)
        {
            positions.push_back((int)pos);
        }
    }

    return positions;
}

// An offer belongs to the donor of its donation.
vector<int> offersfordonor(const dataset& data, const offerstore& store, const EntityId& donor)
{
    vector<int> positions;

    for (int pos = 0; pos < store.count(); pos++)
    {
        int donationpos = 0;

        if (data.donationindex.get(store.at(pos).donationId().toString(), donationpos) &&
            data.donations[donationpos].donorId() == donor)
        {
            positions.push_back(pos);
        }
    }

    return positions;
}

// Every offer of the recipient, or only the waiting ones.
vector<int> offersforrecipient(const offerstore& store, const EntityId& recipient, bool pendingonly)
{
    vector<int> positions;

    for (int pos = 0; pos < store.count(); pos++)
    {
        if (store.at(pos).recipientId() == recipient &&
            (!pendingonly || store.at(pos).status() == OFFER_PENDING))
        {
            positions.push_back(pos);
        }
    }

    return positions;
}

// The offers of the recipient that are finished.
vector<int> pastoffersforrecipient(const offerstore& store, const EntityId& recipient)
{
    vector<int> positions;

    for (int pos = 0; pos < store.count(); pos++)
    {
        if (store.at(pos).recipientId() == recipient && store.at(pos).status() != OFFER_PENDING)
        {
            positions.push_back(pos);
        }
    }

    return positions;
}

// Counts up from 0.
vector<int> allpositions(int count)
{
    vector<int> positions;

    for (int pos = 0; pos < count; pos++)
    {
        positions.push_back(pos);
    }

    return positions;
}

// Follows the console summary: closed donations are wasted, open ones are still open.
totals computetotals(const dataset& data)
{
    totals result;

    for (size_t pos = 0; pos < data.donations.size(); pos++)
    {
        const Donation& donation = data.donations[pos];
        result.offered = result.offered + donation.originalQuantity();

        if (donation.status() == CLOSED)
        {
            result.wasted = result.wasted + donation.remainingQuantity();
        }
        else if (donation.status() == PENDING || donation.status() == PARTIAL)
        {
            result.stillopen = result.stillopen + donation.remainingQuantity();
        }
    }

    for (size_t pos = 0; pos < data.transactions.size(); pos++)
    {
        const Transaction& entry = data.transactions[pos];
        result.allocated = result.allocated + entry.quantity();
        int donationpos = 0;

        if (data.donationindex.get(entry.donationId().toString(), donationpos))
        {
            int kind = (int)data.donations[donationpos].foodType();
            result.byfood[kind] = result.byfood[kind] + entry.quantity();
        }
    }

    return result;
}
