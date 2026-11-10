#ifndef APPSTATE_H
#define APPSTATE_H

// AppState.h: everything the running program keeps in memory. No window code in here.

#include "Datamanager.h"
#include "OfferStore.h"
#include "traveltime.h"

#include <list>
#include <string>

using namespace std;

// Loads all the data when the program starts and keeps it while the program runs.
class AppState
{
public:
    // Does the start-up of the Phase 2 console program (load, seed the id issuers, build the travel
    // times), then loads the offers and adds the warnings about offers that do not fit the data.
    // Throws an AppException (for example DataException) when the files are bad.
    AppState(const string& folder);

    // Frees the datamanager and the travel time provider.
    ~AppState();

    string prefix;                   // the data folder, with a slash at the end
    datamanager* files;              // reads and writes the csv files (it has no default constructor)
    dataset data;                    // donors, recipients, donations, transactions and the map
    list<string> warnings;           // problems found at start-up that do not stop the program
    offerstore offers;               // every offer
    dijkstratraveltime* provider;    // travel times and routes, built after the data is loaded

private:
    // Copying is not allowed, because the object owns the two pointers above.
    AppState(const AppState& other);
    AppState& operator=(const AppState& other);
};

// The one global pointer to the program state (decision D9). It is defined in gui_main.cpp. The
// dialog procedures have no other simple way to reach the data, so every screen reads it here.
extern AppState* theapp;

#endif
