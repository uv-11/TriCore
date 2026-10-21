#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "EntityId.h"
#include "Models.h"
#include "graph.h"
#include "hashindex.h"

#include <list>
#include <string>
#include <vector>

using namespace std;

// stores all loaded data and the indexes used to find records
struct dataset
{
    // creates an empty dataset
    dataset();

    vector<Donor> donors; // all donors loaded from the file
    vector<Recipient> recipients; // all recipients loaded from the file
    vector<Donation> donations; // all donations loaded from the file
    vector<Transaction> transactions; // all completed allocations

    graph network; // road network used for travel calculations
    vector<string> nodelabels; // names of graph nodes
    vector<double> nodexs; // x coordinates of graph nodes
    vector<double> nodeys; // y coordinates of graph nodes

    hashindex donorindex; // maps donor ids to vector positions
    hashindex recipientindex; // maps recipient ids to vector positions
    hashindex donationindex; // maps donation ids to vector positions

    EntityId lasttransaction; // highest transaction id loaded
};

// loads data from csv files and saves changes safely
class datamanager
{
public:
    datamanager(const string& prefix);

    // loads and validates every csv file into the dataset
    void loadall(dataset& target, list<string>& warnings);

    // writes one table safely using a temp file
    void savetable(const string& filename, const string& header, const vector<const CsvRecord*>& records);

    // adds one line to the transactions file
    void appendtransaction(const Transaction& entry);

    // saves a new allocation and its updated records
    void commitallocation(const dataset& source, const Transaction& entry);

    // saves the current donations table
    void commitdonations(const dataset& source);

    // checks the loaded data for consistency problems
    vector<string> checkinvariants(const dataset& source) const;

private:
    string prefix_; // folder path containing the csv files

    // recovers a file after an interrupted save
    void recoverfile(const string& filename, list<string>& warnings);

    // writes the donations table safely
    void savedonations(const dataset& source);

    // writes the recipients table safely
    void saverecipients(const dataset& source);
};

#endif
