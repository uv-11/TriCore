#include "Exceptions.h"
#include "allocationmanager.h"
#include "ConsoleView.h"
#include "Datamanager.h"
#include "traveltime.h"
#include <iostream>
#include <list>
#include <string>

using namespace std;



// menu and orchestration only, all logic lives in the other files
int main(int argc, char* argv[])
{
    // run from the repo root, prefix is put in front of every file name
    string prefix = "Phase2/data/";

    // argc counts the command line words, argv holds them, argv[1] is the first one
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
        // files reads and writes the csv files
        datamanager files(prefix);
        // data holds everything kept in memory
        dataset data;
        // warnings collects startup problems that do not stop the program
        list<string> warnings;

        // loadall throws a dataexception on bad data, caught at the end of main
        files.loadall(data, warnings);
        // new transaction ids continue after the highest id on file
        transactionidissuer::seed(data.lasttransaction);

        // seed donationidissuer from highest donation id already loaded
        EntityId highestDonation("DON", 0);
        for (size_t i = 0; i < data.donations.size(); ++i)
        {
            if (highestDonation < data.donations[i].id())
            {
                highestDonation = data.donations[i].id();
            }
        }
        donationidissuer::seed(highestDonation);

        // seed donoridissuer from highest donor id already loaded
        EntityId highestDonor("DN", 0);
        for (size_t i = 0; i < data.donors.size(); ++i)
        {
            if (highestDonor < data.donors[i].id())
            {
                highestDonor = data.donors[i].id();
            }
        }
        donoridissuer::seed(highestDonor);

        // seed recipientidissuer from highest recipient id already loaded
        EntityId highestRecipient("R", 0);
        for (size_t i = 0; i < data.recipients.size(); ++i)
        {
            if (highestRecipient < data.recipients[i].id())
            {
                highestRecipient = data.recipients[i].id();
            }
        }
        recipientidissuer::seed(highestRecipient);

        // provider gives the travel time and route using dijkstra
        dijkstratraveltime provider(data.network);
        // waiting holds the open donations, most urgent first
        urgencyqueue waiting;
        fillurgencyqueue(data.donations, waiting);

        cout << "CommunityBridge: location-aware travel-time feasibility" << endl;
        cout << "(travel times are simulated minutes, not real road data)" << endl;
        ConsoleView::showWarnings(warnings);

        // running becomes false when the coordinator exits or input ends
        bool running = true;

        while (running)
        {
            ConsoleView::showMenu();
            // choice is the typed menu number, 0 to 9, or -1 at end of input
            int choice = ConsoleView::readMenu();

            if (choice == 1)
            {
                if (waiting.empty())
                {
                    cout << endl << "No donation is waiting." << endl;
                }
                else
                {
                    // index is the most urgent donations position, false means input ended
                    int index = waiting.popmosturgent();
                    running = ConsoleView::processDonation(data, files, provider, index);
                }
            }
            else if (choice == 2)
            {
                // adddonation saves the donation and returns its index, or -1 if cancelled
                int index = ConsoleView::addDonation(data, files);

                if (index >= 0)
                {
                    waiting.push(data.donations[index], index);
                }
            }
            else if (choice == 3)
            {
                ConsoleView::addRecipient(data, files);
            }
            else if (choice == 4)
            {
                ConsoleView::showDonations(data);
            }
            else if (choice == 5)
            {
                ConsoleView::showRecipients(data);
            }
            else if (choice == 6)
            {
                ConsoleView::showTransactions(data);
            }
            else if (choice == 7)
            {
                ConsoleView::showNetwork(data);
            }
            else if (choice == 8)
            {
                ConsoleView::showTravelTimesFromDonor(data, provider);
            }
            else if (choice == 9)
            {
                ConsoleView::showSummary(data);
            }
            else
            {
                // 0 is exit and -1 means the input has ended, both lead here
                running = false;
            }
        }
    }
    catch (const AppException& problem)
    {
        // a bad data file or failed save ends the program with a message
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
