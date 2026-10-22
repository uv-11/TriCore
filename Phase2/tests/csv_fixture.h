#ifndef CSV_FIXTURE_H
#define CSV_FIXTURE_H

#include <cstdio>
#include <fstream>
#include <string>

using namespace std;

// Writes text to a file byte for byte (binary mode), so tests control line endings.
inline void writefile(const string& path, const string& text)
{
    ofstream out(path.c_str(), ios::binary);
    out << text;
}

// Reads a whole file into a string. Returns an empty string if it cannot be opened.
inline string readfile(const string& path)
{
    ifstream in(path.c_str(), ios::binary);
    string text = "";
    string line;

    while (getline(in, line))
    {
        // Text mode output on Windows ends lines with CR LF, so the CR is dropped here
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

// The donors of Appendix A.
inline string donorstext()
{
    return "donor_id,name,type,node_id\n"
           "DN001,City Banquet Hall,CATERER,0\n"
           "DN002,Metro Hotel Kitchen,HOTEL,1\n";
}

// The recipients of Appendix A.
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

// The graph nodes of Appendix A.
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

// The graph edges of Appendix A.
inline string edgestext()
{
    return "u,v,minutes\n"
           "0,2,8\n2,5,17\n0,4,6\n4,6,9\n0,8,12\n2,7,14\n6,3,5\n3,7,7\n0,3,25\n1,3,4\n"
           "1,7,9\n4,10,8\n2,10,6\n";
}

// The two donations of scenario S8 (donations_S8.csv).
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

// Writes the six files of the standard fixture with the given file name prefix.
inline void writefixture(const string& prefix)
{
    writefile(prefix + "donors.csv", donorstext());
    writefile(prefix + "recipients.csv", recipientstext());
    writefile(prefix + "graph_nodes.csv", nodestext());
    writefile(prefix + "graph_edges.csv", edgestext());
    writefile(prefix + "donations.csv", donationstext());
    writefile(prefix + "transactions.csv", transactionstext());
}

// Removes the fixture files and any .tmp files with the given prefix.
inline void removefixture(const string& prefix)
{
    string names[6] = {"donors.csv", "recipients.csv", "graph_nodes.csv",
                       "graph_edges.csv", "donations.csv", "transactions.csv"};

    for (int pos = 0; pos < 6; pos++)
    {
        remove((prefix + names[pos]).c_str());
        remove((prefix + names[pos] + ".tmp").c_str());
    }
}

#endif