#ifndef SCRIPTED_H
#define SCRIPTED_H

#include "EntityId.h"
#include "Models.h"
#include "allocationmanager.h"
#include "datamanager.h"
#include "intqueue.h"
#include "recommendationengine.h"
#include "traveltime.h"
#include <vector>

using namespace std;

// The scripted coordinator of the scenarios S8 and E3. It is a test device only: for every
// donation it takes rank 1 with the full potential acceptance, and closes the donation
// when nobody is feasible. Donations are handled in urgency order (urgencyqueue) or in
// arrival order (intqueue). Everything stays in memory. Returns the total allocated.
inline int runscripted(dataset& data, bool urgency)
{
    dijkstratraveltime provider(data.network);
    urgencyqueue byurgency;
    intqueue byarrival;

    if (urgency)
    {
        fillurgencyqueue(data.donations, byurgency);
    }
    else
    {
        for (size_t pos = 0; pos < data.donations.size(); pos++)
        {
            byarrival.enqueue((int)pos);
        }
    }

    EntityId txn = data.lasttransaction;
    int total = 0;
    bool more = true;

    while (more)
    {
        int index = 0;

        if (urgency && byurgency.empty())
        {
            more = false;
        }
        else if (!urgency && byarrival.empty())
        {
            more = false;
        }
        else if (urgency)
        {
            index = byurgency.popmosturgent();
        }
        else
        {
            index = byarrival.dequeue();
        }

        if (more)
        {
            Donation& donation = data.donations[index];
            int donorpos = 0;
            data.donorindex.get(donation.donorId().toString(), donorpos);
            vector<Duration> times = provider.traveltimesfrom(data.donors[donorpos].nodeId());

            while (donation.status() == PENDING || donation.status() == PARTIAL)
            {
                recommendationresult found = recommend(donation, data.recipients, times);

                if (found.candidates.empty())
                {
                    closedonation(donation);
                }
                else
                {
                    const candidate& top = found.candidates[0];
                    int recipientpos = 0;
                    data.recipientindex.get(top.recipientid().toString(), recipientpos);
                    ++txn;
                    allocate(donation, data.recipients[recipientpos],
                             top.potentialacceptance(), top.traveltime(), txn);
                    total = total + top.potentialacceptance();
                }
            }
        }
    }

    return total;
}

#endif