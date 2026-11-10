#ifndef TESTKIT_H
#define TESTKIT_H

// Small helpers shared by the Phase 3 tests: PASS/FAIL lines, test folders, file helpers and a
// standard set of six CSV files. The test data folder comes from the CB_TESTDATA definition.

#include <direct.h>
#include <io.h>
#include <sys/stat.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

#ifndef CB_TESTDATA
#define CB_TESTDATA "testdata/"
#endif

// Returns the counter of failed checks (a function, so no global variable is needed).
inline int& failcounter()
{
    static int count = 0;
    return count;
}

// Checks a condition and prints PASS or FAIL with the name.
inline void check(bool condition, const string& name)
{
    if (condition)
    {
        cout << "PASS " << name << "\n";
    }
    else
    {
        cout << "FAIL " << name << "\n";
        failcounter()++;
    }
}

// Prints the total and returns the exit code of the test program.
inline int finish()
{
    cout << "Total FAIL: " << failcounter() << "\n";

    if (failcounter() > 0)
    {
        return 1;
    }

    return 0;
}

// Writes text to a file byte for byte, so the test controls the line endings.
inline void writefile(const string& path, const string& text)
{
    ofstream out(path.c_str(), ios::binary);
    out << text;
}

// Reads a whole file into one string, with CR LF turned into LF. Empty when it cannot be opened.
inline string readfile(const string& path)
{
    ifstream in(path.c_str(), ios::binary);
    string text = "";
    string line;

    while (getline(in, line))
    {
        if (!line.empty() && line[line.length() - 1] == '\r')
        {
            line = line.substr(0, line.length() - 1);
        }

        text = text + line + "\n";
    }

    return text;
}

// Checks whether a file exists.
inline bool exists(const string& path)
{
    ifstream in(path.c_str());
    return in.is_open();
}

// Makes a file read-only (true) or writable again (false).
inline void setreadonly(const string& path, bool readonly)
{
    if (readonly)
    {
        _chmod(path.c_str(), _S_IREAD);
    }
    else
    {
        _chmod(path.c_str(), _S_IREAD | _S_IWRITE);
    }
}

// The names of every file a test folder can hold.
inline const char* const* knownfiles()
{
    static const char* const names[] = {"donors.csv", "recipients.csv", "graph_nodes.csv",
                                        "graph_edges.csv", "donations.csv",
                                        "transactions.csv", "offers.csv", NULL};
    return names;
}

// Removes every known file (and its .tmp) from a folder; read-only files are made writable first.
inline void clearfolder(const string& folder)
{
    const char* const* names = knownfiles();

    for (int pos = 0; names[pos] != NULL; pos++)
    {
        string path = folder + names[pos];
        setreadonly(path, false);
        remove(path.c_str());
        setreadonly(path + ".tmp", false);
        remove((path + ".tmp").c_str());
    }
}

// Creates a folder and all the folders above it. Existing folders are fine.
inline void makefolder(const string& path)
{
    for (size_t pos = 0; pos < path.length(); pos++)
    {
        if (path[pos] == '/' || path[pos] == '\\')
        {
            _mkdir(path.substr(0, pos).c_str());
        }
    }

    _mkdir(path.c_str());
}

// Returns a fresh, empty folder for one test, with a trailing slash. The folder is
// <build>/testdata/<testname>/ and it is never anywhere near Phase3/data or Phase2.
inline string testfolder(const string& testname)
{
    string folder = string(CB_TESTDATA) + testname + "/";
    makefolder(folder);
    clearfolder(folder);
    return folder;
}

// The donors of the standard fixture.
inline string donorstext()
{
    return "donor_id,name,type,node_id\n"
           "DN001,City Banquet Hall,CATERER,0\n"
           "DN002,Metro Hotel Kitchen,HOTEL,1\n";
}

// The recipients of the standard fixture.
inline string recipientstext()
{
    return "recipient_id,name,type,available_capacity,accepted_types,accepting,can_pickup,"
           "node_id\n"
           "R001,Hope Shelter,SHELTER,150,COOKED;PACKAGED,1,1,5\n"
           "R002,City Food Bank,FOOD_BANK,300,PACKAGED;PRODUCE;BAKERY,1,1,6\n"
           "R003,Green Community Kitchen,COMMUNITY_KITCHEN,120,COOKED,1,1,7\n"
           "R004,Sunrise Care Home,CARE_HOME,80,COOKED;BAKERY,1,1,8\n"
           "R005,Riverside School Meals,SCHOOL,200,COOKED;PACKAGED,1,1,9\n"
           "R006,Bridge Pantry,PANTRY,100,PACKAGED;PRODUCE,1,0,10\n"
           "R007,Sevak Kitchen,COMMUNITY_KITCHEN,90,COOKED,0,1,8\n";
}

