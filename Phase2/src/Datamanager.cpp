// loads and validates communitybridge data and saves changes safely
// saves use temporary files so incomplete writes do not replace the last saved data

#include "Datamanager.h"
#include "Exceptions.h"
#include <cstdio>
#include <fstream>
#include <sstream>

using namespace std;

// the header row that every table must start with
static const string donorsheader = "donor_id,name,type,node_id";
static const string recipientsheader =
    "recipient_id,name,type,available_capacity,accepted_types,accepting,can_pickup,"
    "node_id";
static const string donationsheader =
    "donation_id,donor_id,food_type,original_quantity,remaining_quantity,usable_minutes,"
    "status";
static const string transactionsheader =
    "transaction_id,donation_id,recipient_id,quantity,travel_minutes";
static const string nodesheader = "node_id,label,x_km,y_km";
static const string edgesheader = "u,v,minutes";

// one row of a file with its line number
struct csvrow
{
    int number; // source line for data errors
    vector<string> fields; // parsed values from this row
};

// one road from the map file
struct edgerow
{
    int from; // starting location
    int to; // ending location
    int minutes; // road travel time
};

// checks whitespace used around csv values
static bool isspacechar(char letter)
{
    return letter == ' ' || letter == '\t' || letter == '\r' || letter == '\n';
}

// trims whitespace from both ends of a value
static string trim(const string& text)
{
    size_t first = 0;

    while (first < text.length() && isspacechar(text[first]))
    {
        first++;
    }

    size_t last = text.length();

    while (last > first && isspacechar(text[last - 1]))
    {
        last--;
    }

    return text.substr(first, last - first);
}

// splits a csv row into trimmed values
static void splitfields(const string& line, char separator, vector<string>& parts)
{
    string current = "";

    for (size_t pos = 0; pos < line.length(); pos++)
    {
        if (line[pos] == separator)
        {
            parts.push_back(trim(current));
            current = "";
        }
        else
        {
            current = current + line[pos];
        }
    }

    parts.push_back(trim(current));
}

// includes the file name and row number in data errors
static string where(const string& name, int line)
{
    stringstream text;
    text << name << " line " << line << ": ";
    return text.str();
}

// stops with a data error that starts with the file name and line number
static void fail(const string& name, int line, const string& problem)
{
    throw DataException(where(name, line) + problem);
}

// checks whether a file exists and can be opened
static bool fileexists(const string& path)
{
    ifstream file(path.c_str());
    return file.is_open();
}

// parses a whole number
static int parseint(const string& text, const string& what)
{
    size_t start = 0;

    if (!text.empty() && text[0] == '-')
    {
        start = 1;
    }

    if (start >= text.length())
    {
        throw FormatException("Missing whole number for " + what);
    }

    for (size_t pos = start; pos < text.length(); pos++)
    {
        if (text[pos] < '0' || text[pos] > '9')
        {
            throw FormatException("Not a whole number for " + what + ": " + text);
        }
    }

    // reading into an int fails when the number is too big
    int value = 0;
    stringstream stream(text);
    stream >> value;

    if (stream.fail())
    {
        throw FormatException("Number is too large for " + what + ": " + text);
    }

    return value;
}

// parses a decimal number
static double parsedouble(const string& text, const string& what)
{
    double value = 0;
    stringstream stream(text);
    stream >> value;

    if (stream.fail() || !stream.eof())
    {
        throw FormatException("Not a number for " + what + ": " + text);
    }

    return value;
}

// parses a boolean stored as 0 or 1
static bool parsebool(const string& text, const string& what)
{
    if (text == "0")
    {
        return false;
    }

    if (text == "1")
    {
        return true;
    }

    throw FormatException("Expected 0 or 1 for " + what + ": " + text);
}

// parses an id and requires at least three digits after the prefix
static EntityId parseid(const string& text, const string& prefix)
{
    EntityId identity = EntityId::parse(text, prefix);

    if (text.length() - prefix.length() < 3)
    {
        throw FormatException("An id needs at least three digits: " + text);
    }

    return identity;
}

