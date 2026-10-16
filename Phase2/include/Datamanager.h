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
    // Initializes an empty dataset.
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

    // Loads all CSV tables into memory and collects non-fatal warnings.
    void loadall(dataset& target, list<string>& warnings);

    // Saves a table of CSV records using an atomic temp-file replace.
    void savetable(const string& filename, const string& header, const vector<const CsvRecord*>& records);

    // Appends a single transaction to transactions.csv.
    void appendtransaction(const Transaction& entry);

    // Saves updated donations, recipients, and the new transaction.
    void commitallocation(const dataset& source, const Transaction& entry);

    // Saves the current state of donations to donations.csv.
    void commitdonations(const dataset& source);

    // Validates business rules across the entire dataset and returns findings.
    vector<string> checkinvariants(const dataset& source) const;

private:
    string prefix_; // folder path containing the csv files

    // Recovers a file from its temp copy if an earlier save was interrupted.
    void recoverfile(const string& filename, list<string>& warnings);

    // Writes all donations to a temporary file and atomically replaces donations.csv.
    void savedonations(const dataset& source);

    // Writes all recipients to a temporary file and atomically replaces recipients.csv.
    void saverecipients(const dataset& source);
};

#endif
