// allocationmanager.cpp: allocation, the urgency queue and the transaction id issuer.

// allocate checks everything first and changes state only when every check has passed,
// so a failed allocation leaves the donation and the recipient exactly as they were.
// Expected user mistakes are result codes (AllocError), not exceptions (Spec A21).

#include "allocationmanager.h"
#include "Exceptions.h"
#include "recommendationengine.h"

using namespace std;

// Turns each code into a sentence for the console.
string allocerrormessage(AllocError error)
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

    throw StructureException("Invalid AllocError value");
}

// Runs the five checks of Spec 7.3 in order and changes state only after all of them
// pass.
AllocError allocate(Donation& donation, Recipient& recipient, int quantity,
                          const Duration& travel, const EntityId& txnid)
{
    (void)txnid;
    // outcome is what allocate returns: an AllocError code.
    AllocError outcome = ALLOC_OK;
    // Assume success. The checks below set a code as soon as one of them fails.
    // outcome starts as ALLOC_OK.

    // Phase 1: find the first failing check. Nothing is changed in this phase. Check 1:
    // only an open donation can be allocated.
    if (donation.status() != PENDING && donation.status() != PARTIAL)
    {
        outcome = DONATION_NOT_OPEN;
    }
    // Check 2: quantity is the number of portions the coordinator wants to place.
    else if (quantity < 1)
    {
        outcome = BAD_QUANTITY;
    }
    // Check 3: not more than what is left of the donation.
    else if (quantity > donation.remainingQuantity())
    {
        outcome = EXCEEDS_REMAINING;
    }
    // Check 4: not more than the recipient can still take.
    else if (quantity > recipient.availableCapacity())
    {
        outcome = EXCEEDS_CAPACITY;
    }
    // Check 5: the recipient must still be feasible. travel is the time found earlier.
    else if (checkFeasibility(recipient, donation, travel) != OK)
    {
        outcome = NOT_FEASIBLE;
    }

    // Phase 2: only now are the donation and the recipient changed. A failure above never
    // reaches this point (test A9).
    if (outcome == ALLOC_OK)
    {
        // The donation has less left now, and its status becomes PARTIAL or COMPLETED.
        donation.allocateQuantity(quantity);
        // The recipient has less capacity left.
        recipient.reduceCapacity(quantity);
        // The transaction entry is created by the caller upon ALLOC_OK.
        (void)txnid;
        // state updated
    }

    return outcome;
}

// Closes the donation if it is still open.
AllocError closedonation(Donation& donation)
{
    if (donation.status() != PENDING && donation.status() != PARTIAL)
    {
        return DONATION_NOT_OPEN;
    }

    // close() sets the status to CLOSED. The leftover stays stored as the wasted amount.
    donation.close();
    return ALLOC_OK;
}

// Stores one heap entry. key1 is the usable time and key2 breaks ties by arrival.
void urgencyqueue::push(const Donation& donation, int index)
{
    // entry is one heap entry for this donation.
    heapitem entry;
    // key1 is the usable minutes and key2 is the donation number (Spec A12). Smaller
    // means more urgent, and the donation number makes the earlier arrival win a tie.
    entry.key1 = donation.usableMinutes();
    entry.key2 = donation.id().number();
    // payload is the position of the donation in the vector, which is what popmosturgent
    // gives back.
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
    // payload holds the index of the donation in the donations vector.
    return top.payload;
}

// Pushes only the donations that can still be allocated.
void fillurgencyqueue(const vector<Donation>& donations, urgencyqueue& queue)
{
    for (size_t pos = 0; pos < donations.size(); pos++)
    {
        if (donations[pos].status() == PENDING || donations[pos].status() == PARTIAL)
        {
            // pos is the position of the donation in the vector, so the queue can give it
            // back later.
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
    // EntityId has its own ++ that adds 1 to the number part (T001 becomes T002).
    ++last_;
    return last_;
}