#ifndef MODELS_H
#define MODELS_H
#include "Duration.h"
#include "EntityId.h"
#include "Exceptions.h"
#include <sstream>
#include <string>
#include <vector>
using namespace std;

enum FoodType
{
    COOKED,
    PACKAGED,
    PRODUCE,
    BAKERY
};

enum DonationStatus
{
    PENDING,
    PARTIAL,
    COMPLETED,
    CLOSED
};

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

// enum to string
string foodToStr(FoodType type);

// string to enum
FoodType strToFood(const string& text);

// enum to string
string statusToStr(DonationStatus status);

// string to enum
DonationStatus strToStatus(const string& text);

// enum to string
string reasonToStr(Reason reason);

// string to enum
Reason strToReason(const string& text);

class CsvRecord
{
public:
    virtual ~CsvRecord();
    virtual string toCsvRow() const = 0;
};

class Organization : public CsvRecord
{
public:
    Organization(const EntityId& id, const string& name, const string& type, int node);
    virtual ~Organization();

    const EntityId& id() const;
    string name() const;
    string type() const;
    int nodeId() const;

protected:
    EntityId id_; // organization id
    string name_; // organization name
    string type_; // organization type
    int node_;    // graph node id
};

class Donor : public Organization
{
public:
    Donor(const EntityId& id, const string& name, const string& type, int node);
    virtual ~Donor();

    string toCsvRow() const;
};

class Recipient : public Organization
{
public:
    Recipient(const EntityId& id, const string& name, const string& type, int capacity,
              const vector<FoodType>& types, bool accepting, bool pickup, int node);
    virtual ~Recipient();

    bool accepts(FoodType t) const;
    int availableCapacity() const;
    void reduceCapacity(int qty);
    bool accepting() const;
    bool canPickup() const;
    const vector<FoodType>& acceptedTypes() const;
    string toCsvRow() const;

private:
    int capacity_;           // available food capacity
    vector<FoodType> types_; // accepted food types
    bool accepting_;         // accepting donations
    bool pickup_;            // can pickup food
};

class Donation : public CsvRecord
{
public:
    Donation(const EntityId& id, const EntityId& donor, FoodType food, int original,
             int remaining, int usable, DonationStatus status);
    virtual ~Donation();

    const EntityId& id() const;
    const EntityId& donorId() const;
    FoodType foodType() const;
    int originalQuantity() const;
    int remainingQuantity() const;
    int usableMinutes() const;
    DonationStatus status() const;

    void allocateQuantity(int qty);
    void close();
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

class Transaction : public CsvRecord
{
public:
    Transaction();
    Transaction(const EntityId& id, const EntityId& donation, const EntityId& recipient,
                int qty, int travel);
    virtual ~Transaction();

    const EntityId& id() const;
    const EntityId& donationId() const;
    const EntityId& recipientId() const;
    int quantity() const;
    int travelMinutes() const;
    string toCsvRow() const;

private:
    EntityId id_;        // transaction id
    EntityId donation_;  // donation id
    EntityId recipient_; // recipient id
    int qty_;            // allocated quantity
    int travel_;         // travel time btw donor & recipient
};

#endif