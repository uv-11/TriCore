#include "ConsoleView.h"

#include <iomanip>
#include <sstream>

using namespace std;

CoordinatorChoice::CoordinatorChoice() : close_(false), candidateNumber_(0), quantity_(0)
{
}

bool CoordinatorChoice::close() const
{
    return close_;
}

int CoordinatorChoice::candidateNumber() const
{
    return candidateNumber_;
}

int CoordinatorChoice::quantity() const
{
    return quantity_;
}

istream& operator>>(istream& in, CoordinatorChoice& choice)
{
    string line;

    if (!getline(in, line))
    {
        return in;
    }

    if (line == "c" || line == "C")
    {
        choice.close_ = true;
        choice.candidateNumber_ = 0;
        choice.quantity_ = 0;

        return in;
    }

    stringstream ss(line);

    int candidateNumber;
    int quantity;

    if (!(ss >> candidateNumber >> quantity))
    {
        in.setstate(ios::failbit);
        return in;
    }

    string extra;

    if (ss >> extra)
    {
        in.setstate(ios::failbit);
        return in;
    }

    choice.close_ = false;
    choice.candidateNumber_ = candidateNumber;
    choice.quantity_ = quantity;

    return in;
}

void ConsoleView::showCandidates(const vector<Candidate>& candidates)
{
    cout << endl;
    cout << "Ranked Recipients" << endl;

    cout << left << setw(8) << "Rank" << setw(12) << "Recipient" << setw(12) << "Accept"
         << setw(12) << "Travel" << setw(12) << "Unused" << endl;

    cout << "--------------------------------------------------------" << endl;

    for (int i = 0; i < static_cast<int>(candidates.size()); ++i)
    {
        const Candidate& candidate = candidates[i];

        cout << left << setw(8) << i + 1 << setw(12) << candidate.recipientId()
             << setw(12) << candidate.potentialAcceptance() << setw(12)
             << candidate.travel() << setw(12) << candidate.unusedCapacity() << endl;
    }
}

