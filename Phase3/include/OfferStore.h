#ifndef OFFERSTORE_H
#define OFFERSTORE_H

// OfferStore.h: all offers in memory, loaded from and saved to offers.csv.

#include "Datamanager.h"
#include "EntityId.h"
#include "Offer.h"

#include <list>
#include <string>
#include <vector>

using namespace std;

// Keeps every offer. Copying a store copies the offers, which is how a failed save is undone.
class offerstore
{
public:
    // Reads prefix + "offers.csv". A missing file means no offers and nothing is created.
    // A bad row throws DataException with "offers.csv line <n>: " in front of the problem.
    // The store is not changed when loading fails. The offer id issuer is seeded afterwards.
    void load(const string& prefix, list<string>& warnings);

    // Saves all offers to offers.csv through the safe save of datamanager.
    void save(datamanager& files) const;

    // Returns how many offers there are.
    int count() const;

    // Returns the offer at a position. Throws StructureException when the position is wrong.
    const Offer& at(int pos) const;

    // Returns the offer at a position so it can be changed. Same exception as above.
    Offer& at(int pos);

    // Looks for an offer id. Returns true and sets pos when it exists.
    bool find(const EntityId& offerid, int& pos) const;

    // Adds a new pending offer with a new id and returns its position. Does not save.
    // Throws StructureException when the quantity is below 1.
    int add(const EntityId& donation, const EntityId& recipient, int qty, int travel);

    // Returns the total quantity of the pending offers of a donation.
    int pendingquantity(const EntityId& donation) const;

    // Checks whether the recipient declined an offer for this donation earlier.
    bool wasdeclined(const EntityId& donation, const EntityId& recipient) const;

    // Withdraws every pending offer of a donation and returns how many were changed.
    int withdrawfordonation(const EntityId& donation);

    // Compares the offers with the data and returns plain-language warnings (never changes anything).
    vector<string> reconcile(const dataset& data) const;

private:
    vector<Offer> offers_; // every offer, in file order
};

#endif
