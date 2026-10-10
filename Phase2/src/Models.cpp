#include "Models.h"

using namespace std;

// enum to string
string foodToStr(FoodType type) {
  if (type == COOKED) {
    return "COOKED";
  }

  if (type == PACKAGED) {
    return "PACKAGED";
  }

  if (type == PRODUCE) {
    return "PRODUCE";
  }

  if (type == BAKERY) {
    return "BAKERY";
  }

  throw StructureException("Invalid FoodType enum value");
}

// string to enum
FoodType strToFood(const string &text) {
  if (text == "COOKED") {
    return COOKED;
  }

  if (text == "PACKAGED") {
    return PACKAGED;
  }

  if (text == "PRODUCE") {
    return PRODUCE;
  }

  if (text == "BAKERY") {
    return BAKERY;
  }

  throw FormatException("Invalid FoodType: " + text);
}

// enum to string
string statusToStr(DonationStatus status) {
  if (status == PENDING) {
    return "PENDING";
  }

  if (status == PARTIAL) {
    return "PARTIAL";
  }

  if (status == COMPLETED) {
    return "COMPLETED";
  }

  if (status == CLOSED) {
    return "CLOSED";
  }

  throw StructureException("Invalid DonationStatus enum value");
}

// string to enum
DonationStatus strToStatus(const string &text) {
  if (text == "PENDING") {
    return PENDING;
  }

  if (text == "PARTIAL") {
    return PARTIAL;
  }

  if (text == "COMPLETED") {
    return COMPLETED;
  }

  if (text == "CLOSED") {
    return CLOSED;
  }

  throw FormatException("Invalid DonationStatus: " + text);
}

// enum to string
string reasonToStr(Reason reason) {
  if (reason == OK) {
    return "OK";
  }

  if (reason == NOT_ACCEPTING) {
    return "NOT_ACCEPTING";
  }

  if (reason == FOOD_INCOMPATIBLE) {
    return "FOOD_INCOMPATIBLE";
  }

  if (reason == NO_CAPACITY) {
    return "NO_CAPACITY";
  }

  if (reason == NO_PICKUP) {
    return "NO_PICKUP";
  }

  if (reason == UNREACHABLE) {
    return "UNREACHABLE";
  }

  if (reason == TOO_FAR) {
    return "TOO_FAR";
  }

  throw StructureException("Invalid Reason enum value");
}

// string to enum
Reason strToReason(const string &text) {
  if (text == "OK") {
    return OK;
  }

  if (text == "NOT_ACCEPTING") {
    return NOT_ACCEPTING;
  }

  if (text == "FOOD_INCOMPATIBLE") {
    return FOOD_INCOMPATIBLE;
  }

  if (text == "NO_CAPACITY") {
    return NO_CAPACITY;
  }

  if (text == "NO_PICKUP") {
    return NO_PICKUP;
  }

  if (text == "UNREACHABLE") {
    return UNREACHABLE;
  }

  if (text == "TOO_FAR") {
    return TOO_FAR;
  }

  throw FormatException("Invalid Reason: " + text);
}

CsvRecord::~CsvRecord() {}

Organization::Organization(const EntityId &id, const string &name,
                           const string &type, int node) {
  id_ = id;
  name_ = name;
  type_ = type;
  node_ = node;
}

Organization::~Organization() {}

const EntityId &Organization::id() const { return id_; }

string Organization::name() const { return name_; }

string Organization::type() const { return type_; }

int Organization::nodeId() const { return node_; }

Donor::Donor(const EntityId &id, const string &name, const string &type,
             int node)
    : Organization(id, name, type, node) {}

Donor::~Donor() {}

string Donor::toCsvRow() const {
  stringstream ss;
  ss << id_.toString() << "," << name_ << "," << type_ << "," << node_;

  return ss.str();
}

Recipient::Recipient(const EntityId &id, const string &name, const string &type,
                     int capacity, const vector<FoodType> &types,
                     bool accepting, bool pickup, int node)
    : Organization(id, name, type, node) {
  capacity_ = capacity;
  types_ = types;
  accepting_ = accepting;
  pickup_ = pickup;
}

