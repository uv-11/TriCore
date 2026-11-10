#ifndef WORKFLOW_H
#define WORKFLOW_H

// Workflow.h: every business operation of the program, in plain C++ (no window code).
// The dialogs only read inputs, call one of these functions and show the result.

#include "Datamanager.h"
#include "EntityId.h"
#include "Match.h"
#include "Models.h"
#include "OfferStore.h"
#include "traveltime.h"

#include <string>

using namespace std;

// Food that is left in a donation and not yet promised in a waiting offer (never below 0).
int effectiveremaining(const dataset& data, const offerstore& store, int donationPos);

// Position of the most urgent donation that still has food to offer, or -1 when there is none.
int nexturgentdonation(const dataset& data, const offerstore& store);

// Looks for recipients for one donation. Never changes any data.
matchview buildmatch(const dataset& data, const offerstore& store, const dijkstratraveltime& provider,
                     int donationPos);

// Sends an offer to the candidate with that number (1 is the first) for qty. Returns the position of
// the new offer, or -1 with a plain message. The offers file is saved; if that fails nothing changes.
int sendoffer(dataset& data, offerstore& store, datamanager& files, const matchview& match,
              int candidateNumber, int qty, string& message);

// The recipient accepts a waiting offer. This runs the Phase 2 allocation and records the transaction.
// Returns false with a plain message when it cannot be done (then the offer stays as it was, except
// when the donation is no longer open: then the offer is withdrawn).
bool acceptoffer(dataset& data, offerstore& store, datamanager& files, const EntityId& offerid,
                 string& message);

// The recipient declines a waiting offer.
bool declineoffer(offerstore& store, datamanager& files, const EntityId& offerid, string& message);

// The coordinator withdraws a waiting offer.
bool withdrawoffer(offerstore& store, datamanager& files, const EntityId& offerid, string& message);

// Closes an open donation: its waiting offers are withdrawn, and the food that is left is recorded as wasted.
bool closedonationwithoffers(dataset& data, offerstore& store, datamanager& files, int donationPos,
                             string& message);

// Adds a donation for a donor. Returns its position, or -1 with a plain message.
int createdonation(dataset& data, datamanager& files, const EntityId& donor, FoodType food, int qty,
                   int usable, string& message);

// Version 1.1. Adds a recipient on a location of the map. The type is stored in capitals with the
// spaces turned into underscores. Returns its position, or -1 with a plain message. The message
// also says so when no donor is connected to the location.
int createrecipient(dataset& data, datamanager& files, const string& name, const string& type,
                    const vector<FoodType>& foods, int capacity, bool accepting, bool pickup, int node,
                    string& message);

// Version 1.1. Adds a donor on a location of the map. Returns its position, or -1 with a plain message.
int createdonor(dataset& data, datamanager& files, const string& name, const string& type, int node,
                string& message);

// Version 1.1. Checks the name, type and location of a new donor without saving anything. Returns false
// with a plain message when one of them is wrong.
bool checkdonordetails(const dataset& data, const string& name, const string& type, int node, string& message);

// Version 1.1. Adds a new donor and its first donation. Everything is checked first, so nothing is
// saved for a bad input, and the donor is saved BEFORE the donation. Returns the position of the
// donation, or -1 with a plain message.
int createdonationwithnewdonor(dataset& data, datamanager& files, const string& donorname,
                               const string& donortype, int node, FoodType food, int qty, int usable,
                               string& message);

#endif
