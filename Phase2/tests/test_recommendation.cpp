#include "RecommendationEngine.h"

#include <iostream>
#include <vector>
#include <cstdlib>

using namespace std;

void check(bool ok, const string& name) {
    if (!ok) {
        cout << "FAIL: " << name << endl;
        exit(1);
    }
}

void checkReason(Reason actual, Reason expected, const string& name) {
    check(actual == expected, name);
}

Recipient makeRecipient(int number, int capacity, const vector<FoodType>& types, bool accepting, bool pickup) {
    return Recipient(
        EntityId("R", number),
        "Test Recipient",
        "TEST",
        capacity,
        types,
        accepting,
        pickup,
        number);
}

Donation makeDonation(FoodType food, int quantity, int usable) {
    return Donation(
        EntityId("DON", 1),
        EntityId("DN", 1),
        food,
        quantity,
        quantity,
        usable,
        PENDING);
}

void testFeasibilityReasons() {
    vector<FoodType> cooked;
    cooked.push_back(COOKED);

    vector<FoodType> packaged;
    packaged.push_back(PACKAGED);

    Donation donation = makeDonation(COOKED, 50, 20);

    Recipient notAccepting = makeRecipient(
        7, 90, cooked, false, true);
    checkReason(
        checkFeasibility(notAccepting, donation, Duration(5)),
        NOT_ACCEPTING,
        "F1 not accepting");

    Recipient wrongFood = makeRecipient(
        2, 90, packaged, true, true);
    checkReason(
        checkFeasibility(wrongFood, donation, Duration(5)),
        FOOD_INCOMPATIBLE,
        "F2 food incompatible");

    vector<FoodType> emptyTypes;
    Recipient noCapacity = makeRecipient(
        3, 0, emptyTypes, true, true);
    checkReason(
        checkFeasibility(noCapacity, donation, Duration(5)),
        NO_CAPACITY,
        "F3 no capacity");

    Recipient noPickup = makeRecipient(
        4, 90, cooked, true, false);
    checkReason(
        checkFeasibility(noPickup, donation, Duration(5)),
        NO_PICKUP,
        "F4 no pickup");

    Recipient unreachable = makeRecipient(
        5, 90, cooked, true, true);
    checkReason(
        checkFeasibility(
            unreachable,
            donation,
            Duration::infinite()),
        UNREACHABLE,
        "F5 unreachable");

    Recipient tooFar = makeRecipient(
        6, 90, cooked, true, true);
    checkReason(
        checkFeasibility(tooFar, donation, Duration(21)),
        TOO_FAR,
        "F6 too far");

    Recipient good = makeRecipient(
        1, 90, cooked, true, true);
    checkReason(
        checkFeasibility(good, donation, Duration(20)),
        OK,
        "F8 equal travel");
}

// Checks that the first failing rule wins when more than one rule fails.
void testFeasibilityPrecedence() {
    vector<FoodType> packaged;
    packaged.push_back(PACKAGED);

    Donation donation = makeDonation(COOKED, 50, 20);
    Recipient recipient = makeRecipient(
        7, 90, packaged, false, true);

    checkReason(
        checkFeasibility(recipient, donation, Duration(100)),
        NOT_ACCEPTING,
        "F7 precedence");
}

void testTravelBoundary() {
    vector<FoodType> cooked;
    cooked.push_back(COOKED);

    Donation donation = makeDonation(COOKED, 50, 12);
    Recipient recipient = makeRecipient(
        4, 80, cooked, true, true);

    checkReason(
        checkFeasibility(recipient, donation, Duration(12)),
        OK,
        "F8 travel equal usable");

    checkReason(
        checkFeasibility(recipient, donation, Duration(13)),
        TOO_FAR,
        "F8 travel above usable");
}

void testRankingAcceptance() {
    Candidate a(EntityId("R", 1), 80, Duration(30), 20);
    Candidate b(EntityId("R", 2), 70, Duration(5), 0);

    check(a < b, "R1 acceptance");
    check(!(b < a), "R1 reverse");
}

void testRankingTravel() {
    Candidate a(EntityId("R", 1), 100, Duration(10), 20);
    Candidate b(EntityId("R", 2), 100, Duration(20), 0);

    check(a < b, "R2 travel");
    check(!(b < a), "R2 reverse");
}