void ConsoleView::showRejected(const vector<Rejection>& rejected)
{
    if (rejected.empty())
    {
        return;
    }

    cout << endl;
    cout << "Rejected Recipients" << endl;

    cout << left << setw(12) << "Recipient" << setw(24) << "Reason" << setw(12)
         << "Travel" << endl;

    cout << "--------------------------------------------------------" << endl;

    for (int i = 0; i < static_cast<int>(rejected.size()); ++i)
    {
        const Rejection& rejection = rejected[i];

        string reasonText;

        switch (rejection.reason())
        {
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

        cout << left << setw(12) << rejection.recipientId() << setw(24) << reasonText
             << setw(12) << rejection.travel() << endl;
    }
}

void ConsoleView::showNoCandidates()
{
    cout << endl;
    cout << "No feasible recipient is available." << endl;
}

void ConsoleView::showAllocationError(AllocError error)
{
    switch (error)
    {
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

void ConsoleView::showDonationClosed()
{
    cout << "Donation closed. Remaining food is recorded as leftover." << endl;
}

void ConsoleView::showAllocationSuccess(const EntityId& recipientId, int quantity)
{
    cout << "Allocated " << quantity << " to " << recipientId << "." << endl;
}

static string formatRoute(const vector<int>& path)
{
    stringstream text;

    for (int i = 0; i < static_cast<int>(path.size()); ++i)
    {
        if (i > 0)
        {
            text << "->";
        }

        text << path[i];
    }

    return text.str();
}

static string joinTypes(const vector<FoodType>& types)
{
    stringstream text;

    for (int i = 0; i < static_cast<int>(types.size()); ++i)
    {
        if (i > 0)
        {
            text << ";";
        }

        text << foodToStr(types[i]);
    }

    return text.str();
}

void ConsoleView::showCandidates(const vector<Candidate>& candidates,
                                 const vector<string>& routes)
{
    cout << endl;
    cout << "Ranked Recipients" << endl;

    cout << left << setw(8) << "Rank" << setw(12) << "Recipient" << setw(12) << "Accept"
         << setw(12) << "Travel" << setw(12) << "Unused" << setw(20) << "Route" << endl;

    cout << "----------------------------------------------------------------------------"
            "----"
         << endl;

    for (int i = 0; i < static_cast<int>(candidates.size()); ++i)
    {
        const Candidate& candidate = candidates[i];
        string route = (i < static_cast<int>(routes.size())) ? routes[i] : "";

        cout << left << setw(8) << i + 1 << setw(12) << candidate.recipientId()
             << setw(12) << candidate.potentialAcceptance() << setw(12)
             << candidate.travel() << setw(12) << candidate.unusedCapacity() << setw(20)
             << route << endl;
    }
}

void ConsoleView::showWarnings(const list<string>& warnings)
{
    for (list<string>::const_iterator it = warnings.begin(); it != warnings.end(); ++it)
    {
        cout << "Warning: " << *it << endl;
    }
}

void ConsoleView::showMenu()
{
    cout << endl;
    cout << "1. Process the most urgent donation" << endl;
    cout << "2. Add a donation" << endl;
    cout << "3. Show donations" << endl;
    cout << "4. Show recipients" << endl;
    cout << "5. Show transactions" << endl;
    cout << "6. About" << endl;
    cout << "0. Exit" << endl;
}

int ConsoleView::readMenu()
{
    while (true)
    {
        cout << "Choice: ";

        string line;

        if (!getline(cin, line))
        {
            return -1;
        }

        stringstream ss(line);
        int choice;

        if (!(ss >> choice))
        {
            cout << "Please enter a number from 0 to 6." << endl;
            continue;
        }

        string extra;

        if (ss >> extra)
        {
            cout << "Please enter a number from 0 to 6." << endl;
            continue;
        }

        if (choice < 0 || choice > 6)
        {
            cout << "Please enter a number from 0 to 6." << endl;
            continue;
        }

        return choice;
    }
}

bool ConsoleView::processDonation(dataset& data, datamanager& files,
                                  const dijkstratraveltime& provider, int index)
{
    Donation& donation = data.donations[index];

    int donorPos = 0;

    if (!data.donorindex.get(donation.donorId().toString(), donorPos))
    {
        cout << "The donor for this donation is not in the files." << endl;
        return true;
    }

    int donorNode = data.donors[donorPos].nodeId();

    vector<Duration> travelTimes = provider.traveltimesfrom(donorNode);

    cout << endl;
    cout << "Processing " << donation.id() << endl;
    cout << "Donor: " << data.donors[donorPos].name() << " (" << donation.donorId()
         << ")   "
         << "Food: " << foodToStr(donation.foodType()) << "   "
         << "Quantity: " << donation.remainingQuantity() << "   "
         << "Usable: " << donation.usableMinutes() << " min" << endl;

    while (donation.remainingQuantity() > 0)
    {
        RecommendationResult result = recommend(donation, data.recipients, travelTimes);

        if (result.candidates().empty())
        {
            showNoCandidates();
            showRejected(result.rejected());
            donation.close();
            files.commitdonations(data);
            showDonationClosed();
            return true;
        }

        vector<string> routes;

        for (int i = 0; i < static_cast<int>(result.candidates().size()); ++i)
        {
            int recipientPos = 0;
            string route = "";

            if (data.recipientindex.get(result.candidates()[i].recipientId().toString(),
                                        recipientPos))
            {
                int targetNode = data.recipients[recipientPos].nodeId();
                route = formatRoute(provider.routeto(donorNode, targetNode));
            }

            routes.push_back(route);
        }

        showCandidates(result.candidates(), routes);
        showRejected(result.rejected());

        CoordinatorChoice choice;

        while (true)
        {
            cout << "Pick a candidate number and a quantity, or c to close: ";

            if (cin >> choice)
            {
                break;
            }

            if (cin.eof())
            {
                return false;
            }

            cin.clear();
            cout << "That line was not understood. Enter a candidate number and a "
                    "quantity, or c."
                 << endl;
        }

        if (choice.close())
        {
            donation.close();
            files.commitdonations(data);
            showDonationClosed();
            return true;
        }

        if (choice.candidateNumber() < 1 ||
            choice.candidateNumber() > static_cast<int>(result.candidates().size()))
        {
            cout << "There is no candidate with that number." << endl;
            continue;
        }

        const Candidate& picked = result.candidates()[choice.candidateNumber() - 1];

        int recipientPos = 0;

        if (!data.recipientindex.get(picked.recipientId().toString(), recipientPos))
        {
            cout << "That recipient is no longer in the files." << endl;
            continue;
        }

        Recipient& recipient = data.recipients[recipientPos];
        EntityId transactionId = transactionidissuer::issuenext();

        AllocError outcome = allocate(donation, recipient, choice.quantity(),
                                      picked.travel(), transactionId);

        if (outcome != ALLOC_OK)
        {
            showAllocationError(outcome);
            continue;
        }

        Transaction entry(transactionId, donation.id(), recipient.id(), choice.quantity(),
                          picked.travel().minutes());
        data.transactions.push_back(entry);
        files.commitallocation(data, entry);
        showAllocationSuccess(recipient.id(), choice.quantity());
    }

    return true;
}

int ConsoleView::addDonation(dataset&, datamanager&)
{
    cout << endl;
    cout << "Adding a new donation is not available yet." << endl;
    return -1;
}

void ConsoleView::showDonations(const dataset& data)
{
    cout << endl;
    cout << left << setw(10) << "ID" << setw(10) << "Donor" << setw(12) << "Food"
         << setw(10) << "Original" << setw(10) << "Remaining" << setw(8) << "Usable"
         << setw(10) << "Status" << endl;
    cout << "----------------------------------------------------------------------------"
            "----"
         << endl;

    for (int i = 0; i < static_cast<int>(data.donations.size()); ++i)
    {
        const Donation& donation = data.donations[i];

        cout << left << setw(10) << donation.id() << setw(10) << donation.donorId()
             << setw(12) << foodToStr(donation.foodType()) << setw(10)
             << donation.originalQuantity() << setw(10) << donation.remainingQuantity()
             << setw(8) << donation.usableMinutes() << setw(10)
             << statusToStr(donation.status()) << endl;
    }
}

void ConsoleView::showRecipients(const dataset& data)
{
    cout << endl;
    cout << left << setw(10) << "ID" << setw(16) << "Name" << setw(10) << "Capacity"
         << setw(24) << "Accepted types" << setw(10) << "Accepts" << setw(8) << "Pickup"
         << endl;
    cout << "----------------------------------------------------------------------------"
            "----"
         << endl;

    for (int i = 0; i < static_cast<int>(data.recipients.size()); ++i)
    {
        const Recipient& recipient = data.recipients[i];
        string accepts = recipient.accepting() ? "yes" : "no";
        string pickup = recipient.canPickup() ? "yes" : "no";

        cout << left << setw(10) << recipient.id() << setw(16) << recipient.name()
             << setw(10) << recipient.availableCapacity() << setw(24)
             << joinTypes(recipient.acceptedTypes()) << setw(10) << accepts << setw(8)
             << pickup << endl;
    }
}

void ConsoleView::showTransactions(const dataset& data)
{
    cout << endl;
    cout << left << setw(8) << "ID" << setw(38) << "Donor" << setw(12) << "Food"
         << setw(50) << "Recipient" << setw(10) << "Quantity" << setw(8) << "Travel"
         << endl;
    cout << string(126, '-') << endl;

    for (int i = 0; i < static_cast<int>(data.transactions.size()); ++i)
    {
        const Transaction& entry = data.transactions[i];

        string donorName = entry.donationId().toString();
        string foodText = "UNKNOWN";
        string recipientName = entry.recipientId().toString();

        int donationPos = 0;

        if (data.donationindex.get(entry.donationId().toString(), donationPos))
        {
            const Donation& donation = data.donations[donationPos];
            foodText = foodToStr(donation.foodType());

            int donorPos = 0;

            if (data.donorindex.get(donation.donorId().toString(), donorPos))
            {
                donorName = data.donors[donorPos].name();
            }
            else
            {
                donorName = donation.donorId().toString();
            }
        }

        int recipientPos = 0;

        if (data.recipientindex.get(entry.recipientId().toString(), recipientPos))
        {
            recipientName = data.recipients[recipientPos].name();
        }

        cout << left << setw(8) << entry.id() << setw(38) << donorName << setw(12)
             << foodText << setw(50) << recipientName << setw(10) << entry.quantity()
             << setw(8) << entry.travelMinutes() << endl;
    }
}

void ConsoleView::showAbout()
{
    cout << endl;
    cout << "CommunityBridge matches surplus food donations to nearby recipients."
         << endl;
    cout << "Travel times between locations are simulated minutes, not real road data."
         << endl;
    cout << "Donations are ranked by acceptance amount, travel time, and leftover "
            "capacity."
         << endl;
}