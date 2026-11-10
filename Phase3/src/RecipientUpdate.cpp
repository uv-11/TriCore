// RecipientUpdate.cpp: the edit of a recipient without any change to Phase 2.
// Recipient has no setters, so a replacement with the same id, name, type, food types and
// location is built and put in the same place of the vector (decision D5).

#include "RecipientUpdate.h"
#include "Exceptions.h"

using namespace std;

// Builds the replacement, saves the whole recipients table and undoes the change if the save fails.
bool updaterecipient(dataset& data, datamanager& files, const EntityId& recipient, int capacity,
                     bool accepting, bool pickup, string& message)
{
    int pos = 0;

    if (!data.recipientindex.get(recipient.toString(), pos))
    {
        message = "That recipient could not be found";
        return false;
    }

    if (capacity < 0)
    {
        message = "The capacity cannot be below 0";
        return false;
    }

    Recipient old = data.recipients[pos];
    data.recipients[pos] = Recipient(old.id(), old.name(), old.type(), capacity, old.acceptedTypes(),
                                     accepting, pickup, old.nodeId());

    try
    {
        // the id is already in the table, so the table is rewritten from memory
        files.appendrecipient(data, data.recipients[pos]);
    }
    catch (const AppException&)
    {
        data.recipients[pos] = old;
        message = "Your details could not be saved so nothing was changed and you can try again";
        return false;
    }

    message = "Your details were saved";
    return true;
}
