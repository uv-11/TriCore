// OfferStore.cpp: loads, checks and saves offers.csv and answers questions about the offers.
// The checks and the messages follow the style of the Phase 2 loader (Datamanager.cpp).

#include "OfferStore.h"
#include "Exceptions.h"
#include "hashindex.h"

#include <cstdio>
#include <fstream>
#include <sstream>

using namespace std;

// The header row that offers.csv must start with.
static const string offersheader = "offer_id,donation_id,recipient_id,quantity,travel_minutes,status,txn_id";

// The name used in every message about this file.
static const string offersname = "offers.csv";

// Checks whitespace used around csv values.
static bool isspacechar(char letter)
{
    return letter == ' ' || letter == '\t' || letter == '\r' || letter == '\n';
}

// Removes whitespace from both ends of a value.
static string trimtext(const string& text)
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

// Splits one csv row at the commas into trimmed values (an empty last value is kept).
static void splitrow(const string& line, vector<string>& parts)
{
    string current = "";

    for (size_t pos = 0; pos < line.length(); pos++)
    {
        if (line[pos] == ',')
        {
            parts.push_back(trimtext(current));
            current = "";
        }
        else
        {
            current = current + line[pos];
        }
    }

    parts.push_back(trimtext(current));
}

// Stops with a data error that starts with the file name and the line number.
static void failline(int line, const string& problem)
{
    stringstream text;
    text << offersname << " line " << line << ": " << problem;
    throw DataException(text.str());
}

// Checks whether a file can be opened.
static bool fileexists(const string& path)
{
    ifstream file(path.c_str());
    return file.is_open();
}

// Reads a whole number (digits with an optional minus sign). Throws FormatException otherwise.
static int parsewhole(const string& text, const string& what)
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

    int value = 0;
    stringstream stream(text);
    stream >> value;

    if (stream.fail())
    {
        throw FormatException("Number is too large for " + what + ": " + text);
    }

    return value;
}

// Reads an id with the given prefix and at least three digits. Throws FormatException otherwise.
static EntityId parsethreedigits(const string& text, const string& prefix)
{
    EntityId identity = EntityId::parse(text, prefix);

    if (text.length() - prefix.length() < 3)
    {
        throw FormatException("An id needs at least three digits: " + text);
    }

    return identity;
}

// Turns one csv row into an offer. Every rule of the file format is checked here.
static Offer parseoffer(const vector<string>& fields)
{
    EntityId offerid = parsethreedigits(fields[0], "OFF");
    EntityId donation = parsethreedigits(fields[1], "DON");
    EntityId recipient = parsethreedigits(fields[2], "R");
    int quantity = parsewhole(fields[3], "quantity");
    int travel = parsewhole(fields[4], "travel minutes");
    OfferStatus status = strtoofferstatus(fields[5]);

    if (quantity < 1)
    {
        throw FormatException("the quantity must be at least 1");
    }

    if (travel < 0)
    {
        throw FormatException("the travel minutes cannot be below 0");
    }

    EntityId txn;

    if (status == OFFER_ACCEPTED)
    {
        if (fields[6].empty())
        {
            throw FormatException("an accepted offer needs a transaction id");
        }

        txn = parsethreedigits(fields[6], "T");
    }
    else if (!fields[6].empty())
    {
        throw FormatException("only an accepted offer can have a transaction id");
    }

    return Offer(offerid, donation, recipient, quantity, travel, status, txn);
}

// Recovers the file after an interrupted save, the same way Phase 2 does for its files.
static void recoveroffers(const string& prefix, list<string>& warnings)
{
    string path = prefix + offersname;
    string temp = path + ".tmp";
    bool hasmain = fileexists(path);
    bool hastemp = fileexists(temp);

    if (!hasmain && hastemp)
    {
        if (rename(temp.c_str(), path.c_str()) != 0)
        {
            throw DataException("Cannot recover " + path);
        }

        warnings.push_back(offersname + " was recovered from a .tmp file.");
    }
    else if (hasmain && hastemp)
    {
        remove(temp.c_str());
        warnings.push_back("A stale .tmp file for " + offersname + " was discarded.");
    }
}

// Loads every row into a temporary list first, so a bad file leaves the store as it was.
void offerstore::load(const string& prefix, list<string>& warnings)
{
    recoveroffers(prefix, warnings);

    vector<Offer> loaded;
    string path = prefix + offersname;
    ifstream file(path.c_str());

    if (file.is_open())
    {
        hashindex seen;
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

            string clean = trimtext(line);

            if (number == 1)
            {
                if (clean != offersheader)
                {
                    failline(1, "unexpected header, expected: " + offersheader);
                }
            }
            else if (!clean.empty())
            {
                vector<string> fields;
                splitrow(clean, fields);

                if (fields.size() != 7)
                {
                    stringstream message;
                    message << "expected 7 fields but found " << fields.size();
                    failline(number, message.str());
                }

                try
                {
                    Offer offer = parseoffer(fields);

                    if (!seen.put(offer.id().toString(), (int)loaded.size()))
                    {
                        failline(number, "the id " + offer.id().toString() + " is used twice");
                    }

                    loaded.push_back(offer);
                }
                catch (const FormatException& problem)
                {
                    failline(number, problem.what());
                }
            }
        }

        if (number == 0)
        {
            throw DataException(offersname + ": the file is empty, the header row is missing");
        }
    }

    // only now is the store replaced, and the id issuer continues after the highest id
    offers_ = loaded;
    EntityId highest("OFF", 0);

    for (size_t pos = 0; pos < offers_.size(); pos++)
    {
        if (highest < offers_[pos].id())
        {
            highest = offers_[pos].id();
        }
    }

    offeridissuer::seed(highest);
}

