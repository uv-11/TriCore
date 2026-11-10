#ifndef RECIPIENTUPDATE_H
#define RECIPIENTUPDATE_H

// RecipientUpdate.h: lets a recipient change its own capacity and flags.

#include "Datamanager.h"
#include "EntityId.h"

#include <string>

using namespace std;

// Changes the capacity, the accepting flag and the pickup flag of one recipient and saves
// recipients.csv. Returns false and a plain message when the recipient is unknown, the capacity
// is below 0 or the save failed (then the old recipient is put back in memory).
bool updaterecipient(dataset& data, datamanager& files, const EntityId& recipient, int capacity,
                     bool accepting, bool pickup, string& message);

#endif
