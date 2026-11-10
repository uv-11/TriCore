// test_p2link: proves that the Phase 2 library, the include paths and the linking work.

#include "Datamanager.h"
#include "Exceptions.h"
#include "RecommendationEngine.h"
#include "allocationmanager.h"
#include "testkit.h"
#include "traveltime.h"

#include <list>
#include <string>
#include <vector>

using namespace std;

// Writes a tiny valid data set: two locations, one road, one donor, one recipient, one donation.
void writetiny(const string& folder)
{
    writefile(folder + "graph_nodes.csv", "node_id,label,x_km,y_km\n0,Donor spot,0,0\n1,Recipient spot,5,0\n");
    writefile(folder + "graph_edges.csv", "u,v,minutes\n0,1,10\n");
    writefile(folder + "donors.csv", "donor_id,name,type,node_id\nDN001,Test Donor,CATERER,0\n");
    writefile(folder + "recipients.csv",
              "recipient_id,name,type,available_capacity,accepted_types,accepting,can_pickup,node_id\n"
              "R001,Test Recipient,SHELTER,50,COOKED;PACKAGED,1,1,1\n");
    writefile(folder + "donations.csv",
              "donation_id,donor_id,food_type,original_quantity,remaining_quantity,usable_minutes,status\n"
              "DON001,DN001,COOKED,30,30,60,PENDING\n");
    writefile(folder + "transactions.csv", transactionstext());
}

// Loads the tiny data set, seeds the issuers and runs one recommendation.
int main()
{
    string folder = testfolder("p2link");
    writetiny(folder);

    datamanager files(folder);
    dataset data;
    list<string> warnings;
    files.loadall(data, warnings);

    check(data.donors.size() == 1, "one donor loaded");
    check(data.recipients.size() == 1, "one recipient loaded");
    check(data.donations.size() == 1, "one donation loaded");
    check(data.transactions.empty(), "no transactions loaded");
    check(data.network.nodecount() == 2, "two map locations loaded");
    check(warnings.empty(), "no start-up warnings");
    check(files.checkinvariants(data).empty(), "invariants are clean");

    transactionidissuer::seed(data.lasttransaction);
    donationidissuer::seed(data.donations[0].id());
    donoridissuer::seed(data.donors[0].id());
    recipientidissuer::seed(data.recipients[0].id());
    check(transactionidissuer::issuenext().toString() == "T001", "transaction issuer gives T001");
    check(donationidissuer::issuenext().toString() == "DON002", "donation issuer gives DON002");
    check(donoridissuer::issuenext().toString() == "DN002", "donor issuer gives DN002");
    check(recipientidissuer::issuenext().toString() == "R002", "recipient issuer gives R002");

    dijkstratraveltime provider(data.network);
    vector<Duration> travel = provider.traveltimesfrom(data.donors[0].nodeId());
    RecommendationResult result = recommend(data.donations[0], data.recipients, travel);

    check(result.candidates().size() == 1, "recommend finds one candidate");
    check(result.rejected().empty(), "recommend rejects nobody");

    if (result.candidates().size() == 1)
    {
        check(result.candidates()[0].recipientId().toString() == "R001", "candidate is R001");
        check(result.candidates()[0].potentialAcceptance() == 30, "acceptance is 30");
        check(result.candidates()[0].travel().minutes() == 10, "travel is 10 minutes");
        check(result.candidates()[0].unusedCapacity() == 20, "unused capacity is 20");
    }

    return finish();
}
