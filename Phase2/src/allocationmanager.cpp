#include "allocationmanager.h"
#include "Exceptions.h"
#include "recommendationengine.h"

using namespace std;

// Turns each code into a sentence for the console.
string allocerrormessage(allocerror error)
{
    if (error == ALLOC_OK)
    {
        return "The allocation was done";
    }

    if (error == DONATION_NOT_OPEN)
    {
        return "The donation is not open (it is completed or closed)";
    }

    if (error == BAD_QUANTITY)
    {
        return "The quantity must be at least 1";
    }

    if (error == EXCEEDS_REMAINING)
    {
        return "The quantity is more than what is left of the donation";
    }

    if (error == EXCEEDS_CAPACITY)
    {
        return "The quantity is more than the capacity of the recipient";
    }

    if (error == NOT_FEASIBLE)
    {
        return "The recipient is no longer feasible for this donation";
    }

    throw StructureException("Invalid allocerror value");
}

// Runs the five checks of Spec 7.3 in order and changes state only after all of them pass.
allocationresult allocate(Donation& donation, Recipient& recipient, int quantity,
                          const Duration& travel, const EntityId& txnid)
{
    allocationresult outcome;
    outcome.error = ALLOC_OK;

    if (donation.status() != PENDING && donation.status() != PARTIAL)
    {
        outcome.error = DONATION_NOT_OPEN;
    }
    else if (quantity < 1)
    {
        outcome.error = BAD_QUANTITY;
    }
    else if (quantity > donation.remainingQuantity())
    {
        outcome.error = EXCEEDS_REMAINING;
    }
    else if (quantity > recipient.availableCapacity())
    {
        outcome.error = EXCEEDS_CAPACITY;
    }
    else if (checkfeasibility(recipient, donation, travel) != OK)
    {
        outcome.error = NOT_FEASIBLE;
    }

    if (outcome.error == ALLOC_OK)
    {
        donation.allocateQuantity(quantity);
        recipient.reduceCapacity(quantity);
        outcome.transaction = Transaction(txnid, donation.id(), recipient.id(), quantity,
                                          travel.minutes());
    }

    return outcome;
}

// Closes the donation if it is still open.
allocerror closedonation(Donation& donation)
{
    if (donation.status() != PENDING && donation.status() != PARTIAL)
    {
        return DONATION_NOT_OPEN;
    }

    donation.close();
    return ALLOC_OK;
}

// Stores one heap entry. key1 is the usable time and key2 breaks ties by arrival.
void urgencyqueue::push(const Donation& donation, int index)
{
    heapitem entry;
    entry.key1 = donation.usableMinutes();
    entry.key2 = donation.id().number();
    entry.payload = index;
    heap_.push(entry);
}

// Checks whether no donation is waiting.
bool urgencyqueue::empty() const
{
    return heap_.empty();
}

// The smallest heap entry is the most urgent donation.
int urgencyqueue::popmosturgent()
{
    heapitem top = heap_.popmin();
    return top.payload;
}

// Pushes only the donations that can still be allocated.
void fillurgencyqueue(const vector<Donation>& donations, urgencyqueue& queue)
{
    for (size_t pos = 0; pos < donations.size(); pos++)
    {
        if (donations[pos].status() == PENDING || donations[pos].status() == PARTIAL)
        {
            queue.push(donations[pos], (int)pos);
        }
    }
}

// The first id issued after a fresh start is T001.
EntityId transactionidissuer::last_ = EntityId("T", 0);

// Remembers the last id that already exists.
void transactionidissuer::seed(const EntityId& lastissued)
{
    last_ = lastissued;
}

// Pre-increment gives the next number, and the stored id keeps that value.
EntityId transactionidissuer::issuenext()
{
    ++last_;
    return last_;
}