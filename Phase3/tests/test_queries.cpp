// test_queries: every function of Queries.h on a data set with two donors, seven recipients,
// five donations, three transactions and six offers.

#include "Datamanager.h"
#include "Exceptions.h"
#include "OfferStore.h"
#include "Queries.h"
#include "testkit.h"

#include <list>
#include <string>
#include <vector>

using namespace std;

// Turns a list of positions into text such as "0,1,5", so it is easy to compare.
string listtext(const vector<int>& positions)
{
    string text = "";

    for (size_t pos = 0; pos < positions.size(); pos++)
    {
        if (pos > 0)
        {
            text = text + ",";
        }

        char number[16];
        sprintf(number, "%d", positions[pos]);
        text = text + number;
    }

    return text;
}

// Runs every check of this test.
int main()
{
    string folder = testfolder("queries");
    writerichfixture(folder);

    datamanager files(folder);
    dataset data;
    offerstore store;
    list<string> warnings;
    files.loadall(data, warnings);
    store.load(folder, warnings);

    check(files.checkinvariants(data).empty(), "the test data is consistent");
    check(store.count() == 6, "six offers loaded");

    EntityId dn001("DN", 1);
    EntityId dn002("DN", 2);
    EntityId r001("R", 1);
    EntityId r002("R", 2);
    EntityId r003("R", 3);
    EntityId r007("R", 7);

    check(listtext(donationsofdonor(data, dn002)) == "0,1", "donations of DN002");
    check(listtext(donationsofdonor(data, dn001)) == "2,3,4", "donations of DN001");
    check(donationsofdonor(data, EntityId("DN", 9)).empty(), "donations of an unknown donor");

    check(listtext(transactionsofdonor(data, dn002)) == "0", "transactions of DN002");
    check(listtext(transactionsofdonor(data, dn001)) == "1,2", "transactions of DN001");
    check(transactionsofdonor(data, EntityId("DN", 9)).empty(), "transactions of an unknown donor");

    check(listtext(transactionsofrecipient(data, r001)) == "0", "transactions of R001");
    check(listtext(transactionsofrecipient(data, r002)) == "1,2", "transactions of R002");
    check(transactionsofrecipient(data, r003).empty(), "transactions of R003");

    check(listtext(offersfordonor(data, store, dn002)) == "0,1,5", "offers for the donations of DN002");
    check(listtext(offersfordonor(data, store, dn001)) == "2,3,4", "offers for the donations of DN001");
    check(offersfordonor(data, store, EntityId("DN", 9)).empty(), "offers for an unknown donor");

    check(listtext(offersforrecipient(store, r001, true)) == "0", "pending offers of R001");
    check(listtext(offersforrecipient(store, r001, false)) == "0,1", "all offers of R001");
    check(listtext(pastoffersforrecipient(store, r001)) == "1", "past offers of R001");
    check(listtext(offersforrecipient(store, r002, true)) == "4", "pending offers of R002");
    check(listtext(offersforrecipient(store, r002, false)) == "2,3,4", "all offers of R002");
    check(listtext(pastoffersforrecipient(store, r002)) == "2,3", "past offers of R002");
    check(listtext(offersforrecipient(store, r003, true)) == "5", "pending offers of R003");
    check(pastoffersforrecipient(store, r003).empty(), "R003 has no past offers");
    check(offersforrecipient(store, r007, false).empty(), "R007 has no offers");

    check(listtext(allpositions(4)) == "0,1,2,3", "allpositions(4)");
    check(allpositions(0).empty(), "allpositions(0) is empty");

    totals sums = computetotals(data);
    check(sums.offered == 350, "offered is 350");
    check(sums.allocated == 130, "allocated is 130");
    check(sums.wasted == 40, "wasted is 40");
    check(sums.stillopen == 180, "still open is 180");
    check(sums.allocated + sums.wasted + sums.stillopen == sums.offered, "allocated + wasted + still open = offered");
    check(sums.byfood[COOKED] == 30, "cooked given out is 30");
    check(sums.byfood[PACKAGED] == 0, "packaged given out is 0");
    check(sums.byfood[PRODUCE] == 100, "produce given out is 100");
    check(sums.byfood[BAKERY] == 0, "bakery given out is 0");

    // the sample data must add up in the same way
    string emptyfolder = testfolder("queries_empty");
    writefixture(emptyfolder);
    datamanager emptyfiles(emptyfolder);
    dataset plain;
    emptyfiles.loadall(plain, warnings);
    totals plainsums = computetotals(plain);
    check(plainsums.offered == 160 && plainsums.allocated == 0 && plainsums.wasted == 0 && plainsums.stillopen == 160,
          "totals of the standard fixture");

    return finish();
}
