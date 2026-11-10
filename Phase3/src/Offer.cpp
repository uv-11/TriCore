// Offer.cpp: the offer record, its status text and the offer id issuer.

#include "Offer.h"
#include "Exceptions.h"

#include <sstream>

using namespace std;

// Turns each status into the text that is stored in offers.csv.
string offerstatustostr(OfferStatus status)
{
    if (status == OFFER_PENDING)
    {
        return "PENDING";
    }

    if (status == OFFER_ACCEPTED)
    {
        return "ACCEPTED";
    }

    if (status == OFFER_DECLINED)
    {
        return "DECLINED";
    }

    if (status == OFFER_WITHDRAWN)
    {
        return "WITHDRAWN";
    }

    throw StructureException("Invalid OfferStatus value");
}

// Turns the stored text back into a status.
OfferStatus strtoofferstatus(const string& text)
{
    if (text == "PENDING")
    {
        return OFFER_PENDING;
    }

    if (text == "ACCEPTED")
    {
        return OFFER_ACCEPTED;
    }

    if (text == "DECLINED")
    {
        return OFFER_DECLINED;
    }

    if (text == "WITHDRAWN")
    {
        return OFFER_WITHDRAWN;
    }

    throw FormatException("Unknown offer status: " + text);
}

// The first id issued after a fresh start is OFF001.
EntityId offeridissuer::last_ = EntityId("OFF", 0);

// Remembers the last id that already exists.
void offeridissuer::seed(const EntityId& lastissued)
{
    last_ = lastissued;
}

// Pre-increment gives the next number, and the stored id keeps that value.
EntityId offeridissuer::issuenext()
{
    ++last_;
    return last_;
}

// Stores every value. The transaction id stays empty for an offer that is not accepted.
Offer::Offer(const EntityId& id, const EntityId& donation, const EntityId& recipient, int qty,
             int travel, OfferStatus status, const EntityId& txn)
{
    id_ = id;
    donation_ = donation;
    recipient_ = recipient;
    qty_ = qty;
    travel_ = travel;
    status_ = status;
    txn_ = txn;
}

// Returns the offer id.
const EntityId& Offer::id() const
{
    return id_;
}

// Returns the donation id.
const EntityId& Offer::donationId() const
{
    return donation_;
}

// Returns the recipient id.
const EntityId& Offer::recipientId() const
{
    return recipient_;
}

// Returns the offered quantity.
int Offer::quantity() const
{
    return qty_;
}

// Returns the travel minutes.
int Offer::travelMinutes() const
{
    return travel_;
}

// Returns the status.
OfferStatus Offer::status() const
{
    return status_;
}

// The empty id (no prefix, number 0) means there is no transaction.
bool Offer::hastransaction() const
{
    return !(txn_ == EntityId());
}

// Returns the transaction id.
const EntityId& Offer::transactionId() const
{
    return txn_;
}

// Only a pending offer can be accepted. The status is changed after the check, never before.
void Offer::markaccepted(const EntityId& txn)
{
    if (status_ != OFFER_PENDING)
    {
        throw StructureException("Only a pending offer can be accepted: " + id_.toString());
    }

    txn_ = txn;
    status_ = OFFER_ACCEPTED;
}

// Only a pending offer can be declined.
void Offer::markdeclined()
{
    if (status_ != OFFER_PENDING)
    {
        throw StructureException("Only a pending offer can be declined: " + id_.toString());
    }

    status_ = OFFER_DECLINED;
}

// Only a pending offer can be withdrawn.
void Offer::markwithdrawn()
{
    if (status_ != OFFER_PENDING)
    {
        throw StructureException("Only a pending offer can be withdrawn: " + id_.toString());
    }

    status_ = OFFER_WITHDRAWN;
}

// Builds the csv row. Without a transaction the row ends with a comma and an empty field.
string Offer::toCsvRow() const
{
    stringstream text;
    text << id_.toString() << "," << donation_.toString() << "," << recipient_.toString() << ","
         << qty_ << "," << travel_ << "," << offerstatustostr(status_) << ",";

    if (hastransaction())
    {
        text << txn_.toString();
    }

    return text.str();
}
