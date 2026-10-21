#ifndef ALLOCATIONMANAGER_H
#define ALLOCATIONMANAGER_H

#include "Duration.h"
#include "EntityId.h"
#include "Models.h"
#include "priorityqueue.h"
#include <string>
#include <vector>

using namespace std;

// The result of an allocation try. These are expected user mistakes, so they are result
// codes and not exceptions.
enum allocerror
{
    ALLOC_OK,
    DONATION_NOT_OPEN,
    BAD_QUANTITY,
    EXCEEDS_REMAINING,
    EXCEEDS_CAPACITY,
    NOT_FEASIBLE
};

// Returns a short sentence that explains an allocation code to the coordinator.
string allocerrormessage(allocerror error);

// What allocate returns: the code and, only when the code is ALLOC_OK, the new transaction.
struct allocationresult
{
    allocerror error;
    Transaction transaction;
};

// Places quantity of the donation at the recipient. Every check runs first and nothing is
// changed unless all of them pass (Spec 7.3). On success the donation and the recipient
// are updated and the transaction carries the id that was given.
allocationresult allocate(Donation& donation, Recipient& recipient, int quantity,
                          const Duration& travel, const EntityId& txnid);

// Closes an open donation. The leftover quantity stays stored as the wasted amount.
// Returns DONATION_NOT_OPEN if the donation is already completed or closed.
allocerror closedonation(Donation& donation);

// Orders donations by urgency: smaller usable minutes first, then smaller donation number.
class urgencyqueue
{
public:
    // Adds a donation. index is its position in the donation array.
    void push(const Donation& donation, int index);

    // Checks whether no donation is waiting.
    bool empty() const;

    // Removes the most urgent donation and returns its index.
    // Throws StructureException when the queue is empty.
    int popmosturgent();

private:
    // Heap with entries (usable minutes, donation number, index)
    priorityqueue heap_;
};

// Puts every open donation (PENDING or PARTIAL) in the queue. The index of a donation is
// its position in the vector. Completed and closed donations are left out.
void fillurgencyqueue(const vector<Donation>& donations, urgencyqueue& queue);

// Hands out transaction ids T001, T002, ... A static member keeps the last id, so numbers
// are never reused. After a restart the last id from transactions.csv is given to seed.
class transactionidissuer
{
public:
    // Sets the last id that was already issued.
    static void seed(const EntityId& lastissued);

    // Increases the last id by one with the ++ of EntityId and returns it.
    static EntityId issuenext();

private:
    // The last id that was issued
    static EntityId last_;
};

#endif