// reads a file, checks the first line and the number of values in each row
static void readtable(const string& path, const string& name, const string& header, int columns, vector<csvrow>& rows)
{
    ifstream file(path.c_str());

    if (!file.is_open())
    {
        throw DataException("Cannot open file: " + path);
    }

    string line;
    int number = 0;

    while (getline(file, line))
    {
        number++;

        // some editors put three hidden bytes at the start of a file, this removes them
        if (number == 1 && line.substr(0, 3) == "\xEF\xBB\xBF")
        {
            line = line.substr(3);
        }

        string clean = trim(line);

        if (number == 1)
        {
            if (clean != header)
            {
                fail(name, 1, "unexpected header, expected: " + header);
            }
        }
        else if (!clean.empty())
        {
            csvrow row;
            row.number = number;
            splitfields(clean, ',', row.fields);

            if ((int)row.fields.size() != columns)
            {
                stringstream message;
                message << "expected " << columns << " values but found " << row.fields.size();
                fail(name, number, message.str());
            }

            rows.push_back(row);
        }
    }

    if (number == 0)
    {
        throw DataException(name + ": the file is empty, the header row is missing");
    }
}

// checks whether donation status matches its quantities
static bool statusmatches(DonationStatus status, int original, int remaining)
{
    // pending nothing was given away yet so the whole quantity is left
    if (status == PENDING)
    {
        return remaining == original;
    }

    // partial something was given away but something is still left
    if (status == PARTIAL)
    {
        return remaining > 0 && remaining < original;
    }

    // completed everything was given away
    if (status == COMPLETED)
    {
        return remaining == 0;
    }

    return true;
}

// loads map locations in numbered order
static void loadnodes(const string& path, const string& name, dataset& target)
{
    vector<csvrow> rows;
    readtable(path, name, nodesheader, 4, rows);

    for (size_t pos = 0; pos < rows.size(); pos++)
    {
        const csvrow& row = rows[pos];

        try
        {
            int node = parseint(row.fields[0], "location number");

            if (node != (int)pos)
            {
                fail(name, row.number, "map locations must be numbered 0, 1, 2, ... in order");
            }

            target.nodelabels.push_back(row.fields[1]);
            target.nodexs.push_back(parsedouble(row.fields[2], "horizontal coordinate"));
            target.nodeys.push_back(parsedouble(row.fields[3], "vertical coordinate"));
        }
        catch (const FormatException& problem)
        {
            fail(name, row.number, problem.what());
        }
    }

    // the number of locations is known now, so the graph is made
    target.network = graph((int)rows.size());
}

// loads roads and keeps the shortest time when a road is repeated
static void loadedges(const string& path, const string& name, dataset& target, list<string>& warnings)
{
    vector<csvrow> rows;
    readtable(path, name, edgesheader, 3, rows);

    int count = target.network.nodecount();
    // roads are kept here first so a road that appears twice is joined into one
    vector<edgerow> edges;
    // remembers which roads were already read, so a repeated road is found quickly
    hashindex seen;

    for (size_t pos = 0; pos < rows.size(); pos++)
    {
        const csvrow& row = rows[pos];

        try
        {
            edgerow edge;
            edge.from = parseint(row.fields[0], "road start location");
            edge.to = parseint(row.fields[1], "road end location");
            edge.minutes = parseint(row.fields[2], "road travel time");

            if (edge.from < 0 || edge.from >= count || edge.to < 0 || edge.to >= count)
            {
                fail(name, row.number, "a road ends at a location that is not in the map file");
            }

            if (edge.from == edge.to)
            {
                fail(name, row.number, "a road cannot start and end at the same location");
            }

            if (edge.minutes < 1)
            {
                fail(name, row.number, "a road needs at least 1 minute");
            }

            // smaller location number first, so both directions give the same key
            int small = edge.from;
            int large = edge.to;

            if (large < small)
            {
                small = edge.to;
                large = edge.from;
            }

            stringstream key;
            key << small << "," << large;
            int earlier = 0; // index of the previously stored road

            if (seen.get(key.str(), earlier))
            {
                stringstream message;
                message << where(name, row.number) << "duplicate road " << small << "-"
                        << large << ", the smaller minutes are kept";
                warnings.push_back(message.str());

                if (edge.minutes < edges[earlier].minutes)
                {
                    edges[earlier].minutes = edge.minutes;
                }
            }
            else
            {
                seen.put(key.str(), (int)edges.size());
                edges.push_back(edge);
            }
        }
        catch (const FormatException& problem)
        {
            fail(name, row.number, problem.what());
        }
    }

    graph built(count);

    for (size_t pos = 0; pos < edges.size(); pos++)
    {
        // a road can be used in both directions
        built.addedge(edges[pos].from, edges[pos].to, edges[pos].minutes);
    }

    target.network = built;
}