// The map locations of the standard fixture (location 9 has no road, so R005 is unreachable).
inline string nodestext()
{
    return "node_id,label,x_km,y_km\n"
           "0,DN001 City Banquet Hall,0,0\n"
           "1,DN002 Metro Hotel Kitchen,9,1\n"
           "2,Junction North,2,4\n"
           "3,Junction East,6,1\n"
           "4,Junction Bridge,4,-3\n"
           "5,R001 Hope Shelter,2,0\n"
           "6,R002 City Food Bank,4,0\n"
           "7,R003 Green Community Kitchen,7,4\n"
           "8,R004 Sunrise Care Home,-3,-1\n"
           "9,R005 Riverside School Meals,10,6\n"
           "10,R006 Bridge Pantry,3,-1\n";
}

// The roads of the standard fixture.
inline string edgestext()
{
    return "u,v,minutes\n"
           "0,2,8\n2,5,17\n0,4,6\n4,6,9\n0,8,12\n2,7,14\n6,3,5\n3,7,7\n0,3,25\n1,3,4\n"
           "1,7,9\n4,10,8\n2,10,6\n";
}

// The donations of the standard fixture (both from DN002, whose location is 1).
inline string donationstext()
{
    return "donation_id,donor_id,food_type,original_quantity,remaining_quantity,"
           "usable_minutes,status\n"
           "DON001,DN002,COOKED,80,80,60,PENDING\n"
           "DON002,DN002,COOKED,80,80,9,PENDING\n";
}

// The header of transactions.csv with no rows.
inline string transactionstext()
{
    return "transaction_id,donation_id,recipient_id,quantity,travel_minutes\n";
}

// Writes the six standard files into the folder (offers.csv is not written).
inline void writefixture(const string& folder)
{
    writefile(folder + "donors.csv", donorstext());
    writefile(folder + "recipients.csv", recipientstext());
    writefile(folder + "graph_nodes.csv", nodestext());
    writefile(folder + "graph_edges.csv", edgestext());
    writefile(folder + "donations.csv", donationstext());
    writefile(folder + "transactions.csv", transactionstext());
}

// The header row of offers.csv.
inline string offersheadertext()
{
    return "offer_id,donation_id,recipient_id,quantity,travel_minutes,status,txn_id\n";
}

// Writes the standard files, then replaces donations, transactions and offers with a richer set:
// five donations of two donors (waiting, partly given, closed, completed, waiting), three
// transactions and six offers. offered 350, given out 130, wasted 40, still open 180.
inline void writerichfixture(const string& folder)
{
    writefixture(folder);
    writefile(folder + "donations.csv",
              "donation_id,donor_id,food_type,original_quantity,remaining_quantity,usable_minutes,status\n"
              "DON001,DN002,COOKED,80,80,60,PENDING\n"
              "DON002,DN002,COOKED,80,50,9,PARTIAL\n"
              "DON003,DN001,PACKAGED,40,40,30,CLOSED\n"
              "DON004,DN001,PRODUCE,100,0,90,COMPLETED\n"
              "DON005,DN001,BAKERY,50,50,20,PENDING\n");
    writefile(folder + "transactions.csv",
              transactionstext() + "T001,DON002,R001,30,5\nT002,DON004,R002,60,9\nT003,DON004,R002,40,9\n");
    writefile(folder + "offers.csv",
              offersheadertext() + "OFF001,DON001,R001,30,5,PENDING,\n"
                                   "OFF002,DON002,R001,30,5,ACCEPTED,T001\n"
                                   "OFF003,DON003,R002,5,5,WITHDRAWN,\n"
                                   "OFF004,DON004,R002,10,9,DECLINED,\n"
                                   "OFF005,DON005,R002,20,9,PENDING,\n"
                                   "OFF006,DON001,R003,10,9,PENDING,\n");
}

#endif
