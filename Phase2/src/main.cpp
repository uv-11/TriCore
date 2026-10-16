#include "Exceptions.h"
#include "allocationmanager.h"
#include "ConsoleView.h"
#include "Datamanager.h"
#include "traveltime.h"
#include <iostream>
#include <list>
#include <string>

using namespace std;

// Puts every donation that is still PENDING or PARTIAL into the urgency queue.
static void fillurgencyqueue(const vector<Donation>& donations, urgencyqueue& waiting)
{
    for (int i = 0; i < static_cast<int>(donations.size()); i++)
    {
        if (donations[i].status() == PENDING || donations[i].status() == PARTIAL)
        {
            waiting.push(donations[i], i);
        }
    }
}


// Menu and orchestration only. All logic lives in the other files.
int main(int argc, char* argv[])
{
    // Run from the repository root. A different data folder prefix can be given.
    // prefix is put in front of every file name, so Phase2/data/ means
    // Phase2/data/donors.csv and so on.
    string prefix = "Phase2/data/";

    // argc is the number of words on the command line and argv holds them. argv[1] is the
    // first word after the program name.
    if (argc > 1)
    {
        prefix = argv[1];
    }

    // the file names are glued directly behind prefix, so a folder must end with a slash
    if (prefix.size() > 0)
    {
        char lastChar = prefix[prefix.size() - 1];

        if (lastChar != '/' && lastChar != '\\')
        {
            prefix = prefix + "/";
        }
    }

    try
    {
        // files reads and writes the CSV files
        datamanager files(prefix);
        // data holds everything that is kept in memory
        dataset data;
        // warnings collects the findings that are shown at the start but do not stop it
        list<string> warnings;

        // loadall refuses bad data by throwing a DataException, caught at the end of main
        files.loadall(data, warnings);
        // new transaction ids continue after the highest id in transactions.csv
        transactionidissuer::seed(data.lasttransaction);

        // provider answers: how long does a trip from node A to node B take, and which
        // route is used (Dijkstra)
        dijkstratraveltime provider(data.network);
        // waiting holds the open donations, the most urgent first
        urgencyqueue waiting;
        fillurgencyqueue(data.donations, waiting);

        cout << "CommunityBridge: location-aware travel-time feasibility" << endl;
        cout << "(travel times are simulated minutes, not real road data)" << endl;
        ConsoleView::showWarnings(warnings);

        // running becomes false when the coordinator exits or the input ends
        bool running = true;

        while (running)
        {
            ConsoleView::showMenu();
            // choice is the menu number that was typed (0 to 6), or -1 when the
            // input has ended. Both 0 and -1 leave the loop.
            int choice = ConsoleView::readMenu();

            if (choice == 1)
            {
                if (waiting.empty())
                {
                    cout << endl << "No donation is waiting." << endl;
                }
                else
                {
                    // index is the position of the most urgent donation. If the input
                    // ends inside the workflow, processDonation returns false and the
                    // program stops.
                    int index = waiting.popmosturgent();
                    running = ConsoleView::processDonation(data, files, provider, index);
                }
            }
            else if (choice == 2)
            {
                // addDonation saves the new donation and returns its index, or -1 if
                // it was cancelled. A new donation joins the urgency queue.
                int index = ConsoleView::addDonation(data, files);

                if (index >= 0)
                {
                    waiting.push(data.donations[index], index);
                }
            }
            else if (choice == 3)
            {
                ConsoleView::showDonations(data);
            }
            else if (choice == 4)
            {
                ConsoleView::showRecipients(data);
            }
            else if (choice == 5)
            {
                ConsoleView::showTransactions(data);
            }
            else if (choice == 6)
            {
                ConsoleView::showAbout();
            }
            else
            {
                // 0 is Exit and -1 means the input has ended; both lead here
                running = false;
            }
        }
    }
    catch (const AppException& problem)
    {
        // a bad data file or a failed save ends the program with a clear message
        cout << endl << "Error: " << problem.what() << endl;
        cout << "Data folder used: " << prefix << endl;
        return 1;
    }
    catch (const exception& problem)
    {
        // any other standard exception also ends the program with a message
        cout << endl << "Unexpected error: " << problem.what() << endl;
        return 1;
    }

    cout << endl << "Goodbye." << endl;
    return 0;
}
