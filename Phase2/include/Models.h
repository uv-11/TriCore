#ifndef MODELS_H
#define MODELS_H
#include "EntityId.h"
#include <string>
#include <vector>
using namespace std;

// category of food offered or accepted
enum FoodType
{
    COOKED,
    PACKAGED,
    PRODUCE,
    BAKERY
};

// lifecycle status of a donation
enum DonationStatus
{
    PENDING,
    PARTIAL,
    COMPLETED,
    CLOSED
};

// reason why a recipient can or cannot accept a donation
enum Reason
{
    OK,
    NOT_ACCEPTING,
    FOOD_INCOMPATIBLE,
    NO_CAPACITY,
    NO_PICKUP,
    UNREACHABLE,
    TOO_FAR
};

// converts a food type to text
string foodToStr(FoodType type);

// converts text to a food type
FoodType strToFood(const string& text);

// converts a donation status to text
string statusToStr(DonationStatus status);

// converts text to a donation status
DonationStatus strToStatus(const string& text);

// converts a reason to text
string reasonToStr(Reason reason);

// converts text to a reason
Reason strToReason(const string& text);

// base class for records serializable to csv
class CsvRecord
{
public:
    // virtual destructor for safe polymorphic deletion
    virtual ~CsvRecord();

    // serializes this record into a csv row
    virtual string toCsvRow() const = 0;
};

// common base class for donors and recipients
class Organization : public CsvRecord
{
public:
    // initializes common organization fields
    Organization(const EntityId& id, const string& name, const string& type, int node);

    // virtual destructor
    virtual ~Organization();

    // returns the organization id
    const EntityId& id() const;

    // returns the organization name
    string name() const;

    // returns the organization type
    string type() const;

    // returns the graph location node id
    int nodeId() const;

protected:
    EntityId id_; // organization id
    string name_; // organization name
    string type_; // organization type
    int node_;    // graph node id
};

// food donor entity
class Donor : public Organization
{
public:
    // constructs a donor record
    Donor(const EntityId& id, const string& name, const string& type, int node);

    // destructor
    virtual ~Donor();

    // serializes the donor to a csv line
    string toCsvRow() const;
};

// food recipient entity
class Recipient : public Organization
{
public:
    // constructs a recipient with capacity accepted types and pickup flags
    Recipient(const EntityId& id, const string& name, const string& type, int capacity,
              const vector<FoodType>& types, bool accepting, bool pickup, int node);

    // destructor
    virtual ~Recipient();

    // checks whether this recipient accepts the given food type
    bool accepts(FoodType t) const;

    // returns the remaining available capacity
    int availableCapacity() const;

    // decreases available capacity by allocated quantity
    void reduceCapacity(int qty);

    // checks if recipient is currently accepting donations
    bool accepting() const;

    // checks if recipient can pick up food
    bool canPickup() const;

    // returns list of accepted food types
    const vector<FoodType>& acceptedTypes() const;

    // serializes the recipient to a csv line
    string toCsvRow() const;

private:
    int capacity_;           // available food capacity
    vector<FoodType> types_; // accepted food types
    bool accepting_;         // accepting donations
    bool pickup_;            // can pickup food
};

// food donation record
class Donation : public CsvRecord
{
public:
    // constructs a donation with quantities usable minutes and status
    Donation(const EntityId& id, const EntityId& donor, FoodType food, int original,
             int remaining, int usable, DonationStatus status);

    // destructor
    virtual ~Donation();

    // returns the donation id
    const EntityId& id() const;

    // returns the id of the donating organization
    const EntityId& donorId() const;

    // returns the type of food offered
    FoodType foodType() const;

    // returns the initial quantity offered
    int originalQuantity() const;

    // returns the quantity still unallocated
    int remainingQuantity() const;

    // returns remaining shelf life in minutes
    int usableMinutes() const;

    // returns current donation lifecycle status
    DonationStatus status() const;

    // allocates part of remaining quantity and updates status
    void allocateQuantity(int qty);

    // closes the donation and marks leftover food as wasted
    void close();

    // serializes the donation to a csv line
    string toCsvRow() const;

private:
    EntityId id_;           // donation id
    EntityId donor_;        // donor id
    FoodType food_;         // food type
    int original_;          // original quantity
    int remaining_;         // remaining quantity
    int usable_;            // usable time in minutes
    DonationStatus status_; // donation status
};

// completed allocation record
class Transaction : public CsvRecord
{
public:
    // default constructor
    Transaction();

    // constructs a transaction linking a donation and recipient
    Transaction(const EntityId& id, const EntityId& donation, const EntityId& recipient,
                int qty, int travel);

    // destructor
    virtual ~Transaction();

    // returns the transaction id
    const EntityId& id() const;

    // returns the referenced donation id
    const EntityId& donationId() const;

    // returns the referenced recipient id
    const EntityId& recipientId() const;

    // returns the allocated quantity
    int quantity() const;

    // returns the travel time in minutes
    int travelMinutes() const;

    // serializes the transaction to a csv line
    string toCsvRow() const;

private:
    EntityId id_;        // transaction id
    EntityId donation_;  // donation id
    EntityId recipient_; // recipient id
    int qty_;            // allocated quantity
    int travel_;         // travel time between donor and recipient
};

#endif