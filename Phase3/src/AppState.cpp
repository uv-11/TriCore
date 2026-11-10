// AppState.cpp: the start-up sequence. It repeats the order of Phase2/src/main.cpp exactly.

#include "AppState.h"
#include "Exceptions.h"
#include "allocationmanager.h"

using namespace std;

// Runs the start-up. If anything throws, the objects made so far are freed before the exception goes on,
// because a destructor does not run for an object whose constructor did not finish.
AppState::AppState(const string& folder)
{
    prefix = folder;
    files = NULL;
    provider = NULL;

    try
    {
        files = new datamanager(prefix);
        files->loadall(data, warnings);

        // new transaction ids continue after the highest id on file
        transactionidissuer::seed(data.lasttransaction);

        // the donation, donor and recipient issuers continue after the highest id that was loaded
        EntityId highestdonation("DON", 0);

        for (size_t pos = 0; pos < data.donations.size(); pos++)
        {
            if (highestdonation < data.donations[pos].id())
            {
                highestdonation = data.donations[pos].id();
            }
        }

        donationidissuer::seed(highestdonation);

        EntityId highestdonor("DN", 0);

        for (size_t pos = 0; pos < data.donors.size(); pos++)
        {
            if (highestdonor < data.donors[pos].id())
            {
                highestdonor = data.donors[pos].id();
            }
        }

        donoridissuer::seed(highestdonor);

        EntityId highestrecipient("R", 0);

        for (size_t pos = 0; pos < data.recipients.size(); pos++)
        {
            if (highestrecipient < data.recipients[pos].id())
            {
                highestrecipient = data.recipients[pos].id();
            }
        }

        recipientidissuer::seed(highestrecipient);

        // provider gives the travel time and the route using dijkstra
        provider = new dijkstratraveltime(data.network);

        // the offers come last, then every offer that does not fit the data becomes a warning
        offers.load(prefix, warnings);
        vector<string> mismatches = offers.reconcile(data);

        for (size_t pos = 0; pos < mismatches.size(); pos++)
        {
            warnings.push_back(mismatches[pos]);
        }
    }
    catch (...)
    {
        delete provider;
        delete files;
        provider = NULL;
        files = NULL;
        throw;
    }
}

// Frees what the constructor made.
AppState::~AppState()
{
    delete provider;
    delete files;
}
