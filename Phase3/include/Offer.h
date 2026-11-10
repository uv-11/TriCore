#ifndef OFFER_H
#define OFFER_H

// Offer.h: an offer from the coordinator to one recipient for part of a donation.
// An offer is soft: nothing is reserved until the recipient accepts it.

#include "EntityId.h"
#include "Models.h"

#include <string>

using namespace std;

// The life of an offer. PENDING is waiting for the recipient, the other three are final.
enum OfferStatus
{
    OFFER_PENDING,
    OFFER_ACCEPTED,
    OFFER_DECLINED,
    OFFER_WITHDRAWN
};

// Turns a status into its csv text (PENDING, ACCEPTED, DECLINED, WITHDRAWN).
string offerstatustostr(OfferStatus status);

// Turns csv text into a status. Throws FormatException for any other text.
OfferStatus strtoofferstatus(const string& text);

// Hands out offer ids OFF001, OFF002, ... A static member keeps the last id, so numbers are
// never reused. After a restart the highest id from offers.csv is given to seed.
class offeridissuer
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

// One row of offers.csv: offer_id,donation_id,recipient_id,quantity,travel_minutes,status,txn_id
class Offer : public CsvRecord
{
public:
    // Creates an offer. txn is the empty EntityId() when there is no transaction.
    Offer(const EntityId& id, const EntityId& donation, const EntityId& recipient, int qty,
          int travel, OfferStatus status, const EntityId& txn);

    // Returns the offer id.
    const EntityId& id() const;

    // Returns the id of the donation the food comes from.
    const EntityId& donationId() const;

    // Returns the id of the recipient that got the offer.
    const EntityId& recipientId() const;

    // Returns the offered quantity.
    int quantity() const;

    // Returns the travel time in minutes that was true when the offer was made.
    int travelMinutes() const;

    // Returns the status of the offer.
    OfferStatus status() const;

    // Checks whether the offer has a transaction (only an accepted offer has one).
    bool hastransaction() const;

    // Returns the id of the transaction (the empty id when there is none).
    const EntityId& transactionId() const;

    // Marks a pending offer as accepted and stores the transaction id.
    // Throws StructureException when the offer is not pending.
    void markaccepted(const EntityId& txn);

    // Marks a pending offer as declined. Throws StructureException when it is not pending.
    void markdeclined();

    // Marks a pending offer as withdrawn. Throws StructureException when it is not pending.
    void markwithdrawn();

    // Writes the offer as one csv row (the last field is empty when there is no transaction).
    string toCsvRow() const;

private:
    EntityId id_;         // offer id
    EntityId donation_;   // donation id
    EntityId recipient_;  // recipient id
    int qty_;             // offered quantity
    int travel_;          // travel minutes
    OfferStatus status_;  // current status
    EntityId txn_;        // transaction id, empty when there is none
};

#endif
