#ifndef RECOMMENDATION_ENGINE_H
#define RECOMMENDATION_ENGINE_H

#include <iostream>
#include <vector>

#include "Duration.h"
#include "EntityId.h"
#include "Models.h"
#include "Sorting.h"


class Candidate {
public:
    Candidate();
    Candidate(const EntityId& id, int accept, const Duration& travel, int unused);
    friend bool operator<(const Candidate& a, const Candidate& b);
    friend std::ostream& operator<<(std::ostream& out, const Candidate& c);
    const EntityId& recipientId() const;
    int potentialAcceptance() const;
    const Duration& travel() const;
    int unusedCapacity() const;
private:
    EntityId recipientId_;
    int potentialAcceptance_;
    Duration travel_;
    int unusedCapacity_;
};


class Rejection {
public:
    Rejection();
    Rejection(const EntityId& id, Reason reason, const Duration& travel);
    const EntityId& recipientId() const;
    Reason reason() const;
    const Duration& travel() const;
private:
    EntityId recipientId_;
    Reason reason_;
    Duration travel_;
};


class RecommendationResult {
public:
    const std::vector<Candidate>& candidates() const;
    const std::vector<Rejection>& rejected() const;
private:
    std::vector<Candidate> candidates_;
    std::vector<Rejection> rejected_;
    friend RecommendationResult recommend(const Donation& donation, const std::vector<Recipient>& recipients, const std::vector<Duration>& travelTimes);
};


Reason checkFeasibility(const Recipient& recipient, const Donation& donation, const Duration& travel);

RecommendationResult recommend( const Donation& donation, const std::vector<Recipient>& recipients, const std::vector<Duration>& travelTimes);

#endif