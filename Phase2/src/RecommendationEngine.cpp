#include "RecommendationEngine.h"

#include <algorithm>
#include <iostream>

using namespace std;



Candidate::Candidate()
    : recipientId_(), potentialAcceptance_(0),
      travel_(), unusedCapacity_(0)
{
}



Candidate::Candidate(const EntityId& id, int accept, const Duration& travel, int unused)
    : recipientId_(id), potentialAcceptance_(accept),
      travel_(travel), unusedCapacity_(unused)
{
}



const EntityId& Candidate::recipientId() const {
    return recipientId_;
}



int Candidate::potentialAcceptance() const {
    return potentialAcceptance_;
}



const Duration& Candidate::travel() const {
    return travel_;
}



int Candidate::unusedCapacity() const {
    return unusedCapacity_;
}



bool operator<(const Candidate& a, const Candidate& b) {
    if (a.potentialAcceptance_ != b.potentialAcceptance_) {
        return a.potentialAcceptance_ > b.potentialAcceptance_;
    }

    if (!(a.travel_ == b.travel_)) {
        return a.travel_ < b.travel_;
    }

    if (a.unusedCapacity_ != b.unusedCapacity_) {
        return a.unusedCapacity_ < b.unusedCapacity_;
    }

    return a.recipientId_.number() < b.recipientId_.number();
}



ostream& operator<<(ostream& out, const Candidate& c) {
    out << c.recipientId_ << " "
        << c.potentialAcceptance_ << " "
        << c.travel_ << " "
        << c.unusedCapacity_;

    return out;
}



Rejection::Rejection()
    : recipientId_(), reason_(OK), travel_()
{
}



Rejection::Rejection(const EntityId& id, Reason reason,
                     const Duration& travel)
    : recipientId_(id), reason_(reason), travel_(travel)
{
}



const EntityId& Rejection::recipientId() const {
    return recipientId_;
}



Reason Rejection::reason() const {
    return reason_;
}



const Duration& Rejection::travel() const {
    return travel_;
}



const vector<Candidate>& RecommendationResult::candidates() const {
    return candidates_;
}



const vector<Rejection>& RecommendationResult::rejected() const {
    return rejected_;
}



Reason checkFeasibility(const Recipient& recipient, const Donation& donation, const Duration& travel) {
    if (!recipient.accepting()) {
        return NOT_ACCEPTING;
    }

    if (!recipient.accepts(donation.foodType())) {
        return FOOD_INCOMPATIBLE;
    }

    if (recipient.availableCapacity() <= 0) {
        return NO_CAPACITY;
    }

    if (!recipient.canPickup()) {
        return NO_PICKUP;
    }

    if (travel.isInfinite()) {
        return UNREACHABLE;
    }

    if (travel.minutes() > donation.usableMinutes()) {
        return TOO_FAR;
    }

    return OK;
}



RecommendationResult recommend(const Donation& donation, const vector<Recipient>& recipients, const vector<Duration>& travelTimes) {
    RecommendationResult result;

    for (int i = 0; i < static_cast<int>(recipients.size()); ++i) {
        const Recipient& recipient = recipients[i];

        Duration travel = Duration::infinite();

        if (recipient.nodeId() >= 0 && recipient.nodeId() < static_cast<int>(travelTimes.size())) {
            travel = travelTimes[recipient.nodeId()];
        }

        Reason reason = checkFeasibility(recipient, donation, travel);

        if (reason != OK) {
            result.rejected_.push_back(
                Rejection(recipient.id(), reason, travel));
        } else {
            int acceptance = donation.remaining();

            if (recipient.availableCapacity() < acceptance) {
                acceptance = recipient.availableCapacity();
            }

            int unused = recipient.availableCapacity() - acceptance;

            result.candidates_.push_back(Candidate(recipient.id(), acceptance, travel, unused));
        }
    }

    if (!result.candidates_.empty()) {
        mergeSort(
            &result.candidates_[0],
            static_cast<int>(result.candidates_.size()));
    }

    return result;
}