void testRankingUnused(){
    Candidate a(EntityId("R", 1), 100, Duration(10), 10);
    Candidate b(EntityId("R", 2), 100, Duration(10), 20);

    check(a < b, "R3 unused capacity");
    check(!(b < a), "R3 reverse");
}


void testRankingId(){
    Candidate a(EntityId("R", 7), 100, Duration(10), 10);
    Candidate b(EntityId("R", 8), 100, Duration(10), 10);

    check(a < b, "R4 recipient number");
    check(!(b < a), "R4 reverse");
}

void testRankingLargeId(){
    Candidate a(EntityId("R", 1000), 100, Duration(10), 10);
    Candidate b(EntityId("R", 999), 100, Duration(10), 10);

    check(b < a, "R5 numeric ID");
    check(!(a < b), "R5 numeric reverse");
}

void testRankingStrictness(){
    Candidate a(EntityId("R", 7), 100, Duration(10), 10);
    Candidate b(EntityId("R", 7), 100, Duration(10), 10);

    check(!(a < a), "R6 a less than itself");
    check(!(a < b), "R6 equal candidates");
    check(!(b < a), "R6 equal reverse");
}

void testS4(){
    vector<FoodType> cooked;
    cooked.push_back(COOKED);

    Donation donation = makeDonation(COOKED, 60, 40);

    vector<Recipient> recipients;
    recipients.push_back(makeRecipient(1, 150, cooked, true, true));
    recipients.push_back(makeRecipient(3, 120, cooked, true, true));
    recipients.push_back(makeRecipient(4, 80, cooked, true, true));

    vector<Duration> travel(5, Duration::infinite());
    travel[1] = Duration(25);
    travel[3] = Duration(22);
    travel[4] = Duration(12);

    RecommendationResult result = recommend(
        donation, recipients, travel);

    check(result.candidates().size() == 3, "R S4 count");
    check(result.candidates()[0].recipientId().number() == 4,
          "R S4 first");
    check(result.candidates()[1].recipientId().number() == 3,
          "R S4 second");
    check(result.candidates()[2].recipientId().number() == 1,
          "R S4 third");
}

void testS5(){
    vector<FoodType> cooked;
    cooked.push_back(COOKED);

    Donation donation = makeDonation(COOKED, 100, 30);

    vector<Recipient> recipients;
    recipients.push_back(makeRecipient(1, 150, cooked, true, true));
    recipients.push_back(makeRecipient(3, 120, cooked, true, true));
    recipients.push_back(makeRecipient(4, 80, cooked, true, true));

    vector<Duration> travel(5, Duration::infinite());
    travel[1] = Duration(25);
    travel[3] = Duration(22);
    travel[4] = Duration(12);

    RecommendationResult result = recommend(
        donation, recipients, travel);

    check(result.candidates().size() == 3, "R S5 count");
    check(result.candidates()[0].recipientId().number() == 3,
          "R S5 first");
    check(result.candidates()[1].recipientId().number() == 1,
          "R S5 second");
    check(result.candidates()[2].recipientId().number() == 4,
          "R S5 third");
}

void testS6(){
    vector<FoodType> packaged;
    packaged.push_back(PACKAGED);

    Donation donation = makeDonation(PACKAGED, 100, 30);

    vector<Recipient> recipients;
    recipients.push_back(makeRecipient(1, 150, packaged, true, true));
    recipients.push_back(makeRecipient(2, 300, packaged, true, true));

    vector<Duration> travel(3, Duration::infinite());
    travel[1] = Duration(25);
    travel[2] = Duration(15);

    RecommendationResult result = recommend(
        donation, recipients, travel);

    check(result.candidates().size() == 2, "R S6 count");
    check(result.candidates()[0].recipientId().number() == 2,
          "R S6 first");
    check(result.candidates()[1].recipientId().number() == 1,
          "R S6 second");
}

int main(){
    testFeasibilityReasons();
    testFeasibilityPrecedence();
    testTravelBoundary();
    testRankingAcceptance();
    testRankingTravel();
    testRankingUnused();
    testRankingId();
    testRankingLargeId();
    testRankingStrictness();
    testS4();
    testS5();
    testS6();

    cout << "All recommendation tests passed." << endl;
    return 0;
}
