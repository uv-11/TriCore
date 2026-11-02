#include "Datamanager.h"
#include "Exceptions.h"
#include "fixtures/load_edge_cases_fixture.h"

#include <iostream>
#include <string>

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

static const string PREFIX = "Phase2/tests/fixtures/temp_dm_load_";

int main()
{
    cout << "--- test_datamanager_load ---\n";

    // 1. Missing file throws DataException
    {
        teardown_load_edge_cases(PREFIX);
        datamanager dm(PREFIX);
        dataset data;
        list<string> warnings;
        bool threw = false;
        try
        {
            dm.loadall(data, warnings);
        }
        catch (const DataException&)
        {
            threw = true;
        }
        check(threw, "L1: loading from non-existent directory throws DataException");
    }

    // 2. Corrupt header in donors.csv throws DataException with line 1
    {
        setup_load_edge_cases(PREFIX);
        writefile(PREFIX + "donors.csv", "bad_header,name,type,node_id\nDN001,Hall,CATERER,0\n");
        datamanager dm(PREFIX);
        dataset data;
        list<string> warnings;
        bool threw = false;
        try
        {
            dm.loadall(data, warnings);
        }
        catch (const DataException& e)
        {
            threw = (string(e.what()).find("line 1") != string::npos);
        }
        check(threw, "L2: corrupt header in donors.csv throws DataException naming line 1");
        teardown_load_edge_cases(PREFIX);
    }

    // 3. Donation referencing non-existent donor throws DataException with (V2)
    {
        setup_load_edge_cases(PREFIX);
        writefile(PREFIX + "donations.csv",
                  "donation_id,donor_id,food_type,original_quantity,remaining_quantity,usable_minutes,status\n"
                  "DON001,DN999,COOKED,50,50,60,PENDING\n");
        datamanager dm(PREFIX);
        dataset data;
        list<string> warnings;
        bool threw = false;
        try
        {
            dm.loadall(data, warnings);
        }
        catch (const DataException& e)
        {
            threw = (string(e.what()).find("is not in donors.csv") != string::npos);
        }
        check(threw, "L3: donation with missing donor throws DataException");
        teardown_load_edge_cases(PREFIX);
    }

    // 4. Invalid food type enum throws DataException
    {
        setup_load_edge_cases(PREFIX);
        writefile(PREFIX + "donations.csv",
                  "donation_id,donor_id,food_type,original_quantity,remaining_quantity,usable_minutes,status\n"
                  "DON001,DN001,PIZZA,50,50,60,PENDING\n");
        datamanager dm(PREFIX);
        dataset data;
        list<string> warnings;
        bool threw = false;
        try
        {
            dm.loadall(data, warnings);
        }
        catch (const DataException&)
        {
            threw = true;
        }
        check(threw, "L4: unknown food type throws DataException");
        teardown_load_edge_cases(PREFIX);
    }

    // 5. Duplicate recipient ID throws DataException with (V1)
    {
        setup_load_edge_cases(PREFIX);
        writefile(PREFIX + "recipients.csv",
                  "recipient_id,name,type,available_capacity,accepted_types,accepting,can_pickup,node_id\n"
                  "R001,Shelter A,SHELTER,100,COOKED,1,1,5\n"
                  "R001,Shelter B,SHELTER,100,COOKED,1,1,6\n");
        datamanager dm(PREFIX);
        dataset data;
        list<string> warnings;
        bool threw = false;
        try
        {
            dm.loadall(data, warnings);
        }
        catch (const DataException& e)
        {
            threw = (string(e.what()).find("is used twice") != string::npos);
        }
        check(threw, "L5: duplicate recipient ID throws DataException");
        teardown_load_edge_cases(PREFIX);
    }

    // 6. Valid complete dataset loads cleanly with zero errors
    {
        setup_load_edge_cases(PREFIX);
        datamanager dm(PREFIX);
        dataset data;
        list<string> warnings;
        bool success = true;
        try
        {
            dm.loadall(data, warnings);
        }
        catch (...)
        {
            success = false;
        }
        check(success && data.donors.size() == 2 && data.recipients.size() == 7,
              "L6: valid fixture loads cleanly with matching counts");
        teardown_load_edge_cases(PREFIX);
    }

    cout << "Total FAIL: " << failcount << "\n";
    return failcount > 0 ? 1 : 0;
}