// Gives every offer to the safe save of datamanager (temp file, then replace).
void offerstore::save(datamanager& files) const
{
    vector<const CsvRecord*> records;

    for (size_t pos = 0; pos < offers_.size(); pos++)
    {
        records.push_back(&offers_[pos]);
    }

    files.savetable(offersname, offersheader, records);
}

// Returns the number of offers.
int offerstore::count() const
{
    return (int)offers_.size();
}

// Returns an offer for reading.
const Offer& offerstore::at(int pos) const
{
    if (pos < 0 || pos >= (int)offers_.size())
    {
        throw StructureException("There is no offer at this position");
    }

    return offers_[pos];
}

// Returns an offer for changing.
Offer& offerstore::at(int pos)
{
    if (pos < 0 || pos >= (int)offers_.size())
    {
        throw StructureException("There is no offer at this position");
    }

    return offers_[pos];
}

// Looks through the offers one by one (the list is small).
bool offerstore::find(const EntityId& offerid, int& pos) const
{
    for (size_t index = 0; index < offers_.size(); index++)
    {
        if (offers_[index].id() == offerid)
        {
            pos = (int)index;
            return true;
        }
    }

    return false;
}

// Checks the quantity before an id is issued, so a refused offer never uses up a number.
int offerstore::add(const EntityId& donation, const EntityId& recipient, int qty, int travel)
{
    if (qty < 1)
    {
        throw StructureException("An offer needs a quantity of at least 1");
    }

    EntityId none;
    offers_.push_back(Offer(offeridissuer::issuenext(), donation, recipient, qty, travel,
                            OFFER_PENDING, none));
    return (int)offers_.size() - 1;
}

// Adds up the pending offers of one donation.
int offerstore::pendingquantity(const EntityId& donation) const
{
    int total = 0;

    for (size_t pos = 0; pos < offers_.size(); pos++)
    {
        if (offers_[pos].status() == OFFER_PENDING && offers_[pos].donationId() == donation)
        {
            total = total + offers_[pos].quantity();
        }
    }

    return total;
}

// A recipient that declined this donation once is not offered it again.
bool offerstore::wasdeclined(const EntityId& donation, const EntityId& recipient) const
{
    for (size_t pos = 0; pos < offers_.size(); pos++)
    {
        if (offers_[pos].status() == OFFER_DECLINED && offers_[pos].donationId() == donation &&
            offers_[pos].recipientId() == recipient)
        {
            return true;
        }
    }

    return false;
}

// Used when a donation is closed: its waiting offers can no longer be accepted.
int offerstore::withdrawfordonation(const EntityId& donation)
{
    int changed = 0;

    for (size_t pos = 0; pos < offers_.size(); pos++)
    {
        if (offers_[pos].status() == OFFER_PENDING && offers_[pos].donationId() == donation)
        {
            offers_[pos].markwithdrawn();
            changed++;
        }
    }

    return changed;
}

// Looks for three kinds of mismatch between the offers and the data. Only warnings are made.
vector<string> offerstore::reconcile(const dataset& data) const
{
    vector<string> warnings;

    for (size_t pos = 0; pos < offers_.size(); pos++)
    {
        const Offer& offer = offers_[pos];
        int donationpos = 0;
        int recipientpos = 0;
        bool donationknown = data.donationindex.get(offer.donationId().toString(), donationpos);
        bool recipientknown = data.recipientindex.get(offer.recipientId().toString(), recipientpos);

        if (!donationknown)
        {
            warnings.push_back("Offer " + offer.id().toString() + " is for donation " +
                               offer.donationId().toString() + ", which is not in the donations file.");
        }

        if (!recipientknown)
        {
            warnings.push_back("Offer " + offer.id().toString() + " is for recipient " +
                               offer.recipientId().toString() + ", who is not in the recipients file.");
        }

        if (offer.status() == OFFER_ACCEPTED)
        {
            bool found = false;

            for (size_t index = 0; index < data.transactions.size(); index++)
            {
                if (data.transactions[index].id() == offer.transactionId())
                {
                    found = true;
                }
            }

            if (!found)
            {
                warnings.push_back("Offer " + offer.id().toString() + " says it was accepted, but transaction " +
                                   offer.transactionId().toString() + " is not in the transactions file.");
            }
        }

        if (offer.status() == OFFER_PENDING && donationknown)
        {
            DonationStatus state = data.donations[donationpos].status();

            if (state == COMPLETED || state == CLOSED)
            {
                warnings.push_back("Offer " + offer.id().toString() + " is still waiting, but donation " +
                                   offer.donationId().toString() + " is already finished.");
            }
        }
    }

    return warnings;
}
