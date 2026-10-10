#include "ConsoleView.h"

#include <iomanip>
#include <sstream>

using namespace std;


CoordinatorChoice::CoordinatorChoice()
    : close_(false), candidateNumber_(0), quantity_(0)
{
}


bool CoordinatorChoice::close() const {
    return close_;
}


int CoordinatorChoice::candidateNumber() const {
    return candidateNumber_;
}


int CoordinatorChoice::quantity() const {
    return quantity_;
}


istream& operator>>(istream& in, CoordinatorChoice& choice) {
    string line;

    if (!getline(in, line)) {
        return in;
    }

    if (line == "c" || line == "C") {
        choice.close_ = true;
        choice.candidateNumber_ = 0;
        choice.quantity_ = 0;

        return in;
    }

    stringstream ss(line);

    int candidateNumber;
    int quantity;

    if (!(ss >> candidateNumber >> quantity)) {
        in.setstate(ios::failbit);
        return in;
    }

    string extra;

    if (ss >> extra) {
        in.setstate(ios::failbit);
        return in;
    }

    choice.close_ = false;
    choice.candidateNumber_ = candidateNumber;
    choice.quantity_ = quantity;

    return in;
}


void ConsoleView::showCandidates(const vector<Candidate>& candidates) {
    cout << endl;
    cout << "Ranked Recipients" << endl;

    cout << left << setw(8) << "Rank" << setw(12) << "Recipient" << setw(12) << "Accept" << setw(12) << "Travel" << setw(12) << "Unused" << endl;

    cout << "--------------------------------------------------------" << endl;

    for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
        const Candidate& candidate = candidates[i];

        cout << left << setw(8) << i + 1 << setw(12) << candidate.recipientId() << setw(12) << candidate.potentialAcceptance() << setw(12) << candidate.travel() << setw(12) << candidate.unusedCapacity() << endl;
    }
}


void ConsoleView::showRejected(const vector<Rejection>& rejected) {
    if (rejected.empty()) {
        return;
    }

    cout << endl;
    cout << "Rejected Recipients" << endl;

    cout << left << setw(12) << "Recipient" << setw(24) << "Reason" << setw(12) << "Travel" << endl;

    cout << "--------------------------------------------------------" << endl;

    for (int i = 0; i < static_cast<int>(rejected.size()); ++i) {
        const Rejection& rejection = rejected[i];

        string reasonText;

        switch (rejection.reason()) {
            case NOT_ACCEPTING:
                reasonText = "NOT_ACCEPTING";
                break;
            case FOOD_INCOMPATIBLE:
                reasonText = "FOOD_INCOMPATIBLE";
                break;
            case NO_CAPACITY:
                reasonText = "NO_CAPACITY";
                break;
            case NO_PICKUP:
                reasonText = "NO_PICKUP";
                break;
            case UNREACHABLE:
                reasonText = "UNREACHABLE";
                break;
            case TOO_FAR:
                reasonText = "TOO_FAR";
                break;
            default:
                reasonText = "OK";
                break;
        }

        cout << left << setw(12) << rejection.recipientId() << setw(24) << reasonText << setw(12) << rejection.travel() << endl;
    }
}


void ConsoleView::showNoCandidates() {
    cout << endl;
    cout << "No feasible recipient is available." << endl;
}


void ConsoleView::showAllocationError(AllocError error) {
    switch (error) {
        case ALLOC_OK:
            cout << "Allocation successful." << endl;
            break;
        case DONATION_NOT_OPEN:
            cout << "Error: donation is not open for allocation." << endl;
            break;
        case BAD_QUANTITY:
            cout << "Error: quantity must be at least 1." << endl;
            break;
        case EXCEEDS_REMAINING:
            cout << "Error: quantity exceeds the donation's remaining amount." << endl;
            break;
        case EXCEEDS_CAPACITY:
            cout << "Error: quantity exceeds recipient capacity." << endl;
            break;
        case NOT_FEASIBLE:
            cout << "Error: recipient is no longer feasible." << endl;
            break;
        default:
            cout << "Error: unknown allocation error." << endl;
            break;
    }
}



void ConsoleView::showDonationClosed() {
    cout << "Donation closed. Remaining food is recorded as leftover."
         << endl;
}



void ConsoleView::showAllocationSuccess(const EntityId& recipientId, int quantity) {
    cout << "Allocated " << quantity << " to " << recipientId << "." << endl;
}