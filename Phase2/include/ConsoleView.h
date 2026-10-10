#ifndef CONSOLE_VIEW_H
#define CONSOLE_VIEW_H

#include <iostream>
#include <string>
#include <vector>

#include "AllocationManager.h"
#include "RecommendationEngine.h"



class ConsoleView {
public:
    static void showCandidates(const std::vector<Candidate>& candidates);

    static void showRejected(const std::vector<Rejection>& rejected);

    static void showNoCandidates();

    static void showAllocationError(AllocError error);

    static void showDonationClosed();

    static void showAllocationSuccess(const EntityId& recipientId, int quantity);
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