// loads donors
static void loaddonors(const string& path, const string& name, dataset& target)
{
    vector<csvrow> rows;
    readtable(path, name, donorsheader, 4, rows);

    for (size_t pos = 0; pos < rows.size(); pos++)
    {
        const csvrow& row = rows[pos];

        try
        {
            EntityId identity = parseid(row.fields[0], "DN");
            int node = parseint(row.fields[3], "location number");

            if (node < 0 || node >= target.network.nodecount())
            {
                fail(name, row.number, "this location is not in the map file (graph_nodes.csv)");
            }

            if (!target.donorindex.put(identity.toString(), (int)target.donors.size()))
            {
                fail(name, row.number, "the id " + identity.toString() + " is used twice");
            }

            target.donors.push_back(Donor(identity, row.fields[1], row.fields[2], node));
        }
        catch (const FormatException& problem)
        {
            fail(name, row.number, problem.what());
        }
    }
}

// loads recipients
static void loadrecipients(const string& path, const string& name, dataset& target)
{
    vector<csvrow> rows;
    readtable(path, name, recipientsheader, 8, rows);

    for (size_t pos = 0; pos < rows.size(); pos++)
    {
        const csvrow& row = rows[pos];

        try
        {
            EntityId identity = parseid(row.fields[0], "R");
            int capacity = parseint(row.fields[3], "available capacity");

            vector<string> words;
            splitfields(row.fields[4], ';', words);
            vector<FoodType> types;

            for (size_t kind = 0; kind < words.size(); kind++)
            {
                // an empty field means the recipient accepts no food types
                if (words[kind] != "")
                {
                    // rejects an unknown food type
                    types.push_back(strToFood(words[kind]));
                }
            }

            bool accepting = parsebool(row.fields[5], "accepting donations");
            bool pickup = parsebool(row.fields[6], "can pick up donations");
            int node = parseint(row.fields[7], "location number");

            if (capacity < 0)
            {
                fail(name, row.number, "available capacity cannot be below 0");
            }

            if (node < 0 || node >= target.network.nodecount())
            {
                fail(name, row.number, "this location is not in the map file (graph_nodes.csv)");
            }

            if (!target.recipientindex.put(identity.toString(),
                                           (int)target.recipients.size()))
            {
                fail(name, row.number, "the id " + identity.toString() + " is used twice");
            }

            target.recipients.push_back(Recipient(identity, row.fields[1], row.fields[2],
                                                  capacity, types, accepting, pickup,
                                                  node));
        }
        catch (const FormatException& problem)
        {
            fail(name, row.number, problem.what());
        }
    }
}

// loads donations
static void loaddonations(const string& path, const string& name, dataset& target)
{
    vector<csvrow> rows;
    readtable(path, name, donationsheader, 7, rows);

    for (size_t pos = 0; pos < rows.size(); pos++)
    {
        const csvrow& row = rows[pos];

        try
        {
            EntityId identity = parseid(row.fields[0], "DON");
            EntityId donor = parseid(row.fields[1], "DN");
            FoodType food = strToFood(row.fields[2]);
            int original = parseint(row.fields[3], "original quantity");
            int remaining = parseint(row.fields[4], "remaining quantity");
            int usable = parseint(row.fields[5], "usable minutes");
            DonationStatus status = strToStatus(row.fields[6]);
            int unused = 0; // only the yes or no answer is needed

            if (!target.donorindex.get(donor.toString(), unused))
            {
                fail(name, row.number, "the donor " + donor.toString() + " is not in donors.csv");
            }

            if (original < 1 || remaining < 0 || remaining > original)
            {
                fail(name, row.number, "wrong quantities: original is 1 or more, remaining is 0 up to original");
            }

            if (usable < 1)
            {
                fail(name, row.number, "usable minutes must be at least 1");
            }

            if (!statusmatches(status, original, remaining))
            {
                fail(name, row.number, "the status does not fit the quantities");
            }

            if (!target.donationindex.put(identity.toString(),
                                          (int)target.donations.size()))
            {
                fail(name, row.number, "the id " + identity.toString() + " is used twice");
            }

            target.donations.push_back(
                Donation(identity, donor, food, original, remaining, usable, status));
        }
        catch (const FormatException& problem)
        {
            fail(name, row.number, problem.what());
        }
    }
}

