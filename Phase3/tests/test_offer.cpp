// test_offer: the Offer record, its status text, its csv row and the offer id issuer.

#include "Exceptions.h"
#include "Offer.h"
#include "testkit.h"

#include <string>

using namespace std;

// Returns true when the text conversion throws a FormatException.
bool statusthrows(const string& text)
{
    try
    {
        strtoofferstatus(text);
    }
    catch (const FormatException&)
    {
        return true;
    }

    return false;
}

// Returns true when markaccepted throws a StructureException.
bool acceptthrows(Offer& offer, const EntityId& txn)
{
    try
    {
        offer.markaccepted(txn);
    }
    catch (const StructureException&)
    {
        return true;
    }

    return false;
}

// Returns true when markdeclined throws a StructureException.
bool declinethrows(Offer& offer)
{
    try
    {
        offer.markdeclined();
    }
    catch (const StructureException&)
    {
        return true;
    }

    return false;
}

// Returns true when markwithdrawn throws a StructureException.
bool withdrawthrows(Offer& offer)
{
    try
    {
        offer.markwithdrawn();
    }
    catch (const StructureException&)
    {
        return true;
    }

    return false;
}

// Makes a pending offer OFF001 for DON001 and R003, 40 portions, 12 minutes.
Offer makepending()
{
    EntityId none;
    return Offer(EntityId("OFF", 1), EntityId("DON", 1), EntityId("R", 3), 40, 12, OFFER_PENDING, none);
}

// Runs every check of this test.
int main()
{
    // status text round trip
    check(offerstatustostr(OFFER_PENDING) == "PENDING", "pending text");
    check(offerstatustostr(OFFER_ACCEPTED) == "ACCEPTED", "accepted text");
    check(offerstatustostr(OFFER_DECLINED) == "DECLINED", "declined text");
    check(offerstatustostr(OFFER_WITHDRAWN) == "WITHDRAWN", "withdrawn text");
    check(strtoofferstatus("PENDING") == OFFER_PENDING, "pending parsed");
    check(strtoofferstatus("ACCEPTED") == OFFER_ACCEPTED, "accepted parsed");
    check(strtoofferstatus("DECLINED") == OFFER_DECLINED, "declined parsed");
    check(strtoofferstatus("WITHDRAWN") == OFFER_WITHDRAWN, "withdrawn parsed");
    check(statusthrows("MAYBE"), "unknown status text throws FormatException");
    check(statusthrows(""), "empty status text throws FormatException");
    check(statusthrows("pending"), "lower case status text throws FormatException");

    // a pending offer and its csv row
    Offer offer = makepending();
    check(offer.id().toString() == "OFF001", "offer id");
    check(offer.donationId().toString() == "DON001", "donation id");
    check(offer.recipientId().toString() == "R003", "recipient id");
    check(offer.quantity() == 40, "quantity");
    check(offer.travelMinutes() == 12, "travel minutes");
    check(offer.status() == OFFER_PENDING, "status is pending");
    check(!offer.hastransaction(), "a pending offer has no transaction");
    check(offer.toCsvRow() == "OFF001,DON001,R003,40,12,PENDING,", "pending row has an empty txn_id");

    // late binding through a CsvRecord pointer
    const CsvRecord* record = &offer;
    check(record->toCsvRow() == "OFF001,DON001,R003,40,12,PENDING,", "row through a CsvRecord pointer");

    // accepting
    offer.markaccepted(EntityId("T", 4));
    check(offer.status() == OFFER_ACCEPTED, "status is accepted after markaccepted");
    check(offer.hastransaction(), "an accepted offer has a transaction");
    check(offer.transactionId().toString() == "T004", "transaction id is kept");
    check(offer.toCsvRow() == "OFF001,DON001,R003,40,12,ACCEPTED,T004", "accepted row ends with the txn id");
    check(record->toCsvRow() == "OFF001,DON001,R003,40,12,ACCEPTED,T004", "accepted row through a CsvRecord pointer");

    // a second change of an accepted offer is refused and changes nothing
    check(acceptthrows(offer, EntityId("T", 9)), "accepting twice throws StructureException");
    check(offer.transactionId().toString() == "T004", "transaction id unchanged after the refused accept");
    check(declinethrows(offer), "declining an accepted offer throws StructureException");
    check(withdrawthrows(offer), "withdrawing an accepted offer throws StructureException");
    check(offer.status() == OFFER_ACCEPTED, "status unchanged after refused changes");

    // declining
    Offer second = makepending();
    second.markdeclined();
    check(second.status() == OFFER_DECLINED, "status is declined after markdeclined");
    check(second.toCsvRow() == "OFF001,DON001,R003,40,12,DECLINED,", "declined row has an empty txn_id");
    check(declinethrows(second), "declining twice throws StructureException");
    check(acceptthrows(second, EntityId("T", 1)), "accepting a declined offer throws StructureException");
    check(withdrawthrows(second), "withdrawing a declined offer throws StructureException");
    check(second.status() == OFFER_DECLINED && !second.hastransaction(), "declined offer unchanged");

    // withdrawing
    Offer third = makepending();
    third.markwithdrawn();
    check(third.status() == OFFER_WITHDRAWN, "status is withdrawn after markwithdrawn");
    check(third.toCsvRow() == "OFF001,DON001,R003,40,12,WITHDRAWN,", "withdrawn row has an empty txn_id");
    check(withdrawthrows(third), "withdrawing twice throws StructureException");
    check(acceptthrows(third, EntityId("T", 1)), "accepting a withdrawn offer throws StructureException");
    check(declinethrows(third), "declining a withdrawn offer throws StructureException");
    check(third.status() == OFFER_WITHDRAWN && !third.hastransaction(), "withdrawn offer unchanged");

    // the id issuer
    offeridissuer::seed(EntityId("OFF", 0));
    check(offeridissuer::issuenext().toString() == "OFF001", "first id is OFF001");
    check(offeridissuer::issuenext().toString() == "OFF002", "second id is OFF002");
    offeridissuer::seed(EntityId("OFF", 7));
    check(offeridissuer::issuenext().toString() == "OFF008", "after seed OFF007 the next id is OFF008");

    return finish();
}