Recipient::~Recipient() {}

bool Recipient::accepts(FoodType t) const {
  int count = types_.size();

  for (int i = 0; i < count; i++) {
    if (types_[i] == t) {
      return true;
    }
  }

  return false;
}

int Recipient::availableCapacity() const { return capacity_; }

void Recipient::reduceCapacity(int qty) {
  if (qty < 0) {
    throw StructureException("Cannot reduce capacity by a negative quantity.");
  }

  if (qty > capacity_) {
    throw StructureException("Cannot reduce capacity below zero.");
  }

  capacity_ -= qty;
}

bool Recipient::accepting() const { return accepting_; }

bool Recipient::canPickup() const { return pickup_; }

const vector<FoodType> &Recipient::acceptedTypes() const { return types_; }

string Recipient::toCsvRow() const {
  stringstream ss;
  ss << id_.toString() << "," << name_ << "," << type_ << "," << capacity_
     << ",";

  int count = types_.size();

  for (int i = 0; i < count; i++) {
    ss << foodToStr(types_[i]);

    if (i < count - 1) {
      ss << ";";
    }
  }

  ss << ",";

  if (accepting_) {
    ss << "1";
  } else {
    ss << "0";
  }

  ss << ",";

  if (pickup_) {
    ss << "1";
  } else {
    ss << "0";
  }

  ss << "," << node_;

  return ss.str();
}

Donation::Donation(const EntityId &id, const EntityId &donor, FoodType food,
                   int original, int remaining, int usable,
                   DonationStatus status) {
  id_ = id;
  donor_ = donor;
  food_ = food;
  original_ = original;
  remaining_ = remaining;
  usable_ = usable;
  status_ = status;
}

Donation::~Donation() {}

const EntityId &Donation::id() const { return id_; }

const EntityId &Donation::donorId() const { return donor_; }

FoodType Donation::foodType() const { return food_; }

int Donation::originalQuantity() const { return original_; }

int Donation::remainingQuantity() const { return remaining_; }

int Donation::usableMinutes() const { return usable_; }

DonationStatus Donation::status() const { return status_; }

void Donation::allocateQuantity(int qty) {
  if (qty < 1) {
    throw StructureException("Quantity must be >= 1.");
  }

  if (qty > remaining_) {
    throw StructureException("Cannot allocate more than remaining quantity.");
  }

  if (status_ == CLOSED || status_ == COMPLETED) {
    throw StructureException(
        "Cannot allocate from a completed or closed donation.");
  }

  remaining_ -= qty;

  if (remaining_ == 0) {
    status_ = COMPLETED;
  } else {
    status_ = PARTIAL;
  }
}

void Donation::close() {
  if (status_ == COMPLETED || status_ == CLOSED) {
    throw StructureException(
        "Cannot close a donation that is already completed or closed.");
  }
  status_ = CLOSED;
}

string Donation::toCsvRow() const {
  stringstream ss;
  ss << id_.toString() << "," << donor_.toString() << "," << foodToStr(food_)
     << "," << original_ << "," << remaining_ << "," << usable_ << ","
     << statusToStr(status_);
  return ss.str();
}

Transaction::Transaction() {
  qty_ = 0;
  travel_ = 0;
}

Transaction::Transaction(const EntityId &id, const EntityId &donation,
                         const EntityId &recipient, int qty, int travel) {
  id_ = id;
  donation_ = donation;
  recipient_ = recipient;
  qty_ = qty;
  travel_ = travel;
}

Transaction::~Transaction() {}

const EntityId &Transaction::id() const { return id_; }

const EntityId &Transaction::donationId() const { return donation_; }

const EntityId &Transaction::recipientId() const { return recipient_; }

int Transaction::quantity() const { return qty_; }

int Transaction::travelMinutes() const { return travel_; }

string Transaction::toCsvRow() const {
  stringstream ss;
  ss << id_.toString() << "," << donation_.toString() << ","
     << recipient_.toString() << "," << qty_ << "," << travel_;
  return ss.str();
}
