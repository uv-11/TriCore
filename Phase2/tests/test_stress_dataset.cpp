#include "Datamanager.h"
#include "Duration.h"
#include "EntityId.h"
#include "Exceptions.h"
#include "Models.h"
#include "RecommendationEngine.h"
#include "traveltime.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

static int failcount = 0;

static void check(bool condition, const string& name)
{
    if (condition)
    {
        cout << "PASS " << name << "\n";
    }
    else
    {
        cout << "FAIL " << name << "\n";
        failcount++;
    }
}

static const string STRESS_DIR = "Phase2/data_check/";

int main()
{
    cout << "--- test_stress_dataset ---\n";

    // 1. Load the real dataset directly from Phase2/data_check/ via DataManager (read-only)
    // Real counts from Phase2/data_check/:
    //   donors.csv: 12 data rows (DN001 to DN012)
    //   recipients.csv: 26 data rows (R001 to R026)
    //   donations.csv: 46 data rows (DON001 to DON046)
    //   graph_nodes.csv: 20 data rows (nodes 0 to 19)
    datamanager dm(STRESS_DIR);
    dataset data;
    list<string> warnings;

    bool loadSuccess = true;
    try
    {
        dm.loadall(data, warnings);
    }
    catch (const exception& e)
    {
        cout << "Load error: " << e.what() << "\n";
        loadSuccess = false;
    }

    check(loadSuccess && data.donors.size() == 12 && data.recipients.size() == 26 &&
          data.donations.size() == 46 && data.network.nodecount() == 20,
          "STRESS1: loads with zero errors (12 donors, 26 recipients, 46 donations, 20 map locations)");

    // 2. Startup reachability warnings
    // Real values:
    //   recipients.csv: "R024,Mayur Vihar Isolated Shelter,SHELTER,120,COOKED;PACKAGED,1,1,16"
    //   recipients.csv: "R025,Patparganj Isolated Food Bank,FOOD_BANK,150,PACKAGED;BAKERY;PRODUCE,1,1,17"
    //   recipients.csv: "R026,Najafgarh Village Relief Centre,RELIEF_POINT,80,COOKED;PRODUCE,1,1,19"
    //   graph_edges.csv: the only road at nodes 16 and 17 is "16,17,7" (no donor is at 16 or 17)
    //   graph_edges.csv: "18,19,5" joins node 19 to node 18
    //   donors.csv: "DN011,Najafgarh Rural Kitchen,CANTEEN,18" (located at node 18)
    // R024 (node 16) and R025 (node 17) are on an island with no donor, so each gets a startup
    // warning: exactly 2 warnings. R026 (node 19) is joined to DN011 at node 18, so it gets none.
    int unreachableCount = 0;
    bool foundR024 = false;
    bool foundR025 = false;
    bool foundR026 = false;

    for (list<string>::const_iterator it = warnings.begin(); it != warnings.end(); ++it)
    {
        if (it->find("can never be recommended") != string::npos)
        {
            unreachableCount++;
            if (it->find("R024 (node 16)") != string::npos)
            {
                foundR024 = true;
            }
            if (it->find("R025 (node 17)") != string::npos)
            {
                foundR025 = true;
            }
            if (it->find("R026") != string::npos)
            {
                foundR026 = true;
            }
        }
    }

    check(unreachableCount == 2 && foundR024 && foundR025 && !foundR026,
          "STRESS2: exactly 2 recipients reported unreachable at startup (R024 at node 16 and R025 at node 17, not R026)");

    // Initialize travel time provider from network
    dijkstratraveltime provider(data.network);

    // 3. Abnormality 1: Quantity 50000 donation (DON041) from DN009 ('Tilak Nagar Wholesale Mart')
    // Real row from Phase2/data_check/donations.csv:
    //   "DON041,DN009,PRODUCE,50000,50000,60,PENDING"
    // Real row from Phase2/data_check/donors.csv:
    //   "DN009,Tilak Nagar Wholesale Mart,MARKET,11"
    int pos50k = -1;
    for (size_t i = 0; i < data.donations.size(); ++i)
    {
        if (data.donations[i].id() == EntityId("DON", 41) &&
            data.donations[i].originalQuantity() == 50000)
        {
            pos50k = (int)i;
            break;
        }
    }
    check(pos50k != -1, "STRESS3: found quantity 50000 donation DON041 from donor DN009 ('Tilak Nagar Wholesale Mart')");
    if (pos50k != -1)
    {
        const Donation& don50k = data.donations[pos50k];
        stringstream ss;
        ss << don50k.toCsvRow();
        check(don50k.originalQuantity() == 50000 && don50k.remainingQuantity() == 50000 &&
              ss.str().find("50000,50000") != string::npos,
              "STRESS3: quantity 50000 math does not overflow and serializes to CSV correctly");
    }

    // 4. Abnormality 2: Usable minutes 999999999 donation (DON042) from DN010 ('Connaught Place Hotel Grand')
    // Real row from Phase2/data_check/donations.csv:
    //   "DON042,DN010,PACKAGED,500,500,999999999,PENDING"
    // Real row from Phase2/data_check/donors.csv:
    //   "DN010,Connaught Place Hotel Grand,HOTEL,6"
    int posHugeMins = -1;
    for (size_t i = 0; i < data.donations.size(); ++i)
    {
        if (data.donations[i].id() == EntityId("DON", 42) &&
            data.donations[i].usableMinutes() == 999999999)
        {
            posHugeMins = (int)i;
            break;
        }
    }
    check(posHugeMins != -1, "STRESS4: found usable_minutes 999999999 donation DON042 from donor DN010 ('Connaught Place Hotel Grand')");
    if (posHugeMins != -1)
    {
        const Donation& donHuge = data.donations[posHugeMins];
        int donorPos = 0;
        data.donorindex.get(donHuge.donorId().toString(), donorPos);
        int donorNode = data.donors[donorPos].nodeId();
        vector<Duration> travelTimes = provider.traveltimesfrom(donorNode);

        RecommendationResult res = recommend(donHuge, data.recipients, travelTimes);
        bool hasTooFar = false;
        for (size_t r = 0; r < res.rejected().size(); ++r)
        {
            if (res.rejected()[r].reason() == TOO_FAR)
            {
                hasTooFar = true;
            }
        }
        check(!hasTooFar && !res.candidates().empty(),
              "STRESS4: usable_minutes 999999999 donation is never rejected as TOO_FAR against any real finite travel time");
    }

    // 5. Abnormality 3: Boundary donation whose usable_minutes equals exact travel time
    // Real row from Phase2/data_check/donations.csv:
    //   "DON005,DN002,COOKED,30,30,25,PENDING" (usable_minutes = 25)
    // Real row from Phase2/data_check/donors.csv:
    //   "DN002,Hotel Rajdhani Kitchen Preet Vihar,HOTEL,1" (node 1)
    // Real row from Phase2/data_check/recipients.csv:
    //   "R006,Dastarkhwan Soup Kitchen Jangpura,COMMUNITY_KITCHEN,150,COOKED;PACKAGED;PRODUCE;BAKERY,1,1,15" (node 15)
    //   "R010,Lodhi Colony Senior Care Mess,CARE_HOME,90,COOKED;PACKAGED;PRODUCE;BAKERY,1,1,10" (node 10)
    // Shortest path from node 1 to node 15: 1->3 (4 min) + 3->6 (5 min) + 6->15 (16 min) = 25 min == usable 25 min.
    // Shortest path from node 1 to node 10: 1->3 (4 min) + 3->6 (5 min) + 6->4 (9 min) + 4->10 (8 min) = 26 min > usable 25 min.
    int posBoundary = -1;
    for (size_t i = 0; i < data.donations.size(); ++i)
    {
        if (data.donations[i].id() == EntityId("DON", 5) &&
            data.donations[i].usableMinutes() == 25)
        {
            posBoundary = (int)i;
            break;
        }
    }
    check(posBoundary != -1, "STRESS5: found boundary donation DON005 from DN002 with usable 25 min");
    if (posBoundary != -1)
    {
        const Donation& donBound = data.donations[posBoundary];
        int donorPos = 0;
        data.donorindex.get(donBound.donorId().toString(), donorPos);
        int donorNode = data.donors[donorPos].nodeId();
        vector<Duration> travelTimes = provider.traveltimesfrom(donorNode);

        RecommendationResult res = recommend(donBound, data.recipients, travelTimes);
        bool r006Accepted = false;
        bool r010OffByOne = false;
        for (size_t c = 0; c < res.candidates().size(); ++c)
        {
            if (res.candidates()[c].recipientId() == EntityId("R", 6))
            {
                r006Accepted = (res.candidates()[c].travel().minutes() == 25);
            }
        }
        for (size_t r = 0; r < res.rejected().size(); ++r)
        {
            if (res.rejected()[r].recipientId() == EntityId("R", 10) &&
                res.rejected()[r].reason() == TOO_FAR &&
                res.rejected()[r].travel().minutes() == 26)
            {
                r010OffByOne = true;
            }
        }
        check(r006Accepted && r010OffByOne,
              "STRESS5: boundary donation's exact-equal recipient R006 (travel 25 == usable 25) is accepted, off-by-one R010 (travel 26 > usable 25) is rejected as TOO_FAR");
    }

    // 6. Abnormality 4 & 5: Isolated donor DN011 ('Najafgarh Rural Kitchen', node 18)
    // connected only to node 19 via edge 18-19 (5 min).
    // Real row from Phase2/data_check/donors.csv:
    //   "DN011,Najafgarh Rural Kitchen,CANTEEN,18"
    // Real rows from Phase2/data_check/donations.csv:
    //   "DON044,DN011,COOKED,50,50,30,PENDING"
    //   "DON045,DN011,BAKERY,40,40,30,PENDING"
    // Real rows from Phase2/data_check/recipients.csv:
    //   "R024,Mayur Vihar Isolated Shelter,SHELTER,120,COOKED;PACKAGED,1,1,16" (node 16, no BAKERY)
    //   "R025,Patparganj Isolated Food Bank,FOOD_BANK,150,PACKAGED;BAKERY;PRODUCE,1,1,17" (node 17, no COOKED)
    //   "R026,Najafgarh Village Relief Centre,RELIEF_POINT,80,COOKED;PRODUCE,1,1,19" (node 19, 5 min from DN011, no BAKERY)
    // So from node 18 only R026 can be reached. It takes the COOKED donation (the only candidate)
    // and cannot take the BAKERY donation (no candidate at all).
    int posIso1 = -1; // DON044: COOKED, quantity 50, usable 30 min from DN011 (node 18)
    int posIso2 = -1; // DON045: BAKERY, quantity 40, usable 30 min from DN011 (node 18)

    for (size_t i = 0; i < data.donations.size(); ++i)
    {
        if (data.donations[i].id() == EntityId("DON", 44)) posIso1 = (int)i;
        if (data.donations[i].id() == EntityId("DON", 45)) posIso2 = (int)i;
    }

    check(posIso1 != -1 && posIso2 != -1,
          "STRESS6: found both isolated-donor donations (DON044 COOKED and DON045 BAKERY) from DN011 ('Najafgarh Rural Kitchen')");

    if (posIso1 != -1)
    {
        const Donation& donIso1 = data.donations[posIso1];
        int donorPos = 0;
        data.donorindex.get(donIso1.donorId().toString(), donorPos);
        int donorNode = data.donors[donorPos].nodeId();
        vector<Duration> travelTimes = provider.traveltimesfrom(donorNode);

        RecommendationResult res1 = recommend(donIso1, data.recipients, travelTimes);

        // Real outcome for DON044 (COOKED): R025 accepts no COOKED (FOOD_INCOMPATIBLE, checked
        // before reach). 12 recipients pass the first four rules but have no road from node 18:
        // R001 R003 R004 R005 R006 R007 R008 R010 R012 R013 R014 R024. The rest fail earlier rules.
        int unreachableCountIso1 = 0;
        bool r025IncompatibleIso1 = false;
        bool r024UnreachableIso1 = false;
        for (size_t r = 0; r < res1.rejected().size(); ++r)
        {
            if (res1.rejected()[r].reason() == UNREACHABLE) unreachableCountIso1++;
            if (res1.rejected()[r].recipientId() == EntityId("R", 25) &&
                res1.rejected()[r].reason() == FOOD_INCOMPATIBLE)
            {
                r025IncompatibleIso1 = true;
            }
            if (res1.rejected()[r].recipientId() == EntityId("R", 24) &&
                res1.rejected()[r].reason() == UNREACHABLE)
            {
                r024UnreachableIso1 = true;
            }
        }

        check(res1.candidates().size() == 1 &&
              res1.candidates()[0].recipientId() == EntityId("R", 26) &&
              res1.candidates()[0].travel().minutes() == 5 &&
              r025IncompatibleIso1 && r024UnreachableIso1 &&
              unreachableCountIso1 == 12,
              "STRESS6a: isolated donor COOKED donation (DON044) recommends only R026 (5 min); R025 is FOOD_INCOMPATIBLE, R024 and 11 other recipients are UNREACHABLE");
    }

    if (posIso2 != -1)
    {
        const Donation& donIso2 = data.donations[posIso2];
        int donorPos = 0;
        data.donorindex.get(donIso2.donorId().toString(), donorPos);
        int donorNode = data.donors[donorPos].nodeId();
        vector<Duration> travelTimes = provider.traveltimesfrom(donorNode);

        RecommendationResult res2 = recommend(donIso2, data.recipients, travelTimes);

        // Real outcome for DON045 (BAKERY): nobody is a candidate. R024 and R026 accept no BAKERY
        // (FOOD_INCOMPATIBLE). 7 recipients take BAKERY and pass the first four rules but have no
        // road from node 18: R002 R004 R005 R008 R010 R014 R025.
        bool r024Incompatible = false;
        bool r026Incompatible = false;
        bool r025Unreachable = false;
        int unreachableCountIso2 = 0;

        for (size_t r = 0; r < res2.rejected().size(); ++r)
        {
            if (res2.rejected()[r].recipientId() == EntityId("R", 24) &&
                res2.rejected()[r].reason() == FOOD_INCOMPATIBLE)
            {
                r024Incompatible = true;
            }
            if (res2.rejected()[r].recipientId() == EntityId("R", 26) &&
                res2.rejected()[r].reason() == FOOD_INCOMPATIBLE)
            {
                r026Incompatible = true;
            }
            if (res2.rejected()[r].recipientId() == EntityId("R", 25) &&
                res2.rejected()[r].reason() == UNREACHABLE)
            {
                r025Unreachable = true;
            }
            if (res2.rejected()[r].reason() == UNREACHABLE)
            {
                unreachableCountIso2++;
            }
        }

        check(res2.candidates().empty() &&
              r024Incompatible && r026Incompatible && r025Unreachable &&
              unreachableCountIso2 == 7,
              "STRESS6b: isolated donor BAKERY donation (DON045) has no candidate; R024 and R026 are FOOD_INCOMPATIBLE, R025 and 6 other BAKERY takers are UNREACHABLE");
    }

    cout << "Total FAIL: " << failcount << "\n";
    return failcount > 0 ? 1 : 0;
}