// loads transactions and remembers the highest transaction id
static void loadtransactions(const string& path, const string& name, dataset& target)
{
    vector<csvrow> rows;
    readtable(path, name, transactionsheader, 5, rows);
    hashindex seen;

    for (size_t pos = 0; pos < rows.size(); pos++)
    {
        const csvrow& row = rows[pos];

        try
        {
            EntityId identity = parseid(row.fields[0], "T");
            EntityId donation = parseid(row.fields[1], "DON");
            EntityId recipient = parseid(row.fields[2], "R");
            int quantity = parseint(row.fields[3], "quantity");
            int travel = parseint(row.fields[4], "travel time in minutes");
            int unused = 0; // only the yes or no answer is needed

            if (!target.donationindex.get(donation.toString(), unused) ||
                !target.recipientindex.get(recipient.toString(), unused))
            {
                fail(name, row.number, "the donation or recipient is not in the files");
            }

            if (!seen.put(identity.toString(), (int)pos))
            {
                fail(name, row.number, "the id " + identity.toString() + " is used twice");
            }

            if (target.lasttransaction < identity)
            {
                target.lasttransaction = identity;
            }

            target.transactions.push_back(
                Transaction(identity, donation, recipient, quantity, travel));
        }
        catch (const FormatException& problem)
        {
            fail(name, row.number, problem.what());
        }
    }
}

// warns when a recipient cannot reach any donor
static void warnunconnected(const dataset& source, list<string>& warnings)
{
    vector<int> label;
    labelcomponents(source.network, label);

    for (size_t pos = 0; pos < source.recipients.size(); pos++)
    {
        int node = source.recipients[pos].nodeId();
        bool hasdonor = false;

        for (size_t other = 0; other < source.donors.size(); other++)
        {
            if (label[source.donors[other].nodeId()] == label[node])
            {
                hasdonor = true;
            }
        }

        if (!hasdonor)
        {
            stringstream message;
            message << source.recipients[pos].id().toString() << " (location " << node
                    << ") is not connected to any donor; it can never be recommended.";
            warnings.push_back(message.str());
        }
    }
}

// initializes the highest transaction id
dataset::dataset()
{
    lasttransaction = EntityId("T", 0);
}

// stores the directory containing the data files
datamanager::datamanager(const string& prefix)
{
    prefix_ = prefix;
}

// loads files in dependency order and checks the complete dataset
void datamanager::loadall(dataset& target, list<string>& warnings)
{
    target = dataset();

    recoverfile("donations.csv", warnings);
    recoverfile("recipients.csv", warnings);

    loadnodes(prefix_ + "graph_nodes.csv", "graph_nodes.csv", target);
    loadedges(prefix_ + "graph_edges.csv", "graph_edges.csv", target, warnings);
    loaddonors(prefix_ + "donors.csv", "donors.csv", target);
    loadrecipients(prefix_ + "recipients.csv", "recipients.csv", target);
    loaddonations(prefix_ + "donations.csv", "donations.csv", target);
    loadtransactions(prefix_ + "transactions.csv", "transactions.csv", target);

    // disconnected recipients are warnings rather than load errors
    warnunconnected(target, warnings);

    vector<string> problems = checkinvariants(target);

    if (!problems.empty())
    {
        throw DataException(problems[0]);
    }
}

// saves a complete table through a temporary file
void datamanager::savetable(const string& filename, const string& header, const vector<const CsvRecord*>& records)
{
    string path = prefix_ + filename;
    string temp = path + ".tmp";

    ofstream out(temp.c_str());

    if (!out.is_open())
    {
        throw DataException("Cannot write file: " + path);
    }

    // every csv table starts with its header
    out << header << "\n";

    for (size_t pos = 0; pos < records.size(); pos++)
    {
        out << records[pos]->toCsvRow() << "\n";
    }

    out.flush();

    if (!out.good())
    {
        throw DataException("Writing failed for file: " + path);
    }

    out.close();

    if (out.fail())
    {
        throw DataException("Closing failed for file: " + path);
    }

    // windows cannot rename onto an existing file so the old file goes first
    if (fileexists(path) && remove(path.c_str()) != 0)
    {
        throw DataException("Cannot replace the old file: " + path);
    }

    if (rename(temp.c_str(), path.c_str()) != 0)
    {
        throw DataException("Could not save " + path + ". Restart the program to recover it.");
    }
}

// checks whether a file ends with a newline
static bool endswithnewline(const string& path)
{
    ifstream file(path.c_str(), ios::binary);
    char last = '\n';
    char letter;

    while (file.get(letter))
    {
        last = letter;
    }

    return last == '\n';
}

// appends one transaction without joining it to an unterminated last row
void datamanager::appendtransaction(const Transaction& entry)
{
    string path = prefix_ + "transactions.csv";
    bool needheader = !fileexists(path);
    bool needbreak = !needheader && !endswithnewline(path);

    ofstream out(path.c_str(), ios::app);

    if (!out.is_open())
    {
        throw DataException("Cannot write file: " + path);
    }

    if (needheader)
    {
        out << transactionsheader << "\n";
    }

    if (needbreak)
    {
        out << "\n";
    }

    out << entry.toCsvRow() << "\n";
    out.flush();

    if (!out.good())
    {
        throw DataException("Writing failed for file: " + path);
    }
}

