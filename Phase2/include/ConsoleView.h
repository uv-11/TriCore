#ifndef CONSOLE_VIEW_H
#define CONSOLE_VIEW_H

#include <iostream>
#include <list>
#include <string>
#include <vector>

#include "AllocationManager.h"
#include "Datamanager.h"
#include "RecommendationEngine.h"
#include "traveltime.h"
// ASSUMPTION: "dijkstratraveltime" has travelTimesFrom(int) and routeTo(int, int). Rename if the real header differs.



class ConsoleView {
public:
    static void showCandidates(const std::vector<Candidate>& candidates);

    // adds a route string per candidate
    static void showCandidates(const std::vector<Candidate>& candidates,
                                const std::vector<std::string>& routes);

    static void showRejected(const std::vector<Rejection>& rejected);

    static void showNoCandidates();

    static void showAllocationError(AllocError error);

    static void showDonationClosed();

    static void showAllocationSuccess(const EntityId& recipientId, int quantity);

    static void showWarnings(const std::list<std::string>& warnings);

    static void showMenu();

    // returns 0 to 6, or -1 when the input has ended
    static int readMenu();

    // runs one donation until allocated or closed; false only at end of input
    static bool processDonation(dataset& data, datamanager& files,
                                 const dijkstratraveltime& provider, int index);

    // not available yet; returns -1
    static int addDonation(dataset& data, datamanager& files);

    static void showDonations(const dataset& data);

    static void showRecipients(const dataset& data);

    static void showTransactions(const dataset& data);

    static void showAbout();
};


class CoordinatorChoice {
public:
    CoordinatorChoice();
    bool close() const;
    int candidateNumber() const;
    int quantity() const;
    friend std::istream& operator>>(std::istream& in, CoordinatorChoice& choice);
private:
    bool close_;
    int candidateNumber_;
    int quantity_;
};

#endif