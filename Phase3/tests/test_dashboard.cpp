// test_dashboard: the Home numbers on the rich data set (two donors, seven recipients, five donations
// that are waiting, partly given, closed, completed and waiting, three transactions, six offers),
// on the standard fixture without offers, and for people that do not exist.

#include "Dashboard.h"
#include "Datamanager.h"
#include "OfferStore.h"
#include "testkit.h"

#include <list>
#include <string>

using namespace std;

// Runs every check of this test.
int main()
{
    string folder = testfolder("dashboard");
    writerichfixture(folder);

    datamanager files(folder);
    dataset data;
    offerstore store;
    list<string> warnings;
    files.loadall(data, warnings);
    store.load(folder, warnings);

    check(files.checkinvariants(data).empty(), "the test data is consistent");

    // DON001, DON002 and DON005 are open; DON001 has 40 free, DON002 50, DON005 30
    coordinatorstats all = coordinatorstatsof(data, store);
    check(all.opendonations == 3, "coordinator: 3 open donations");
    check(all.waiting == 3, "coordinator: 3 donations wait for an offer");
    check(all.pendingoffers == 3, "coordinator: 3 offers are pending");
    check(all.givenout == 130, "coordinator: 130 given out");
    check(all.wasted == 40, "coordinator: 40 wasted (DON003 is closed)");

    // an offer for the whole free part of DON005 means it no longer waits for an offer
    EntityId don005("DON", 5);
    EntityId r002("R", 2);
    store.add(don005, r002, 30, 9);
    coordinatorstats full = coordinatorstatsof(data, store);
    check(full.opendonations == 3, "coordinator: still 3 open donations after the offer");
    check(full.waiting == 2, "coordinator: only 2 donations wait for an offer now");
    check(full.pendingoffers == 4, "coordinator: 4 offers are pending now");

    // a donor
    donorstats dn001 = donorstatsof(data, store, EntityId("DN", 1));
    check(dn001.donations == 3, "DN001: 3 donations");
    check(dn001.opendonations == 1, "DN001: 1 open donation (DON005)");
    check(dn001.pendingoffers == 2, "DN001: 2 pending offers (the two for DON005)");
    check(dn001.givenout == 100, "DN001: 100 given out");

    donorstats dn002 = donorstatsof(data, store, EntityId("DN", 2));
    check(dn002.donations == 2, "DN002: 2 donations");
    check(dn002.opendonations == 2, "DN002: 2 open donations");
    check(dn002.pendingoffers == 2, "DN002: 2 pending offers (OFF001 and OFF006)");
    check(dn002.givenout == 30, "DN002: 30 given out");

    donorstats nobody = donorstatsof(data, store, EntityId("DN", 9));
    check(nobody.donations == 0 && nobody.opendonations == 0 && nobody.pendingoffers == 0 && nobody.givenout == 0,
          "an unknown donor gives zeros");

    // a recipient
    recipientstats r001 = recipientstatsof(data, store, EntityId("R", 1));
    check(r001.incoming == 1, "R001: 1 incoming offer");
    check(r001.received == 30, "R001: 30 received");

    recipientstats r002stats = recipientstatsof(data, store, r002);
    check(r002stats.incoming == 2, "R002: 2 incoming offers after the new one");
    check(r002stats.received == 100, "R002: 100 received");

    recipientstats r007 = recipientstatsof(data, store, EntityId("R", 7));
    check(r007.incoming == 0 && r007.received == 0, "R007 has nothing");

    recipientstats unknown = recipientstatsof(data, store, EntityId("R", 99));
    check(unknown.incoming == 0 && unknown.received == 0, "an unknown recipient gives zeros");

    // the standard fixture: two waiting donations, no offers, no transactions
    string plainfolder = testfolder("dashboard_plain");
    writefixture(plainfolder);
    datamanager plainfiles(plainfolder);
    dataset plain;
    offerstore none;
    plainfiles.loadall(plain, warnings);
    coordinatorstats start = coordinatorstatsof(plain, none);
    check(start.opendonations == 2 && start.waiting == 2 && start.pendingoffers == 0 && start.givenout == 0 &&
              start.wasted == 0,
          "standard fixture: 2 open, 2 waiting, nothing else");

    // an empty data set
    dataset empty;
    offerstore noneatall;
    coordinatorstats zero = coordinatorstatsof(empty, noneatall);
    check(zero.opendonations == 0 && zero.waiting == 0 && zero.pendingoffers == 0 && zero.givenout == 0 &&
              zero.wasted == 0,
          "an empty data set gives zeros");

    return finish();
}