// records the transaction before saving updated donation and recipient data
void datamanager::commitallocation(const dataset& source, const Transaction& entry)
{
    appendtransaction(entry);
    savedonations(source);
    saverecipients(source);
}

// saves donation changes
void datamanager::commitdonations(const dataset& source)
{
    savedonations(source);
}

// checks data consistency without changing records
vector<string> datamanager::checkinvariants(const dataset& source) const
{
    vector<string> problems;
    vector<int> placed(source.donations.size(), 0); // transaction quantity per donation

    for (size_t pos = 0; pos < source.donations.size(); pos++)
    {
        const Donation& donation = source.donations[pos];
        stringstream message;

        if (donation.remainingQuantity() < 0 ||
            donation.remainingQuantity() > donation.originalQuantity())
        {
            message << "Data problem: donation " << donation.id().toString()
                    << " has a wrong remaining quantity";
            problems.push_back(message.str());
        }

        if (!statusmatches(donation.status(), donation.originalQuantity(),
                           donation.remainingQuantity()))
        {
            stringstream second;
            second << "Data problem: the status of donation " << donation.id().toString()
                   << " does not fit its quantities";
            problems.push_back(second.str());
        }
    }

    for (size_t pos = 0; pos < source.recipients.size(); pos++)
    {
        if (source.recipients[pos].availableCapacity() < 0)
        {
            problems.push_back("Data problem: recipient " +
                               source.recipients[pos].id().toString() +
                               " has a capacity below 0");
        }
    }

    for (size_t pos = 0; pos < source.transactions.size(); pos++)
    {
        const Transaction& entry = source.transactions[pos];
        int donationpos = 0;
        int recipientpos = 0;
        bool donationfound = source.donationindex.get(entry.donationId().toString(),
                                                      donationpos);
        bool recipientfound = source.recipientindex.get(entry.recipientId().toString(),
                                                        recipientpos);

        if (!donationfound || !recipientfound)
        {
            problems.push_back("Data problem: transaction " + entry.id().toString() +
                               " uses a donation or recipient that is not in the files");
        }
        else
        {
            placed[donationpos] = placed[donationpos] + entry.quantity();
        }

        if (pos > 0 && !(source.transactions[pos - 1].id() < entry.id()))
        {
            problems.push_back("Data problem: transaction " + entry.id().toString() +
                               " is out of order in transactions.csv");
        }
    }

    for (size_t pos = 0; pos < source.donations.size(); pos++)
    {
        const Donation& donation = source.donations[pos];
        // quantity the donation says was given out
        int done = donation.originalQuantity() - donation.remainingQuantity();

        if (done != placed[pos])
        {
            stringstream message;
            message << "Data problem: donation " << donation.id().toString()
                    << " has original "
                    << donation.originalQuantity() << " and remaining "
                    << donation.remainingQuantity() << ", but its transactions add up to "
                    << placed[pos];
            problems.push_back(message.str());
        }
    }

    return problems;
}

// restores a complete temporary file or removes an outdated temporary file
void datamanager::recoverfile(const string& filename, list<string>& warnings)
{
    string path = prefix_ + filename;
    string temp = path + ".tmp";
    bool hasmain = fileexists(path);
    bool hastemp = fileexists(temp);

    if (!hasmain && hastemp)
    {
        if (rename(temp.c_str(), path.c_str()) != 0)
        {
            throw DataException("Cannot recover " + path);
        }

        warnings.push_back(filename + " was restored after an interrupted save.");
    }
    else if (hasmain && hastemp)
    {
        remove(temp.c_str());
        warnings.push_back("An unfinished save of " + filename + " was discarded.");
    }
}

// saves all donations
void datamanager::savedonations(const dataset& source)
{
    vector<const CsvRecord*> records;

    for (size_t pos = 0; pos < source.donations.size(); pos++)
    {
        records.push_back(&source.donations[pos]);
    }

    savetable("donations.csv", donationsheader, records);
}

// saves all recipients
void datamanager::saverecipients(const dataset& source)
{
    vector<const CsvRecord*> records;

    for (size_t pos = 0; pos < source.recipients.size(); pos++)
    {
        records.push_back(&source.recipients[pos]);
    }

    savetable("recipients.csv", recipientsheader, records);
}
